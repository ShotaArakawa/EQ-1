cmake_minimum_required(VERSION 3.22)

project(EQ1 VERSION 1.0.0)

add_subdirectory(../JUCE JUCE)

juce_add_plugin(EQ1
    COMPANY_NAME "AudioN"
    PLUGIN_MANUFACTURER_CODE AudN
    PLUGIN_CODE Eq01
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "EQ-1"
    VST3_CATEGORIES "Fx" "EQ"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

juce_generate_juce_header(EQ1)

target_sources(EQ1
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/FilterBand.cpp
        Source/DSP/SpectrumAnalyzer.cpp
        Source/GUI/FlatLookAndFeel.cpp
        Source/GUI/EQGraphComponent.cpp
        Source/GUI/BandControlStrip.cpp
        Source/GUI/TopBarComponent.cpp)

target_compile_definitions(EQ1
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(EQ1
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
