#include "IntegrationTest.h"

// Prevent conflict with GoogleTest ASSERT_TRUE macro definition
#ifdef ASSERT_TRUE
#undef ASSERT_TRUE
#endif

#include <gtest/gtest.h>

//----------------------------------------------------------------------------
// GetInstance
//----------------------------------------------------------------------------
IntegrationTest& IntegrationTest::GetInstance()
{
    static IntegrationTest instance;
    return instance;
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
IntegrationTest::IntegrationTest() :
    m_thread("IntegrationTestThread")
{
    m_thread.CreateThread();

    // Start integration tests 500mS after system startup. Alteratively, 
    // create your own worker thread and call Run() directly.
    (*m_timer.Expired) += MakeDelegate(this, &IntegrationTest::Run, m_thread);
    m_timer.Start(std::chrono::milliseconds(500));
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
IntegrationTest::~IntegrationTest()
{
    (*m_timer.Expired) -= MakeDelegate(this, &IntegrationTest::Run, m_thread);
}

//----------------------------------------------------------------------------
// Run
//----------------------------------------------------------------------------
void IntegrationTest::Run()
{
    m_timer.Stop();

    // Initialize Google Test
    ::testing::InitGoogleTest();

    // Run all tests and return the result
    int retVal = RUN_ALL_TESTS();

    std::cout << "RUN_ALL_TESTS() return value: " << retVal << std::endl;

    m_complete = true;
}

