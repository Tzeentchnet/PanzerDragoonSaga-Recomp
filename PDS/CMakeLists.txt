cmake_minimum_required(VERSION 3.21)

file(GLOB PDS_DATA
    "${CMAKE_SOURCE_DIR}/data/*"
)

file(GLOB SRC_ROOT
    "*.cpp"
    "*.h"
)

file(GLOB_RECURSE SRC_KERNEL
    "kernel/*.cpp"
    "kernel/*.h"
)

file(GLOB_RECURSE SRC_FIELD_A3
    "field/field_a3/*.cpp"
    "field/field_a3/*.h"
)

set(SOURCES
    ${SRC_KERNEL}
    ${SRC_ROOT}
    ${SRC_FIELD_A3}
)

if(PDS_ENABLE_TRACY)
    list(APPEND SOURCES "${PDS_TRACY_SOURCE_DIR}/public/TracyClient.cpp")
    set_source_files_properties(
        "${PDS_TRACY_SOURCE_DIR}/public/TracyClient.cpp"
        PROPERTIES CXX_STANDARD 17
    )
endif()

source_group("src" FILES ${SRC_ROOT})
source_group("src\\kernel" FILES ${SRC_KERNEL})
source_group("src\\field\\field_a3" FILES ${SRC_FIELD_A3})

add_executable(PDS ${SOURCES})

set_target_properties(PDS PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data"
    WIN32_EXECUTABLE FALSE
)

target_compile_definitions(PDS PRIVATE
    USE_SDL
    _HAS_EXCEPTIONS=0
)

target_include_directories(PDS PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/AzelLib"
    "${PDS_IMGUI_SOURCE_DIR}"
    "${PDS_IMGUI_SOURCE_DIR}/backends"
    "${PDS_GLM_SOURCE_DIR}"
    "${PDS_TRACY_SOURCE_DIR}/public"
)

if(MSVC)
    target_link_options(PDS PRIVATE
        /DEBUG
        "$<$<CONFIG:Release,Shipping>:/OPT:REF>"
        "$<$<CONFIG:Release,Shipping>:/OPT:ICF>"
        "$<$<CONFIG:Release,Shipping>:/INCREMENTAL:NO>"
    )
    pds_copy_msvc_asan_runtime(PDS)
endif()

if(APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES "iOS|tvOS")
        set(FRAMEWORKS
            Foundation
            AudioToolbox
            CoreGraphics
            QuartzCore
            UIKit
            OpenGLES
            MobileCoreServices
            AVFoundation
            GameController
            Metal
        )
        if(CMAKE_SYSTEM_NAME MATCHES iOS)
            list(APPEND FRAMEWORKS CoreMotion)
        endif()
        foreach(FW IN LISTS FRAMEWORKS)
            target_link_options(PDS PRIVATE "SHELL:-framework ${FW}")
        endforeach()
    else()
        find_library(COCOA_LIBRARY Cocoa)
        find_library(GLUT_LIBRARY GLUT)
        find_library(OpenGL_LIBRARY OpenGL)
        find_library(OpenAL_LIBRARY OpenAL)
        find_library(AVFoundation_LIBRARY AVFoundation)
        mark_as_advanced(
            COCOA_LIBRARY
            GLUT_LIBRARY
            OpenGL_LIBRARY
            OpenAL_LIBRARY
            AVFoundation_LIBRARY
        )
        target_link_libraries(PDS PRIVATE
            ${SDL2_LIBRARY}
            ${COCOA_LIBRARY}
            ${GLUT_LIBRARY}
            ${OpenGL_LIBRARY}
            ${OpenAL_LIBRARY}
            ${AVFoundation_LIBRARY}
        )
    endif()
endif()

if(EMSCRIPTEN)
    target_link_libraries(PDS PRIVATE GLESv2)
    target_compile_options(PDS PRIVATE -O3 -g0)
    target_link_options(PDS PRIVATE
        "SHELL:-s TOTAL_MEMORY=1073741824"
        "SHELL:-s EXTRA_EXPORTED_RUNTIME_METHODS=['Pointer_stringify']"
        "SHELL:-s USE_WEBGL2=1"
        -O3
        -g0
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/data@/"
    )
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "iOS|tvOS")
    target_link_libraries(PDS PRIVATE
        Azel::AzelLib
        SoLoud::SoLoud
        "${PDS_SDL_SOURCE_DIR}/Xcode-iOS/SDL/build/Release\\\${EFFECTIVE_PLATFORM_NAME}/libSDL2.a"
    )
else()
    target_link_libraries(PDS PRIVATE
        Azel::AzelLib
        SoLoud::SoLoud
        bgfx
        SDL3::SDL3-static
    )
    if(UNIX)
        target_link_libraries(PDS PRIVATE GL)
    endif()
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT PDS)

if(CMAKE_SYSTEM_NAME MATCHES "iOS|tvOS")
    add_custom_command(TARGET PDS PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:PDS>"
    )

    set_target_properties(PDS PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/build/ios.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "Azel"
        MACOSX_FRAMEWORK_BUNDLE_VERSION 1.0
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING 1.0
        MACOSX_BUNDLE_BUNDLE_VERSION 1.0
        MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "net.yaz0r.azel"
    )
endif()
