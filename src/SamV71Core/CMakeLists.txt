add_library(SamV71Core STATIC)
target_sources(SamV71Core
  PRIVATE
  SamV71Core.c
  PUBLIC
  SamV71Core.h)
target_include_directories(SamV71Core
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamV71Core
  PRIVATE
  SAMV71::Pmc
  SAMV71::Wdt
  SAMV71::Tic)

add_format_target(SamV71Core)

set_target_properties(SamV71Core PROPERTIES OUTPUT_NAME "samv71core")
add_library(SAMV71::Runtime::Core ALIAS SamV71Core)
