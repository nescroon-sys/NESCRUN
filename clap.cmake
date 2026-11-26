# Formato CLAP para NESCRUN (FL Studio 2025, Reaper, Bitwig...)
include(FetchContent)
FetchContent_Declare(clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG 3837f4a6ed2ab0ba46aba69ae5b7041c7c64ce74
    GIT_SUBMODULES_RECURSE ON)
FetchContent_MakeAvailable(clap-juce-extensions)

clap_juce_extensions_plugin(TARGET NESCRUN
    CLAP_ID "com.nescrun.vocalsuite"
    CLAP_FEATURES audio-effect mixing pitch-correction compressor equalizer reverb delay)
