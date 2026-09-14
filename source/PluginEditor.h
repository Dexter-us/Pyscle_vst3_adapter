#pragma once

#include <juce_gui_processors/juce_gui_processors.h>
#include "PluginProcessor.h"

// ==============================================================================
/** Dexter U.S. Psycle Effect Loader Interface Header
*/
class PluginEditor  : public juce::AudioProcessorEditor,
                      public juce::Button::Listener
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    // ==============================================================================
    // JUCE Lifecycle Methods
    void paint (juce::Graphics&) override;
    void resized() override;
    
    // Callback interface for user UI clicks
    void buttonClicked (juce::Button* button) override;

private:
    // Reference to the backend audio processing framework
    PluginProcessor& audioProcessor;

    // UI Interactive Layout Components
    juce::TextButton loadButton   { "Load Psycle Effect (.dll)" };
    juce::Label statusLabel       { "status", "No plugin loaded." };
    juce::TextButton openUiButton  { "Open Native Editor" };

    // Managed file pointer container to securely open native OS browser boxes
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
