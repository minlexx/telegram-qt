/*
   Copyright (C) 2018 Alexandr Akulich <akulichalexander@gmail.com>

   This file is a part of TelegramQt library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

 */

#ifndef TELEGRAM_SERVER_CLUSTER_HPP
#define TELEGRAM_SERVER_CLUSTER_HPP

#include <QObject>
#include <QVector>

#include "DcConfiguration.hpp"
#include "TelegramNamespace.hpp"
#include "ServerNamespace.hpp"

namespace Telegram {

namespace Server {

namespace Authorization {

class Provider;

} // Authorization namespace

class Server;
class Session;
class ServerApi;
class User;
class RemoteUser;

class LocalCluster : public QObject
{
    Q_OBJECT
public:
    explicit LocalCluster(QObject *parent = nullptr);
    using ServerConstructor = Server *(*)(QObject *parent);
    void setServerContructor(ServerConstructor constructor);

    void setAuthorizationProvider(Authorization::Provider *provider);

    DcConfiguration serverConfiguration() { return m_serverConfiguration; }
    void setServerConfiguration(const DcConfiguration &config);

    RsaKey serverRsaKey() const { return m_key; }
    void setServerPrivateRsaKey(const Telegram::RsaKey &key);

    bool start();
    void stop();

    void sendMessage(const QString &userId, const QString &text);

    User *addUser(const QString &identifier, quint32 dcId);
    User *getUser(const QString &identifier);
    RemoteUser *getServiceUser();

    QVector<Server*> getServerInstances() { return m_serverInstances; }
    Server *getServerInstance(quint32 dcId);
    ServerApi *getServerApiInstance(quint32 dcId);

protected:
    ServerConstructor m_constructor;
    QVector<Server*> m_serverInstances;
    DcConfiguration m_serverConfiguration;
    RsaKey m_key;
    Authorization::Provider *m_authProvider = nullptr;
};

} // Server

} // Telegram

#endif // TELEGRAM_SERVER_CLUSTER_HPP
