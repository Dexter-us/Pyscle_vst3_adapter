#pragma once
#include "PluginProcessor.h"

class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::Button::Listener
{
public:
    PluginEditor (PluginProcessor& p);
    ~PluginEditor();
    
    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* button) override;

private:
    PluginProcessor& audioProcessor;
    
    juce::TextButton loadButton;
    juce::Label statusLabel;
    juce::TextButton openUiButton;
    
    std::unique_ptr<juce::FileChooser> fileChooser;
};
