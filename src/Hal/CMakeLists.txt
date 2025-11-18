add_library(SamV71Hal STATIC)
target_sources(SamV71Hal
  PRIVATE
  Hal.c
  PUBLIC
  Hal.h)
target_include_directories(SamV71Hal
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamV71Hal
  PUBLIC      SAMV71::Nvic
              SAMV71::Pio
              SAMV71::Pmc
              SAMV71::Tic
              SAMV71::Wdt
              SAMV71::Utils
              SAMV71::Runtime::Mocks
              SAMV71::Runtime::Xdmac
              SAMV71::Runtime::Core
              SAMV71::Uart)

add_format_target(SamV71Hal)

set_target_properties(SamV71Hal PROPERTIES OUTPUT_NAME "samv71hal")
add_library(SAMV71::Runtime::Hal ALIAS SamV71Hal)
