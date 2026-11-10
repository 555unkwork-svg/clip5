# clip5 — plugin (JUCE 8). VST3 / AU / AAX (si hay SDK) / CLAP / Standalone.
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

FetchContent_Declare(clap_juce_extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_SUBMODULES_RECURSE TRUE)
FetchContent_MakeAvailable(clap_juce_extensions)

set(CLIP5_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND CLIP5_FORMATS AU)
endif()
if(CLIP5_AAX_SDK_PATH)
    juce_set_aax_sdk_path(${CLIP5_AAX_SDK_PATH})
    list(APPEND CLIP5_FORMATS AAX)
endif()

juce_add_plugin(clip5
    COMPANY_NAME "clip5"
    COMPANY_WEBSITE "https://clip5.audio"
    BUNDLE_ID "audio.clip5.clip5"
    PLUGIN_MANUFACTURER_CODE Cl5x
    PLUGIN_CODE Cp5a
    PRODUCT_NAME "clip5"
    DESCRIPTION "Clipper de mastering de alta eficiencia"
    FORMATS ${CLIP5_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Fx Dynamics Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect
    AAX_CATEGORY AAX_ePlugInCategory_Dynamics
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(clip5_assets
    HEADER_NAME Clip5Assets.h
    NAMESPACE Clip5Assets
    SOURCES
        Resources/fonts/Inter-Regular.otf
        Resources/fonts/Inter-SemiBold.otf
        Resources/fonts/JetBrainsMono-Regular.ttf)

target_sources(clip5 PRIVATE
    Source/Params.cpp
    Source/Presets.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ui/Controls.cpp
    Source/ui/Display.cpp
    Source/ui/MeterPanel.cpp
    Source/ui/Theme.cpp)

target_include_directories(clip5 PRIVATE Source)

target_compile_definitions(clip5 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    CLIP5_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(clip5
    PRIVATE
        clip5_dsp
        clip5_assets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

clap_juce_extensions_plugin(TARGET clip5
    CLAP_ID "audio.clip5.clip5"
    CLAP_FEATURES audio-effect distortion mastering stereo mono)

# ------------------------------------------------------------------ herramienta de capturas de la UI
# La herramienta de capturas usa un bucle de mensajes modal: solo se compila en Linux.
if(UNIX AND NOT APPLE)
    set(CLIP5_TOOLS_DEFAULT ON)
else()
    set(CLIP5_TOOLS_DEFAULT OFF)
endif()
option(CLIP5_BUILD_TOOLS "Compilar herramientas de desarrollo" ${CLIP5_TOOLS_DEFAULT})
if(CLIP5_BUILD_TOOLS)
    juce_add_console_app(clip5_snapshot PRODUCT_NAME "clip5_snapshot")
    target_sources(clip5_snapshot PRIVATE ${CMAKE_SOURCE_DIR}/tools/ui_snapshot.cpp)
    target_include_directories(clip5_snapshot PRIVATE Source)
    target_compile_definitions(clip5_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(clip5_snapshot PRIVATE clip5 clip5_dsp juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
