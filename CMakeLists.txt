cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(metromix)

set(CMAKE_C_STANDARD 11)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# FreeRTOS (Fetch remote) TODO: Fix this
#include(FetchContent)
#FetchContent_Declare( FreeRTOS-Kernel
#		GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
#		GIT_TAG        main #Note: Best practice to use specific git-hash or tagged version
#)
#
#add_library(freertos_config INTERFACE)
#
#target_include_directories(freertos_config SYSTEM
#		INTERFACE
#		include
#)
#
#target_compile_definitions(freertos_config
#		INTERFACE
#		projCOVERAGE_TEST=0
#)
#
#set( FREERTOS_HEAP "4" CACHE STRING "" FORCE)
## Select the native compile PORT
#set( FREERTOS_PORT "GCC_POSIX" CACHE STRING "" FORCE)
## Select the cross-compile PORT
#if (CMAKE_CROSSCOMPILING)
#	set(FREERTOS_PORT "GCC_ARM_CM0" CACHE STRING "" FORCE)
#endif()
#
#target_compile_definitions(freertos_config INTERFACE ${definitions})
#target_compile_options(freertos_config INTERFACE ${options})
#
#FetchContent_MakeAvailable(freertos_kernel)


# Rest of project
add_executable(metromix
		main.c display/ssd1306.c
)

# FreeRTOS (Local)
FILE(GLOB FreeRTOS_src FreeRTOS-Kernel/*c)

add_library( FreeRTOS STATIC
  ${FreeRTOS_src}
  FreeRTOS-Kernel/portable/GCC/ARM_CM0/port.c
  FreeRTOS-Kernel/portable/MemMang/heap_4.c
)

target_include_directories(FreeRTOS PUBLIC
        FreeRTOS-Kernel/include
        include/
        FreeRTOS-Kernel/portable/GCC/ARM_CM0
)

target_include_directories(metromix
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(metromix
        pico_stdlib
        hardware_i2c
        FreeRTOS
)

pico_enable_stdio_usb(metromix 1) 
pico_enable_stdio_uart(metromix 0) 

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(metromix)
