add_library(SamV71ThreadsCommon STATIC)
target_sources(SamV71ThreadsCommon
  PRIVATE
  ThreadsCommon.c
  PUBLIC
  ThreadsCommon.h)
target_include_directories(SamV71ThreadsCommon
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamV71ThreadsCommon
  PRIVATE
  SAMV71::Runtime::Hal
  SAMV71::Runtime::Mocks)

add_format_target(SamV71ThreadsCommon)

set_target_properties(SamV71ThreadsCommon PROPERTIES OUTPUT_NAME "samv71threadscommon")
add_library(SAMV71::Runtime::ThreadsCommon ALIAS SamV71ThreadsCommon)
