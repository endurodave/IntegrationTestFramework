// Integration tests for the Logger subsystem
// 
// @see https://github.com/endurodave/IntegrationTestFramework
// David Lafreniere, Oct 2024.
//
// All tests run within the IntegrationTest thread context. Logger subsystem runs 
// within the Logger thread context. The Delegate library is used to invoke 
// functions across thread boundaries. The Google Test library is used to execute 
// tests and collect results.

#include "Logger.h"
#include "DelegateMQ.h"
#include "SignalThread.h"
#include "IT_Util.h"		// Include this last

using namespace std;
using namespace std::chrono;
using namespace dmq;

// Local integration test variables
static SignalThread signalThread;
static vector<string> callbackStatus;
static milliseconds flushDuration;
static mutex mtx;

// Logger callback handler function invoked from Logger thread context
void FlushTimeCb(milliseconds duration)
{
	// Protect flushTime against multiple thread access by IntegrationTest 
	// thread and Logger thread
	lock_guard<mutex> lock(mtx);

	// Save the flush time
	flushDuration = duration;
}

// Logger callback handler function invoked from Logger thread context
void LoggerStatusCb(const string& status)
{
	// Protect callbackStatus against multiple thread access by IntegrationTest 
	// thread and Logger thread
	lock_guard<mutex> lock(mtx);

	// Save logger callback status
	callbackStatus.push_back(status);

	// Signal the waiting thread to continue
	signalThread.SetSignal();
}

// Test the Logger::Write() subsystem public API. 
TEST(Logger_IT, Write) 
{
	// Register to receive a Logger status callback
	Logger::GetInstance().SetCallback(&LoggerStatusCb);

	// Write a Logger string value using public API
	Logger::GetInstance().Write("LoggerTest, Write");

	// Wait for LoggerStatusCb callback up to 500mS
	bool success = signalThread.WaitForSignal(500);

	// Wait for 2nd LoggerStatusCb callback up to 2 seconds
	bool success2 = signalThread.WaitForSignal(2000);

	// Check test results
	EXPECT_TRUE(success);
	EXPECT_TRUE(success2);

	{
		// Protect access to callbackStatus
		lock_guard<mutex> lock(mtx);

		EXPECT_EQ(callbackStatus.size(), 2);
		if (callbackStatus.size() >= 2)
		{
			EXPECT_EQ(callbackStatus[0], "Write success!");
			EXPECT_EQ(callbackStatus[1], "Flush success!");
		}
	}

	// Test cleanup
	Logger::GetInstance().SetCallback(nullptr);
}

// Test LogData::Flush() subsystem internal class. The internal LogData class is 
// not normally called directly by client code because it is not thread-safe. 
// However, the Delegate library easily calls functions on the Logger thread context.
TEST(Logger_IT, Flush)
{
	// Create an asynchronous blocking delegate targeted at the LogData::Flush function
	auto flushAsyncBlockingDelegate = MakeDelegate(
		&Logger::GetInstance().m_logData,	// LogData object within Logger class
		&LogData::Flush,					// LogData function to invoke
		Logger::GetInstance(),				// Thread to invoke Flush (Logger is-a DelegateThread)
		milliseconds(100));					// Wait up to 100mS for Flush function to be called

	// Invoke LogData::Flush on the Logger thread and obtain the return value
	std::optional<bool> retVal = flushAsyncBlockingDelegate.AsyncInvoke();

	// Check test results
	EXPECT_TRUE(retVal.has_value()); // Did async LogData::Flush function call succeed?
	if (retVal.has_value())
		EXPECT_TRUE(retVal.value()); // Did LogData::Flush return true?
}

// Test LogData::Flush executes in under 10mS
TEST(Logger_IT, FlushTime)
{
	{
		// Protect access to flushDuration
		lock_guard<mutex> lock(mtx);
		flushDuration = milliseconds(-1);
	}

	// Register for a callback from Logger thread
	Logger::GetInstance().m_logData.FlushTimeDelegate += MakeDelegate(&FlushTimeCb);

	// Clear the m_msgData list on Logger thread
	auto retVal1 = MakeDelegate(
		&Logger::GetInstance().m_logData.m_msgData,	// Object instance
		&std::list<std::string>::clear,				// Object function
		Logger::GetInstance(),						// Thread to invoke object function
		milliseconds(50)).AsyncInvoke();

	// Check asynchronous function call succeeded
	EXPECT_TRUE(retVal1.has_value());

	// Write 10 lines of log data
	for (int i = 0; i < 10; i++)
	{
		//  Call LogData::Write on Logger thread
		auto retVal = MakeDelegate(
			&Logger::GetInstance().m_logData,
			&LogData::Write,
			Logger::GetInstance(),
			milliseconds(50)).AsyncInvoke("Flush Timer String");

		// Check asynchronous function call succeeded
		EXPECT_TRUE(retVal.has_value());

		// Check that LogData::Write returned true
		if (retVal.has_value())
			EXPECT_TRUE(retVal.value());
	}

	// Call LogData::Flush on Logger thread
	auto retVal2 = MakeDelegate(
		&Logger::GetInstance().m_logData,
		&LogData::Flush,
		Logger::GetInstance(),
		milliseconds(100)).AsyncInvoke();

	// Check asynchronous function call succeeded
	EXPECT_TRUE(retVal2.has_value());
	if (retVal2.has_value())
		EXPECT_TRUE(retVal2.value());

	{
		// Protect access to flushDuration
		lock_guard<mutex> lock(mtx);

		// Check that flush executed in 10mS or less
		EXPECT_GE(flushDuration, std::chrono::milliseconds(0));
		EXPECT_LE(flushDuration, std::chrono::milliseconds(10));
	}

	// Unregister from callback
	Logger::GetInstance().m_logData.FlushTimeDelegate -= MakeDelegate(&FlushTimeCb);
}

