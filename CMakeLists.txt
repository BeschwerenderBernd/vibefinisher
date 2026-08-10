cmake_minimum_required(VERSION 3.22)

project(VibeFinisher VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0)

find_package(JUCE CONFIG QUIET)

if (NOT JUCE_FOUND)
    add_subdirectory(third_party/JUCE third_party/JUCE)
endif()

juce_add_plugin(VibeFinisher
    COMPANY_NAME "VibeFinisher"
    COMPANY_WEBSITE "https://example.com"
    COMPANY_EMAIL "dev@example.com"
    FORMATS AU VST3
    PRODUCT_NAME "VibeFinisher"
    PLUGIN_MANUFACTURER_CODE ViFi
    PLUGIN_CODE Vfin
    DESCRIPTION "Multi-saturation vibe finisher for mix bus processing"
    VST3_CATEGORIES "Fx|Distortion"
    VST3_AUTO_MANIFEST TRUE
    AU_MAIN_TYPE kAudioUnitType_Effect
    AU_EXPORT_PREFIX VibeFinisherAU
    BUNDLE_ID "com.vibefinisher.plugin"
    VST3_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
    AU_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/Components"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(VibeFinisher PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(VibeFinisher PRIVATE Source)

target_compile_definitions(VibeFinisher PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VibeFinisher PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

set_target_properties(VibeFinisher PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
)
