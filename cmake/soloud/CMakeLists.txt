if(CMAKE_SYSTEM_NAME MATCHES iOS)
    set(SOLOUD_LIB_NAME soloud-ios)
elseif(CMAKE_SYSTEM_NAME MATCHES tvOS)
    set(SOLOUD_LIB_NAME soloud-tvos)
elseif(CMAKE_SYSTEM_NAME MATCHES Darwin)
    set(SOLOUD_LIB_NAME soloud-darwin)
else()
    set(SOLOUD_LIB_NAME soloud)
endif()

file(GLOB_RECURSE SOLOUD_HEADERS CONFIGURE_DEPENDS
    "${PDS_SOLOUD_SOURCE_DIR}/include/*.h"
)
file(GLOB_RECURSE SOLOUD_SOURCES CONFIGURE_DEPENDS
    "${PDS_SOLOUD_SOURCE_DIR}/src/core/*.cpp"
    "${PDS_SOLOUD_SOURCE_DIR}/src/audiosource/wav/*.c*"
    "${PDS_SOLOUD_SOURCE_DIR}/src/backend/sdl3_static/*.cpp"
    "${PDS_SOLOUD_SOURCE_DIR}/src/backend/null/*.cpp"
)

add_library(${SOLOUD_LIB_NAME} STATIC
    ${SOLOUD_HEADERS}
    ${SOLOUD_SOURCES}
)
add_library(SoLoud::SoLoud ALIAS ${SOLOUD_LIB_NAME})

set_target_properties(${SOLOUD_LIB_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(${SOLOUD_LIB_NAME}
    PUBLIC "${PDS_SOLOUD_SOURCE_DIR}/include"
)

if(TARGET SDL3::SDL3-static)
    target_link_libraries(${SOLOUD_LIB_NAME} PRIVATE SDL3::SDL3-static)
    target_include_directories(${SOLOUD_LIB_NAME} PRIVATE
        "${PDS_SDL_SOURCE_DIR}/include/SDL3"
    )
else()
    target_include_directories(${SOLOUD_LIB_NAME} PRIVATE
        "${PDS_SDL_SOURCE_DIR}/include"
        "${PDS_SDL_SOURCE_DIR}/include/SDL3"
    )
endif()

target_compile_options(${SOLOUD_LIB_NAME} PRIVATE
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3>"
)

if(CMAKE_SYSTEM_NAME STREQUAL watchOS)
    target_compile_definitions(${SOLOUD_LIB_NAME} PRIVATE WITH_NULL=1)
else()
    target_compile_definitions(${SOLOUD_LIB_NAME} PRIVATE WITH_SDL3_STATIC=1)
endif()
