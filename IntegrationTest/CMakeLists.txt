# Collect all .cpp files in this subdirectory
file(GLOB SUBDIR_SOURCES "*.cpp")

# Collect all .h files in this subdirectory
file(GLOB SUBDIR_HEADERS "*.h")

# Create a library target 
add_library(IntegrationTestLib STATIC ${SUBDIR_SOURCES} ${SUBDIR_HEADERS})

# Include directories for the library
target_include_directories(IntegrationTestLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(IntegrationTestLib LINK_PUBLIC 
    	PortLib
)