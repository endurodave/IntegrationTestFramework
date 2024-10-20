#ifndef _LOG_DATA_H
#define _LOG_DATA_H

#include <string>
#include <list>

/// @brief LogData stores log data. LogData is not thread-safe.
class LogData
{
public:
	/// Write log data
	/// @param[in] msg - data to log
	void Write(const std::string& msg);	

	/// Flush log data to disk
	/// @return True if success. 
	bool Flush();

private:
	/// List to hold log data messages
	std::list<std::string> m_logData;
};

#endif