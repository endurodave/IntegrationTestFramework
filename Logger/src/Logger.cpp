#include "Logger.h"
#include "Fault.h"
#include "DelegateLib.h"

using namespace std;

// TODO C++ enum
// Worker thread message ID's
#define MSG_WRITE				1
#define MSG_EXIT_THREAD			2
#define MSG_TIMER				3
#define MSG_DISPATCH_DELEGATE	4

// Base class for all thread queue messages
class Msg
{
public:
	Msg(int id) : m_id(id) { }
	int GetId() const { return m_id; }

private:
	const int m_id;
};

// Log message sent through worker thread message queue
class LogMsg : public Msg
{
public:
	LogMsg(int id, const std::string& data) : Msg(id), m_data(data) { }
	const std::string& GetMsg() { return m_data; }

private:
	const std::string m_data;
};

// Delegate message sent through worker thread message queue
class DelegateMsg : public Msg
{
public:
	DelegateMsg(int id, std::shared_ptr<DelegateLib::DelegateMsgBase> data) : Msg(id), m_data(data) { }
	std::shared_ptr<DelegateLib::DelegateMsgBase> GetMsg() { return m_data; }

private:
	std::shared_ptr<DelegateLib::DelegateMsgBase> m_data;
};

//----------------------------------------------------------------------------
// GetInstance
//----------------------------------------------------------------------------
Logger& Logger::GetInstance()
{
	static Logger instance;
	return instance;
}

//----------------------------------------------------------------------------
// Logger
//----------------------------------------------------------------------------
Logger::Logger() : m_thread(nullptr), m_timerExit(false), THREAD_NAME("LoggerThread")
{
	CreateThread();
}

//----------------------------------------------------------------------------
// ~Logger
//----------------------------------------------------------------------------
Logger::~Logger()
{
	ExitThread();
}

//----------------------------------------------------------------------------
// Write
//----------------------------------------------------------------------------
void Logger::Write(const std::string& msg)
{
	ASSERT_TRUE(m_thread);

	// Create a write log message
	std::shared_ptr<LogMsg> logMsg(new LogMsg(MSG_WRITE, msg));

	// Add message to queue and notify worker thread
	std::unique_lock<std::mutex> lk(m_mutex);
	m_queue.push(logMsg);
	m_cv.notify_one();
}

//----------------------------------------------------------------------------
// CreateThread
//----------------------------------------------------------------------------
bool Logger::CreateThread()
{
	if (!m_thread)
		m_thread = std::unique_ptr<std::thread>(new thread(&Logger::Process, this));
	return true;
}

//----------------------------------------------------------------------------
// GetThreadId
//----------------------------------------------------------------------------
std::thread::id Logger::GetThreadId()
{
	ASSERT_TRUE(m_thread != nullptr);
	return m_thread->get_id();
}

//----------------------------------------------------------------------------
// GetCurrentThreadId
//----------------------------------------------------------------------------
std::thread::id Logger::GetCurrentThreadId()
{
	return this_thread::get_id();
}

//----------------------------------------------------------------------------
// ExitThread
//----------------------------------------------------------------------------
void Logger::ExitThread()
{
	if (!m_thread)
		return;

	// Create an exit thread message
	std::shared_ptr<Msg> msg(new Msg(MSG_EXIT_THREAD));

	// Put exit thread message into the queue
	{
		lock_guard<mutex> lock(m_mutex);
		m_queue.push(msg);
		m_cv.notify_one();
	}

    m_thread->join();
    m_thread = nullptr;
}

//----------------------------------------------------------------------------
// DispatchDelegate
//----------------------------------------------------------------------------
void Logger::DispatchDelegate(std::shared_ptr<DelegateLib::DelegateMsgBase> msg)
{
	ASSERT_TRUE(m_thread);

	// Create a new ThreadMsg
	std::shared_ptr<DelegateMsg> threadMsg(new DelegateMsg(MSG_DISPATCH_DELEGATE, msg));

	// Add dispatch delegate msg to queue and notify worker thread
	std::unique_lock<std::mutex> lk(m_mutex);
	m_queue.push(threadMsg);
	m_cv.notify_one();
}

//----------------------------------------------------------------------------
// TimerThread
//----------------------------------------------------------------------------
void Logger::TimerThread()
{
	// Generate timer messages until exit
    while (!m_timerExit)
    {
		// Insert a timer message every 1000ms
        std::this_thread::sleep_for(1000ms);

		// Create a timer message
        std::shared_ptr<Msg> msg(new Msg(MSG_TIMER));

        // Add timer message to queue and notify worker thread
        std::unique_lock<std::mutex> lk(m_mutex);
        m_queue.push(msg);
        m_cv.notify_one();
    }
}

//----------------------------------------------------------------------------
// Process
//----------------------------------------------------------------------------
void Logger::Process()
{
    m_timerExit = false;
    std::thread timerThread(&Logger::TimerThread, this);

	while (1)
	{
		std::shared_ptr<Msg> msg;
		{
			// Wait for a message to be added to the queue
			std::unique_lock<std::mutex> lk(m_mutex);
			while (m_queue.empty())
				m_cv.wait(lk);

			if (m_queue.empty())
				continue;

			msg = m_queue.front();
			m_queue.pop();
		}

		switch (msg->GetId())
		{
			case MSG_WRITE:
			{
				// Cast base pointer to LogMsg
				std::shared_ptr<LogMsg> logMsg = std::static_pointer_cast<LogMsg>(msg);

				// Write log data
				m_logData.Write(logMsg->GetMsg());

				// Notify client of success
				if (m_pLoggerStatusCb)
					m_pLoggerStatusCb("Write success!");
				break;
			}

            case MSG_TIMER:
			{
				// Flush data to disk when timer expires
				bool success = m_logData.Flush();
				if (success)
				{
					// Notify client of success
					if (m_pLoggerStatusCb)
						m_pLoggerStatusCb("Flush success!");
				}
				else
				{
					// Notify client of failure
					if (m_pLoggerStatusCb)
						m_pLoggerStatusCb("Flush failure!");
				}
				break;
			}

			case MSG_DISPATCH_DELEGATE:
			{
				// Cast pointer to a DelegateMsg
				std::shared_ptr<DelegateMsg> delegateMsg = std::static_pointer_cast<DelegateMsg>(msg);

				// Get the delegate message base
				auto delegateMsgBase = delegateMsg->GetMsg();

				// Invoke the delegate target function on the target thread context
				delegateMsgBase->GetDelegateInvoker()->DelegateInvoke(delegateMsgBase);
				break;
			}

			case MSG_EXIT_THREAD:
			{
                m_timerExit = true;
                timerThread.join();
                return;
			}

			default:
				ASSERT();
		}
	}
}

