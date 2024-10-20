// Integration tests for the Logger subsystem
// 
// @see https://github.com/endurodave/IntegrationTestFramework
// David Lafreniere, Oct 2024.

#include "Logger.h"

#include "DelegateLib.h"
#include "Signal.h"

// Prevent conflict with GoogleTest ASSERT_TRUE macro definition
#ifdef ASSERT_TRUE
#undef ASSERT_TRUE
#endif

#include <gtest/gtest.h>

using namespace std;
using namespace DelegateLib;

// Local integration test variables
static Signal signal;
static vector<string> callbackStatus;
static int mapIdx;

// Logger callback handler function.
void LoggerStatusCb(const string& status)
{
	// Save logger callback status
	callbackStatus.push_back(status);

	// Signal the waiting thread to continue
	signal.SetSignal();
}

// Test the Logger::Write() subsystem public API.
TEST(Logger_IT, Write) 
{
	// Register a Logger callback
	Logger::GetInstance().SetCallback(&LoggerStatusCb);

	// Write a Logger string value
	Logger::GetInstance().Write("LoggerTest, Write");

	// Wait for LoggerStatusCb callback up to 500mS
	bool success = signal.WaitForSignal(500);

	// Wait for LoggerStatusCb callback up to 2 seconds
	bool success2 = signal.WaitForSignal(2000);

	// Check test results
	EXPECT_TRUE(success);
	EXPECT_TRUE(success2);
	EXPECT_EQ(callbackStatus.size(), 2);
	if (callbackStatus.size() >= 2)
	{
		EXPECT_EQ(callbackStatus[0], "Write success!");
		EXPECT_EQ(callbackStatus[1], "Flush success!");
	}

	// Test cleanup
	Logger::GetInstance().SetCallback(nullptr);
}

// Test LogData::Flush() subsystem internal API. This API normally is 
// not called by client code because it is not thread-safe. However, using 
// Delegate library it can be invoked on the Logger's thread of control.
TEST(Logger_IT, Flush)
{
	// Create a asynchronous blocking delegate targeted at the Logger::Flush function
	auto flushAsyncBlockingDelegate = MakeDelegate(
		&Logger::GetInstance().m_logData,	// LogData object within Logger class
		&LogData::Flush,					// LogData function to invoke
		Logger::GetInstance(),				// Thread to invoke Flush (Logger is-a DelegateThread)
		chrono::milliseconds(500));			// Wait up to 500mS for Flush function to be called

	// Invoke LogData::Flush on the Logger thread and obtain the return value
	std::optional<bool> retVal = flushAsyncBlockingDelegate.AsyncInvoke();

	// Check test results
	EXPECT_TRUE(retVal.has_value()); // Did async LogData::Flush function call succeed?
	if (retVal.has_value())
	{
		EXPECT_TRUE(retVal.value()); // Did LogData::Flush return true?
	}
}

// Dummy function to force linker to keep the code in this file
void Logger_IT_ForceLink() { }