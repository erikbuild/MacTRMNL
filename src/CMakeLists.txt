# To use this example as a standalone project using CMake:
# mkdir build
# cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
# make

cmake_minimum_required(VERSION 4.1)

project(MacTRMNL)

add_application(MacTRMNL
    MacTRMNL.c
    MacTCPHelper.c
    Logging.c
    Preferences.c
    MacTRMNL.r
    MacTRMNL_dialogs.r
    )

# Enable -ffunction-sections and -gc-sections to make the app as small as possible
# Target 68000 for maximum compatibility with System 7.0
# On 68K, also enable --mac-single to build it as a single-segment app (so that this code path doesn't rot)
set_target_properties(MacTRMNL PROPERTIES COMPILE_OPTIONS "-ffunction-sections;-m68000")
if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    set_target_properties(MacTRMNL PROPERTIES LINK_FLAGS "-Wl,-gc-sections -Wl,--mac-single")

else()
    set_target_properties(MacTRMNL PROPERTIES LINK_FLAGS "-Wl,-gc-sections")
endif()