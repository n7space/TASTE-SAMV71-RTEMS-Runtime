add_library(SamV71BootHelper STATIC)
target_sources(SamV71BootHelper
  PRIVATE
  BootHelper.c
  PUBLIC
  BootHelper.h)
target_include_directories(SamV71BootHelper
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamV71BootHelper
  PUBLIC      SAMV71::Rstc)

add_format_target(SamV71BootHelper)

set_target_properties(SamV71BootHelper PROPERTIES OUTPUT_NAME "samv71BootHelper")
add_library(SAMV71::Runtime::BootHelper ALIAS SamV71BootHelper)
