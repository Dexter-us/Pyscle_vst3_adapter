#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <windows.h> // Targets Windows DAWs

#include <JuceHeader.h> 

// Define the exact audio processing signature expected by the Psycle machine
typedef void (*PsycleProcessFunc)(float* leftChannel, float* rightChannel, int sampleCount);
PsycleProcessFunc remotePsycleProcess = nullptr;
HINSTANCE psycleModule = nullptr;

void PsycleLoaderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Locates the directory where your VST3 plugin is installed inside your DAW
    juce::File pluginDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    juce::File psycleDll = pluginDir.getChildFile("native_psycle_plugin.dll");

    // Dynamically open the 64-bit pre-compiled binary
    if (psycleDll.existsAsFile()) {
        psycleModule = LoadLibraryA(psycleDll.getFullPathName().toRawUTF8());
        if (psycleModule != nullptr) {
            // Point directly to the audio rendering function inside the Psycle DLL
            remotePsycleProcess = (PsycleProcessFunc)GetProcAddress(psycleModule, "ProcessAudio");
        }
    }
}

void PsycleLoaderAudioProcessor::releaseResources()
{
    // Clean up memory and unload the Psycle plugin when the VST3 is removed from a track
    if (psycleModule != nullptr) {
        FreeLibrary(psycleModule);
        psycleModule = nullptr;
        remotePsycleProcess = nullptr;
    }
}

void PsycleLoaderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Get direct 64-bit audio stream pointers from your DAW
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();

    // If the 64-bit Psycle file is found and hooked, stream the audio through it
    if (remotePsycleProcess != nullptr) {
        remotePsycleProcess(leftChannel, rightChannel, numSamples);
    }
}
