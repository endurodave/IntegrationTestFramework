# Example CMake command line to create project build files:
#
# *** Windows ***
# cmake -G "Visual Studio 17 2022" -A Win32 -B Build -S . -DENABLE_IT=ON
#
# *** Linux ***
# cmake -G "Unix Makefiles" -B Build -S . -DENABLE_IT=ON

# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language (C or C++)
project(IntegrationTestFramework VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ensure all libraries use the dynamically linked runtime (/MDd for Debug, /MD for Release)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")

# Force GoogleTest to use the dynamic runtime libraries
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Collect all .cpp and *.h source files in the current directory
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/*.cpp" "${CMAKE_SOURCE_DIR}/*.h")

# Collect all header files in the Delegate directory
file(GLOB Delegate_HEADERS "${CMAKE_SOURCE_DIR}/Delegate/*.h")

# Organize Delegate headers into a "Delegate Files" folder in Visual Studio
source_group("Delegate Files" FILES ${Delegate_HEADERS})

# Platform-specific linker flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang: Use --no-as-needed to prevent the removal of unused libraries
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")
endif()

# Define the IT_ENABLE macro for the IntegrationTestFrameworkApp target
if (ENABLE_IT)
    add_compile_definitions(IT_ENABLE)
endif()

# Add subdirectories to include path
include_directories( 
    ${CMAKE_SOURCE_DIR}/Logger/src
    ${CMAKE_SOURCE_DIR}/Port/src
)

# Add subdirectories to include path if building integration tests
if (ENABLE_IT)
    include_directories(
        ${CMAKE_SOURCE_DIR}/Logger/it
        ${CMAKE_SOURCE_DIR}/Delegate
        ${CMAKE_SOURCE_DIR}/IntegrationTest
        ${CMAKE_SOURCE_DIR}/GoogleTest/googletest/include
    )
endif()

# Add an executable target
add_executable(IntegrationTestFrameworkApp ${SOURCES} ${Delegate_HEADERS})

# Add subdirectories to build (product related code)
add_subdirectory(Logger/src)
add_subdirectory(Port/src)

# Add subdirectories to build (integration test related code)
if (ENABLE_IT)
    add_subdirectory(Logger/it)
    add_subdirectory(IntegrationTest)
    add_subdirectory(GoogleTest)
endif()

target_link_libraries(IntegrationTestFrameworkApp PRIVATE 
    LoggerLib
    PortLib
)

if (ENABLE_IT)
    target_link_libraries(IntegrationTestFrameworkApp PRIVATE 
        Logger_ITLib
        IntegrationTestLib
        gtest
        gtest_main
    )
endif()