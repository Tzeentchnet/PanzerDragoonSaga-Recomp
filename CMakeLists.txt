cmake_minimum_required(VERSION 4.4)

cmake_policy(SET CMP0091 NEW)

if(CMAKE_SYSTEM_NAME MATCHES iOS)
    project(PDS-ios)
elseif(CMAKE_SYSTEM_NAME MATCHES tvOS)
    project(PDS-tvos)
elseif(CMAKE_SYSTEM_NAME MATCHES Darwin)
    project(PDS-Darwin)
else()
    project(PDS)
endif()

include(CTest)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    set(
        CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()

option(
    PDS_ENABLE_ASAN
    "Enable MSVC AddressSanitizer instrumentation"
    OFF
)
option(
    PDS_ENABLE_TRACY
    "Enable Tracy instrumentation and its profiling client"
    OFF
)
option(
    PDS_EXPERIMENTAL_RBG0_COEFFICIENT_ADDRESSING
    "Enable experimental per-pixel RBG0 coefficient table-base addressing"
    OFF
)
option(
    PDS_EXPERIMENTAL_NTSC_FRAME_CADENCE
    "Enable the default-off 59.826 Hz NTSC raster cadence experiment"
    OFF
)

if(MSVC AND CMAKE_CONFIGURATION_TYPES)
    if(PDS_ENABLE_ASAN OR PDS_ENABLE_TRACY)
        set(pds_configuration_types Release)
    else()
        set(pds_configuration_types Debug Release Shipping)
    endif()
    set(
        CMAKE_CONFIGURATION_TYPES
        ${pds_configuration_types}
        CACHE STRING "Available build configurations"
        FORCE
    )
endif()

if(PDS_ENABLE_ASAN AND NOT MSVC)
    message(FATAL_ERROR "PDS_ENABLE_ASAN currently requires MSVC.")
endif()

function(pds_apply_msvc_settings target)
    get_target_property(target_type "${target}" TYPE)
    if(NOT target_type MATCHES "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$")
        return()
    endif()

    set_property(TARGET "${target}" PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )

    target_compile_definitions("${target}" PRIVATE
        "$<$<CONFIG:Debug>:_DEBUG>"
        "$<$<CONFIG:Debug>:_MSVC_STL_HARDENING=1>"
        "$<$<CONFIG:Release,Shipping>:NDEBUG>"
        "$<$<CONFIG:Shipping>:SHIPPING_BUILD>"
        "$<$<BOOL:${PDS_ENABLE_TRACY}>:TRACY_ENABLE>"
        "$<$<BOOL:${PDS_ENABLE_ASAN}>:_DISABLE_STRING_ANNOTATION>"
        "$<$<BOOL:${PDS_ENABLE_ASAN}>:_DISABLE_VECTOR_ANNOTATION>"
    )

    target_compile_options("${target}" PRIVATE
        "$<$<COMPILE_LANGUAGE:C,CXX>:/MP>"
        "$<$<COMPILE_LANGUAGE:C,CXX>:/Zc:preprocessor>"
        "$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>"
        "$<$<COMPILE_LANGUAGE:CXX>:/GR->"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:/Od>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:/Ob0>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:/RTC1>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug,Release,Shipping>>:/Zi>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,Shipping>>:/O2>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,Shipping>>:/Ob2>"
    )
endfunction()

function(pds_enable_msvc_asan target)
    if(NOT PDS_ENABLE_ASAN)
        return()
    endif()

    target_compile_options("${target}" PRIVATE
        "$<$<COMPILE_LANGUAGE:C,CXX>:/fsanitize=address>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,Shipping>>:/Oy->"
    )
endfunction()

function(pds_copy_msvc_asan_runtime target)
    if(NOT MSVC OR NOT PDS_ENABLE_ASAN)
        return()
    endif()

    get_filename_component(pds_msvc_runtime_directory "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_custom_command(TARGET "${target}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-Druntime_directory=${pds_msvc_runtime_directory}"
            "-Ddestination_directory=$<TARGET_FILE_DIR:${target}>"
            "-Dconfiguration=$<CONFIG>"
            -P "${CMAKE_SOURCE_DIR}/cmake/CopyMsvcAsanRuntime.cmake"
        VERBATIM
    )
endfunction()

# Build only the bgfx components used by PDS. shaderc is required by AzelLib.
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLE_COMMON OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_SHADER ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_CMAKE_USER_SCRIPT "" CACHE FILEPATH "" FORCE)

# Build only the static SDL library used by PDS.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_CPACK OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_UNINSTALL OFF CACHE BOOL "" FORCE)
set(SDL_DUMMYVIDEO OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_D3D OFF CACHE BOOL "" FORCE)
set(SDL_DISKAUDIO OFF CACHE BOOL "" FORCE)
set(SDL_DUMMYAUDIO OFF CACHE BOOL "" FORCE)
set(SDL_X11_XSCRNSAVER OFF CACHE BOOL "" FORCE)
set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)

include("${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake")

add_subdirectory(AzelLib)

if(NOT CMAKE_SYSTEM_NAME MATCHES "iOS|tvOS" AND
   NOT CMAKE_SYSTEM_NAME STREQUAL watchOS AND
   NOT CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_subdirectory(PDS)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(MSVC)
    foreach(target IN ITEMS
        PDS
        AzelLib
        pds_core_tests
        pds_input_recording_tests
        pds_scsp_register_tests
        pds_scsp_dsp_tests
        pds_vdp2_color_tests
        pds_rbg0_coefficient_tests
        pds_frame_timing_tests
        pds_renderer_config_tests
        pds_game_config_tests
    )
        if(TARGET "${target}")
            pds_apply_msvc_settings("${target}")
            pds_enable_msvc_asan("${target}")
        endif()
    endforeach()

    foreach(target IN ITEMS PDS AzelLib)
        if(TARGET "${target}")
            set_property(
                TARGET "${target}"
                PROPERTY INTERPROCEDURAL_OPTIMIZATION_SHIPPING TRUE
            )
        endif()
    endforeach()
endif()
