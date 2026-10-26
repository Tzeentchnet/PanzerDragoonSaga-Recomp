cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE SRC_ROOT CONFIGURE_DEPENDS
    "*.cpp"
    "*.h"
)

set(SOURCES
 ${SOURCES}
 ${SRC_ROOT}
)

function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)


if(MSVC)
    ADD_MSVC_PRECOMPILED_HEADER("PDS.h" "PDS_Precompiled.cpp" SOURCES)
endif()
                                        

set(SOURCES
 ${SOURCES}

 ${THIRD_PARTY}/imgui/imgui.cpp
 ${THIRD_PARTY}/imgui/imgui.h
 ${THIRD_PARTY}/imgui/imgui_draw.cpp
 ${THIRD_PARTY}/imgui/imgui_internal.h
 ${THIRD_PARTY}/imgui/imgui_tables.cpp
 ${THIRD_PARTY}/imgui/imgui_widgets.cpp
 ${THIRD_PARTY}/imgui/backends/imgui_impl_sdl3.cpp

 ${THIRD_PARTY}/ImGuizmo/src/ImGuizmo.cpp
 ${THIRD_PARTY}/ImGuizmo/src/ImGuizmo.h

 ${THIRD_PARTY}/aosdk/eng_ssf/m68kcpu.c
 ${THIRD_PARTY}/aosdk/eng_ssf/m68kopac.c
 ${THIRD_PARTY}/aosdk/eng_ssf/m68kopdm.c
 ${THIRD_PARTY}/aosdk/eng_ssf/m68kopnz.c
 ${THIRD_PARTY}/aosdk/eng_ssf/m68kops.c
 ${THIRD_PARTY}/aosdk/eng_ssf/sat_hw.c
 ${THIRD_PARTY}/aosdk/eng_ssf/scsp.c
 ${THIRD_PARTY}/aosdk/eng_ssf/scspdsp.c
 #${THIRD_PARTY}/aosdk/eng_ssf/scsplfo.c

 "shaderc_prebuilt.cmake"
)

include(shaderc_prebuilt.cmake)

set(VDP1_VERTEX_SHADERS
    shaders/VDP1_vs.sc
    shaders/VDP1_vertexColor_vs.sc
    shaders/VDP1_worldTextured_vs.sc
    shaders/VDP1_2dUI_vs.sc
    shaders/VDP1_2dUIVertexColor_vs.sc
)

set(VDP1_FRAGMENT_SHADERS
    shaders/VDP1_ps.sc
    shaders/VDP1_vertexColor_ps.sc
    shaders/VDP1_worldTextured_ps.sc
    shaders/VDP1_2dUI_ps.sc
    shaders/VDP1_2dUIVertexColor_ps.sc
)

set(VDP2_VERTEX_SHADERS
    shaders/VDP2_vs.sc
)

set(VDP2_FRAGMENT_SHADERS
    shaders/VDP2_ps.sc
    shaders/VDP2_RBG0_ps.sc
    shaders/VDP2_RBG0_dual_ps.sc
)

set(VDP2_BLIT_VERTEX_SHADERS
    shaders/VDP2_blit_vs.sc
)

set(VDP2_BLIT_FRAGMENT_SHADERS
    shaders/VDP2_blit_ps.sc
    shaders/VDP2_blit_vdp1_ps.sc
    shaders/VDP2_composite_ps.sc
)

set(AZEL_SHADER_SOURCES
    ${VDP1_VERTEX_SHADERS}
    ${VDP1_FRAGMENT_SHADERS}
    shaders/VDP1.varying.def.sc
    ${VDP2_VERTEX_SHADERS}
    ${VDP2_FRAGMENT_SHADERS}
    shaders/VDP2_vs.varying.def.sc
    ${VDP2_BLIT_VERTEX_SHADERS}
    ${VDP2_BLIT_FRAGMENT_SHADERS}
    shaders/VDP2_blit.varying.def.sc
)

set(SOURCES
    ${SOURCES}
    ${AZEL_SHADER_SOURCES}
)

