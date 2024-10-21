#ifndef _LOGGER_H
#define _LOGGER_H

#include "LogData.h"
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "IT_Common.h"

class Msg;

class Logger : public DelegateLib::DelegateThread
{
public:
	typedef void (*LoggerStatusCb)(const std::string& status);

	/// Get the singleton logger instance
	static Logger& GetInstance();

	/// Write a message to the log
	/// @param[in] msg - the message string to write
	void Write(const std::string& msg);

	/// Register to receive a callback when the system mode changes. The callback
	/// will be invoked on the Logger::m_thread context. 
	/// @param[in] callbackFunc - a pointer to a callback function 
	void SetCallback(LoggerStatusCb callbackFunc)
	{
		m_pLoggerStatusCb = callbackFunc;
	}

	// TODO: virtual? public? 
	virtual void DispatchDelegate(std::shared_ptr<DelegateLib::DelegateMsgBase> msg);

private:
IT_PRIVATE_ACCESS:

	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	/// Constructor
	Logger();

	/// Destructor
	~Logger();

	/// Called once to create the worker thread
	/// @return TRUE if thread is created. FALSE otherise. 
	bool CreateThread();

	/// Called once at program exit to exit the worker thread
	void ExitThread();

	/// Get the ID of this thread instance
	std::thread::id GetThreadId();

	/// Get the ID of the currently executing thread
	static std::thread::id GetCurrentThreadId();

	/// Entry point for the thread
	void Process();

    /// Entry point for timer thread
    void TimerThread();

	/// Class to collect and save log data
	LogData m_logData;

	// Registered client callback function pointer
	LoggerStatusCb m_pLoggerStatusCb;

	std::unique_ptr<std::thread> m_thread;
	std::queue<std::shared_ptr<Msg>> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_cv;
    std::atomic<bool> m_timerExit;
	const char* THREAD_NAME;
};

#endif 

