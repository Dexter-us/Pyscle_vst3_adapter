//#include "PluginProcessor.h"
//#include "PluginEditor.h"
#pragma once
#include "PluginProcessor.h"

// Forward declaration or full class definition
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    PluginEditor (PluginProcessor& p);
    ~PluginEditor();
    
    // ... rest of class definition



private:
    PluginProcessor& audioProcessor;
    
    juce::TextButton loadButton;
    juce::Label statusLabel;
    juce::TextButton openUiButton;
    
    // Add this line inside your private properties to manage memory safely:
    std::unique_ptr<juce::FileChooser> fileChooser; 


PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Configure Window Dimensions
    setSize (450, 220);

    // Setup Status Label (Displays what DLL is active)
    statusLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::black); // Dark text for light background
    addAndMakeVisible (statusLabel);

    // Setup Load Button (Custom dark look to stand out against yellow)
    loadButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF1C1C1F));
    loadButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    loadButton.addListener (this);
    addAndMakeVisible (loadButton);

    // Setup Native UI Opener Button
    openUiButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2A2A2E));
    openUiButton.setColour (juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    openUiButton.addListener (this);
    openUiButton.setEnabled (false); // Disabled until a valid DLL is verified
    addAndMakeVisible (openUiButton);

    // Sync initial UI state with the processor's active path
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

// ==============================================================================
// GRAPHICS & PAINTING
// ==============================================================================

void PluginEditor::paint (juce::Graphics& g)
{
    // 1. Fill background with a clean, solid Yellow
    g.fillAll (juce::Colour (0xFFFFDE00)); 
    
    // 2. Draw a subtle inner border for a polished UI look
    g.setColour (juce::Colours::black.withAlpha (0.15f));
    g.drawRect (getLocalBounds(), 4);

    // 3. Render the Custom Header Text directly onto the canvas
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    
    // Position text cleanly across the top area
    g.drawText ("Dexter U.S. Psycle Effect Loader", 
                0, 15, getWidth(), 30, 
                juce::Justification::centred, true);

    // 4. Draw a visual divider line under the title
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.drawLine (30.0f, 50.0f, getWidth() - 30.0f, 50.0f, 1.5f);
}

// ==============================================================================
// LAYOUT & POSITIONING
// ==============================================================================

void PluginEditor::resized()
{
    // Create a bounding box and carve out padding
    auto area = getLocalBounds().reduced (20);
    
    // Skip past our custom graphics header space
    area.removeFromTop (45);

    int elementHeight = 36;
    int spacing = 12;

    // Arrange elements vertically down the window
    statusLabel.setBounds (area.removeFromTop (elementHeight));
    area.removeFromTop (spacing);
    
    loadButton.setBounds (area.removeFromTop (elementHeight));
    area.removeFromTop (spacing);
    
    openUiButton.setBounds (area.removeFromTop (elementHeight));
}

// ==============================================================================
// USER INTERACTION
// ==============================================================================

void PluginEditor::buttonClicked (juce::Button* button)
{
    if (button == &loadButton)
    {
        // Initialize Native OS File Chooser Dialog
        fileChooser = std::make_unique<juce::FileChooser> (
            "Select a Psycle Effect DLL...",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.dll"
        );

        // Open Dialog Box Asynchronously
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                // Push the file selection to the processing core
                audioProcessor.loadPsycleDll (file);
                
                // Update UI Feedback loops
                statusLabel.setText ("Loaded: " + file.getFileName(), juce::dontSendNotification);
                openUiButton.setEnabled (true);
            }
        });
    }
    else if (button == &openUiButton)
    {
        // Optional hook: Call internal edit windows exported from the native Psycle DLL
    }
}
