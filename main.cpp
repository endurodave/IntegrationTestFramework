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
using namespace DelegateLib;
#endif

using namespace std;

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
int main(void)
{
#ifdef IT_ENABLE
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
#endif

	return 0;
}

