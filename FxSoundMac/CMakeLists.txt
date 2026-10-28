cmake_minimum_required(VERSION 3.22)
project(FxSoundMac VERSION 0.3.0 LANGUAGES C CXX OBJCXX)

# Must be called before any add_test() calls, including those in subdirectories.
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary archs")

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DfxDspLib.cmake)

include(FetchContent)
# Patch JUCE 7.0.12 for the macOS 15+ / Xcode 26 SDK, where CGWindowListCreateImage
# was removed (not just deprecated). PATCH_COMMAND runs right after checkout, BEFORE
# JUCE configures and builds juceaide at MakeAvailable time — applying it afterwards
# is too late: juceaide compiles the unpatched source and the configure fails.
# The command runs in JUCE's source dir, so the file path is relative. The script is
# idempotent (already-patched -> no-op) so reconfigures are safe.
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 4f43011b96eb0636104cb3e433894cda98243626
    PATCH_COMMAND python3
        "${CMAKE_CURRENT_SOURCE_DIR}/patches/patch_juce_windowing.py"
        "modules/juce_gui_basics/native/juce_Windowing_mac.mm"
)
FetchContent_MakeAvailable(juce)

# Original settings glyph. No upstream logos or proprietary font binaries.
juce_add_binary_data(FxSoundMacAssets
    SOURCES Resources/Images/settings.svg
)

juce_add_gui_app(FxSoundMac
    PRODUCT_NAME "Audio Enhancer for Mac"
    BUNDLE_ID "local.fxsound.community"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/AppIcon.svg"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "FxSound needs audio input access to capture system audio from the BlackHole virtual device for real-time processing."
)

target_sources(FxSoundMac PRIVATE
    Source/App/FxSoundMacApp.cpp
    Source/App/MainWindow.cpp
    Source/DSP/LegacyDspAdapter.cpp
    Source/Audio/DeviceValidation.cpp
    Source/Audio/MacAudioEngine.cpp
    Source/Audio/AudioActivity.cpp
    Source/Audio/SystemAudioRoute.cpp
    Source/Support/PresetLibrary.cpp
    Source/Support/FxController.cpp
    Source/UI/FxTheme.cpp
    Source/UI/FxPowerButton.cpp
    Source/UI/FxEffects.cpp
    Source/UI/FxEqualizer.cpp
    Source/UI/NativeMotion.mm
    Source/UI/MainComponent.cpp
)

target_compile_definitions(FxSoundMac PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:FxSoundMac,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:FxSoundMac,JUCE_VERSION>"
)

target_link_libraries(FxSoundMac PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    FxSoundMacAssets
    DfxDspMac
)

target_include_directories(FxSoundMac PRIVATE
    Source/DSP
    Source/Audio
    Source/Support
    Source/UI
)

# Ad-hoc codesign for local development/testing.
set_target_properties(FxSoundMac PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED YES
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
)

juce_add_console_app(FxSoundMacTests PRODUCT_NAME "FxSoundMacTests")

target_sources(FxSoundMacTests PRIVATE
    Tests/TestMain.cpp
    Tests/ReliabilityTests.cpp
    Tests/DspProbeTest.cpp
    Tests/LegacyDspAdapterTest.cpp
    Tests/SystemVolumeBridgeTest.cpp
    Tests/AudioActivityTest.cpp
    Tests/DeviceValidationTest.cpp
    Tests/PresetLibraryTest.cpp
    Tests/FxControllerTest.cpp
    Source/DSP/LegacyDspAdapter.cpp
    Source/Audio/DeviceValidation.cpp
    Source/Audio/AudioActivity.cpp
    Source/Support/PresetLibrary.cpp
    Source/Support/FxController.cpp
)

target_link_libraries(FxSoundMacTests PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    DfxDspMac
)

target_include_directories(FxSoundMacTests PRIVATE
    ${REPO_ROOT}/dsp/include
    Source/DSP
    Source/Audio
    Source/Support
)

target_compile_definitions(FxSoundMacTests PRIVATE FXMAC_TEST_PRESET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Resources/Presets")
add_test(NAME FxSoundMacTests COMMAND FxSoundMacTests)

# Verify NSMicrophoneUsageDescription is present in the built app bundle.
add_test(NAME InfoPlistMicCheck
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Tests/check_info_plist.sh
            $<TARGET_BUNDLE_DIR:FxSoundMac>)

# Bundle curated presets into the app at Contents/Resources/Presets/
set(FXMAC_PRESET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Presets)

add_custom_command(TARGET FxSoundMac POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_CONTENT_DIR:FxSoundMac>/Resources/Presets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${FXMAC_PRESET_DIR}"
        "$<TARGET_BUNDLE_CONTENT_DIR:FxSoundMac>/Resources/Presets"
    COMMENT "Bundling presets into FxSoundMac.app"
)

# The original 1024px vector icon is rendered by JUCE; no upstream icon override.
