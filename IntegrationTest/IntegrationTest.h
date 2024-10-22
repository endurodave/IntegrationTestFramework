// @see https://github.com/endurodave/IntegrationTestFramework
// David Lafreniere, Oct 2024.

#ifndef _INTEGRATION_TEST_H
#define _INTEGRATION_TEST_H

#include "WorkerThreadStd.h"
#include "Timer.h"
#include <atomic>

// The IntegrationTest class executes all integration tests created using the 
// Google Test framework on a private internal thread of control. 
class IntegrationTest
{
public:
	/// Get singleton instance of this class
	static IntegrationTest& GetInstance();

	// Return true if integration test is complete
	std::atomic<bool>& IsComplete() { return m_complete; }

private:
	IntegrationTest();
	~IntegrationTest();

	// Called to run all integration tests
	void Run();

	// The integration test worker thread that executes Google Test
	WorkerThread m_thread;

	// Timer to start integration tests
	Timer m_timer;

	std::atomic<bool> m_complete = false;
};

#endif