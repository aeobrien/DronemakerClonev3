cmake_minimum_required(VERSION 3.22)

project(DronemakerClonev3 VERSION 0.1.0)

# --- JUCE ---
# Set JUCE_DIR to your local JUCE checkout.
# On macOS (Aidan's machine):  /Users/aidan/Dev/Gong/JUCE
# On Raspberry Pi:             ~/JUCE
# You can override at configure time: cmake -DJUCE_DIR=/path/to/JUCE ..
if(NOT DEFINED JUCE_DIR)
    if(APPLE)
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Gong/JUCE" CACHE PATH "Path to JUCE")
    else()
        set(JUCE_DIR "$ENV{HOME}/JUCE" CACHE PATH "Path to JUCE")
    endif()
endif()

add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/juce_build)

# --- Application target ---
juce_add_gui_app(DronemakerClonev3
    PRODUCT_NAME "DronemakerClone"
    VERSION "0.1.0"
    NEEDS_MIDI_INPUT TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "DronemakerClone needs microphone access for live audio input."
)

# Generate JuceHeader.h (compatibility with Projucer-style includes)
juce_generate_juce_header(DronemakerClonev3)

# --- Source files ---
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DronemakerClonev3/Source")

target_sources(DronemakerClonev3 PRIVATE
    # Core
    ${SOURCE_DIR}/Main.cpp
    ${SOURCE_DIR}/MainComponent.h
    ${SOURCE_DIR}/MainComponent.cpp
    ${SOURCE_DIR}/FFTProcessor.h
    ${SOURCE_DIR}/FFTProcessor.cpp
    ${SOURCE_DIR}/LoopRecorder.h
    ${SOURCE_DIR}/LoopRecorder.cpp
    ${SOURCE_DIR}/LoopAutomation.h
    ${SOURCE_DIR}/LoopSequenceExecutor.h
    ${SOURCE_DIR}/LoopSequenceExecutor.cpp
    ${SOURCE_DIR}/LoopAutomationEditor.h
    ${SOURCE_DIR}/LoopAutomationEditor.cpp
    ${SOURCE_DIR}/PiLayout.h
    ${SOURCE_DIR}/PiLayout.cpp
    ${SOURCE_DIR}/VirtualEncoderBank.h

    # Effects
    ${SOURCE_DIR}/Effects/EffectBase.h
    ${SOURCE_DIR}/Effects/EffectsChain.h
    ${SOURCE_DIR}/Effects/EffectsChain.cpp
    ${SOURCE_DIR}/Effects/FilterEffect.h
    ${SOURCE_DIR}/Effects/FilterEffect.cpp
    ${SOURCE_DIR}/Effects/DelayEffect.h
    ${SOURCE_DIR}/Effects/DelayEffect.cpp
    ${SOURCE_DIR}/Effects/GranularEffect.h
    ${SOURCE_DIR}/Effects/GranularEffect.cpp
    ${SOURCE_DIR}/Effects/TremoloEffect.h
    ${SOURCE_DIR}/Effects/TremoloEffect.cpp
    ${SOURCE_DIR}/Effects/DistortionEffect.h
    ${SOURCE_DIR}/Effects/DistortionEffect.cpp
    ${SOURCE_DIR}/Effects/TapeEffect.h
    ${SOURCE_DIR}/Effects/TapeEffect.cpp

    # Modulation
    ${SOURCE_DIR}/Modulation/ModulationSource.h
    ${SOURCE_DIR}/Modulation/ModulationManager.h
    ${SOURCE_DIR}/Modulation/ModulationManager.cpp
    ${SOURCE_DIR}/Modulation/ModulationPanel.h
)

# --- Include directories ---
target_include_directories(DronemakerClonev3 PRIVATE
    ${SOURCE_DIR}
)

# --- Compile definitions ---
target_compile_definitions(DronemakerClonev3 PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DronemakerClonev3,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DronemakerClonev3,JUCE_VERSION>"
)

# --- Link JUCE modules ---
target_link_libraries(DronemakerClonev3 PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
