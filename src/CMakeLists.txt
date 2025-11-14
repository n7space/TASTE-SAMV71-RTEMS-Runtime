add_subdirectory(Mocks)
add_subdirectory(BrokerLock)
add_subdirectory(SamV71Core)
add_subdirectory(Xdmac)

add_library(Samv71Runtime STATIC)
target_sources(Samv71Runtime
  PRIVATE     Hal.c
  Monitor.c
  ThreadsCommon.c
  FaultHandler.c
  DeathReportWriter.c
  CustomExceptionVector.c
  PUBLIC      Hal.h
  Monitor.h
  ThreadsCommon.h
  FaultHandler.h
  DeathReport.h
  DeathReportWriter.h)
target_include_directories(Samv71Runtime
    PRIVATE     ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Samv71Runtime
    PRIVATE     SAMV71::Startup
                SAMV71::Nvic
                SAMV71::Pio
                SAMV71::Pmc
                SAMV71::Tic
                SAMV71::Wdt
                SAMV71::Utils
                SAMV71::Runtime::Mocks
                SAMV71::Runtime::Xdmac
                SAMV71::Runtime::Core
    PUBLIC      SAMV71::Uart)

add_format_target(Samv71Runtime)

set_target_properties(Samv71Runtime PROPERTIES OUTPUT_NAME "runtime")
add_library(SAMV71::Runtime ALIAS Samv71Runtime)

add_subdirectory(app)
