#pragma once
#include <JuceHeader.h>
#include "LocalDatabase.h"
#include "HeaderBar.h"

//edit component
class EditComponent : public juce::Component, public juce::Timer
{
public:
    EditComponent();
    ~EditComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setRecording(const RecordingEntry& entry);

    std::function<void()> onBack;
    std::function<void()> onEditSaved;

private:
    HeaderBar        headerBar;
    juce::Label      titleLabel;
    juce::Label      recordingNameLabel;

    // Sliders & Labels
    juce::Label  pitchLabel, volumeLabel, warmthLabel;
    juce::Slider pitchSlider, volumeSlider, warmthSlider;

    // Sidebar Image Buttons
    juce::ImageButton pauseButton;
    juce::ImageButton playButton;
    juce::ImageButton saveButton;
    juce::ImageButton deleteButton;

    juce::Label pauseLabel;
    juce::Label playLabel;
    juce::Label saveLabel;
    juce::Label deleteLabel;

    juce::Image pauseImage;
    juce::Image playImage;
    juce::Image saveImage;
    juce::Image deleteImage;

    RecordingEntry currentRecording;

    juce::AudioDeviceManager   previewDeviceManager;
    juce::AudioFormatManager   formatManager;
    juce::AudioSourcePlayer    audioSourcePlayer;
    juce::AudioTransportSource transportSource;
    juce::ResamplingAudioSource resamplerSource{ &transportSource, false, 2 };
    juce::ReverbAudioSource reverbSource{ &resamplerSource, false };
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    LocalDatabase database;

    std::vector<float> waveformPeaks;
    std::vector<float> computeWaveformPeaks01(const juce::File& file, int numPoints);
    float playheadPosition = 0.0f;
    juce::Rectangle<int> waveAreaBounds; // stored for hit-testing

    void saveEditedVersion();
    void deleteCurrentEdit();
    void playPreview();
    void pausePreview();
    void stopPreview();
    void updateTransportParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditComponent)
};