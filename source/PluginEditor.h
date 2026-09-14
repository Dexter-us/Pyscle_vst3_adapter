#pragma once
#include "PluginProcessor.h"

class PluginEditor : public juce::AudioProcessorEditor
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
    juce::TextButton inspectButton;
    
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<melatonin::Inspector> inspector;
};
