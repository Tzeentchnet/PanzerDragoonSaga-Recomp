if(configuration STREQUAL "Debug")
    set(runtime_name "clang_rt.asan_dbg_dynamic-x86_64.dll")
else()
    set(runtime_name "clang_rt.asan_dynamic-x86_64.dll")
endif()

set(runtime_source "${runtime_directory}/${runtime_name}")
if(NOT EXISTS "${runtime_source}")
    message(FATAL_ERROR "MSVC AddressSanitizer runtime not found: ${runtime_source}")
endif()

file(COPY_FILE
    "${runtime_source}"
    "${destination_directory}/${runtime_name}"
    ONLY_IF_DIFFERENT
)
