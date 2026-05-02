cmake_minimum_required(VERSION 3.22)
project(BC2000DL VERSION 0.1.0)

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
    COMPANY_NAME             "BC2000DL Audio"
    BUNDLE_ID                "com.bc2000dl.audio.plugin"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  TRUE
    PLUGIN_MANUFACTURER_CODE BcAU
    PLUGIN_CODE              Bc2k
    FORMATS                  AU VST3 Standalone
    PRODUCT_NAME             "BC2000DL"
    DESCRIPTION              "Danish Tape 2000 — VST/AU emulation of a 1968 reel-to-reel"
)

target_sources(BC2000DL PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/WebEditor.cpp
    Source/WebEditor.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/knob_chrome_filmstrip.png
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

target_link_libraries(BC2000DL PRIVATE
    BC2000DL_Assets
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Spara generated header med projektversion
juce_generate_juce_header(BC2000DL)
