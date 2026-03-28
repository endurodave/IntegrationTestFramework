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
#include <filesystem>
#include "IT_Util.h"		// Include this last

using namespace std;
using namespace std::chrono;
using namespace dmq;

// Local integration test variables
static SignalThread signalThread;
static vector<string> callbackStatus;
static milliseconds flushDuration;
static mutex mtx;

// State for WriteSequence test
static atomic<int> writeSeqCount{ 0 };
static int writeSeqTarget = 0;
static SignalThread writeSeqSignal;

// State for ConcurrentWrites test
static atomic<int> concurrentWriteCount{ 0 };
static int concurrentWriteTarget = 0;
static SignalThread concurrentDoneSignal;

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

// Logger callback for WriteSequence test. Records Write success! callbacks into
// callbackStatus and signals once writeSeqTarget writes have been confirmed.
// Flush callbacks from the Logger timer are ignored to avoid signal interference.
void WriteSeqCb(const string& status)
{
	if (status != "Write success!")
		return;

	{
		lock_guard<mutex> lock(mtx);
		callbackStatus.push_back(status);
	}

	if (++writeSeqCount == writeSeqTarget)
		writeSeqSignal.SetSignal();
}

// Logger callback for ConcurrentWrites test. Counts Write success! callbacks
// and signals once all concurrentWriteTarget writes have been confirmed.
void ConcurrentWriteCb(const string& status)
{
	if (status == "Write success!")
		if (++concurrentWriteCount == concurrentWriteTarget)
			concurrentDoneSignal.SetSignal();
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
		Logger::GetInstance(),				// Thread to invoke Flush (Logger is-a Thread)
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

// Verify that callbacks from multiple Write operations arrive in the correct sequence.
// Fires 3 writes without waiting between them, then confirms the Logger thread
// delivered all 3 callbacks in FIFO order — the same order the writes were issued.
// Flush callbacks from the timer are filtered out by WriteSeqCb to avoid
// interference with the sequence count.
TEST(Logger_IT, WriteSequence)
{
	writeSeqCount = 0;
	writeSeqTarget = 3;
	{
		lock_guard<mutex> lock(mtx);
		callbackStatus.clear();
	}

	Logger::GetInstance().SetCallback(&WriteSeqCb);

	// Queue all 3 writes without waiting between them
	Logger::GetInstance().Write("First");
	Logger::GetInstance().Write("Second");
	Logger::GetInstance().Write("Third");

	// Wait until all 3 write callbacks have arrived from the Logger thread
	EXPECT_TRUE(writeSeqSignal.WaitForSignal(1000));

	{
		lock_guard<mutex> lock(mtx);

		// Verify all 3 write callbacks arrived in sequence.
		// The Logger's FIFO message queue guarantees the callbacks are
		// delivered in the same order the Write calls were submitted.
		ASSERT_EQ(callbackStatus.size(), 3u);
		EXPECT_EQ(callbackStatus[0], "Write success!");
		EXPECT_EQ(callbackStatus[1], "Write success!");
		EXPECT_EQ(callbackStatus[2], "Write success!");
	}

	Logger::GetInstance().SetCallback(nullptr);
}

// Verify that a Flush failure is correctly detected and that buffered log data
// is preserved across the failure for a subsequent retry.
// Injects a fault by removing write permission from the log file, confirms
// Flush returns false, restores permissions, then confirms Flush recovers.
TEST(Logger_IT, FlushFaultInjection)
{
	namespace fs = std::filesystem;

	// Ensure LogData.txt exists on disk so permissions can be applied to it
	auto ensureRet = AsyncInvoke(
		&Logger::GetInstance().m_logData,
		&LogData::Flush,
		Logger::GetInstance(),
		milliseconds(100));
	ASSERT_TRUE(ensureRet.has_value());

	// Write a data entry that will be held in the buffer during the fault
	AsyncInvoke(
		&Logger::GetInstance().m_logData,
		&LogData::Write,
		Logger::GetInstance(),
		milliseconds(50),
		string("Fault injection test data"));

	// Inject fault: remove write permission from the log file
	std::error_code ec;
	fs::permissions("LogData.txt",
		fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write,
		fs::perm_options::remove, ec);
	ASSERT_FALSE(ec) << "Failed to remove write permission: " << ec.message();

	// Flush must fail because the file is now read-only
	auto failRet = AsyncInvoke(
		&Logger::GetInstance().m_logData,
		&LogData::Flush,
		Logger::GetInstance(),
		milliseconds(100));
	EXPECT_TRUE(failRet.has_value());
	if (failRet.has_value())
		EXPECT_FALSE(failRet.value());	// Flush should return false on fault

	// Restore write permission to recover from the fault
	fs::permissions("LogData.txt", fs::perms::owner_write, fs::perm_options::add, ec);
	ASSERT_FALSE(ec) << "Failed to restore write permission: " << ec.message();

	// Buffered data must have survived the failed flush - retry must succeed.
	// LogData::Flush does not clear m_msgData on failure, preserving the entry.
	auto retryRet = AsyncInvoke(
		&Logger::GetInstance().m_logData,
		&LogData::Flush,
		Logger::GetInstance(),
		milliseconds(100));
	EXPECT_TRUE(retryRet.has_value());
	if (retryRet.has_value())
		EXPECT_TRUE(retryRet.value());	// Flush should succeed after recovery
}

// Stress test that fires many Write calls without waiting between them and
// verifies the Logger thread processes every one without dropping a callback.
// Demonstrates that the Logger's message queue handles concurrent producer load
// and that every callback is delivered exactly once.
TEST(Logger_IT, ConcurrentWrites)
{
	const int NUM_WRITES = 20;
	concurrentWriteCount = 0;
	concurrentWriteTarget = NUM_WRITES;

	Logger::GetInstance().SetCallback(&ConcurrentWriteCb);

	// Fire all writes without waiting — stress the Logger's message queue
	for (int i = 0; i < NUM_WRITES; i++)
		Logger::GetInstance().Write("ConcurrentWrite " + to_string(i));

	// Wait until all NUM_WRITES callbacks have been received
	EXPECT_TRUE(concurrentDoneSignal.WaitForSignal(2000))
		<< "Timed out: only " << concurrentWriteCount.load()
		<< " of " << NUM_WRITES << " write callbacks received";

	// Every write must have produced exactly one callback
	EXPECT_EQ(concurrentWriteCount.load(), NUM_WRITES);

	Logger::GetInstance().SetCallback(nullptr);
}

// Dummy function to force linker to keep the code in this file
void Logger_IT_ForceLink() { }