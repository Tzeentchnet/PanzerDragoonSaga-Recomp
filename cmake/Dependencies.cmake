include_guard(GLOBAL)

include(FetchContent)

set(
    FETCHCONTENT_UPDATES_DISCONNECTED
    ON
    CACHE BOOL
    "Do not update already-populated dependencies during configure."
)

FetchContent_Declare(
    pds_bgfx_modern
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG 8c0a5f4b5f0c5ff94292435a5c8ccaef78688b47
    GIT_PROGRESS TRUE
)
FetchContent_Declare(
    pds_sdl
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG 829a65d769d935c4852f8159e964312c0957260a
    GIT_PROGRESS TRUE
)
FetchContent_Declare(
    pds_imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG f1cc2ae15e53a861a874c3034aae6798fde194ab
    GIT_PROGRESS TRUE
    SOURCE_SUBDIR _pds_source_only
)
FetchContent_Declare(
    pds_glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e
    GIT_PROGRESS TRUE
    SOURCE_SUBDIR _pds_source_only
)
FetchContent_Declare(
    pds_tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG 30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c
    GIT_PROGRESS TRUE
    SOURCE_SUBDIR _pds_source_only
)
FetchContent_Declare(
    pds_soloud
    GIT_REPOSITORY https://github.com/yaz0r/soloud.git
    GIT_TAG e9e8aea38d65a6f3a565ccf6103a4883ae8bb9c6
    GIT_PROGRESS TRUE
    SOURCE_SUBDIR _pds_source_only
)

unset(BGFX_DIR CACHE)
unset(BIMG_DIR CACHE)
unset(BX_DIR CACHE)

FetchContent_MakeAvailable(
    pds_bgfx_modern
    pds_sdl
    pds_imgui
    pds_glm
    pds_tracy
    pds_soloud
)

FetchContent_GetProperties(
    bgfx_source
    SOURCE_DIR PDS_BGFX_CORE_SOURCE_DIR
)
FetchContent_GetProperties(
    bx_source
    SOURCE_DIR PDS_BX_SOURCE_DIR
)
FetchContent_GetProperties(
    bimg_source
    SOURCE_DIR PDS_BIMG_SOURCE_DIR
)
if(NOT IS_DIRECTORY "${PDS_BGFX_CORE_SOURCE_DIR}/src")
    message(FATAL_ERROR "Unable to locate the fetched bgfx source directory.")
endif()
if(NOT IS_DIRECTORY "${PDS_BX_SOURCE_DIR}/include")
    message(FATAL_ERROR "Unable to locate the fetched bx source directory.")
endif()
if(NOT IS_DIRECTORY "${PDS_BIMG_SOURCE_DIR}/include")
    message(FATAL_ERROR "Unable to locate the fetched bimg source directory.")
endif()

set(PDS_BGFX_SOURCE_DIR "${pds_bgfx_modern_SOURCE_DIR}")
set(BGFX_SHADER_INCLUDE_PATH "${PDS_BGFX_CORE_SOURCE_DIR}/src")
set(PDS_SDL_SOURCE_DIR "${pds_sdl_SOURCE_DIR}")
set(PDS_IMGUI_SOURCE_DIR "${pds_imgui_SOURCE_DIR}")
set(PDS_GLM_SOURCE_DIR "${pds_glm_SOURCE_DIR}")
set(PDS_TRACY_SOURCE_DIR "${pds_tracy_SOURCE_DIR}")
set(PDS_SOLOUD_SOURCE_DIR "${pds_soloud_SOURCE_DIR}")
set(PDS_AOSDK_SOURCE_DIR "${CMAKE_SOURCE_DIR}/ThirdParty/aosdk")

include("${PDS_BGFX_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")

add_subdirectory("${CMAKE_SOURCE_DIR}/cmake/soloud")
