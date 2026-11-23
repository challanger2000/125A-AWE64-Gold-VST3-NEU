cmake_minimum_required(VERSION 3.25)
project(AWE64GoldInProcessVST3 VERSION 0.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)
# Only real-time transport prerequisites are compiled at this stage.
# Never label this as a compiled or functioning VST3.
if(BUILD_TESTING)
    find_package(Threads REQUIRED)
    add_executable(awe64_realtime_queues_test tests/RealtimeQueuesTest.cpp)
    target_include_directories(awe64_realtime_queues_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(awe64_realtime_queues_test PRIVATE Threads::Threads)
    add_test(NAME AWE64_RealtimeQueues COMMAND awe64_realtime_queues_test)
endif()
