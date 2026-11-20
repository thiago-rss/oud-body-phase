cmake_minimum_required(VERSION 3.22)
project(OudBodyPhase VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{JUCE_DIR} AND NOT "$ENV{JUCE_DIR}" STREQUAL "")
    set(JUCE_PATH "$ENV{JUCE_DIR}")
else()
    set(JUCE_PATH "$ENV{HOME}/JUCE")
endif()

if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE nao encontrado em: ${JUCE_PATH}")
endif()

add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/juce_build")

juce_add_plugin(OudBodyPhase
    COMPANY_NAME "OudAudioLab"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE OudL
    PLUGIN_CODE Obp1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Oud Body Phase")

juce_generate_juce_header(OudBodyPhase)

target_sources(OudBodyPhase
    PRIVATE
        Source/OudParameters.h
        Source/OudDspChain.h
        Source/OudLookAndFeel.h
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp)

target_compile_definitions(OudBodyPhase
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(OudBodyPhase
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
