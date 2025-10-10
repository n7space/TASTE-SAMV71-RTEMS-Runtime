project(Mocks VERSION 1.0.0 LANGUAGES C)

add_library(Mocks STATIC)
target_sources(Mocks
    PRIVATE     interfaces_info.c
    PUBLIC      interfaces_info.h)
target_include_directories(Mocks
    PUBLIC     ${CMAKE_CURRENT_SOURCE_DIR})

add_format_target(Mocks)

add_library(SAMV71::Runtime::Mocks ALIAS Mocks)
