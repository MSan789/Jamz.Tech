#pragma once
#include <JuceHeader.h>
#include "LocalDatabase.h"

//slider settings now added
class FilterWindow : public juce::Component
{
public:
    FilterWindow(float initialPitch, float initialVolume, float initialWarmth)
    {
        setSize(400, 320);

        // ── Pitch ─────────────────────────────────────────────────────────
        pitchLabel.setText("Pitch  (-12 to +12 semitones):", juce::dontSendNotification);
        pitchLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(pitchLabel);

        pitchSlider.setRange(-12.0, 12.0, 0.5);
        pitchSlider.setValue((double)initialPitch);
        pitchSlider.setTextValueSuffix(" st");
        pitchSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        pitchSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
        addAndMakeVisible(pitchSlider);

        //volume
        volumeLabel.setText("Volume  (0x = silent,  1x = original,  2x = double):", juce::dontSendNotification);
        volumeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(volumeLabel);

        volumeSlider.setRange(0.0, 2.0, 0.01);
        volumeSlider.setValue((double)initialVolume);
        volumeSlider.setTextValueSuffix("x");
        volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
        addAndMakeVisible(volumeSlider);

        //warmth
        warmthLabel.setText("Warmth / Reverb  (0 = dry,  1 = full warmth):", juce::dontSendNotification);
        warmthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(warmthLabel);

        warmthSlider.setRange(0.0, 1.0, 0.01);
        warmthSlider.setValue((double)initialWarmth);
        warmthSlider.setTextValueSuffix(" warmth");
        warmthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        warmthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
        addAndMakeVisible(warmthSlider);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(16);
        int labelH = 22;
        int sliderH = 36;
        int gap = 12;

        pitchLabel.setBounds(area.removeFromTop(labelH));
        pitchSlider.setBounds(area.removeFromTop(sliderH));
        area.removeFromTop(gap);

        volumeLabel.setBounds(area.removeFromTop(labelH));
        volumeSlider.setBounds(area.removeFromTop(sliderH));
        area.removeFromTop(gap);

        warmthLabel.setBounds(area.removeFromTop(labelH));
        warmthSlider.setBounds(area.removeFromTop(sliderH));
    }

    float getPitch()  const { return (float)pitchSlider.getValue(); }
    float getVolume() const { return (float)volumeSlider.getValue(); }
    float getWarmth() const { return (float)warmthSlider.getValue(); }

private:
    juce::Label  pitchLabel, volumeLabel, warmthLabel;
    juce::Slider pitchSlider, volumeSlider, warmthSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterWindow)
};


//edit component
class EditComponent : public juce::Component
{
public:
    EditComponent();
    ~EditComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setRecording(const RecordingEntry& entry);

    std::function<void()> onBack;
    std::function<void()> onEditSaved;

private:
    juce::TextButton backButton{ "Back" };
    juce::Label      titleLabel;
    juce::Label      recordingNameLabel;

    juce::TextButton filterButton{ "Filter Sound" };
    juce::TextButton saveButton{ "Save Edited Version" };
    juce::TextButton publishButton{ "Publish" };
    juce::TextButton deleteButton{ "Delete Edit" };

    RecordingEntry currentRecording;

    float pitchSemitones = 0.0f;
    float volumeGain = 1.0f;
    float warmth = 0.0f;

    juce::AudioDeviceManager   previewDeviceManager;
    juce::AudioFormatManager   formatManager;
    juce::AudioSourcePlayer    audioSourcePlayer;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    LocalDatabase database;

    void showFilterPopup();
    void previewWithSettings(float pitch, float volume, float warmthAmt);
    void saveEditedVersion();
    void stopPreview();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditComponent)
};