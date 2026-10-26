add_executable(pds_core_tests
    fixed_point_tests.cpp
    "${PROJECT_SOURCE_DIR}/AzelLib/fixedPoint.cpp"
)

add_executable(pds_input_recording_tests
    input_recording_tests.cpp
    "${PROJECT_SOURCE_DIR}/AzelLib/inputRecordingFormat.cpp"
)

add_executable(pds_scsp_register_tests
    scsp_register_tests.cpp
    "${THIRD_PARTY}/aosdk/eng_ssf/scsp.c"
    "${THIRD_PARTY}/aosdk/eng_ssf/scspdsp.c"
)

add_executable(pds_scsp_dsp_tests
    scsp_dsp_tests.cpp
    "${THIRD_PARTY}/aosdk/eng_ssf/scspdsp.c"
)

add_executable(pds_vdp2_color_tests
    vdp2_color_tests.cpp
)

add_executable(pds_rbg0_coefficient_tests
    rbg0_coefficient_tests.cpp
)

add_executable(pds_frame_timing_tests
    frame_timing_tests.cpp
)

function(pds_configure_test_target target)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        FOLDER "Tests"
    )

    target_compile_definitions(${target} PRIVATE
        "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug_Asan,Release_Asan,Shipping_Asan>>:PDS_TEST_EXPECT_ASAN=1>"
    )

    if(MSVC)
        target_link_options(${target} PRIVATE
            "$<$<CONFIG:Debug_Asan,Release_Asan,Shipping_Asan>:/DEBUG>"
            "$<$<CONFIG:Debug_Asan,Release_Asan,Shipping_Asan>:/INCREMENTAL:NO>"
        )

        get_filename_component(pds_msvc_runtime_directory "${CMAKE_CXX_COMPILER}" DIRECTORY)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                "-Druntime_directory=${pds_msvc_runtime_directory}"
                "-Ddestination_directory=$<TARGET_FILE_DIR:${target}>"
                "-Dconfiguration=$<CONFIG>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/copy_msvc_asan_runtime.cmake"
            VERBATIM
        )
    endif()
endfunction()

pds_configure_test_target(pds_core_tests)
pds_configure_test_target(pds_input_recording_tests)
pds_configure_test_target(pds_scsp_register_tests)
pds_configure_test_target(pds_scsp_dsp_tests)
pds_configure_test_target(pds_vdp2_color_tests)
pds_configure_test_target(pds_rbg0_coefficient_tests)
pds_configure_test_target(pds_frame_timing_tests)

target_include_directories(pds_core_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
)

target_include_directories(pds_input_recording_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
)

target_include_directories(pds_scsp_register_tests PRIVATE
    "${THIRD_PARTY}/aosdk"
    "${THIRD_PARTY}/aosdk/eng_ssf"
)

target_include_directories(pds_scsp_dsp_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
    "${THIRD_PARTY}/aosdk"
    "${THIRD_PARTY}/aosdk/eng_ssf"
)

target_include_directories(pds_vdp2_color_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
)

target_include_directories(pds_rbg0_coefficient_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
)

target_include_directories(pds_frame_timing_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
)

add_test(NAME pds.core.fixed_point COMMAND pds_core_tests)
add_test(NAME pds.core.input_recording COMMAND pds_input_recording_tests)
add_test(NAME pds.audio.scsp_registers COMMAND pds_scsp_register_tests)
add_test(NAME pds.audio.scsp_dsp COMMAND pds_scsp_dsp_tests)
add_test(NAME pds.video.vdp2_color COMMAND pds_vdp2_color_tests)
add_test(NAME pds.video.rbg0_coefficients COMMAND pds_rbg0_coefficient_tests)
add_test(NAME pds.timing.frame_cadence COMMAND pds_frame_timing_tests)

set_tests_properties(pds.core.fixed_point PROPERTIES
    LABELS "core;unit"
)
set_tests_properties(pds.core.input_recording PROPERTIES
    LABELS "core;unit;replay"
)
set_tests_properties(pds.audio.scsp_registers pds.audio.scsp_dsp PROPERTIES
    LABELS "audio;unit"
)
set_tests_properties(pds.video.vdp2_color PROPERTIES
    LABELS "video;unit"
)
set_tests_properties(pds.video.rbg0_coefficients PROPERTIES
    LABELS "video;unit;experimental"
)
set_tests_properties(pds.timing.frame_cadence PROPERTIES
    LABELS "timing;unit;experimental"
)

add_custom_target(pds_tests DEPENDS
    pds_core_tests
    pds_input_recording_tests
    pds_scsp_register_tests
    pds_scsp_dsp_tests
    pds_vdp2_color_tests
    pds_rbg0_coefficient_tests
    pds_frame_timing_tests
)
set_target_properties(pds_tests PROPERTIES FOLDER "Tests")

find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME pds.tools.compare_regression
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_regression_tests.py"
    )
    set_tests_properties(pds.tools.compare_regression PROPERTIES
        LABELS "tools;unit"
    )
endif()
