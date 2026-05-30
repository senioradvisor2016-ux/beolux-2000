cmake_minimum_required(VERSION 3.22)
project(BC2000DL VERSION 0.61.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "macOS deployment target")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS build architectures (universal)")

# JUCE: använd lokal kopia om JUCE_PATH är satt eller ~/JUCE finns,
# annars FetchContent (kräver internet vid första bygget).
if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(STATUS "Using local JUCE at: ${JUCE_PATH}")
    add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE-build)
elseif(EXISTS "$ENV{HOME}/JUCE/CMakeLists.txt")
    message(STATUS "Using JUCE at: $ENV{HOME}/JUCE")
    add_subdirectory($ENV{HOME}/JUCE ${CMAKE_BINARY_DIR}/JUCE-build)
else()
    message(STATUS "JUCE not found locally; fetching via Git...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.6
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# Plugin-target
juce_add_plugin(BC2000DL
    VERSION                  ${PROJECT_VERSION}
    COMPANY_NAME             "Soundboys"
    BUNDLE_ID                "com.soundboys.beolux2000.plugin"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  TRUE
    PLUGIN_MANUFACTURER_CODE Sbys
    PLUGIN_CODE              Blx2
    FORMATS                  AU VST3 Standalone
    PRODUCT_NAME             "Beolux 2000"
    DESCRIPTION              "Beolux 2000 — Danish tape emulation by Soundboys"
)

target_sources(BC2000DL PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/WebEditor.cpp
    Source/WebEditor.h
    Source/NativeEditor.cpp
    Source/NativeEditor.h
    Source/WireframeEditor.cpp
    Source/WireframeEditor.h
    Source/ui/FontAudioIcons.h
    Source/ui/BC2000LookAndFeel.cpp
    Source/ui/BC2000LookAndFeel.h
    Source/dsp/Constants.h
    Source/dsp/Ge2N2613Stage.h
    Source/dsp/Ge2N2613Stage.cpp
    Source/dsp/GeLowNoiseStage.h
    Source/dsp/GeLowNoiseStage.cpp
    Source/dsp/EqDIN1962.h
    Source/dsp/EqDIN1962.cpp
    Source/dsp/TapeSaturation.h
    Source/dsp/TapeSaturation.cpp
    Source/dsp/Echo.h
    Source/dsp/Echo.cpp
    Source/dsp/ToneAndOutput.h
    Source/dsp/ToneAndOutput.cpp
    Source/dsp/WowFlutter.h
    Source/dsp/WowFlutter.cpp
    Source/dsp/MicAndPower.h
    Source/dsp/MicAndPower.cpp
    Source/dsp/Multiplay.h
    Source/dsp/Multiplay.cpp
    Source/dsp/PhonoPreamp.h
    Source/dsp/PhonoPreamp.cpp
    Source/dsp/SignalChain.h
    Source/dsp/SignalChain.cpp
    Source/ui/BCColours.h
    Source/ui/BCLookAndFeel.h
    Source/ui/BCLookAndFeel.cpp
    Source/ui/VUMeter.h
    Source/ui/VUMeter.cpp
    Source/ui/HybridHeroPanel.h
    Source/ui/HybridHeroPanel.cpp
    Source/ui/TransportLever.h
    Source/ui/TransportLever.cpp
    Source/ui/DualSlideFader.h
    Source/ui/DualSlideFader.cpp
    Source/ui/PhotorealReel.h
    Source/ui/PhotorealReel.cpp
    Source/ui/AssetRegistry.h
    Source/ui/AssetRegistry.cpp
    Source/ui/UADStyleSelector.h
    Source/ui/UADStyleSelector.cpp
    Source/ui/PresetBrowser.h
    Source/ui/PresetBrowser.cpp
    Source/presets/Presets.h
    Source/presets/Presets.cpp
)

target_compile_definitions(BC2000DL PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Konsoliderad binary_data: BEOFLUX HTML + photoreal-assets.
# Ett enda target → en BinaryData.h, inga include-konflikter.
juce_add_binary_data(BC2000DL_Assets SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../web/beoflux.html
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/fontaudio.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/knob_chrome_filmstrip.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/fader_thumb_chrome.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/walnut_bezel.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/matte_panel.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/reel_left.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/reel_right.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/chassis_alu.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/panel_main.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/panel_dark_strip.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/knob_metal.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/btn_color_1.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/btn_color_2.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/btn_color_3.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/btn_color_4.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/btn_color_5.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/btn_rect_1.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/btn_rect_2.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/btn_rect_3.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/btn_rect_4.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/jack_small.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/vu_dial.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/vu_face.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/knob_filmstrip.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/knob_filmstrip_big.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/knob_filmstrip_medium.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/knob_filmstrip_small.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/teac_deck_bg.png
)

# melatonin_blur — fast CPU blur + drop shadows (UAD/Soundtoys-grade visuals)
juce_add_module(${CMAKE_CURRENT_SOURCE_DIR}/vendor/melatonin_blur)

# melatonin_inspector — Chrome DevTools-style component inspector
# (toggle with Cmd+Shift+I in debug builds)
juce_add_module(${CMAKE_CURRENT_SOURCE_DIR}/vendor/melatonin_inspector)

target_link_libraries(BC2000DL PRIVATE
    BC2000DL_Assets
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    melatonin_blur
    melatonin_inspector
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Spara generated header med projektversion
juce_generate_juce_header(BC2000DL)

# ─────────────────────────────────────────────────────────────────
# DSP Smoke Tests — headless konsol-app, inga GUI-beroenden
#   Bygg:  cmake --build build --config Release --target BC2000DL_Tests
#   Kör:   ./build/BC2000DL_Tests_artefacts/Release/BC2000DL\ Tests
# ─────────────────────────────────────────────────────────────────
juce_add_console_app(BC2000DL_Tests
    PRODUCT_NAME "BC2000DL Tests"
)

target_sources(BC2000DL_Tests PRIVATE
    Source/tests/DspSmokeTest.cpp
    Source/dsp/Ge2N2613Stage.h
    Source/dsp/Ge2N2613Stage.cpp
    Source/dsp/GeLowNoiseStage.h
    Source/dsp/GeLowNoiseStage.cpp
    Source/dsp/EqDIN1962.h
    Source/dsp/EqDIN1962.cpp
    Source/dsp/TapeSaturation.h
    Source/dsp/TapeSaturation.cpp
    Source/dsp/Echo.h
    Source/dsp/Echo.cpp
    Source/dsp/ToneAndOutput.h
    Source/dsp/ToneAndOutput.cpp
    Source/dsp/WowFlutter.h
    Source/dsp/WowFlutter.cpp
    Source/dsp/MicAndPower.h
    Source/dsp/MicAndPower.cpp
    Source/dsp/Multiplay.h
    Source/dsp/Multiplay.cpp
    Source/dsp/PhonoPreamp.h
    Source/dsp/PhonoPreamp.cpp
    Source/dsp/SignalChain.h
    Source/dsp/SignalChain.cpp
    Source/presets/Presets.h
    Source/presets/Presets.cpp
)

target_include_directories(BC2000DL_Tests PRIVATE
    Source
)

target_compile_definitions(BC2000DL_Tests PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STANDALONE_APPLICATION=1
)

target_link_libraries(BC2000DL_Tests PRIVATE
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# ─────────────────────────────────────────────────────────────────
# Bug-fix regression tests — Christoffer/Logic-rapporten
#   Bygg:  cmake --build build --config Release --target BC2000DL_BugFixTests
#   Kör:   ./build/BC2000DL_BugFixTests_artefacts/Release/BC2000DL_BugFixTests
# ─────────────────────────────────────────────────────────────────
juce_add_console_app(BC2000DL_BugFixTests
    PRODUCT_NAME "BC2000DL BugFixTests"
)

target_sources(BC2000DL_BugFixTests PRIVATE
    Source/tests/BugFixTests.cpp
    Source/dsp/Ge2N2613Stage.cpp
    Source/dsp/GeLowNoiseStage.cpp
    Source/dsp/EqDIN1962.cpp
    Source/dsp/TapeSaturation.cpp
    Source/dsp/Echo.cpp
    Source/dsp/ToneAndOutput.cpp
    Source/dsp/WowFlutter.cpp
    Source/dsp/MicAndPower.cpp
    Source/dsp/Multiplay.cpp
    Source/dsp/PhonoPreamp.cpp
    Source/dsp/SignalChain.cpp
)

target_include_directories(BC2000DL_BugFixTests PRIVATE Source)

target_compile_definitions(BC2000DL_BugFixTests PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STANDALONE_APPLICATION=1
)

target_link_libraries(BC2000DL_BugFixTests PRIVATE
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# ─────────────────────────────────────────────────────────────────
# Audibility Test — verifierar att VARJE parameter påverkar audio.
# Letar efter "döda kontroller" (placebo-knappar som bias-buggen).
#   Bygg: cmake --build build --target BC2000DL_AudibilityTest
# ─────────────────────────────────────────────────────────────────
juce_add_console_app(BC2000DL_AudibilityTest
    PRODUCT_NAME "BC2000DL AudibilityTest"
)
target_sources(BC2000DL_AudibilityTest PRIVATE
    Source/tests/AudibilityTest.cpp
    Source/dsp/Ge2N2613Stage.cpp
    Source/dsp/GeLowNoiseStage.cpp
    Source/dsp/EqDIN1962.cpp
    Source/dsp/TapeSaturation.cpp
    Source/dsp/Echo.cpp
    Source/dsp/ToneAndOutput.cpp
    Source/dsp/WowFlutter.cpp
    Source/dsp/MicAndPower.cpp
    Source/dsp/Multiplay.cpp
    Source/dsp/PhonoPreamp.cpp
    Source/dsp/SignalChain.cpp
)
target_include_directories(BC2000DL_AudibilityTest PRIVATE Source)
target_compile_definitions(BC2000DL_AudibilityTest PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STANDALONE_APPLICATION=1
)
target_link_libraries(BC2000DL_AudibilityTest PRIVATE
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# ─────────────────────────────────────────────────────────────────
#  Framework-baserade DSP-enhetstester — Catch2 + GoogleTest
#  (FetchContent vid första bygget — kräver internet)
#    Bygg:  cmake --build build --target BC2000DL_Catch2 BC2000DL_GTest
#    Kör:   ./build/BC2000DL_Catch2_artefacts/Release/BC2000DL_Catch2
#           ./build/BC2000DL_GTest_artefacts/Release/BC2000DL_GTest
# ─────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2 googletest)

# Delad DSP-källista för båda testmålen
set(BC2000DL_DSP_TEST_SOURCES
    Source/dsp/Ge2N2613Stage.cpp  Source/dsp/GeLowNoiseStage.cpp
    Source/dsp/EqDIN1962.cpp      Source/dsp/TapeSaturation.cpp
    Source/dsp/Echo.cpp           Source/dsp/ToneAndOutput.cpp
    Source/dsp/WowFlutter.cpp     Source/dsp/MicAndPower.cpp
    Source/dsp/Multiplay.cpp      Source/dsp/PhonoPreamp.cpp
    Source/dsp/SignalChain.cpp)

# --- Catch2 ---
juce_add_console_app(BC2000DL_Catch2 PRODUCT_NAME "BC2000DL_Catch2")
target_sources(BC2000DL_Catch2 PRIVATE Source/tests/Catch2Tests.cpp ${BC2000DL_DSP_TEST_SOURCES})
target_include_directories(BC2000DL_Catch2 PRIVATE Source)
target_compile_definitions(BC2000DL_Catch2 PRIVATE
    JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(BC2000DL_Catch2 PRIVATE
    Catch2::Catch2WithMain
    juce::juce_dsp juce::juce_audio_basics juce::juce_core
    juce::juce_recommended_config_flags)

# --- GoogleTest ---
juce_add_console_app(BC2000DL_GTest PRODUCT_NAME "BC2000DL_GTest")
target_sources(BC2000DL_GTest PRIVATE Source/tests/GTests.cpp ${BC2000DL_DSP_TEST_SOURCES})
target_include_directories(BC2000DL_GTest PRIVATE Source)
target_compile_definitions(BC2000DL_GTest PRIVATE
    JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(BC2000DL_GTest PRIVATE
    GTest::gtest_main
    juce::juce_dsp juce::juce_audio_basics juce::juce_core
    juce::juce_recommended_config_flags)

# --- State round-trip (länkar mot plugin-SharedCode för riktiga processorn) ---
juce_add_console_app(BC2000DL_StateTest PRODUCT_NAME "BC2000DL_StateTest")
target_sources(BC2000DL_StateTest PRIVATE Source/tests/StateRoundTripTest.cpp)
target_include_directories(BC2000DL_StateTest PRIVATE Source)
target_link_libraries(BC2000DL_StateTest PRIVATE
    BC2000DL                       # plugin-SharedCode: processor + editors (symboler)
    BC2000DL_Assets
    juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
    juce::juce_gui_basics juce::juce_gui_extra
    melatonin_blur melatonin_inspector
    juce::juce_recommended_config_flags)

# ─────────────────────────────────────────────────────────────────
#  ctest — ett mål som samlar ALLA testsviter (CI-redo)
#    Allt:          ctest --test-dir build --output-on-failure
#    Bara snabba:   ctest --test-dir build -L unit
#    Bara integr.:  ctest --test-dir build -L integration
#  (integrations-tester kräver att pluginen byggts + installerats först,
#   samt python3+pedalboard / pluginval på maskinen.)
# ─────────────────────────────────────────────────────────────────
enable_testing()

# C++-enhetstester (snabba, inga externa beroenden)
add_test(NAME Catch2         COMMAND BC2000DL_Catch2)
add_test(NAME GTest          COMMAND BC2000DL_GTest)
add_test(NAME DspSmoke       COMMAND BC2000DL_Tests)
add_test(NAME BugFixTests    COMMAND BC2000DL_BugFixTests)
add_test(NAME AudibilityTest COMMAND BC2000DL_AudibilityTest)
add_test(NAME StateRoundTrip COMMAND BC2000DL_StateTest)
set_tests_properties(Catch2 GTest DspSmoke BugFixTests AudibilityTest StateRoundTrip
                     PROPERTIES LABELS "unit")

# Integrations-tester (kräver installerad VST3 + pedalboard / pluginval)
find_program(PYTHON3_EXE NAMES python3)
if(PYTHON3_EXE)
    add_test(NAME SignalTest_pedalboard
             COMMAND ${PYTHON3_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/Source/tests/signal_test.py)
    add_test(NAME StudioSound1968
             COMMAND ${PYTHON3_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/Source/tests/vs_studio_sound_1968.py)
    set_tests_properties(SignalTest_pedalboard PROPERTIES
        LABELS "integration" PASS_REGULAR_EXPRESSION "0 underkända")
    set_tests_properties(StudioSound1968 PROPERTIES
        LABELS "integration" PASS_REGULAR_EXPRESSION "29/29")
endif()

if(EXISTS "/Applications/pluginval.app/Contents/MacOS/pluginval")
    add_test(NAME Pluginval_strict10
             COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/validate.sh --strict 10)
    set_tests_properties(Pluginval_strict10 PROPERTIES LABELS "integration")
endif()
