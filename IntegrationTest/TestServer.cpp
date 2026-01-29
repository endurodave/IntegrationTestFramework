#include "TestServer.h"

using namespace dmq;
using namespace std;

TestServer::TestServer()
    : m_startMsgDel(ids::START_MSG_ID, &m_dispatcher),
    m_statusMsgDel(ids::STATUS_MSG_ID, &m_dispatcher),
    m_resultMsgDel(ids::RESULT_MSG_ID, &m_dispatcher)
{
}

int TestServer::Create()
{
    // Bind signals
    m_startMsgDel.Bind(OnStart.get(), &SignalSafe<void(StartMsg&)>::operator(), ids::START_MSG_ID);
    m_statusMsgDel.Bind(OnStatus.get(), &SignalSafe<void(StatusMsg&)>::operator(), ids::STATUS_MSG_ID);
    m_resultMsgDel.Bind(OnResult.get(), &SignalSafe<void(ResultMsg&)>::operator(), ids::RESULT_MSG_ID);

    // Register for error callbacks
    m_startMsgDel.OnError += MakeDelegate(this, &TestServer::OnError);
    m_statusMsgDel.OnError += MakeDelegate(this, &TestServer::OnError);
    m_resultMsgDel.OnError += MakeDelegate(this, &TestServer::OnError);

    // Register endpoints with the Base Engine (So Incoming() can find them)
    RegisterEndpoint(ids::START_MSG_ID, &m_startMsgDel);
    RegisterEndpoint(ids::STATUS_MSG_ID, &m_statusMsgDel);
    RegisterEndpoint(ids::RESULT_MSG_ID, &m_resultMsgDel);

    // Initialize Base Engine
#ifdef SERVER_APP
    // Server Publishes on 50000, Listens on 50001
    return Initialize("127.0.0.1", 50000, "127.0.0.1", 50001);
#else
    // Client Publishes on 50001, Listens on 50000
    return Initialize("127.0.0.1", 50001, "127.0.0.1", 50000);
#endif
}

// Override hooks to fire signals
void TestServer::OnError(DelegateRemoteId id, DelegateError error, DelegateErrorAux aux) {
    if (OnNetworkError) (*OnNetworkError)(id, error, aux);
}

//TODO
//void TestServer::OnStatus(DelegateRemoteId id, uint16_t seq, TransportMonitor::Status status) {
//    if (OnSendStatus) (*OnSendStatus)(id, seq, status);
//}

void TestServer::SendStartMsg(StartMsg& msg) {
    if (Thread::GetCurrentThreadId() != m_thread.GetThreadId())
        return MakeDelegate(this, &TestServer::SendStartMsg, m_thread)(msg);
    m_startMsgDel(msg);
}

void TestServer::SendStatusMsg(StatusMsg& msg) {
    if (Thread::GetCurrentThreadId() != m_thread.GetThreadId())
        return MakeDelegate(this, &TestServer::SendStatusMsg, m_thread)(msg);
    m_statusMsgDel(msg);
}

void TestServer::SendResultMsg(ResultMsg& msg) {
    if (Thread::GetCurrentThreadId() != m_thread.GetThreadId())
        return MakeDelegate(this, &TestServer::SendResultMsg, m_thread)(msg);
    m_resultMsgDel(msg);
}

