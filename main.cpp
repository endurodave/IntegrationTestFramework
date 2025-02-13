// @see https://github.com/endurodave/IntegrationTestFramework
// David Lafreniere, Oct 2024.
//
// The integration test framework relies upon two external libraries.
// 
// Google Test: 
// https://github.com/google/googletest
// 
// Asynchronous Multicast Delgates: 
// https://github.com/endurodave/AsyncMulticastDelegateModern
//
// See CMakeLists.txt for build instructions.
// 
// Logger is the hypothetical production subsystem under test. Code marked within IT_ENABLE 
// conditional compile is the code necessary to support integration testing of the 
// production code. 

#include "Logger.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <utility>

#ifdef IT_ENABLE
#include "IntegrationTest.h"
extern void Logger_IT_ForceLink();
using namespace dmq;
#endif

using namespace std;

#ifdef IT_ENABLE
std::atomic<bool> processTimerExit = false;
static void ProcessTimers()
{
	while (!processTimerExit.load())
	{
		// Process all delegate-based timers
		Timer::ProcessTimers();
		std::this_thread::sleep_for(std::chrono::microseconds(50));
	}
}
#endif

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
int main(void)
{
#ifdef IT_ENABLE
	// Start the thread that will run ProcessTimers
	std::thread timerThread(ProcessTimers);

	// Dummy function call to prevent linker from discarding Logger_IT code
	Logger_IT_ForceLink();

	IntegrationTest::GetInstance();
#endif

	// Instantiate subsystems
	Logger::GetInstance();

#ifdef IT_ENABLE
	// Wait for integration tests to complete
	while (!IntegrationTest::GetInstance().IsComplete())
		this_thread::sleep_for(std::chrono::seconds(1));

	// Ensure the timer thread completes before main exits
	processTimerExit.store(true);
	if (timerThread.joinable())
		timerThread.join();
#endif

	return 0;
}

