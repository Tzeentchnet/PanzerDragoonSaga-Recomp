cmake_minimum_required(VERSION 3.21)
project(soloud C CXX)

IF(${CMAKE_SYSTEM_NAME} MATCHES iOS)
set(SOLOUD_LIB_NAME soloud-ios)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES tvOS)
Set(SOLOUD_LIB_NAME soloud-tvos)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
Set(SOLOUD_LIB_NAME soloud-darwin)
ELSE()
set(SOLOUD_LIB_NAME soloud)
ENDIF()

file(GLOB_RECURSE SOLOUD_HEADERS ${soloud_SOURCE_DIR}/soloud/include/*.h)
file(GLOB_RECURSE SOLOUD_SOURCES
    ${soloud_SOURCE_DIR}/soloud/src/core/*.cpp
    ${soloud_SOURCE_DIR}/soloud/src/audiosource/wav/*.c*
    ${soloud_SOURCE_DIR}/soloud/src/backend/sdl3_static/*.cpp
    ${soloud_SOURCE_DIR}/soloud/src/backend/null/*.cpp
)

set(SOLOUD_SOURCES
${SOLOUD_HEADERS}
${SOLOUD_SOURCES}
)

add_library(${SOLOUD_LIB_NAME} STATIC ${SOLOUD_SOURCES})
add_library(SoLoud::SoLoud ALIAS ${SOLOUD_LIB_NAME})

target_include_directories(${SOLOUD_LIB_NAME}
    PUBLIC "${soloud_SOURCE_DIR}/soloud/include"
)

if(TARGET SDL3::SDL3-static)
    target_link_libraries(${SOLOUD_LIB_NAME} PRIVATE SDL3::SDL3-static)
    target_include_directories(${SOLOUD_LIB_NAME} PRIVATE
        "${THIRD_PARTY}/SDL/include/SDL3"
    )
else()
    target_include_directories(${SOLOUD_LIB_NAME} PRIVATE
        "${THIRD_PARTY}/SDL/include"
        "${THIRD_PARTY}/SDL/include/SDL3"
    )
endif()

target_compile_options(${SOLOUD_LIB_NAME} PRIVATE
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3>"
)

IF(${CMAKE_SYSTEM_NAME} STREQUAL watchOS)
target_compile_definitions(${SOLOUD_LIB_NAME} PRIVATE WITH_NULL=1)
ELSE()
target_compile_definitions(${SOLOUD_LIB_NAME} PRIVATE WITH_SDL3_STATIC=1)
ENDIF()
