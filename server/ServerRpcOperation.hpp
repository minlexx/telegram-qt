#ifndef TELEGRAM_SERVER_RPC_OPERATION_HPP
#define TELEGRAM_SERVER_RPC_OPERATION_HPP

#include "PendingOperation.hpp"
#include "TLFunctions.hpp"
#include "RpcError.hpp"

class CTelegramStream;
class RpcProcessingContext;

namespace Telegram {

namespace Server {

class RpcLayer;
class LocalServerApi;

class RpcOperation : public PendingOperation
{
    Q_OBJECT
//    Q_DISABLE_COPY(ServerRpcOperation)
public:
    explicit RpcOperation(RpcLayer *rpcLayer);

    void setRequestId(quint64 messageId);

//    void sendReply(const QByteArray &reply);

    LocalServerApi *api() { return m_api; }
    RpcLayer *layer() { return m_rpcLayer; }

    bool sendRpcError(const RpcError &error);

    template <typename TLType>
    bool sendRpcReply(const TLType &reply);

    bool verifyHasUserOrWantedUser();

protected:
    virtual bool processNotImplementedMethod(TLValue functionCode);

    RpcLayer *m_rpcLayer = nullptr;
    LocalServerApi *m_api = nullptr;
    quint64 m_requestId = 0;
    quint32 m_layer = 0;
//    QByteArray m_request;
};

} // Server namespace

} //Telegram namespace

#endif // TELEGRAM_SERVER_RPC_OPERATION_HPP
