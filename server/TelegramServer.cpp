#include "TelegramServer.hpp"

#include <QLoggingCategory>
#include <QTcpServer>
#include <QTcpSocket>

#include "ApiUtils.hpp"
#include "TelegramServerUser.hpp"
#include "RemoteClientConnection.hpp"
#include "RemoteServerConnection.hpp"
#include "Session.hpp"

#include "CServerTcpTransport.hpp"

// Generated RPC Operation Factory includes
#include "AccountOperationFactory.hpp"
#include "AuthOperationFactory.hpp"
#include "BotsOperationFactory.hpp"
#include "ChannelsOperationFactory.hpp"
#include "ContactsOperationFactory.hpp"
#include "HelpOperationFactory.hpp"
#include "LangpackOperationFactory.hpp"
#include "MessagesOperationFactory.hpp"
#include "PaymentsOperationFactory.hpp"
#include "PhoneOperationFactory.hpp"
#include "PhotosOperationFactory.hpp"
#include "StickersOperationFactory.hpp"
#include "UpdatesOperationFactory.hpp"
#include "UploadOperationFactory.hpp"
#include "UsersOperationFactory.hpp"
// End of generated RPC Operation Factory includes

Q_LOGGING_CATEGORY(loggingCategoryServer, "telegram.server.main", QtInfoMsg)
Q_LOGGING_CATEGORY(loggingCategoryServerApi, "telegram.server.api", QtWarningMsg)

