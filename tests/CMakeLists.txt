add_executable(ets_tests TestMain.cpp
                         TestCircularBuffer.cpp
                         TestSlidingWindow.cpp
                         TestThrottler.cpp)

target_link_libraries(ets_tests ets)