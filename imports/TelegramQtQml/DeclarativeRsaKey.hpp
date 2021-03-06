#ifndef TELEGRAM_DECLARATIVE_RSA_KEY_HPP
#define TELEGRAM_DECLARATIVE_RSA_KEY_HPP

#include "telegramqt_qml_global.h"

#include "ClientSettings.hpp"

#include <QQmlListProperty>
#include <qqml.h>
#include <QVector>

namespace Telegram {

namespace Client {

class TELEGRAMQT_QML_EXPORT DeclarativeRsaKey : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)
    Q_PROPERTY(bool loadDefault READ loadDefault WRITE setLoadDefault NOTIFY loadDefaultChanged)
    Q_PROPERTY(QString fingerprint READ fingerprint NOTIFY fingerprintChanged)
public:
    explicit DeclarativeRsaKey(QObject *parent = nullptr);
    QString fileName() const { return m_fileName; }
    bool isValid() const;
    bool loadDefault() const;
    QString fingerprint() const;
    RsaKey key() const { return m_key; }

public slots:
    void setFileName(const QString &fileName);
    void setLoadDefault(bool loadDefault);

signals:
    void fileNameChanged(const QString &fileName);
    void validChanged(bool valid);
    void loadDefaultChanged(bool loadDefault);
    void fingerprintChanged();

protected:
    void setKey(const RsaKey &key);

    bool m_loadDefaultKey = false;
    QString m_fileName;
    RsaKey m_key;
};

} // Client

} // Telegram

#endif // TELEGRAM_DECLARATIVE_RSA_KEY_HPP
