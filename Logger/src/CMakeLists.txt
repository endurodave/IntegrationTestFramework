# Collect all .cpp files in this subdirectory
file(GLOB SUBDIR_SOURCES "*.cpp")

# Collect all .h files in this subdirectory
file(GLOB SUBDIR_HEADERS "*.h")

# Create a library target 
add_library(LoggerLib STATIC ${SUBDIR_SOURCES} ${SUBDIR_HEADERS})

# Include directories for the library
target_include_directories(LoggerLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
