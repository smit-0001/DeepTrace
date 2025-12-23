cmake_minimum_required(VERSION 3.10)
project(DeepTraceSniffer)

set(CMAKE_CXX_STANDARD 17)

# Find required libraries
find_library(PCAP_LIBRARY pcap REQUIRED)
find_library(HIREDIS_LIBRARY hiredis REQUIRED)

# Include directories
include_directories(include)

# Add the executable
add_executable(deeptrace_sniffer
    src/main.cpp
    src/flow_table.cpp
    src/redis_producer.cpp
)

# Link libraries
target_link_libraries(deeptrace_sniffer
    ${PCAP_LIBRARY}
    ${HIREDIS_LIBRARY}
)