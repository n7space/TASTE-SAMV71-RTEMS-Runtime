add_subdirectory(Mocks)
add_subdirectory(BrokerLock)
add_subdirectory(SamV71Core)
add_subdirectory(Hal)
add_subdirectory(Monitor)
add_subdirectory(ThreadsCommon)
add_subdirectory(Xdmac)

# add_library(Samv71Runtime INTERFACE)
# target_include_directories(Samv71Runtime
#     INTERFACE     ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(Samv71Runtime
#     INTERFACE      SAMV71::Runtime::Hal
#                 SAMV71::Runtime::Monitor
#                 SAMV71::Runtime::ThreadsCommon
#                 SAMV71::Runtime::BrokerLock
#                 SAMV71::Runtime::Mocks
#                 SAMV71::Runtime::Core
#                 SAMV71::Runtime::Xdmac)

# add_format_target(Samv71Runtime)

# set_target_properties(Samv71Runtime PROPERTIES OUTPUT_NAME "runtime")
# set_target_properties(Samv71Runtime PROPERTIES LINKER_LANGUAGE C)
# add_library(SAMV71::Runtime ALIAS Samv71Runtime)

# add_subdirectory(app)
