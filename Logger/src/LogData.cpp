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
    // Write log data to disk
    std::ofstream logFile("LogData.txt", std::ios::app);
    if (logFile.is_open()) 
    {
        for (const std::string& str : m_msgData) 
        {
            logFile << str << std::endl;
        }
        logFile.close();
        return true;
    }
    return false;
}