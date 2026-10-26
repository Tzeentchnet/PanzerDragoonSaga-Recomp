cmake_minimum_required(VERSION 3.21)

set(SOURCES
    PDS_Tool.cpp
    overlay.cpp
    overlay.h
    sh2dasm.cpp
    sh2dasm.h
    imgui_memory_editor.h

    ${THIRD_PARTY}/imgui/imgui.cpp
    ${THIRD_PARTY}/imgui/imgui_draw.cpp
    ${THIRD_PARTY}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY}/imgui/examples/imgui_impl_opengl3.cpp
    ${THIRD_PARTY}/imgui/examples/imgui_impl_sdl.cpp
)

if(MSVC)
    list(APPEND SOURCES
        ${THIRD_PARTY}/imgui/examples/libs/gl3w/GL/gl3w.c
    )
endif()

add_executable(PDS_Tool ${SOURCES})

set_target_properties(PDS_Tool PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data"
    WIN32_EXECUTABLE FALSE
)

target_include_directories(PDS_Tool PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/AzelLib"
    "${THIRD_PARTY}/imgui"
    "${THIRD_PARTY}/SDL2/include"
    "${THIRD_PARTY}/imgui/examples"
    "${THIRD_PARTY}/imgui/examples/libs/gl3w"
    "${THIRD_PARTY}/soloud/soloud/include"
    "${THIRD_PARTY}/glm"
)

if(APPLE)
    target_include_directories(PDS_Tool SYSTEM PRIVATE /System/Library/Frameworks)
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
    target_link_libraries(PDS_Tool PRIVATE
        ${SDL2_LIBRARY}
        ${COCOA_LIBRARY}
        ${GLUT_LIBRARY}
        ${OpenGL_LIBRARY}
        ${OpenAL_LIBRARY}
        ${AVFoundation_LIBRARY}
    )
endif()

if(EMSCRIPTEN)
    target_link_libraries(PDS_Tool PRIVATE GLESv2)
    target_compile_options(PDS_Tool PRIVATE -O3 -g)
    target_link_options(PDS_Tool PRIVATE
        "SHELL:-s TOTAL_MEMORY=1073741824"
        "SHELL:-s USE_WEBGL2=1"
        "SHELL:-s USE_SDL=2"
        "SHELL:--llvm-lto 3"
        -O3
        -g0
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/data@/"
    )
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
    target_link_libraries(PDS_Tool PRIVATE SDL2main SDL2-static soloud AzelLib)
endif()

if(UNIX)
    target_link_libraries(PDS_Tool PRIVATE GL)
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT PDS_Tool)
