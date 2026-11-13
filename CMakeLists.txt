cmake_minimum_required(VERSION 3.16)

if(APPLE)
    execute_process(
            COMMAND xcrun --sdk macosx --show-sdk-path
            OUTPUT_VARIABLE MACOS_SDK
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CMAKE_OSX_SYSROOT "${MACOS_SDK}" CACHE PATH "macOS SDK")
endif()

project(renderer3d LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

add_custom_command(TARGET renderer3d POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:renderer3d>/assets"
        COMMENT "Copying assets"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/vendor/fenster"
    "${CMAKE_SOURCE_DIR}/vendor/fast_obj"
    "${CMAKE_SOURCE_DIR}/vendor/stb"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/lib"
)

if(APPLE)
    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
        "${CMAKE_OSX_SYSROOT}/usr/include"
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework AudioToolbox"
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE objc)
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        gdi32
        winmm
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "renderer3d"
)
