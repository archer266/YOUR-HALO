cmake_minimum_required(VERSION 3.22)
project(YOUR_HALO VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(HALO_JUCE_PATH "" CACHE PATH "Existing JUCE 8 source checkout")
if(HALO_JUCE_PATH)
    add_subdirectory("${HALO_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.12 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(YOUR_HALO
    COMPANY_NAME VoxxLab BUNDLE_ID com.voxxlab.yourhalo
    PLUGIN_MANUFACTURER_CODE VxLb PLUGIN_CODE YH01
    PRODUCT_NAME "YOUR HALO" DESCRIPTION "One-knob vocal air, presence and polish"
    FORMATS VST3 IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Fx EQ)
juce_add_binary_data(HaloArtwork HEADER_NAME HaloArtwork.h NAMESPACE HaloArtwork SOURCES Assets/haloLogo.png)
target_sources(YOUR_HALO PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/DSP/HaloEngine.cpp
    Source/UI/HaloOrbComponent.cpp)
target_include_directories(YOUR_HALO PUBLIC Source)
target_compile_features(YOUR_HALO PUBLIC cxx_std_17)
target_compile_definitions(YOUR_HALO PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(YOUR_HALO PRIVATE HaloArtwork juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
option(HALO_BUILD_TESTS "Build regression and VST3 host tests" ON)
if(HALO_BUILD_TESTS)
    enable_testing()
    add_executable(HaloTests Tests/HaloTests.cpp Tests/HaloVisualTests.cpp)
    target_link_libraries(HaloTests PRIVATE YOUR_HALO juce::juce_audio_utils psapi)
    target_compile_definitions(HaloTests PRIVATE JUCE_PLUGINHOST_VST3=1)
    add_dependencies(HaloTests YOUR_HALO_VST3)
    add_test(NAME HaloTests COMMAND HaloTests "$<TARGET_FILE_DIR:YOUR_HALO_VST3>/../.." "${CMAKE_CURRENT_SOURCE_DIR}/Docs/DSPUpgrade")
    add_test(NAME HaloVisualTests COMMAND HaloTests --visual "$<TARGET_FILE_DIR:YOUR_HALO_VST3>/../.."
        "${CMAKE_CURRENT_SOURCE_DIR}/dist/YOUR HALO.vst3" "${CMAKE_CURRENT_SOURCE_DIR}/Docs/DSPUpgrade")
endif()
