#include "DelegateLib.h"
#include "IntegrationTest.h"
#include "Logger.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <utility>
#include "WorkerThreadStd.h"

// @see https://github.com/endurodave/IntegrationTestFramework
// David Lafreniere, Oct 2024.

using namespace std;
using namespace DelegateLib;

extern void Logger_IT_ForceLink();

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
int main(void)
{
	// Dummy function call to prevent linker from discarding Logger_IT code
	Logger_IT_ForceLink();

	// Instantiate subsystems
	Logger::GetInstance();
	IntegrationTest::GetInstance();

	// Wait for integration tests to complete
	std::this_thread::sleep_for(std::chrono::seconds(3));
	return 0;
}

