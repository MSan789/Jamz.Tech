#pragma once
#include <JuceHeader.h>
#include "LocalDatabase.h"
#include "HeaderBar.h"

class RecordComponent : public juce::Component,
    public juce::AudioIODeviceCallback,
    private juce::Timer
{
public:
    RecordComponent();
    ~RecordComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    std::function<void()> onBack;

    void startRecording();
    void stopRecording();

    void setAccountName(const juce::String& name);

private:
    bool shouldStartRecordingWhenReady = false;

    HeaderBar headerBar;
    juce::Label titleLabel;
    juce::Label statusLabel;

    // Sidebar Image Buttons
    juce::ImageButton recordButton;
    juce::ImageButton pauseButton;
    juce::ImageButton playButton;
    juce::ImageButton saveButton;
    juce::ImageButton deleteButton;

    juce::Label recordLabel;
    juce::Label pauseLabel;
    juce::Label playLabel;
    juce::Label saveLabel;
    juce::Label deleteLabel;

    juce::Image recordImage;
    juce::Image pauseImage;
    juce::Image playImage;
    juce::Image saveImage;
    juce::Image deleteImage;

    juce::CriticalSection waveformLock;
    std::vector<float> recentPeaks;
    int maxRecentPeaks = 400;
    void timerCallback() override;

    juce::AudioDeviceManager deviceManager;
    juce::TimeSliceThread backgroundThread{ "Recorder Thread" };

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    juce::AudioFormatWriter::ThreadedWriter* activeWriter = nullptr;
    juce::CriticalSection writerLock;

    // Playback
    juce::AudioFormatManager formatManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    // Database
    LocalDatabase database;
    juce::File currentRecordingFile;
    juce::Time recordingStartTime;
    juce::String currentAccountName = "Unknown User";
    juce::String currentCategory = "General";
    juce::String currentAudioTitle = "Untitled Recording";

    // Recording
    juce::String makeName(const juce::String& name);

    double sampleRate = 0.0;
    bool isRecording = false;

    void promptForTitleAndSave();
    void deleteCurrentRecording();
    void playPreview();
    void pausePreview();
    void stopPreview();
    
    float playheadPosition = 0.0f;
    juce::Rectangle<int> playbackWaveArea;
    void mouseDown(const juce::MouseEvent& event) override;

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordComponent)
};