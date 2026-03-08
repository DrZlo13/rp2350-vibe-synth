# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in Raspberry Pi Pico Extras
set(PICO_EXTRAS_FETCH_FROM_GIT ON)
include(pico_extras_import.cmake)

project(2350-vibe-synth C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add sources under src folder
file(GLOB SRC_FILES src/*.c src/*.cpp)

# Add executable. Default name is the project name, version 0.1

add_executable(${PROJECT_NAME} ${SRC_FILES})

pico_set_program_name(${PROJECT_NAME} "2350-vibe-synth")
pico_set_program_version(${PROJECT_NAME} "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

set(CMSISCORE ${PICO_SDK_PATH}/src/rp2_common/cmsis/stub/CMSIS/Core)
add_compile_definitions(USE_ARM_DSP)

add_subdirectory(lib/DaisySP)
add_subdirectory(lib/CMSIS-DSP)

add_library(segger_rtt STATIC
    lib/RTT/RTT/SEGGER_RTT.c
    lib/RTT/RTT/SEGGER_RTT_printf.c
)
target_include_directories(segger_rtt PUBLIC lib/RTT/RTT lib/RTT/Config)

target_link_libraries(${PROJECT_NAME} DaisySP)
target_link_libraries(${PROJECT_NAME} CMSISDSP)
target_link_libraries(${PROJECT_NAME} pico_audio_i2s)
target_link_libraries(${PROJECT_NAME} segger_rtt)

# Add the standard library to the build
target_link_libraries(${PROJECT_NAME} pico_stdlib tinyusb_device tinyusb_board)
 
# Add the standard include files to the build
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

pico_add_extra_outputs(${PROJECT_NAME})

