cmake_minimum_required(VERSION 3.18)
project(speed_companion LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT ANDROID_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "This project targets AArch64 only: configure with -DANDROID_ABI=arm64-v8a")
endif()

foreach(f zygisk.hpp And64InlineHook.hpp And64InlineHook.cpp)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${f}")
        message(FATAL_ERROR "Missing ${f} (the workflow downloads it)")
    endif()
endforeach()

add_library(speed_companion SHARED
    main.cpp worker.cpp config.cpp log.cpp
    il2cpp_api.cpp resolver.cpp hook_util.cpp
    unity_speed.cpp game_speed.cpp damage_rate.cpp
    And64InlineHook.cpp)

target_include_directories(speed_companion PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(speed_companion PRIVATE log)

# Export only the Zygisk entry points so this module's hook code cannot clash with Kuro's.
target_compile_options(speed_companion PRIVATE -O2 -fvisibility=hidden -fvisibility-inlines-hidden)
target_link_options(speed_companion PRIVATE -Wl,--exclude-libs,ALL -Wl,--gc-sections -Wl,-z,max-page-size=16384)
