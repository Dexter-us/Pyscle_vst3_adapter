#include "PluginEditor.h"
#include "PluginProcessor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (450, 220);

    statusLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    addAndMakeVisible (statusLabel);

    loadButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF1C1C1F));
    loadButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    loadButton.addListener (this);
    addAndMakeVisible (loadButton);

    openUiButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2A2A2E));
    openUiButton.setColour (juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    openUiButton.addListener (this);
    openUiButton.setEnabled (false);
    addAndMakeVisible (openUiButton);

    inspectButton.setButtonText ("Inspect");
    inspectButton.onClick = [this] {
        if (!inspector)
            inspector = std::make_unique<melatonin::Inspector> (*this);
        inspector->onClose = [this]() { inspector.reset(); };
    };
    addAndMakeVisible (inspectButton);

    if (audioProcessor.currentDllPath.existsAsFile())
    {
        statusLabel.setText ("Loaded: " + audioProcessor.currentDllPath.getFileName(), juce::dontSendNotification);
        openUiButton.setEnabled (true);
    }
}

PluginEditor::~PluginEditor()
{
    loadButton.removeListener (this);
    openUiButton.removeListener (this);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFFFFDE00)); 
    g.setColour (juce::Colours::black.withAlpha (0.15f));
    g.drawRect (getLocalBounds(), 4);

    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("Dexter U.S. Psycle Effect Loader", 
                0, 15, getWidth(), 30, 
                juce::Justification::centred, true);

    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.drawLine (30.0f, 50.0f, getWidth() - 30.0f, 50.0f, 1.5f);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (45);

    int elementHeight = 36;
    int spacing = 12;

    statusLabel.setBounds (area.removeFromTop (elementHeight));
    area.removeFromTop (spacing);
    loadButton.setBounds (area.removeFromTop (elementHeight));
    area.removeFromTop (spacing);
    openUiButton.setBounds (area.removeFromTop (elementHeight));
}

void PluginEditor::buttonClicked (juce::Button* button)
{
    if (button == &loadButton)
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Select a Psycle Effect DLL...",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.dll"
        );

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                audioProcessor.loadPsycleDll (file);
                statusLabel.setText ("Loaded: " + file.getFileName(), juce::dontSendNotification);
                openUiButton.setEnabled (true);
            }
        });
    }
    else if (button == &openUiButton)
    {
        // Optional hook for native Psycle DLL UI windows
    }
}
