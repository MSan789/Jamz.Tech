#pragma once
#include <JuceHeader.h>
#include "LocalDatabase.h"

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
    juce::TextButton backButton{ "Back" };
    juce::TextButton recordButton{ "Start Recording" };
    juce::Label titleLabel;
    juce::Label statusLabel;

    juce::CriticalSection waveformLock;
    std::vector<float> recentPeaks;
    int maxRecentPeaks = 400;
    void timerCallback() override;

    juce::AudioDeviceManager deviceManager;
    juce::TimeSliceThread backgroundThread{ "Recorder Thread" };

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    juce::AudioFormatWriter::ThreadedWriter* activeWriter = nullptr;
    juce::CriticalSection writerLock;

    //Database
	LocalDatabase database;
	juce::File currentRecordingFile;
	juce::Time recordingStartTime;
    juce::String currentAccountName = "Unknown User";
    juce::String currentCategory = "General";
	juce::String currentAudioTitle = "Untitled Recording";

    //Recording
	juce::String makeName(const juce::String& name);


    double sampleRate = 0.0;
    bool isRecording = false;
    bool navigateBackAfterSavePrompt = false;

    void promptForTitleAndSave();
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
