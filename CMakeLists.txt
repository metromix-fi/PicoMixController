cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
set(WIFI_SSID $ENV{WIFI_SSID})
set(WIFI_PASSWORD $ENV{WIFI_PASSWORD})

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(metromix)

set(CMAKE_C_STANDARD 11)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

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

# Time of Flight lib
include(${PROJECT_SOURCE_DIR}/VL53L1X-C-API-Pico/library/import.cmake)

# Rest of project
add_executable(metromix
		main.c
		display/ssd1306.c
		networking/httpclient.c
		networking/httpclient.h
		networking/tls_common.c
		input/inputhandler.c
		input/inputhandler.h
        utils/GlobalState.c
        utils/GlobalState.h
		display/animationtask.c
		display/animationtask.h
        display/renderer.c
        display/renderer.h
		rfid/rfidtask.c
		rfid/rfidtask.h
		rfid/mfrc522.c
		rfid/mfrc522.h
		rfid/rfid-spi.c
		rfid/rfid-spi.h
		toftask/toftask.c
		toftask/toftask.h
		utils/helper.c
		utils/helper.h
)

target_link_libraries(metromix
        pico_stdlib
        hardware_i2c
		hardware_spi
        FreeRTOS
		pico_lwip_mbedtls
		pico_mbedtls
		pico_cyw43_arch#_lwip_poll
		pico_cyw43_arch_lwip_poll
		VL53L1X_pico_api
#		picow_tls_verify_background
#		pico_cyw43_arch_lwip_threadsafe_background
#		picow_tls_client_poll
)

target_include_directories(metromix PUBLIC
		${CMAKE_CURRENT_LIST_DIR}
		networking/include
)

target_compile_definitions(metromix PRIVATE
		WIFI_SSID="${WIFI_SSID}"
		WIFI_PASSWORD="${WIFI_PASSWORD}"
)


pico_enable_stdio_usb(metromix 0)
pico_enable_stdio_uart(metromix 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(metromix)

# copy project to pico
#add_custom_command(TARGET metromix

#POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/metromix.uf2 /run/media/jesse/RPI-RP2 )