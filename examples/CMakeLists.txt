# examples/CMakeLists.txt

# Simple example
add_executable(producer_simple simple/producer.cpp)
target_link_libraries(producer_simple PRIVATE ztrace)

add_executable(consumer_simple simple/consumer.cpp)
target_link_libraries(consumer_simple PRIVATE ztrace)

# Streaming example
add_executable(producer_streaming streaming/producer.cpp)
target_link_libraries(producer_streaming PRIVATE ztrace)

add_executable(consumer_streaming streaming/consumer.cpp)
target_link_libraries(consumer_streaming PRIVATE ztrace)

