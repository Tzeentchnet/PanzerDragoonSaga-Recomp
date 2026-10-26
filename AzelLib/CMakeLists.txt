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


set(SOURCES
 ${SOURCES}

 ${PDS_IMGUI_SOURCE_DIR}/imgui.cpp
 ${PDS_IMGUI_SOURCE_DIR}/imgui.h
 ${PDS_IMGUI_SOURCE_DIR}/imgui_draw.cpp
 ${PDS_IMGUI_SOURCE_DIR}/imgui_internal.h
 ${PDS_IMGUI_SOURCE_DIR}/imgui_tables.cpp
 ${PDS_IMGUI_SOURCE_DIR}/imgui_widgets.cpp
 ${PDS_IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp

 ${PDS_AOSDK_SOURCE_DIR}/eng_ssf/m68kcpu.c
 ${PDS_AOSDK_SOURCE_DIR}/eng_ssf/m68kopac.c
 ${PDS_AOSDK_SOURCE_DIR}/eng_ssf/m68kopdm.c
 ${PDS_AOSDK_SOURCE_DIR}/eng_ssf/m68kopnz.c
 ${PDS_AOSDK_SOURCE_DIR}/eng_ssf/m68kops.c
 ${PDS_AOSDK_SOURCE_DIR}/eng_ssf/sat_hw.c
 ${PDS_AOSDK_SOURCE_DIR}/eng_ssf/scsp.c
 ${PDS_AOSDK_SOURCE_DIR}/eng_ssf/scspdsp.c

)

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

if(WIN32)
    set(AZEL_SHADER_PROFILES s_5_0 s_6_0 spirv)
elseif(APPLE)
    set(AZEL_SHADER_PROFILES metal spirv)
else()
    set(AZEL_SHADER_PROFILES 430 spirv)
endif()

bgfx_compile_shaders(
    SHADERS ${VDP1_VERTEX_SHADERS}
    TYPE VERTEX
    PROFILES ${AZEL_SHADER_PROFILES}
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP1.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    AS_HEADERS
    OUT_FILES_VAR VDP1_VERTEX_SHADER_HEADERS
)
bgfx_compile_shaders(
    SHADERS ${VDP1_FRAGMENT_SHADERS}
    TYPE FRAGMENT
    PROFILES ${AZEL_SHADER_PROFILES}
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP1.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    AS_HEADERS
    OUT_FILES_VAR VDP1_FRAGMENT_SHADER_HEADERS
)
bgfx_compile_shaders(
    SHADERS ${VDP2_VERTEX_SHADERS}
    TYPE VERTEX
    PROFILES ${AZEL_SHADER_PROFILES}
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP2_vs.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    AS_HEADERS
    OUT_FILES_VAR VDP2_VERTEX_SHADER_HEADERS
)
bgfx_compile_shaders(
    SHADERS ${VDP2_FRAGMENT_SHADERS}
    TYPE FRAGMENT
    PROFILES ${AZEL_SHADER_PROFILES}
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP2_vs.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    AS_HEADERS
    OUT_FILES_VAR VDP2_FRAGMENT_SHADER_HEADERS
)
bgfx_compile_shaders(
    SHADERS ${VDP2_BLIT_VERTEX_SHADERS}
    TYPE VERTEX
    PROFILES ${AZEL_SHADER_PROFILES}
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP2_blit.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
    AS_HEADERS
    OUT_FILES_VAR VDP2_BLIT_VERTEX_SHADER_HEADERS
)
bgfx_compile_shaders(
    SHADERS ${VDP2_BLIT_FRAGMENT_SHADERS}
    TYPE FRAGMENT
    PROFILES ${AZEL_SHADER_PROFILES}
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/VDP2_blit.varying.def.sc
    OUTPUT_DIR ${AZEL_SHADER_OUTPUT_DIR}
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

target_precompile_headers(${AZEL_LIBRARY_TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/PDS.h>"
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
        "${PDS_IMGUI_SOURCE_DIR}"
        "${PDS_GLM_SOURCE_DIR}"
        "${PDS_TRACY_SOURCE_DIR}/public"
        "${PDS_BX_SOURCE_DIR}/include"
        "${PDS_BX_SOURCE_DIR}/include/compat/msvc"
    PRIVATE
        "${AZEL_SHADER_OUTPUT_DIR}"
        "${PDS_IMGUI_SOURCE_DIR}/backends"
        "${PDS_BGFX_CORE_SOURCE_DIR}/3rdparty"
        "${PDS_BIMG_SOURCE_DIR}/include"
        "${PDS_AOSDK_SOURCE_DIR}"
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
