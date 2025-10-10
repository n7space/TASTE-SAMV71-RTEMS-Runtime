add_subdirectory(Mocks)
add_subdirectory(BrokerLock)
add_subdirectory(Xdmac)

add_library(Samv71Hal STATIC)
target_sources(Samv71Hal
  PRIVATE     Hal.c
  Monitor.c
  ThreadsCommon.c
  PUBLIC      Hal.h
  Monitor.h
  ThreadsCommon.h)
target_include_directories(Samv71Hal
    PRIVATE     ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Samv71Hal
    PRIVATE     SAMV71::Startup
                SAMV71::Nvic
                SAMV71::Pio
                SAMV71::Pmc
                SAMV71::Utils
                SAMV71::Runtime::Mocks
                SAMV71::Runtime::Xdmad
    PUBLIC      SAMV71::Uart)

add_format_target(Samv71Hal)

set_target_properties(Samv71Hal PROPERTIES OUTPUT_NAME "hal")
add_library(SAMV71::Runtime::Hal ALIAS Samv71Hal)

add_subdirectory(app)
