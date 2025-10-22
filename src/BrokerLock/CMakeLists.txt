add_library(BrokerLock STATIC)
target_sources(BrokerLock
    PRIVATE     BrokerLock.c
    PUBLIC      BrokerLock.h)
target_include_directories(BrokerLock
    PRIVATE     ${CMAKE_CURRENT_SOURCE_DIR})

add_format_target(BrokerLock)

set_target_properties(BrokerLock PROPERTIES OUTPUT_NAME "brokerLock")
add_library(SAMV71::Runtime::BrokerLock ALIAS BrokerLock)
