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
    "${PDS_AOSDK_SOURCE_DIR}/eng_ssf/scsp.c"
    "${PDS_AOSDK_SOURCE_DIR}/eng_ssf/scspdsp.c"
)

add_executable(pds_scsp_dsp_tests
    scsp_dsp_tests.cpp
    "${PDS_AOSDK_SOURCE_DIR}/eng_ssf/scspdsp.c"
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

add_executable(pds_renderer_config_tests
    renderer_config_tests.cpp
    "${PROJECT_SOURCE_DIR}/AzelLib/renderer/renderer_config.cpp"
)

add_executable(pds_game_config_tests
    game_config_tests.cpp
    "${PROJECT_SOURCE_DIR}/AzelLib/game_config.cpp"
    "${PROJECT_SOURCE_DIR}/AzelLib/renderer/renderer_config.cpp"
)

function(pds_configure_test_target target)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        FOLDER "Tests"
    )

    if(MSVC AND PDS_ENABLE_ASAN)
        target_compile_definitions(${target} PRIVATE PDS_TEST_EXPECT_ASAN=1)
    endif()

    if(MSVC)
        if(PDS_ENABLE_ASAN)
            target_link_options(${target} PRIVATE /DEBUG /INCREMENTAL:NO)
            pds_copy_msvc_asan_runtime(${target})
        endif()
    endif()
endfunction()

pds_configure_test_target(pds_core_tests)
pds_configure_test_target(pds_input_recording_tests)
pds_configure_test_target(pds_scsp_register_tests)
pds_configure_test_target(pds_scsp_dsp_tests)
pds_configure_test_target(pds_vdp2_color_tests)
pds_configure_test_target(pds_rbg0_coefficient_tests)
pds_configure_test_target(pds_frame_timing_tests)
pds_configure_test_target(pds_renderer_config_tests)
pds_configure_test_target(pds_game_config_tests)

target_include_directories(pds_core_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
)

target_include_directories(pds_input_recording_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
)

target_include_directories(pds_scsp_register_tests PRIVATE
    "${PDS_AOSDK_SOURCE_DIR}"
    "${PDS_AOSDK_SOURCE_DIR}/eng_ssf"
)

target_include_directories(pds_scsp_dsp_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
    "${PDS_AOSDK_SOURCE_DIR}"
    "${PDS_AOSDK_SOURCE_DIR}/eng_ssf"
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

target_include_directories(pds_renderer_config_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
)

target_include_directories(pds_game_config_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/AzelLib"
)

add_test(NAME pds.core.fixed_point COMMAND pds_core_tests)
add_test(NAME pds.core.input_recording COMMAND pds_input_recording_tests)
add_test(NAME pds.audio.scsp_registers COMMAND pds_scsp_register_tests)
add_test(NAME pds.audio.scsp_dsp COMMAND pds_scsp_dsp_tests)
add_test(NAME pds.video.vdp2_color COMMAND pds_vdp2_color_tests)
add_test(NAME pds.video.rbg0_coefficients COMMAND pds_rbg0_coefficient_tests)
add_test(NAME pds.timing.frame_cadence COMMAND pds_frame_timing_tests)
add_test(NAME pds.renderer.configuration COMMAND pds_renderer_config_tests)
add_test(NAME pds.configuration.game COMMAND pds_game_config_tests)

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
set_tests_properties(pds.renderer.configuration PROPERTIES
    LABELS "renderer;unit"
)
set_tests_properties(pds.configuration.game PROPERTIES
    LABELS "configuration;unit"
)

if(WIN32)
    find_program(PDS_POWERSHELL_EXECUTABLE NAMES powershell.exe)
    if(PDS_POWERSHELL_EXECUTABLE)
        add_test(
            NAME pds.tools.configuration_launcher
            COMMAND
                "${PDS_POWERSHELL_EXECUTABLE}"
                -NoProfile
                -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/tools/pds-config.ps1"
                -ValidateOnly
                -ConfigPath
                "${PROJECT_SOURCE_DIR}/config/pds-settings.example.ini"
        )
        set_tests_properties(pds.tools.configuration_launcher PROPERTIES
            LABELS "tools;configuration;unit"
        )
    endif()
endif()

add_custom_target(pds_tests DEPENDS
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

    if(WIN32 AND PDS_POWERSHELL_EXECUTABLE)
        add_test(
            NAME pds.tools.game_data_preparation
            COMMAND
                "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/game_data_preparation_tests.py"
                --powershell
                "${PDS_POWERSHELL_EXECUTABLE}"
                --script
                "${PROJECT_SOURCE_DIR}/tools/prepare-game-data.ps1"
        )
        set_tests_properties(pds.tools.game_data_preparation PROPERTIES
            LABELS "tools;unit;data"
        )
    endif()
endif()
