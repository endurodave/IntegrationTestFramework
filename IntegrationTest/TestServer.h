/// @file NetworkMgr.h
/// @brief Application-specific network manager for Alarms, Commands, and Data.
/// @see https://github.com/endurodave/DelegateMQ
/// David Lafreniere, 2025.

#ifndef TEST_SERVER_H
#define TEST_SERVER_H

#include "DelegateMQ.h" 
#include "RemoteIds.h"
#include "StartMsg.h"
#include "StatusMsg.h"
#include "ResultMsg.h"

// TODO
/// @brief NetworkMgr sends and receives data using a DelegateMQ transport implemented
/// with Windows UDP sockets and msg_serialize. Class is thread safe. All public APIs are 
/// asynchronous.
/// 
/// @details NetworkMgr inherits from NetworkEngine, which manages the internal thread 
/// of control. All public APIs are asynchronous (blocking and non-blocking). Register 
/// with OnError or OnSendStatus to handle success or errors.
/// 
/// The underlying UDP transport layer managed by NetworkEngine is accessed only by a 
/// single internal thread. Therefore, when invoking a remote delegate, the call is 
/// automatically dispatched to the internal NetworkEngine thread.
/// 
/// **Key Responsibilities:**
/// * **Asynchronous Communication:** Exposes a fully thread-safe, asynchronous public API for network operations, 
///   utilizing an internal thread managed by `NetworkEngine` to handle all I/O.
/// * **Transport Abstraction:** Implements specific UDP transport logic (using Windows or Linux sockets) while abstracting 
///   these details from the application logic. Two sockets are created: one for sending and one for receiving.
/// * **Message Dispatching:** Automatically marshals all outgoing remote delegate invocations to the internal 
///   network thread, ensuring safe single-threaded access to the underlying UDP resources.
/// * **Invocation Modes:** Support for three distinct remote invocation patterns:
///     1. *Fire-and-Forget (Non-blocking):* Sends messages immediately without waiting for confirmation.
///     2. *Synchronous Wait (Blocking):* Blocks the calling thread until an acknowledgment (ACK) is received or a timeout occurs.
///     3. *Future-based:* Returns a `std::future` immediately, allowing retrieval of the result at a later time.
/// * **Error & Status Reporting:** Provides registration points (`OnNetworkError`, `OnSendStatus`) for clients to subscribe 
///   to transmission results and error notifications.
class TestServer : public NetworkEngine
{
public:
    // Public Signal Types
    // Clients Connect() to these safely using RAII.
    using StartSignal = dmq::SignalSafe<void(StartMsg&)>;
    using StatusSignal = dmq::SignalSafe<void(StatusMsg&)>;
    using ResultSignal = dmq::SignalSafe<void(ResultMsg&)>;

    using ErrorSignal = dmq::SignalSafe<void(dmq::DelegateRemoteId, dmq::DelegateError, dmq::DelegateErrorAux)>;
    using SendStatusSignal = dmq::SignalSafe<void(dmq::DelegateRemoteId, uint16_t, TransportMonitor::Status)>;

    static inline std::shared_ptr<ErrorSignal>      OnNetworkError = std::make_shared<ErrorSignal>();
    static inline std::shared_ptr<SendStatusSignal> OnSendStatus = std::make_shared<SendStatusSignal>();

    static inline std::shared_ptr<StartSignal>      OnStart = std::make_shared<StartSignal>();
    static inline std::shared_ptr<StatusSignal>     OnStatus = std::make_shared<StatusSignal>();
    static inline std::shared_ptr<ResultSignal>     OnResult = std::make_shared<ResultSignal>();

    static TestServer& Instance() { static TestServer instance; return instance; }

    int Create();

    // Send Functions (non-blocking)
    void SendStartMsg(StartMsg& msg);
    void SendStatusMsg(StatusMsg& msg);
    void SendResultMsg(ResultMsg& msg);

protected:
    // Override base class hooks to fire our static Signals
    void OnError(dmq::DelegateRemoteId id, dmq::DelegateError error, dmq::DelegateErrorAux aux) override;
    //void OnStatus(dmq::DelegateRemoteId id, uint16_t seq, TransportMonitor::Status status) override;  TODO

private:
    TestServer();
    ~TestServer() = default;

    // Specific endpoints
    RemoteEndpoint<dmq::MulticastDelegateSafe<void(StartMsg&)>, void(StartMsg&)> m_startMsgDel;
    RemoteEndpoint<dmq::MulticastDelegateSafe<void(StatusMsg&)>, void(StatusMsg&)> m_statusMsgDel;
    RemoteEndpoint<dmq::MulticastDelegateSafe<void(ResultMsg&)>, void(ResultMsg&)> m_resultMsgDel;
};

#endif