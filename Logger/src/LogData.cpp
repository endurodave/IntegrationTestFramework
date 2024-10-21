#include "LogData.h"
#include <iostream>
#include <fstream>
#include <list>
#include <string>

using namespace std;

//----------------------------------------------------------------------------
// Write
//----------------------------------------------------------------------------
void LogData::Write(const std::string& msg)
{
	m_msgData.push_back(msg);
}

//----------------------------------------------------------------------------
// Flush
//----------------------------------------------------------------------------
bool LogData::Flush()
{
#ifdef IT_ENABLE
    auto startTime = std::chrono::high_resolution_clock::now();
#endif

    // Write log data to disk
    std::ofstream logFile("LogData.txt", std::ios::app);
    if (logFile.is_open()) 
    {
        for (const std::string& str : m_msgData) 
        {
            logFile << str << std::endl;
        }
        logFile.close();

#ifdef IT_ENABLE
        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // Callback integration test with elapsed time
        FlushTimeDelegate(elapsedTime);
#endif
        return true;
    }
    return false;
}