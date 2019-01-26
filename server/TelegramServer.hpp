#ifndef TELEGRAMSERVER_HPP
#define TELEGRAMSERVER_HPP

#include <QObject>
#include "TLTypes.hpp"
#include "TelegramNamespace.hpp"

#include "ServerApi.hpp"

QT_FORWARD_DECLARE_CLASS(QTcpServer)
QT_FORWARD_DECLARE_CLASS(QTcpSocket)
QT_FORWARD_DECLARE_CLASS(QTimer)

#include <QHash>
#include <QSet>
#include <QVector>

namespace Telegram {

namespace Server {

class User;
class Session;
class RemoteClientConnection;
class RemoteServerConnection;
class RemoteUser;
class RpcOperationFactory;

class Server : public QObject, public ServerApi
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);

    void setDcOption(const DcOption &option);

    void setServerPrivateRsaKey(const Telegram::RsaKey &key);

    bool start();
    void stop();
    void addServiceUser();
    void loadData();

    void setServerConfiguration(const DcConfiguration &config);
    void addServerConnection(RemoteServerConnection *remoteServer);

    QSet<RemoteClientConnection*> getConnections() { return m_activeConnections; }

    quint32 getDcIdForUserIdentifier(const QString &phoneNumber);

    void setAuthorizationProvider(Authorization::Provider *provider);

    // ServerAPI:
    Authorization::Provider *getAuthorizationProvider() override { return m_authProvider; }

    DcConfiguration serverConfiguration() const override { return m_dcConfiguration; }
    quint32 dcId() const override { return m_dcOption.id; }

    PhoneStatus getPhoneStatus(const QString &identifier) const override;
    PasswordInfo getPassword(const QString &identifier) override;
    bool checkPassword(const QString &identifier, const QByteArray &hash) override;
    bool identifierIsValid(const QString &identifier) override;

    RemoteUser *getRemoteUser(quint32 userId) const override;
    RemoteUser *getRemoteUser(const QString &identifier) const override;
    RemoteUser *getReallyRemoteUser(quint32 userId) const;
    RemoteUser *getReallyRemoteUser(const QString &identifier) const;

    bool setupTLUser(TLUser *output, const RemoteUser *input, const User *applicant) const override;
    bool setupTLUpdatesState(TLUpdatesState *output, const User *forUser) const override;

    Telegram::Peer getPeer(const TLInputPeer &peer, const User *applicant) const override;

    User *getUser(const QString &identifier) const override;
    User *getUser(quint32 userId) const override;
    User *getUser(const TLInputUser &inputUser, User *self) const override;
    User *tryAccessUser(quint32 userId, quint64 accessHash, User *applicant) const override;
    User *addUser(const QString &identifier) override;

    Session *createSession(quint64 authId, const QByteArray &authKey, const QString &address) override;
    Session *getSessionByAuthId(quint64 authKeyId) const override;

    void insertUser(User *user);

    RemoteUser *getServiceUser();

signals:

public slots:

protected slots:
    void onNewConnection();
    void onUserSessionAdded(Session *newSession);

protected:
    void onClientConnectionStatusChanged();

protected:
    Authorization::Provider *m_authProvider = nullptr;

private:
    QTcpServer *m_serverSocket;
    DcOption m_dcOption;
    Telegram::RsaKey m_key;

    QHash<QString, quint32> m_phoneToUserId;
    QHash<quint64, Session*> m_authIdToSession;
    QHash<quint32, User*> m_users; // userId to User
    QSet<RemoteClientConnection*> m_activeConnections;
    QSet<RemoteServerConnection*> m_remoteServers;
    QVector<RpcOperationFactory*> m_rpcOperationFactories;
    DcConfiguration m_dcConfiguration;
    quint32 m_serviceUserId = 0;
};

} // Server

} // Telegram

#endif // TELEGRAMSERVER_HPP
