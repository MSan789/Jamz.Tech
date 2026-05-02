/*
  ==============================================================================

    AudioCards.cpp
    Created: 28 Feb 2026 10:35:43pm
    Author:  Tanishi Kumar

  ==============================================================================
*/

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

    playButton.setImages(false, true, true, playImage, 1.0f, juce::Colour(), playImage, 1.0f, juce::Colour(), playImage, 1.0f, juce::Colour());
    editButton.setImages(false, true, true, editImage, 1.0f, juce::Colour(), editImage, 1.0f, juce::Colour(), editImage, 1.0f, juce::Colour());
    favoriteButton.setImages(false, true, true, unfavoriteImage, 1.0f, juce::Colour(), unfavoriteImage, 1.0f, juce::Colour(), unfavoriteImage, 1.0f, juce::Colour());
    buyButton.setImages(false, true, true, buyImage, 1.0f, juce::Colour(), buyImage, 1.0f, juce::Colour(), buyImage, 1.0f, juce::Colour());

    playButton.onClick = [this]()
        {
            if (onPlayClicked)
                onPlayClicked(recording);

            isPlaying = !isPlaying;
            repaint(); 
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
            favoriteButton.setImages(false, true, true, img, 1.0f, juce::Colour(), img, 1.0f, juce::Colour(), img, 1.0f, juce::Colour());
            if (onFavoriteToggled)
                onFavoriteToggled(recording, isFavorite);
        };

    buyButton.onClick = [this]()
        {
            if (recording.filePath.isEmpty()) return;
            juce::File sourceFile(recording.filePath);
            if (!sourceFile.existsAsFile()) return;

            auto* chooser = new juce::FileChooser(
                "Save Audio File",
                juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile(sourceFile.getFileName()),
                "*.wav");

            chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [chooser, sourceFile](const juce::FileChooser& fc)
                {
                    auto targetFile = fc.getResult();
                    if (targetFile != juce::File())
                    {
                        if (sourceFile.copyFileTo(targetFile))
                        {
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::AlertWindow::InfoIcon,
                                "Download Complete",
                                "The audio file was successfully saved!");
                        }
                    }
                    delete chooser;
                });
        };

    shadowEffect.setShadowProperties(juce::DropShadow(juce::Colours::black.withAlpha(0.6f), 8, juce::Point<int>(0, 3)));
    setComponentEffect(&shadowEffect);
}

AudioCards::~AudioCards()
{
    setComponentEffect(nullptr);
}

void AudioCards::setFavorite(bool fav)
{
    isFavorite = fav;
    auto& img = isFavorite ? favoriteImage : unfavoriteImage;
    favoriteButton.setImages(false, true, true, img, 1.0f, juce::Colour(), img, 1.0f, juce::Colour(), img, 1.0f, juce::Colour());
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
    g.setFont(juce::Font(juce::FontOptions(18.0f)).boldened());
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
    g.setFont(juce::Font(juce::FontOptions(18.0f)));
    float rowHeight1 = col1.getHeight() / 2.0f;

    g.drawText("Play", juce::Rectangle<float>(col1.getX() + 45.0f, col1.getY(), col1.getWidth() - 45.0f, rowHeight1), juce::Justification::centredLeft, false);
    g.drawText("Edit", juce::Rectangle<float>(col1.getX() + 45.0f, col1.getY() + rowHeight1, col1.getWidth() - 45.0f, rowHeight1), juce::Justification::centredLeft, false);

    // Col 2: Fav, Buy
    float rowHeight2 = col2.getHeight() / 2.0f;
    g.drawText("Like", juce::Rectangle<float>(col2.getX() + 45.0f, col2.getY(), col2.getWidth() - 45.0f, rowHeight2), juce::Justification::centredLeft, false);

    juce::String priceStr = recording.price;
    if (priceStr.isEmpty()) priceStr = "Free";
    else if (priceStr != "Free" && !priceStr.startsWith("$")) priceStr = "$" + priceStr;

    g.drawText(priceStr, juce::Rectangle<float>(col2.getX() + 45.0f, col2.getY() + rowHeight2, col2.getWidth() - 45.0f, rowHeight2), juce::Justification::centredLeft, false);

    // Col 3: Waveform
    auto waveArea = col3.reduced(6.0f);
    auto midY = waveArea.getCentreY();
    auto x0 = waveArea.getX();
    
    float barWidth = 3.0f;
    float gap = 2.0f;
    float step = barWidth + gap;
    int numBars = juce::jmax(1, (int)(waveArea.getWidth() / step));

    if (!waveformPeaks01.empty())
    {
        for (int i = 0; i < numBars; ++i)
        {
            float x = x0 + i * step;
            
            float startRatio = (float)i / (float)numBars;
            float endRatio = (float)(i + 1) / (float)numBars;
            
            int startIdx = (int)(startRatio * waveformPeaks01.size());
            int endIdx = (int)(endRatio * waveformPeaks01.size());
            
            float peak = 0.0f;
            if (startIdx == endIdx)
            {
                 int idx = juce::jlimit(0, (int)waveformPeaks01.size() - 1, startIdx);
                 peak = waveformPeaks01[(size_t)idx];
            }
            else
            {
                 for (int j = startIdx; j < endIdx && j < (int)waveformPeaks01.size(); ++j)
                     peak = juce::jmax(peak, waveformPeaks01[(size_t)j]);
            }
            
            float barHeight = juce::jmax(2.0f, peak * (waveArea.getHeight() * 0.9f));
            
            // Color based on playhead position
            if (isPlaying && startRatio <= currentPlayheadPosition)
                g.setColour(juce::Colours::deeppink);
            else
                g.setColour(juce::Colours::lightpink.withAlpha(0.6f));
                
            g.fillRoundedRectangle(x, midY - barHeight * 0.5f, barWidth, barHeight, 1.5f);
        }
    }
    else
    {
        for (int i = 0; i < numBars; ++i)
        {
            float x = x0 + i * step;
            float startRatio = (float)i / (float)numBars;
            float off = std::abs(std::sin(i * 0.15f)) * (waveArea.getHeight() * 0.8f);
            float barHeight = juce::jmax(2.0f, off);
            
            if (isPlaying && startRatio <= currentPlayheadPosition)
                g.setColour(juce::Colours::deeppink);
            else
                g.setColour(juce::Colours::lightpink.withAlpha(0.6f));
                
            g.fillRoundedRectangle(x, midY - barHeight * 0.5f, barWidth, barHeight, 1.5f);
        }
    }
}

void AudioCards::mouseDown(const juce::MouseEvent& event)
{
    // Only handle seek clicks - buttons handle their own clicks
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(36); // skip header row
    area.removeFromLeft(200); // skip col1 + col2
    auto waveArea = area.reduced(6);
    
    if (waveArea.contains(event.getPosition()) && onSeekRequested)
    {
        float ratio = (float)(event.x - waveArea.getX()) / (float)waveArea.getWidth();
        ratio = juce::jlimit(0.0f, 1.0f, ratio);
        onSeekRequested(ratio);
    }
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
