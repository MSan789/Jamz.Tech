// This file forces compilation of the JUCE audio module implementations
// so that linker has the necessary symbols when the generated include
// files are not added to the Visual Studio project.

#include "../JuceLibraryCode/include_juce_audio_basics.cpp"
#include "../JuceLibraryCode/include_juce_audio_devices.cpp"
#include "../JuceLibraryCode/include_juce_audio_formats.cpp"
