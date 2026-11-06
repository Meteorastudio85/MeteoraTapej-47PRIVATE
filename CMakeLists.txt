cmake_minimum_required(VERSION 3.22)
# macOS: Mac Intel e Apple Silicon (l'architettura si passa da riga di comando), da macOS 10.13 in su
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Versione minima di macOS")
project(MeteoraTape VERSION 0.4.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Codici invariati (Mtra / Mtp1): i progetti salvati con le versioni precedenti si riaprono senza problemi.
# Su Windows esce il VST3, su macOS il VST3 e l'Audio Unit (AU).
juce_add_plugin(MeteoraTape
    COMPANY_NAME "Meteora Plugins"
    COMPANY_COPYRIGHT "Copyright (c) 2026 Meteora Plugins. Tutti i diritti riservati."
    BUNDLE_ID "com.meteoraplugins.meteoratape"
    PLUGIN_MANUFACTURER_CODE Mtra
    PLUGIN_CODE Mtp1
    FORMATS VST3 AU
    PRODUCT_NAME "Tape J-47"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx)

juce_add_binary_data(MeteoraTapeAssets SOURCES
    Assets/ui.jpg Assets/logo_glow.png Assets/lofi_on.png Assets/lofi_off.png
    Assets/reelL_face.jpg Assets/reelR_face.jpg Assets/reelL_light.png Assets/reelR_light.png
    Assets/fgL.png Assets/fgR.png)

target_sources(MeteoraTape PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(MeteoraTape PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(MeteoraTape
    PRIVATE MeteoraTapeAssets juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
