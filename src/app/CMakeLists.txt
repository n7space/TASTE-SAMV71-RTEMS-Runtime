add_executable(RtemsApp)
target_sources(RtemsApp
    PRIVATE     main.c)
target_include_directories(RtemsApp
    PRIVATE     ..)
target_link_libraries(RtemsApp
    PRIVATE     SAMV71::Uart
                SAMV71::Runtime::Hal
                SAMV71::Runtime::Monitor
                SAMV71::Runtime::ThreadsCommon
                SAMV71::Runtime::BrokerLock
                SAMV71::Runtime::Mocks
                SAMV71::Runtime::Core
                SAMV71::Runtime::Xdmac)

add_format_target(RtemsApp)
