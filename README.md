# Integration Test Framework using Google Test and Delegates
An integration test framework used for testing multi-threaded C++ based projects using Google Test and Asynchronous Multicast Delegate libraries.

## Overview
Testing mission critical software is crucial. For medical devices, the IEC 62304 provided guidance on the required software development process. Three levels of software testing are defined:

1. **Unit Testing** - Verifies the correctness of individual units of code in isolation.
2. **Integration Testing** - Tests how different units of code interact and work together.
3. **System Testing** - Evaluates the entire system as a whole to ensure it meets specified requirements.

Unit testing is well understood, with numerous frameworks (including Google Test) readily available and easily incorporated into a CI/CD toolchain pipeline. System testing is also well-documented, with various techniques and tools available. However, in my experience, integration testing is more challenging to write and integrate into an automated build toolchain.

This project implements an integration testing framework for multi-threaded C++ applications, compatible with any system supporting C++17 or higher, including Windows and Linux. It has no external OS dependencies or libraries beyond the standard C++ library. Multi-threading is achieved using the C++ thread support library, eliminating the need for OS-specific dependencies.

## Logger Subsystem
The `Logger` class is the subsystem public interface. `Logger` executes in its own thread of control. The `Write()` API is thread-safe. 

* `void Write(const std::string& msg)`

`Logger` has an internal instance of `LogData` that performs the underlying logging. `LogData` is not thread safe and must only execute within the `Logger` thread context.

* `void Write(const std::string& msg);`
* `bool Flush();`

The project goal is to create integration tests to test the entire `Logger` subsystem operating in a multi-threaded environment. 

## Source Code
The project contains the following directories:

* **Delegate** - the Delegate library source code directory
* **GoogleTest** - the Google Test library source code directory
* **IntegrationTest** - the integration test framework source code
* **Logger** - the Logger subsystem under test
* **Logger/it** - the Logger subsystem integration test source code
* **Logger/src** - the Logger subsystem implementation source code
* **Port** - supporting utilities source code files

## Build
See CMakeLists.txt file for how to build the project.