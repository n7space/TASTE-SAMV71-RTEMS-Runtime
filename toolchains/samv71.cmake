set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER arm-rtems6-gcc)
set(CMAKE_CXX_COMPILER arm-rtems6-g++)

set(CMAKE_C_FLAGS
"-mcpu=cortex-m7 \
-mfloat-abi=hard \
-mfpu=fpv5-d16 \
-mlittle-endian \
-mthumb \
-ffunction-sections \
-DN7S_TARGET_SAMV71Q21")
set(CMAKE_CXX_FLAGS
"-mcpu=cortex-m7 \
-mfloat-abi=hard \
-mfpu=fpv5-d16 \
-mlittle-endian \
-mthumb \
-ffunction-sections \
-DN7S_TARGET_SAMV71Q21")
set(CMAKE_EXE_LINKER_FLAGS
"-qnolinkcmds \
-Wl,-T/opt/rtems/arm-rtems6/atsamv/lib/linkcmds.intsram \
-L/opt/rtems/arm-rtems6/atsamv/lib \
-qrtems \
-Wl,--start-group \
-lrtemscpu \
-lrtemsbsp \
-lgcc \
-Wl,--end-group \
-Wl,--gc-sections \
-Wl,-z,origin,-rpath,/opt/rtems/arm-rtems6/atsamv/lib \
-lm")
