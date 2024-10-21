#ifndef _SIGNAL_THREAD_H
#define _SIGNAL_THREAD_H

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class SignalThread
{
public:
    // Constructor initializes the signal to false
    SignalThread() : m_signalSet(false) { }

    // This function waits for the signal for a maximum of milliseconds. It
    // returns true if the signal was set within the timeout, false otherwise.
    // @param[in] ms - milliseconds to wait for signal
    // @return Returns true if signalled, false otherwise.
    bool WaitForSignal(int ms) 
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        // Wait until the signal is set or the timeout expires
        if (m_cv.wait_for(lock, std::chrono::milliseconds(ms), [this] { return m_signalSet; })) 
        {
            // Reset the signal
            m_signalSet = false; 

            // Signal was set within the timeout
            return true;
        }
        else 
        {
            // Timeout expired
            return false;
        }
    }

    // This function sets the signal and notifies the waiting thread.
    void SetSignal() 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_signalSet = true;
        m_cv.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_signalSet; // Shared state to indicate if the signal was set
};

#endif