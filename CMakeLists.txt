cmake_minimum_required(VERSION 3.22)
# macOS: must be set before project(); ignored on other systems.
# Universal (Apple Silicon + Intel). For a faster Intel-only build: -DCMAKE_OSX_ARCHITECTURES=x86_64
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
project(Binocular VERSION 1.0.6 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: -DJUCE_DIR=/path/to/JUCE (default: ../juce next to this folder)
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../juce" CACHE PATH "JUCE source folder")
add_subdirectory(${JUCE_DIR} juce EXCLUDE_FROM_ALL)

set(BN_FORMATS VST3)
if(APPLE)
  list(APPEND BN_FORMATS AU)
endif()

juce_add_plugin(Binocular
  COMPANY_NAME "dmajorsound"
  COMPANY_WEBSITE "https://dmajorsound.com"
  PLUGIN_MANUFACTURER_CODE Dmjs
  PLUGIN_CODE Bnoc
  FORMATS ${BN_FORMATS}
  PRODUCT_NAME "Binocular"
  VERSION 1.0.6
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  VST3_CATEGORIES Fx Analyzer
  AU_MAIN_TYPE kAudioUnitType_Effect
  COPY_PLUGIN_AFTER_BUILD FALSE)

file(GLOB BN_GEN ${CMAKE_CURRENT_SOURCE_DIR}/gen/*.cpp)
target_sources(Binocular PRIVATE
  src/PluginProcessor.cpp src/PluginEditor.cpp src/UpdateCheck.cpp src/JuceGfx.cpp src/rt.cpp src/engine.cpp
  third_party/wdl/fft.c ${BN_GEN})
target_include_directories(Binocular PRIVATE src gen third_party/wdl)

target_compile_definitions(Binocular PUBLIC
  WDL_FFT_REALSIZE=8
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)
option(BN_MINIMAL_X11 "Linux: build without the Xrandr/Xinerama/Xcursor/XInput2/Xrender dev headers" OFF)
if(UNIX AND NOT APPLE AND BN_MINIMAL_X11)
  target_compile_definitions(Binocular PUBLIC JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
                                                JUCE_USE_XINPUT=0 JUCE_USE_XRENDER=0)
endif()

# bit-exact arithmetic with the JSFX: no FMA contraction, no fast-math
if(MSVC)
  target_compile_options(Binocular PRIVATE /fp:precise /bigobj)
else()
  target_compile_options(Binocular PRIVATE -ffp-contract=off -fno-fast-math)
endif()

target_link_libraries(Binocular PRIVATE
  juce::juce_audio_processors juce::juce_gui_basics
  PUBLIC juce::juce_recommended_config_flags)

# CLAP (via clap-juce-extensions, fetched next to JUCE; skipped if missing)
set(CLAP_JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../clap-juce-extensions" CACHE PATH "clap-juce-extensions checkout")
if(EXISTS "${CLAP_JUCE_DIR}/CMakeLists.txt")
  add_subdirectory(${CLAP_JUCE_DIR} clap-juce EXCLUDE_FROM_ALL)
  clap_juce_extensions_plugin(TARGET Binocular
    CLAP_ID "com.dmajorsound.binocular"
    CLAP_FEATURES audio-effect analyzer stereo)
endif()
