#include "AudioCards.h"

AudioCards::AudioCards()
{
    playImage = juce::ImageCache::getFromMemory(BinaryData::play_png, BinaryData::play_pngSize);
    editImage = juce::ImageCache::getFromMemory(BinaryData::edit_png, BinaryData::edit_pngSize);
    unfavoriteImage = juce::ImageCache::getFromMemory(BinaryData::unfavorite_png, BinaryData::unfavorite_pngSize);
    favoriteImage = juce::ImageCache::getFromMemory(BinaryData::favorite_png, BinaryData::favorite_pngSize);
    buyImage = juce::ImageCache::getFromMemory(BinaryData::buy_png, BinaryData::buy_pngSize);

    addAndMakeVisible(playButton);
    addAndMakeVisible(editButton);
    addAndMakeVisible(favoriteButton);
    addAndMakeVisible(buyButton);

    playButton.setImages(true, true, true, playImage, 1.0f, juce::Colour(), playImage, 1.0f, juce::Colour(), playImage, 1.0f, juce::Colour());
    editButton.setImages(true, true, true, editImage, 1.0f, juce::Colour(), editImage, 1.0f, juce::Colour(), editImage, 1.0f, juce::Colour());
    favoriteButton.setImages(true, true, true, unfavoriteImage, 1.0f, juce::Colour(), unfavoriteImage, 1.0f, juce::Colour(), unfavoriteImage, 1.0f, juce::Colour());
    buyButton.setImages(true, true, true, buyImage, 1.0f, juce::Colour(), buyImage, 1.0f, juce::Colour(), buyImage, 1.0f, juce::Colour());

    playButton.onClick = [this]()
        {
            if (onPlayClicked)
                onPlayClicked(recording);
        };

    editButton.onClick = [this]()
        {
            if (onEditClicked)
                onEditClicked(recording);
        };

    favoriteButton.onClick = [this]()
        {
            isFavorite = !isFavorite;
            auto& img = isFavorite ? favoriteImage : unfavoriteImage;
            favoriteButton.setImages(true, true, true, img, 1.0f, juce::Colour(), img, 1.0f, juce::Colour(), img, 1.0f, juce::Colour());
        };

    shadowEffect.setShadowProperties(juce::DropShadow(juce::Colours::black.withAlpha(0.6f), 8, juce::Point<int>(0, 3)));
    setComponentEffect(&shadowEffect);
}

AudioCards::~AudioCards() 
{
    setComponentEffect(nullptr);
}

void AudioCards::setRecording(const RecordingEntry& newRecording)
{
    recording = newRecording;
    waveformPeaks01.clear();

    repaint();
}

void AudioCards::setIsPlaying(bool shouldShowWaveform)
{
    if (isPlaying == shouldShowWaveform)
        return;

    isPlaying = shouldShowWaveform;
    repaint();
}

void AudioCards::setWaveformPeaks(std::vector<float> peaks01)
{
    waveformPeaks01 = std::move(peaks01);
    repaint();
}

void AudioCards::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced(8.0f);

    // card background: Frosted glass
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.fillRoundedRectangle(area, 12.0f);

    // border
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRoundedRectangle(area, 12.0f, 1.5f);

    // Header Area
    auto headerArea = area.removeFromTop(36.0f);
    g.drawLine(headerArea.getX(), headerArea.getBottom(), headerArea.getRight(), headerArea.getBottom(), 1.5f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    juce::String title = recording.audioTitle.isNotEmpty() ? recording.audioTitle : "Audio.mp3";
    juce::String creator = recording.accountName.isNotEmpty() ? recording.accountName : "Creator";
    g.drawText(title + " - " + creator, headerArea, juce::Justification::centred, false);

    // Bottom Area
    float col1Width = 100.0f;
    float col2Width = 100.0f;

    auto col1 = area.removeFromLeft(col1Width);
    auto col2 = area.removeFromLeft(col2Width);
    auto col3 = area;

    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawLine(col1.getRight(), col1.getY(), col1.getRight(), col1.getBottom(), 1.5f);
    g.drawLine(col2.getRight(), col2.getY(), col2.getRight(), col2.getBottom(), 1.5f);

    // Col 1: Play, Edit
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(18.0f);
    float rowHeight1 = col1.getHeight() / 2.0f;

    g.drawText("Play", col1.getX() + 45.0f, col1.getY(), col1.getWidth() - 45.0f, rowHeight1, juce::Justification::centredLeft, false);
    g.drawText("Edit", col1.getX() + 45.0f, col1.getY() + rowHeight1, col1.getWidth() - 45.0f, rowHeight1, juce::Justification::centredLeft, false);

    // Col 2: Fav, Buy
    float rowHeight2 = col2.getHeight() / 2.0f;
    g.drawText("Fav", col2.getX() + 45.0f, col2.getY(), col2.getWidth() - 45.0f, rowHeight2, juce::Justification::centredLeft, false);
    g.drawText("$ Buy", col2.getX() + 45.0f, col2.getY() + rowHeight2, col2.getWidth() - 45.0f, rowHeight2, juce::Justification::centredLeft, false);

    // Col 3: Waveform
    auto waveArea = col3.reduced(6.0f);
    g.setColour(juce::Colours::lightpink.withAlpha(0.6f));
    juce::Path p;
    auto midY = waveArea.getCentreY();
    auto x0 = waveArea.getX();
    float dx = waveArea.getWidth() / 50.0f;

    if (!waveformPeaks01.empty())
    {
        dx = waveArea.getWidth() / (float) juce::jmax(1, (int) waveformPeaks01.size() - 1);
        p.startNewSubPath(x0, midY);
        for (int i = 0; i < (int) waveformPeaks01.size(); ++i)
        {
            auto x = x0 + dx * (float) i;
            auto y = midY - waveformPeaks01[(size_t) i] * (waveArea.getHeight() * 0.45f);
            p.lineTo(x, y);
        }
    }
    else
    {
        p.startNewSubPath(x0, midY);
        for (int i = 0; i <= 50; ++i)
        {
            auto x = x0 + dx * i;
            float off = std::sin(i * 1.5f) * (waveArea.getHeight() * 0.4f);
            p.lineTo(x, midY + off);
        }
    }
    g.strokePath(p, juce::PathStrokeType(3.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
}

void AudioCards::resized()
{
    auto area = getLocalBounds().reduced(8);

    auto headerArea = area.removeFromTop(36);

    int col1Width = 100;
    int col2Width = 100;

    auto col1 = area.removeFromLeft(col1Width);
    auto col2 = area.removeFromLeft(col2Width);
    auto col3 = area;
    
    int rowHeight1 = col1.getHeight() / 2;
    auto playRow = col1.removeFromTop(rowHeight1);
    auto editRow = col1;

    int rowHeight2 = col2.getHeight() / 2;
    auto favRow = col2.removeFromTop(rowHeight2);
    auto buyRow = col2;

    playButton.setBounds(playRow.removeFromLeft(45).withSizeKeepingCentre(24, 24));
    editButton.setBounds(editRow.removeFromLeft(45).withSizeKeepingCentre(24, 24));

    favoriteButton.setBounds(favRow.removeFromLeft(45).withSizeKeepingCentre(24, 24));
    buyButton.setBounds(buyRow.removeFromLeft(45).withSizeKeepingCentre(24, 24));
}
