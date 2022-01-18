add_library(ets INTERFACE)

target_sources(ets INTERFACE ets/CircularBuffer.h
                             ets/SlidingWindow.h
                             ets/Throttler.h)

target_include_directories(ets INTERFACE  "${PROJECT_SOURCE_DIR}/lib")