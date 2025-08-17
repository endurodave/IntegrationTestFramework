#ifndef _LOG_DATA_H
#define _LOG_DATA_H

#include <string>
#include <list>
#include <chrono>
#include "IT_Client.h"

/// @brief LogData stores log data strings. LogData is not thread-safe. Must only 
/// be called on the Logger thread of control.
class LogData
{
public:
#ifdef IT_ENABLE
    dmq::MulticastDelegateSafe<void(std::chrono::milliseconds)> FlushTimeDelegate;
#endif

    LogData() {}
    ~LogData() { m_msgData.clear(); }

    /// Write log data
    /// @param[in] msg - data to log
    void Write(const std::string& msg);

    /// Flush log data to disk
    /// @return True if success. 
    bool Flush();

private:
    IT_PRIVATE_ACCESS :

    /// List to hold log data messages
    std::list<std::string> m_msgData;
};

#endif