add_library(FaultHandler STATIC)
target_sources(FaultHandler
  PRIVATE
  FaultHandler.c
  DeathReportWriter.c
  CustomExceptionVector.c
  PUBLIC
  FaultHandler.h
  DeathReport.h
  DeathReportWriter.h)
target_include_directories(FaultHandler
    PRIVATE     ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(FaultHandler
  PRIVATE       Samv71Scb
                SAMV71::Nvic
                SAMV71::Rstc)

add_format_target(FaultHandler)

set_target_properties(FaultHandler PROPERTIES OUTPUT_NAME "faulthandler")
add_library(SAMV71::Runtime::FaultHandler ALIAS FaultHandler)
