cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rc-mouse)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(NEC_SRC src/nec-decoder.c)
set(IR_INPUT_SRC src/ir-input.c)
set(MOUSE_HID_SRC src/mouse-hid.c)
set(APP_SRC src/main.c)
set(RC_MOUSE_SRC src/rc-mouse.c)

target_sources(app PRIVATE ${APP_SRC} ${NEC_SRC} ${IR_INPUT_SRC} ${MOUSE_HID_SRC} ${RC_MOUSE_SRC})

