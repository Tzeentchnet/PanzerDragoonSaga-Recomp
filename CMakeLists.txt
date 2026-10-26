cmake_minimum_required(VERSION 3.21)

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

macro(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
    if(MSVC)
        get_filename_component(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
        get_filename_component(
            PrecompiledSourceAbsolute
            "${PrecompiledSource}"
            ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        )
        set(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${PrecompiledBasename}.pch")
        set(Sources ${${SourcesVar}})
        list(REMOVE_ITEM Sources "${PrecompiledSource}" "${PrecompiledSourceAbsolute}")

        set_source_files_properties("${PrecompiledSourceAbsolute}"
            PROPERTIES
                COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                OBJECT_OUTPUTS "${PrecompiledBinary}"
        )
        set_source_files_properties(${Sources}
            PROPERTIES
                COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                OBJECT_DEPENDS "${PrecompiledBinary}"
        )
        list(APPEND ${SourcesVar} "${PrecompiledSourceAbsolute}")
        list(REMOVE_DUPLICATES ${SourcesVar})
    endif()
endmacro()

if(MSVC AND CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES
        Debug
        Release
        Shipping
        Release_Tracy
        Debug_Asan
        Release_Asan
        Shipping_Asan
        CACHE STRING "Available build configurations" FORCE
    )
endif()

function(pds_collect_targets output directory)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)

    foreach(subdirectory IN LISTS subdirectories)
        pds_collect_targets(subdirectory_targets "${subdirectory}")
        list(APPEND targets ${subdirectory_targets})
    endforeach()

    set(${output} "${targets}" PARENT_SCOPE)
endfunction()

function(pds_apply_msvc_settings target)
    get_target_property(target_type "${target}" TYPE)
    if(NOT target_type MATCHES "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$")
        return()
    endif()

    set_property(TARGET "${target}" PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug,Debug_Asan>:Debug>DLL"
    )

    target_compile_definitions("${target}" PRIVATE
        "$<$<CONFIG:Debug,Debug_Asan>:_DEBUG>"
        "$<$<CONFIG:Debug,Debug_Asan>:_ITERATOR_DEBUG_LEVEL=1>"
        "$<$<CONFIG:Debug,Debug_Asan>:_MSVC_STL_HARDENING=1>"
        "$<$<CONFIG:Release,Shipping,Release_Tracy,Release_Asan,Shipping_Asan>:NDEBUG>"
        "$<$<CONFIG:Shipping,Shipping_Asan>:SHIPPING_BUILD>"
        "$<$<CONFIG:Release_Tracy>:TRACY_ENABLE>"
        "$<$<CONFIG:Debug_Asan,Release_Asan,Shipping_Asan>:_DISABLE_STRING_ANNOTATION>"
        "$<$<CONFIG:Debug_Asan,Release_Asan,Shipping_Asan>:_DISABLE_VECTOR_ANNOTATION>"
    )

    target_compile_options("${target}" PRIVATE
        "$<$<COMPILE_LANGUAGE:C,CXX>:/MP>"
        "$<$<COMPILE_LANGUAGE:C,CXX>:/Zc:preprocessor>"
        "$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>"
        "$<$<COMPILE_LANGUAGE:CXX>:/GR->"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug,Debug_Asan>>:/Od>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug,Debug_Asan>>:/Ob0>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug,Debug_Asan>>:/RTC1>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:/ZI>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug_Asan,Release,Shipping,Release_Tracy,Release_Asan,Shipping_Asan>>:/Zi>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,Shipping,Release_Tracy,Release_Asan,Shipping_Asan>>:/O2>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,Shipping,Release_Tracy,Release_Asan,Shipping_Asan>>:/Ob2>"
    )
endfunction()

function(pds_enable_msvc_asan target)
    target_compile_options("${target}" PRIVATE
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug_Asan,Release_Asan,Shipping_Asan>>:/fsanitize=address>"
        "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release_Asan,Shipping_Asan>>:/Oy->"
    )
endfunction()

set(THIRD_PARTY "${CMAKE_SOURCE_DIR}/ThirdParty")

# Build only the bgfx components used by PDS. shaderc is required by AzelLib.
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLE_COMMON OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_SHADER ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_CMAKE_USER_SCRIPT
    "${CMAKE_SOURCE_DIR}/cmake/bgfx_shaderc_only.cmake"
    CACHE FILEPATH "" FORCE
)
add_subdirectory("${THIRD_PARTY}/bgfx.cmake")

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

if(NOT CMAKE_SYSTEM_NAME MATCHES "iOS|tvOS" AND
   NOT CMAKE_SYSTEM_NAME STREQUAL watchOS AND
   NOT CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_subdirectory("${THIRD_PARTY}/SDL")
endif()

add_subdirectory("${THIRD_PARTY}/soloud")
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
    pds_collect_targets(pds_targets "${CMAKE_CURRENT_SOURCE_DIR}")
    foreach(target IN LISTS pds_targets)
        pds_apply_msvc_settings("${target}")
    endforeach()

    foreach(target IN ITEMS
        PDS
        AzelLib
        soloud
        pds_core_tests
        pds_input_recording_tests
        pds_scsp_register_tests
        pds_scsp_dsp_tests
        pds_vdp2_color_tests
        pds_rbg0_coefficient_tests
        pds_frame_timing_tests
    )
        if(TARGET "${target}")
            pds_enable_msvc_asan("${target}")
        endif()
    endforeach()
endif()