set(AZEL_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders/generated/$<CONFIG>")

bgfx_compile_shaders(
    SHADERS ${VDP1_VERTEX_SHADERS}
    TYPE VERTEX
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP1.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    INCLUDE_DIRS ${BGFX_DIR}/src
    AS_HEADERS
    OUT_FILES_VAR VDP1_VERTEX_SHADER_HEADERS
)
bgfx_compile_shaders(
    SHADERS ${VDP1_FRAGMENT_SHADERS}
    TYPE FRAGMENT
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP1.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    INCLUDE_DIRS ${BGFX_DIR}/src
    AS_HEADERS
    OUT_FILES_VAR VDP1_FRAGMENT_SHADER_HEADERS
)
bgfx_compile_shaders(
    SHADERS ${VDP2_VERTEX_SHADERS}
    TYPE VERTEX
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP2_vs.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    INCLUDE_DIRS ${BGFX_DIR}/src
    AS_HEADERS
    OUT_FILES_VAR VDP2_VERTEX_SHADER_HEADERS
)
bgfx_compile_shaders(
    SHADERS ${VDP2_FRAGMENT_SHADERS}
    TYPE FRAGMENT
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP2_vs.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    INCLUDE_DIRS ${BGFX_DIR}/src
    AS_HEADERS
    OUT_FILES_VAR VDP2_FRAGMENT_SHADER_HEADERS
)
bgfx_compile_shaders(
    SHADERS ${VDP2_BLIT_VERTEX_SHADERS}
    TYPE VERTEX
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP2_blit.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    INCLUDE_DIRS ${BGFX_DIR}/src
    AS_HEADERS
    OUT_FILES_VAR VDP2_BLIT_VERTEX_SHADER_HEADERS
)
bgfx_compile_shaders(
    SHADERS ${VDP2_BLIT_FRAGMENT_SHADERS}
    TYPE FRAGMENT
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP2_blit.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    INCLUDE_DIRS ${BGFX_DIR}/src
    AS_HEADERS
    OUT_FILES_VAR VDP2_BLIT_FRAGMENT_SHADER_HEADERS
)

set(AZEL_SHADER_HEADERS
    ${VDP1_VERTEX_SHADER_HEADERS}
    ${VDP1_FRAGMENT_SHADER_HEADERS}
    ${VDP2_VERTEX_SHADER_HEADERS}
    ${VDP2_FRAGMENT_SHADER_HEADERS}
    ${VDP2_BLIT_VERTEX_SHADER_HEADERS}
    ${VDP2_BLIT_FRAGMENT_SHADER_HEADERS}
)

add_custom_target(AzelLib-shaders DEPENDS ${AZEL_SHADER_HEADERS})

assign_source_group(${SOURCES})

IF(${CMAKE_SYSTEM_NAME} MATCHES iOS)
add_library (AzelLib-ios ${SOURCES})
set(AZEL_LIBRARY_TARGET AzelLib-ios)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES tvOS)
add_library (AzelLib-tvos ${SOURCES})
set(AZEL_LIBRARY_TARGET AzelLib-tvos)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
add_library (AzelLib-darwin ${SOURCES})
set(AZEL_LIBRARY_TARGET AzelLib-darwin)
ELSE()
add_library (AzelLib ${SOURCES})
set(AZEL_LIBRARY_TARGET AzelLib)
ENDIF()

add_library(Azel::AzelLib ALIAS ${AZEL_LIBRARY_TARGET})

set_target_properties(${AZEL_LIBRARY_TARGET} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_definitions(${AZEL_LIBRARY_TARGET}
    PRIVATE
        USE_SDL
        _HAS_EXCEPTIONS=0
        "$<$<BOOL:${PDS_EXPERIMENTAL_RBG0_COEFFICIENT_ADDRESSING}>:PDS_EXPERIMENTAL_RBG0_COEFFICIENT_ADDRESSING=1>"
        "$<$<BOOL:${PDS_EXPERIMENTAL_NTSC_FRAME_CADENCE}>:PDS_EXPERIMENTAL_NTSC_FRAME_CADENCE=1>"
    PUBLIC
        BX_CONFIG_DEBUG=0
)

target_include_directories(${AZEL_LIBRARY_TARGET}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${THIRD_PARTY}/imgui"
        "${THIRD_PARTY}/glm"
        "${THIRD_PARTY}/tracy/public"
        "${THIRD_PARTY}/bgfx.cmake/bx/include"
        "${THIRD_PARTY}/bgfx.cmake/bx/include/compat/msvc"
    PRIVATE
        "${AZEL_SHADER_OUTPUT_DIR}"
        "${THIRD_PARTY}/imgui/backends"
        "${THIRD_PARTY}/ImGuizmo/src"
        "${THIRD_PARTY}/bgfx.cmake/bgfx/3rdparty"
        "${THIRD_PARTY}/bgfx.cmake/bimg/include"
        "${THIRD_PARTY}/aosdk"
)

if(TARGET SoLoud::SoLoud)
    target_link_libraries(${AZEL_LIBRARY_TARGET} PUBLIC SoLoud::SoLoud)
endif()
if(TARGET bgfx)
    target_link_libraries(${AZEL_LIBRARY_TARGET} PUBLIC bgfx)
endif()
if(TARGET SDL3::SDL3-static)
    target_link_libraries(${AZEL_LIBRARY_TARGET} PUBLIC SDL3::SDL3-static)
endif()

add_dependencies(${AZEL_LIBRARY_TARGET} AzelLib-shaders)
