add_library(SamV71Monitor STATIC)
target_sources(SamV71Monitor
  PRIVATE
  Monitor.c
  PUBLIC
  Monitor.h)
target_include_directories(SamV71Monitor
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamV71Monitor
  PRIVATE SAMV71::Runtime::Hal
          SAMV71::Runtime::Mocks)

add_format_target(SamV71Monitor)

set_target_properties(SamV71Monitor PROPERTIES OUTPUT_NAME "samv71monitor")
add_library(SAMV71::Runtime::Monitor ALIAS SamV71Monitor)
