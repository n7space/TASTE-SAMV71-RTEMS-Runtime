project(Samv71RuntimeXdmac VERSION 1.0.0 LANGUAGES C)

add_library(Samv71RuntimeXdmac STATIC)
target_sources(Samv71RuntimeXdmac
    PRIVATE     xdma_hardware_interface.c
                xdmac.c
                xdmad.c
    PUBLIC      xdmad.h
                xdmac.h
                xdma_hardware_interface.h)
target_include_directories(Samv71RuntimeXdmac
    PRIVATE     ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Samv71RuntimeXdmac
    PRIVATE     SAMV71::Pmc
                Samv71Scb
                SAMV71::Runtime::Hal)

add_format_target(Samv71RuntimeXdmac)

set_target_properties(Samv71RuntimeXdmac PROPERTIES OUTPUT_NAME "xdmac")
add_library(SAMV71::Runtime::Xdmac ALIAS Samv71RuntimeXdmac)
