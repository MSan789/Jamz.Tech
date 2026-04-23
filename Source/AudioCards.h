#pragma once
#include <JuceHeader.h>
#include "LocalDatabase.h"

class AudioCards : public juce::Component
{
public:
    AudioCards();
    ~AudioCards() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setRecording(const RecordingEntry& newRecording);
    const RecordingEntry& getRecording() const { return recording; }
    void setIsPlaying(bool shouldShowWaveform);
    void setWaveformPeaks(std::vector<float> peaks01);

    std::function<void(const RecordingEntry&)> onPlayClicked;
    std::function<void(const RecordingEntry&)> onEditClicked;

private:
    RecordingEntry recording;

    juce::ImageButton playButton;
    juce::ImageButton editButton;
    juce::ImageButton favoriteButton;
    juce::ImageButton buyButton;

    juce::Image playImage;
    juce::Image editImage;
    juce::Image unfavoriteImage;
    juce::Image favoriteImage;
    juce::Image buyImage;

    juce::DropShadowEffect shadowEffect;

    bool isPlaying = false;
    bool isFavorite = false;
    std::vector<float> waveformPeaks01;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioCards)
};
