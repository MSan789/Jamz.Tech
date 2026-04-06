#include "AudioCards.h"

AudioCards::AudioCards()
{
    addAndMakeVisible(playButton);
    addAndMakeVisible(editButton);
    playImage = juce::ImageCache::getFromMemory(BinaryData::play_png, BinaryData::play_pngSize);
    playButton.setImages(true, true, true, playImage, 1.0f, juce::Colours::transparentBlack, playImage, 1.0f, juce::Colours::transparentBlack, playImage, 1.0f, juce::Colours::transparentBlack);

    //play button:
    playButton.onClick = [this]()
        {
            if (onPlayClicked)
            {
                onPlayClicked(recording);
            }
        };

    //edit button:
    editButton.onClick = [this]()
        {
            if (onEditClicked)
            {
                onEditClicked(recording);
            }
        };
}

AudioCards::~AudioCards() 
{
}

void AudioCards::setRecording(const RecordingEntry& newRecording)
{
    recording = newRecording;
    repaint();
}

void AudioCards::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced(8.0f);

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRoundedRectangle(area.translated(3, 3), 12.0f);

    g.setColour(juce::Colour(35, 35, 35));
    g.fillRoundedRectangle(area, 12.0f);

    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRoundedRectangle(area, 12.0f, 1.0f);

    auto bounds = getLocalBounds().reduced(16);

    int leftWidth = 70;
    int rightWidth = 90;

    auto leftArea = bounds.removeFromLeft(leftWidth);
    auto rightArea = bounds.removeFromRight(rightWidth);
    auto centerArea = bounds;

    auto textArea = centerArea.withSizeKeepingCentre(centerArea.getWidth(),
        50);

    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText(recording.audioTitle.isNotEmpty() ? recording.audioTitle : "Untitled Recording",
        textArea.removeFromTop(26),
        juce::Justification::centred);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(14.0f);
    g.drawText(recording.accountName.isNotEmpty() ? recording.accountName : "Unknown User",
        textArea.removeFromTop(20),
        juce::Justification::centred);
}

void AudioCards::resized()
{
    auto bounds = getLocalBounds().reduced(16);

    int playSize = 40;
    int editWidth = 70;
    int editHeight = 30;

    auto leftArea = bounds.removeFromLeft(70);
    auto rightArea = bounds.removeFromRight(90);
    auto centerArea = bounds;

    playButton.setBounds(leftArea.withSizeKeepingCentre(playSize, playSize));
    editButton.setBounds(rightArea.withSizeKeepingCentre(editWidth, editHeight));
}