// Exact same test as FlushTime above, but uses the AsyncInvoke helper function
// to simplify syntax and automatically check for async invoke errors.
TEST(Logger_IT, FlushTimeSimplified)
{
	{
		// Protect access to flushDuration
		lock_guard<mutex> lock(mtx);
		flushDuration = milliseconds(-1);
	}

	// Register for a callback from Logger thread
	Logger::GetInstance().m_logData.FlushTimeDelegate += MakeDelegate(&FlushTimeCb);

	// Clear the m_msgData list on Logger thread
	auto retVal1 = AsyncInvoke(
		&Logger::GetInstance().m_logData.m_msgData,	// Object instance
		&std::list<std::string>::clear,				// Object function
		Logger::GetInstance(),						// Thread to invoke object function
		milliseconds(50));							// Wait up to 50mS for async invoke

	// Write 10 lines of log data
	for (int i = 0; i < 10; i++)
	{
		//  Call LogData::Write on Logger thread
		auto retVal = AsyncInvoke(
			&Logger::GetInstance().m_logData,
			&LogData::Write,
			Logger::GetInstance(),
			milliseconds(50), 
			"Flush Timer String");

		// Check that LogData::Write returned true
		if (retVal.has_value())
			EXPECT_TRUE(retVal.value());  
	}

	// Call LogData::Flush on Logger thread
	auto retVal2 = AsyncInvoke(
		&Logger::GetInstance().m_logData,
		&LogData::Flush,	
		Logger::GetInstance(),
		milliseconds(100));

	{
		// Protect access to flushDuration
		lock_guard<mutex> lock(mtx);

		// Check that flush executed in 10mS or less
		EXPECT_GE(flushDuration, std::chrono::milliseconds(0));
		EXPECT_LE(flushDuration, std::chrono::milliseconds(10));
	}

	// Unregister from callback
	Logger::GetInstance().m_logData.FlushTimeDelegate -= MakeDelegate(&FlushTimeCb);
}


// Exact same test as FlushTimeSimplified above, but use a private lambda callback 
// function to centralize the callback inside the test case. 
TEST(Logger_IT, FlushTimeSimplifiedWithLambda)
{
	// Logger callback handler lambda function invoked from Logger thread context
	auto FlushTimeLambdaCb = +[](milliseconds duration) -> void
	{
		// Protect flushTime against multiple thread access by IntegrationTest 
		// thread and Logger thread
		lock_guard<mutex> lock(mtx);

		// Save the flush time
		flushDuration = duration;
	};

	{
		// Protect access to flushDuration
		lock_guard<mutex> lock(mtx);
		flushDuration = milliseconds(-1);
	}

	// Register for a callback from Logger thread
	Logger::GetInstance().m_logData.FlushTimeDelegate += MakeDelegate(FlushTimeLambdaCb);

	// Clear the m_msgData list on Logger thread
	auto retVal1 = AsyncInvoke(
		&Logger::GetInstance().m_logData.m_msgData,	// Object instance
		&std::list<std::string>::clear,				// Object function
		Logger::GetInstance(),						// Thread to invoke object function
		milliseconds(50));							// Wait up to 50mS for async invoke

	// Write 10 lines of log data
	for (int i = 0; i < 10; i++)
	{
		//  Call LogData::Write on Logger thread
		auto retVal = AsyncInvoke(
			&Logger::GetInstance().m_logData,
			&LogData::Write,
			Logger::GetInstance(),
			milliseconds(50),
			"Flush Timer String");

		// Check that LogData::Write returned true
		if (retVal.has_value())
			EXPECT_TRUE(retVal.value());
	}

	// Call LogData::Flush on Logger thread
	auto retVal2 = AsyncInvoke(
		&Logger::GetInstance().m_logData,
		&LogData::Flush,
		Logger::GetInstance(),
		milliseconds(100));

	{
		// Protect access to flushDuration
		lock_guard<mutex> lock(mtx);

		// Check that flush executed in 10mS or less
		EXPECT_GE(flushDuration, std::chrono::milliseconds(0));
		EXPECT_LE(flushDuration, std::chrono::milliseconds(10));
	}

	// Unregister from callback
	Logger::GetInstance().m_logData.FlushTimeDelegate -= MakeDelegate(FlushTimeLambdaCb);
}

// Dummy function to force linker to keep the code in this file
void Logger_IT_ForceLink() { }