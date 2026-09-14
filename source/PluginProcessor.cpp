#include "PluginProcessor.h"
#include "PluginEditor.h"

// 1. Cleanly inject Windows API functions without disrupting CMake macros
#ifndef NOMINMAX
  #define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>

// 2. Explicit modern JUCE audio processor module inclusion
#include <juce_audio_processors/juce_audio_processors.h>

// --- Dynamic Psycle 64-bit Loader Function Signatures ---
typedef void (*PsycleProcessFunc)(float* leftChannel, float* rightChannel, int sampleCount);
PsycleProcessFunc remotePsycleProcess = nullptr;
HINSTANCE psycleModule = nullptr;

// ==============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ==============================================================================

PluginProcessor::PluginProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

PluginProcessor::~PluginProcessor()
{
    // Clean up memory when the plugin instance is destroyed
    if (psycleModule != nullptr) {
        FreeLibrary(psycleModule);
        psycleModule = nullptr;
        remotePsycleProcess = nullptr;
    }
}

// ==============================================================================
// DYNAMIC DLL LOADING METHOD
// ==============================================================================

void PluginProcessor::loadPsycleDll(const juce::File& dllFile)
{
    // 1. Thread-safe: Stop audio processing briefly while we hot-swap the DLL
    suspendProcessing(true);

    // 2. Safely unload any currently active engine first to prevent memory leaks
    if (psycleModule != nullptr) {
        FreeLibrary(psycleModule);
        psycleModule = nullptr;
        remotePsycleProcess = nullptr;
    }

    // 3. Open the new target path passed from the UI
    if (dllFile.existsAsFile()) {
        currentDllPath = dllFile;
        psycleModule = LoadLibraryA(dllFile.getFullPathName().toRawUTF8());
        
        if (psycleModule != nullptr) {
            // Point directly to the audio rendering function inside the selected Psycle DLL
            remotePsycleProcess = (PsycleProcessFunc)GetProcAddress(psycleModule, "ProcessAudio");
        }
    }

    // 4. Resume audio processing
    suspendProcessing(false);
}

// ==============================================================================
// AUDIO CORE IMPLEMENTATIONS
// ==============================================================================

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);

    // If a DLL was previously chosen, attempt to reload it upon activation
    if (currentDllPath.existsAsFile()) {
        loadPsycleDll(currentDllPath);
    }
}

void PluginProcessor::releaseResources()
{
    // Keeping resources allocated during state transitions to prevent DAWs from clicking,
    // final cleanup is fully handled inside the destructor (~PluginProcessor).
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear extra channels to avoid random digital noise bursts
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Fallback if there are no channels available to prevent crashing
    if (totalNumInputChannels == 0 || buffer.getNumSamples() == 0)
        return;

    // Get direct audio stream pointers from your DAW
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = totalNumInputChannels > 1 ? buffer.getWritePointer(1) : leftChannel;
    int numSamples = buffer.getNumSamples();

    // If the Psycle function pointer is valid, route the audio block straight through it
    if (remotePsycleProcess != nullptr) {
        remotePsycleProcess(leftChannel, rightChannel, numSamples);
    }
}

// ==============================================================================
// BUS LAYOUT
// ==============================================================================

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

// ==============================================================================
// EDITOR
// ==============================================================================

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

bool PluginProcessor::hasEditor() const { return true; }

// ==============================================================================
// INFO
// ==============================================================================

const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const   { return false; } // Changed to false: Pure effect loader
bool PluginProcessor::producesMidi() const  { return false; } // Changed to false: Pure effect loader
bool PluginProcessor::isMidiEffect() const  { return false; }
double PluginProcessor::getTailLengthSeconds() const { return 0.0; }

// ==============================================================================
// PROGRAMS
// ==============================================================================

int PluginProcessor::getNumPrograms()                                        { return 1; }
int PluginProcessor::getCurrentProgram()                                     { return 0; }
void PluginProcessor::setCurrentProgram (int index)                          { juce::ignoreUnused (index); }
const juce::String PluginProcessor::getProgramName (int index)               { juce::ignoreUnused (index); return {}; }
void PluginProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

// ==============================================================================
// STATE (Saves/Restores plugin setup in DAW projects)
// ==============================================================================

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save the last loaded path as a string so your DAW session remembers it
    destData.append (currentDllPath.getFullPathName().toRawUTF8(), 
                     currentDllPath.getFullPathName().getNumBytesAsUTF8());
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (sizeInBytes > 0)
    {
        juce::String restoredPath = juce::String::createStringFromData (data, sizeInBytes);
        juce::File targetFile (restoredPath);
        if (targetFile.existsAsFile()) {
            loadPsycleDll (targetFile);
        }
    }
}

// ==============================================================================
// MANDATORY ENTRY POINT HOOK: Fixes undefined symbol link errors
// ==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
