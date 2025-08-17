#ifndef _IT_UTIL_H
#define _IT_UTIL_H

// Common utilities used within the integration test modules

// Prevent conflict with GoogleTest ASSERT_TRUE macro definition between 
// gtest.h and Fault.h
#ifdef ASSERT_TRUE
#undef ASSERT_TRUE
#endif

#include <chrono>
#include <utility>
#include <gtest/gtest.h>

// Helper function to simplify asynchronous function invoke within a test
template <typename C, typename F, typename T, typename M = std::chrono::milliseconds, typename... Args>
auto AsyncInvoke(C obj, F func, T& thread, M timeout, Args&&... args)
{
    // Asynchronously call target function and wait up to timeout milliseconds to complete
    auto retVal = MakeDelegate(obj, func, thread, timeout).AsyncInvoke(std::forward<Args>(args)...);

    // Check that the target function call succeeded within the timeout specified
    EXPECT_TRUE(retVal.has_value());

    return retVal;
}

#endif