namespace Telegram {

namespace Server {

Server::Server(QObject *parent) :
    QObject(parent)
{
    m_rpcOperationFactories = {
        // Generated RPC Operation Factory initialization
        new AccountOperationFactory(),
        new AuthOperationFactory(),
        new BotsOperationFactory(),
        new ChannelsOperationFactory(),
        new ContactsOperationFactory(),
        new HelpOperationFactory(),
        new LangpackOperationFactory(),
        new MessagesOperationFactory(),
        new PaymentsOperationFactory(),
        new PhoneOperationFactory(),
        new PhotosOperationFactory(),
        new StickersOperationFactory(),
        new UpdatesOperationFactory(),
        new UploadOperationFactory(),
        new UsersOperationFactory(),
        // End of generated RPC Operation Factory initialization
    };
    m_serverSocket = new QTcpServer(this);
    connect(m_serverSocket, &QTcpServer::newConnection, this, &Server::onNewConnection);
}

void Server::setDcOption(const DcOption &option)
{
    m_dcOption = option;
}

void Server::setServerPrivateRsaKey(const Telegram::RsaKey &key)
{
    m_key = key;
}

bool Server::start()
{
    if (!m_serverSocket->listen(QHostAddress(m_dcOption.address), m_dcOption.port)) {
        qCWarning(loggingCategoryServer) << "Unable to listen port" << m_dcOption.port;
        return false;
    }
    qCInfo(loggingCategoryServer).nospace().noquote() << this << " start server (DC " << m_dcOption.id << ") "
                                                      << "on " << m_dcOption.address << ":" << m_dcOption.port
                                                      << "; Key:" << hex << showbase << m_key.fingerprint;

    addServiceUser();
    return true;
}

void Server::stop()
{
    qCInfo(loggingCategoryServer).nospace().noquote() << this << " stop server (DC " << m_dcOption.id << ") "
                                                      << "on " << m_dcOption.address << ":" << m_dcOption.port;
    if (m_serverSocket) {
        m_serverSocket->close();
    }

    // Connections removed from the set on disconnected.
    // Copy connections to a variable to iterate over a constant container instead of
    // (virtually) simultanously mutated member variable.
    QSet<RemoteClientConnection*> activeConnections = m_activeConnections;
    for (RemoteClientConnection *client : activeConnections) {
        client->transport()->disconnectFromHost();
    }
}

void Server::addServiceUser()
{
    User *serviceUser = new User(this);
    serviceUser->setPhoneNumber(QStringLiteral("+42777"));
    serviceUser->setFirstName(QStringLiteral("Telegram"));
    serviceUser->setDcId(0);
    m_serviceUserId = serviceUser->id();
    insertUser(serviceUser);
}

void Server::loadData()
{
    const int number = 10;
    for (int i = 0; i < number; ++i) {
        User *newUser = new User(this);
        newUser->setPhoneNumber(QStringLiteral("%1").arg(i, 6, 10, QLatin1Char('0')));
        insertUser(newUser);
    }
}

void Server::setServerConfiguration(const DcConfiguration &config)
{
    m_dcConfiguration = config;
}

void Server::addServerConnection(RemoteServerConnection *remoteServer)
{
    m_remoteServers.insert(remoteServer);
}

quint32 Server::getDcIdForUserIdentifier(const QString &phoneNumber)
{
    if (m_phoneToUserId.contains(phoneNumber)) {
        return m_dcOption.id;
    }
    return 0;
}

void Server::setAuthorizationProvider(Authorization::Provider *provider)
{
    m_authProvider = provider;
}

void Server::onNewConnection()
{
    QTcpSocket *socket = m_serverSocket->nextPendingConnection();
    if (!socket) {
        qCDebug(loggingCategoryServer) << "expected pending connection does not exist";
        return;
    }
    qCInfo(loggingCategoryServer) << this << "An incoming connection from" << socket->peerAddress().toString();
    TcpTransport *transport = new TcpTransport(socket, this);
    socket->setParent(transport);
    RemoteClientConnection *client = new RemoteClientConnection(this);
    connect(client, &BaseConnection::statusChanged, this, &Server::onClientConnectionStatusChanged);
    client->setServerRsaKey(m_key);
    client->setTransport(transport);
    client->setServerApi(this);
    client->setRpcFactories(m_rpcOperationFactories);

    m_activeConnections.insert(client);
}

void Server::onUserSessionAdded(Session *newSession)
{
    RemoteUser *sender = getServiceUser();
    User *recipient = newSession->user();

    QString text = QStringLiteral("Detected login from IP address %1").arg(newSession->ip);

    TLMessage m;
    m.tlType = TLValue::Message;
    m.message = text;
    m.toId = recipient->toTLPeer();
    m.fromId = sender->id();
    m.flags |= TLMessage::FromId;
    recipient->addMessage(m);
}

void Server::onClientConnectionStatusChanged()
{
    RemoteClientConnection *client = qobject_cast<RemoteClientConnection*>(sender());
    if (client->status() == RemoteClientConnection::Status::HasDhKey) {
        if (!client->session()) {
            qCDebug(loggingCategoryServer) << Q_FUNC_INFO << "Connected a client with a new auth key"
                                              << "from" << client->transport()->remoteAddress();
        }
    } else if (client->status() == RemoteClientConnection::Status::Disconnected) {
        if (client->session()) {
            qCInfo(loggingCategoryServer) << this << __func__ << "Disconnected a client with session id"
                                          << hex << showbase << client->session()->id()
                                          << "from" << client->transport()->remoteAddress();
            client->session()->setConnection(nullptr);
        } else {
            qCInfo(loggingCategoryServer) << this << __func__ << "Disconnected a client without a session"
                                          << "from" << client->transport()->remoteAddress();
        }
        // TODO: Initiate session cleanup after session expiration time out
        m_activeConnections.remove(client);
        client->deleteLater();
    }
}

bool Server::setupTLUser(TLUser *output, const RemoteUser *input, const User *applicant) const
{
    output->id = input->id();
    output->tlType = TLValue::User;
    output->firstName = input->firstName();
    output->lastName = input->lastName();
    // TODO: Check if the user has access to the requested user phone
    output->phone = input->phoneNumber();

    quint32 flags = 0;
    if (!output->firstName.isEmpty()) {
        flags |= TLUser::FirstName;
    }
    if (!output->lastName.isEmpty()) {
        flags |= TLUser::LastName;
    }
    if (!output->username.isEmpty()) {
        flags |= TLUser::Username;
    }
    if (!output->phone.isEmpty()) {
        flags |= TLUser::Phone;
    }
    if (output->id == applicant->id()) {
        flags |= TLUser::Self;
    }
    if (applicant->contactList().contains(output->id)) {
        flags |= TLUser::Contact;
        if (input->contactList().contains(applicant->id())) {
            flags |= TLUser::MutualContact;
        }
    }
    output->flags = flags;

    return true;
}

bool Server::setupTLUpdatesState(TLUpdatesState *output, const User *forUser) const
{
    output->pts = forUser->pts();
    output->date = Utils::getCurrentTime();
    output->seq = 1; // FIXME
    output->qts = 0;
    output->unreadCount = 0;
    return true;
}

Peer Server::getPeer(const TLInputPeer &peer, const User *applicant) const
{
    switch (peer.tlType) {
    case TLValue::InputPeerEmpty:
        return Peer();
    case TLValue::InputPeerSelf:
        return Peer::fromUserId(applicant->id());
    case TLValue::InputPeerChat:
        return Peer::fromChatId(peer.chatId);
    case TLValue::InputPeerUser:
        return Peer::fromUserId(peer.userId);
    case TLValue::InputPeerChannel:
        return Peer::fromChannelId(peer.channelId);
    default:
        qCWarning(loggingCategoryServerApi) << this << __func__ << "Invalid input peer type" << peer.tlType;
        return Peer();
    };
}

User *Server::getUser(const QString &identifier) const
{
    quint32 id = m_phoneToUserId.value(identifier);
    if (!id) {
        return nullptr;
    }
    return m_users.value(id);
}

User *Server::getUser(quint32 userId) const
{
    return m_users.value(userId);
}

User *Server::getUser(const TLInputUser &inputUser, User *self) const
{
    switch (inputUser.tlType) {
    case TLValue::InputUserSelf:
        return self;
    case TLValue::InputUser:
        return tryAccessUser(inputUser.userId, inputUser.accessHash, self);
    case TLValue::InputUserEmpty:
        return nullptr;
    default:
        return nullptr;
    }
}

User *Server::tryAccessUser(quint32 userId, quint64 accessHash, User *applicant) const
{
    User *u = getUser(userId);
    // TODO: Check access hash
    return u;
}

User *Server::addUser(const QString &identifier)
{
    qCDebug(loggingCategoryServerApi) << Q_FUNC_INFO << identifier;
    User *user = new User(this);
    user->setPhoneNumber(identifier);
    user->setDcId(dcId());
    insertUser(user);
    return user;
}

Session *Server::createSession(quint64 authId, const QByteArray &authKey, const QString &address)
{
    Session *session = new Session();
    session->authId = authId;
    session->authKey = authKey;
    session->ip = address;
    m_authIdToSession.insert(authId, session);
    return session;
}

Session *Server::getSessionByAuthId(quint64 authKeyId) const
{
    return m_authIdToSession.value(authKeyId);
}

void Server::insertUser(User *user)
{
    qCDebug(loggingCategoryServerApi) << Q_FUNC_INFO << user << user->phoneNumber() << user->id();
    m_users.insert(user->id(), user);
    m_phoneToUserId.insert(user->phoneNumber(), user->id());
    for (Session *session : user->sessions()) {
        m_authIdToSession.insert(session->authId, session);
    }

    connect(user, &User::sessionAdded, this, &Server::onUserSessionAdded);
}

RemoteUser *Server::getServiceUser()
{
    return m_users.value(m_serviceUserId);
}

PhoneStatus Server::getPhoneStatus(const QString &identifier) const
{
    PhoneStatus result;
    RemoteUser *user = getRemoteUser(identifier);
    if (user) {
        result.online = user->isOnline();
        result.dcId = user->dcId();
    }
    return result;
}

PasswordInfo Server::getPassword(const QString &identifier)
{
    PasswordInfo result;
    User *user = getUser(identifier);
    if (user && user->hasPassword()) {
        result.currentSalt = user->passwordSalt();
        result.hint = user->passwordHint();
    }
    return result;
}

bool Server::checkPassword(const QString &identifier, const QByteArray &hash)
{
    User *user = getUser(identifier);
    if (user && user->hasPassword()) {
        return user->passwordHash() == hash;
    }
    return false;

}

bool Server::identifierIsValid(const QString &identifier)
{
    const bool result = identifier.length() > 4;
    qCDebug(loggingCategoryServerApi) << "identifierIsValid(" << identifier << "):" << result;
    return result;
}

RemoteUser *Server::getRemoteUser(quint32 userId) const
{
    RemoteUser *user = getUser(userId);
    if (!user) {
        user = getReallyRemoteUser(userId);
    }
    return user;
}

RemoteUser *Server::getRemoteUser(const QString &identifier) const
{
    RemoteUser *user = getUser(identifier);
    if (!user) {
        user = getReallyRemoteUser(identifier);
    }
    return user;
}

RemoteUser *Server::getReallyRemoteUser(quint32 userId) const
{
    for (RemoteServerConnection *remoteServer : m_remoteServers) {
        RemoteUser *u = remoteServer->api()->getUser(userId);
        if (u) {
            return u;
        }
    }
    return nullptr;
}

RemoteUser *Server::getReallyRemoteUser(const QString &identifier) const
{
    for (RemoteServerConnection *remoteServer : m_remoteServers) {
        RemoteUser *u = remoteServer->api()->getUser(identifier);
        if (u) {
            return u;
        }
    }
    return nullptr;
}

} // Server

} // Telegram
