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

# Stress example
add_executable(producer_stress stress/producer.cpp)
target_link_libraries(producer_stress PRIVATE ztrace)

add_executable(consumer_stress stress/consumer.cpp)
target_link_libraries(consumer_stress PRIVATE ztrace)

# Generator example
add_executable(generator generator/producer.cpp)
target_link_libraries(generator PRIVATE ztrace)

add_executable(synthetic_generator generator/synthetic_generator.cpp)
target_link_libraries(synthetic_generator PRIVATE ztrace)

