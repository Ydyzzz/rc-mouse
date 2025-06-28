cmake_minimum_required(VERSION 3.20.0)
project(testing)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_include_directories(testbinary PRIVATE ${PROJECT_SOURCE_DIR}/../src)
target_sources(testbinary PRIVATE main.c ${PROJECT_SOURCE_DIR}/../src/nec-decoder.c)

