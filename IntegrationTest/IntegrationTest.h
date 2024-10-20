// @see https://github.com/endurodave/IntegrationTestFramework
// David Lafreniere, Oct 2024.

#ifndef _INTEGRATION_TEST_H
#define _INTEGRATION_TEST_H

#include "WorkerThreadStd.h"
#include "Timer.h"

class IntegrationTest
{
public:
	/// Get singleton instance of this class
	static IntegrationTest& GetInstance();

private:
	IntegrationTest();
	~IntegrationTest();

	// Called to run all integration tests
	void Run();

	// The integration test worker thread that executes Google Test
	WorkerThread m_thread;

	// Timer to start integration tests
	Timer m_timer;
};

#endif