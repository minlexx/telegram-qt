#ifndef TELEGRAM_DECLARATIVE_AUTH_OPERATION_HPP
#define TELEGRAM_DECLARATIVE_AUTH_OPERATION_HPP

#include <QObject>

#include "DeclarativeOperation.hpp"

namespace Telegram {

namespace Client {

class DeclarativeClient;
class DeclarativeSettings;

class AuthOperation;

class DeclarativeAuthOperation : public DeclarativeOperation
{
    Q_OBJECT
    Q_PROPERTY(QString phoneNumber READ phoneNumber WRITE setPhoneNumber NOTIFY phoneNumberChanged)
    Q_PROPERTY(QString passwordHint READ passwordHint NOTIFY passwordHintChanged)
    Q_PROPERTY(bool hasRecovery READ hasRecovery NOTIFY hasRecoveryChanged)
//    Q_PROPERTY(bool callAvailable READ isCallAvailable NOTIFY callAvailableChanged)
    Q_PROPERTY(AuthStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
//    Q_PROPERTY(bool passwordParamsKnown READ passwordParamsKnown)
public:
    explicit DeclarativeAuthOperation(QObject *parent = nullptr);

    enum AuthStatus {
        Idle,
        Connecting,
        Handshake,
        Phone,
        AuthCodeRequired,
        PasswordRequired,
        SignedIn
    };
    Q_ENUM(AuthStatus)

    bool isBusy() const;
    AuthStatus status() const;

    QString phoneNumber() const;
    QString passwordHint() const;
    bool hasRecovery() const;

public slots:
    void abort();

    bool submitAuthCode(const QString &code);
    bool submitPassword(const QString &password);

    void setPhoneNumber(const QString &phoneNumber);

    bool recovery();

    bool requestCall();
    bool requestSms();

Q_SIGNALS:
    void busyChanged(bool busy);

    void passwordHintChanged(const QString &hint);
    void hasRecoveryChanged();

    void phoneNumberRequired();
    void authCodeRequired();
    void passwordRequired();
    void passwordCheckFailed();

    void invalid();

    void phoneNumberChanged();
//    void callAvailable();

    void statusChanged(AuthStatus newStatus);

    void authSignErrorReceived(TelegramNamespace::AuthSignError errorCode, const QString &errorMessage); // Error message description: https://core.telegram.org/api/errors#400-bad-request
    void authorizationErrorReceived(TelegramNamespace::UnauthorizedError errorCode, const QString &errorMessage);

protected:
    void setStatus(const DeclarativeAuthOperation::AuthStatus status);
    void setBusy(bool busy);

    void unsetBusy();
    void onPasswordRequired();

    AuthOperation *m_authOperation;

    void startEvent() override;

    bool m_busy = false;
    AuthStatus m_status;
    QString m_phoneNumber;

};

} // Client

} // Telegram

#endif // TELEGRAM_DECLARATIVE_AUTH_OPERATION_HPP
