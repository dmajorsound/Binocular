cmake_minimum_required(VERSION 3.22)
project(bnhost VERSION 1.0.0)
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../juce" CACHE PATH "JUCE source folder")
add_subdirectory(${JUCE_DIR} juce EXCLUDE_FROM_ALL)
juce_add_console_app(bnhost PRODUCT_NAME "bnhost")
target_sources(bnhost PRIVATE Main.cpp)
target_compile_definitions(bnhost PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
  JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XINPUT=0 JUCE_USE_XRENDER=0)
target_link_libraries(bnhost PRIVATE juce::juce_audio_processors juce::juce_gui_basics juce::juce_recommended_config_flags)
