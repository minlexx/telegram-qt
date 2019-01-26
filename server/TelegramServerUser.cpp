#include "TelegramServerUser.hpp"

#include "ApiUtils.hpp"
#include "ServerApi.hpp"
#include "ServerRpcLayer.hpp"
#include "Session.hpp"
#include "RemoteClientConnection.hpp"
#include "Utils.hpp"

#include <QDateTime>
#include <QLoggingCategory>

namespace Telegram {

namespace Server {

TLPeer MessageRecipient::toTLPeer() const
{
    const Peer p = toPeer();
    if (Q_UNLIKELY(!p.isValid())) {
        qCritical() << Q_FUNC_INFO << "Invalid peer" << this;
        return TLPeer();
    }
    TLPeer result;
    switch (p.type) {
    case Peer::User:
        result.tlType = TLValue::PeerUser;
        result.userId = p.id;
        break;
    case Peer::Chat:
        result.tlType = TLValue::PeerChat;
        result.chatId = p.id;
        break;
    case Peer::Channel:
        result.tlType = TLValue::PeerChannel;
        result.channelId = p.id;
        break;
    }
    return result;
}

QVector<quint32> LocalGroup::members() const
{
    return {};
}

quint32 LocalGroup::addMessage(const TLMessage &message, Session *excludeSession)
{
    for (const quint32 userId : members()) {
        if (userId == message.fromId) {
            continue;
        }
        RemoteUser *user = api()->getRemoteUser(userId);
        user->addMessage(message, excludeSession);
    }
    return 0;
}

QVector<quint32> LocalChannel::members() const
{
    return {};
}

quint32 LocalChannel::addMessage(const TLMessage &message, Session *excludeSession)
{
    // Local-server addMessage to channel:
    //     channel->addMessage()
    //     Foreach (user : member)
    //         user->updateChannelPts()
    return 0;
}

UserContact RemoteUser::toContact() const
{
    UserContact contact;
    contact.id = id();
    contact.phone = phoneNumber();
    contact.firstName = firstName();
    contact.lastName = lastName();
    return contact;
}

User::User(QObject *parent) :
    QObject(parent)
{
}

void User::setPhoneNumber(const QString &phoneNumber)
{
    m_phoneNumber = phoneNumber;
    m_id = qHash(m_phoneNumber);
}

void User::setFirstName(const QString &firstName)
{
    m_firstName = firstName;
}

void User::setLastName(const QString &lastName)
{
    m_lastName = lastName;
}

bool User::isOnline() const
{
    return true;
}

void User::setDcId(quint32 id)
{
    m_dcId = id;
}

Session *User::getSession(quint64 authId) const
{
    for (Session *s : m_sessions) {
        if (s->authId == authId) {
            return s;
        }
    }
    return nullptr;
}

QVector<Session *> User::activeSessions() const
{
    QVector<Session *> result;
    for (Session *s : m_sessions) {
        if (s->isActive()) {
            result.append(s);
        }
    }
    return result;
}

bool User::hasActiveSession() const
{
    for (Session *s : m_sessions) {
        if (s->isActive()) {
            return true;
        }
    }
    return false;
}

void User::addSession(Session *session)
{
    m_sessions.append(session);
    session->setUser(this);
    emit sessionAdded(session);
}

void User::setPlainPassword(const QString &password)
{
    if (password.isEmpty()) {
        m_passwordSalt.clear();
        m_passwordHash.clear();
        return;
    }
    QByteArray pwdSalt(8, Qt::Uninitialized);
    Utils::randomBytes(&pwdSalt);
    const QByteArray pwdData = pwdSalt + password.toUtf8() + pwdSalt;
    const QByteArray pwdHash = Utils::sha256(pwdData);
    setPassword(pwdSalt, pwdHash);
}

void User::setPassword(const QByteArray &salt, const QByteArray &hash)
{
    m_passwordSalt = salt;
    m_passwordHash = hash;
}

quint32 User::addMessage(const TLMessage &message, Session *excludeSession)
{
    m_messages.append(message);
    m_messages.last().id = addPts();
    const Telegram::Peer messagePeer = Telegram::Utils::getMessagePeer(message, id());
    UserDialog *dialog = ensureDialog(messagePeer);
    dialog->lastMessageId = message.id;

    // Post update to other sessions
    bool needUpdates = false;
    for (Session *s : activeSessions()) {
        if (s == excludeSession) {
            continue;
        }
        needUpdates = true;
        break;
    }
    if (!needUpdates) {
        return m_messages.last().id;
    }

    ServerApi *api = activeSessions().first()->rpcLayer()->api();
    RemoteUser *sender = api->getRemoteUser(message.fromId);

    TLUpdate newMessageUpdate;
    newMessageUpdate.tlType = TLValue::UpdateNewMessage;
    newMessageUpdate.message = m_messages.last();
    newMessageUpdate.pts = pts();
    newMessageUpdate.ptsCount = 1;

    TLUpdates updates;
    updates.tlType = TLValue::Updates;
    updates.updates = { newMessageUpdate };
    updates.chats = {};

    if (sender) {
        updates.users = { TLUser() }; // Sender
        api->setupTLUser(&updates.users[0], sender, this);
    }

    updates.date = message.date;
    //updates.seq = 0;

    for (Session *s : activeSessions()) {
        if (s == excludeSession) {
            continue;
        }
        s->rpcLayer()->sendUpdates(updates);
    }

    return m_messages.last().id;
}

TLVector<TLMessage> User::getHistory(const Peer &peer,
                                     quint32 offsetId,
                                     quint32 offsetDate,
                                     quint32 addOffset,
                                     quint32 limit,
                                     quint32 maxId,
                                     quint32 minId,
                                     quint32 hash) const
{
    if (offsetId || offsetDate || addOffset || minId || maxId || hash) {
        qWarning() << Q_FUNC_INFO << "Unsupported request";
        return {};
    }

    const int actualLimit = qMin<quint32>(limit, 30);
    QVector<int> wantedIndices;
    wantedIndices.reserve(actualLimit);

    for (int i = m_messages.count() - 1; i >= 0; --i) {
        const TLMessage &message = m_messages.at(i);
        if (peer.isValid()) {
            Telegram::Peer messagePeer = Telegram::Utils::getMessagePeer(message, id());
            if (peer != messagePeer) {
                continue;
            }
        }

        wantedIndices.append(i);
        if (wantedIndices.count() == actualLimit) {
            break;
        }
    }

    TLVector<TLMessage> result;
    result.reserve(wantedIndices.count());
    for (int i : wantedIndices) {
        result.append(m_messages.at(i));
    }
    return result;
}

const TLMessage *User::getMessage(quint32 messageId) const
{
    if (!messageId || m_messages.isEmpty()) {
        return nullptr;
    }
    if (m_messages.count() < messageId) {
        return nullptr;
    }
    return &m_messages.at(messageId - 1);
}

quint32 User::addPts()
{
    return ++m_pts;
}

void User::importContact(const UserContact &contact)
{
    // Check for contact registration status and the contact id setup performed out of this function
    m_importedContacts.append(contact);

    if (contact.id) {
        m_contactList.append(contact.id);
    }
}

UserDialog *User::ensureDialog(const Telegram::Peer &peer)
{
    for (int i = 0; i < m_dialogs.count(); ++i) {
        if (m_dialogs.at(i)->peer == peer) {
            return m_dialogs[i];
        }
    }
    m_dialogs.append(new UserDialog());
    m_dialogs.last()->peer = peer;
    return m_dialogs.last();
}

} // Server

} // Telegram
