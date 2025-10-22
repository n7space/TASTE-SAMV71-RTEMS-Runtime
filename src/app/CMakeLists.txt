add_executable(RtemsApp)
target_sources(RtemsApp
    PRIVATE     main.c)
target_include_directories(RtemsApp
    PRIVATE     ..)
target_link_libraries(RtemsApp
    PRIVATE     SAMV71::Uart
                SAMV71::Runtime)

add_format_target(RtemsApp)
