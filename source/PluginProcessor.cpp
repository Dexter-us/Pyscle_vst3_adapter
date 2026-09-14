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

// Add these implementations after the include statements and before prepareToPlay()

PluginProcessor::PluginProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

PluginProcessor::~PluginProcessor()
{
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

bool PluginProcessor::acceptsMidi() const   { return true; }
bool PluginProcessor::producesMidi() const  { return true; }
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
// STATE
// ==============================================================================

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}
// --- Psycle 64-bit Loader Configuration ---
typedef void (*PsycleProcessFunc)(float* leftChannel, float* rightChannel, int sampleCount);
PsycleProcessFunc remotePsycleProcess = nullptr;
HINSTANCE psycleModule = nullptr;

// ==============================================================================
// Audio Processor Core Implementations
// ==============================================================================

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
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

void PluginProcessor::releaseResources()
{
    // Clean up memory and unload the Psycle plugin when the VST3 is removed from a track
    if (psycleModule != nullptr) {
        FreeLibrary(psycleModule);
        psycleModule = nullptr;
        remotePsycleProcess = nullptr;
    }
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Clear extra channels to avoid random noise bursts
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Fallback if there are no channels available to prevent crashing
    if (totalNumInputChannels == 0 || buffer.getNumSamples() == 0)
        return;

    // Get direct 64-bit audio stream pointers from your DAW
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = totalNumInputChannels > 1 ? buffer.getWritePointer(1) : leftChannel;
    int numSamples = buffer.getNumSamples();

    // If the 64-bit Psycle file is found and hooked, stream the audio through it
    if (remotePsycleProcess != nullptr) {
        remotePsycleProcess(leftChannel, rightChannel, numSamples);
    }
}

// ==============================================================================
// MANDATORY ENTRY POINT HOOK: Fixes undefined symbol link errors
// ==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
