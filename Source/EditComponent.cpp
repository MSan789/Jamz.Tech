#include "EditComponent.h"

EditComponent::EditComponent()
{
    addAndMakeVisible(headerBar);
    headerBar.onLogoClicked = [this]() {
        stopPreview();
        if (onBack) onBack();
    };

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Edit", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(32.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);

    // Initialize labels
    auto setupLabel = [this](juce::Label& lbl, const juce::String& text) {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setColour(juce::Label::textColourId, juce::Colours::white);
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setFont(18.0f);
        addAndMakeVisible(lbl);
    };

    setupLabel(pitchLabel, "Pitch");
    setupLabel(volumeLabel, "Volume");
    setupLabel(warmthLabel, "Reverb");
    
    // Sliders
    pitchSlider.setRange(-12.0, 12.0, 0.5);
    pitchSlider.setValue(0.0);
    pitchSlider.setTextValueSuffix(" st");
    
    volumeSlider.setRange(0.0, 2.0, 0.01);
    volumeSlider.setValue(1.0);
    volumeSlider.setTextValueSuffix("x");
    
    warmthSlider.setRange(0.0, 1.0, 0.01);
    warmthSlider.setValue(0.0);
    warmthSlider.setTextValueSuffix(" warmth");

    auto setupSlider = [this](juce::Slider& s) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
        s.onValueChange = [this]() { updateTransportParameters(); };
        addAndMakeVisible(s);
    };

    setupSlider(pitchSlider);
    setupSlider(volumeSlider);
    setupSlider(warmthSlider);

    // Sidebar
    pauseImage = juce::ImageCache::getFromMemory(BinaryData::pause_png, BinaryData::pause_pngSize);
    playImage = juce::ImageCache::getFromMemory(BinaryData::play_png, BinaryData::play_pngSize);
    saveImage = juce::ImageCache::getFromMemory(BinaryData::save_png, BinaryData::save_pngSize);
    deleteImage = juce::ImageCache::getFromMemory(BinaryData::delete_png, BinaryData::delete_pngSize);

    auto setupBtn = [this](juce::ImageButton& btn, juce::Label& lbl, const juce::String& text, juce::Image& img) {
        btn.setImages(false, true, true, img, 1.0f, juce::Colour(), img, 1.0f, juce::Colour(), img, 1.0f, juce::Colour());
        addAndMakeVisible(btn);
        
        lbl.setText(text, juce::dontSendNotification);
        lbl.setColour(juce::Label::textColourId, juce::Colours::white);
        lbl.setFont(16.0f);
        lbl.setJustificationType(juce::Justification::centredTop);
        addAndMakeVisible(lbl);
    };

    setupBtn(pauseButton, pauseLabel, "Pause", pauseImage);
    setupBtn(playButton, playLabel, "Play", playImage);
    setupBtn(saveButton, saveLabel, "Save", saveImage);
    setupBtn(deleteButton, deleteLabel, "Delete", deleteImage);

    pauseButton.onClick = [this]() { pausePreview(); };
    playButton.onClick = [this]() { playPreview(); };
    saveButton.onClick = [this]() { saveEditedVersion(); };
    deleteButton.onClick = [this]() { deleteCurrentEdit(); };

    formatManager.registerBasicFormats();
    previewDeviceManager.initialise(0, 2, nullptr, true);
    audioSourcePlayer.setSource(&reverbSource);
    previewDeviceManager.addAudioCallback(&audioSourcePlayer);
}

EditComponent::~EditComponent()
{
    stopPreview();
    stopTimer();
    previewDeviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
}

void EditComponent::setRecording(const RecordingEntry& entry)
{
    currentRecording = entry;
    pitchSlider.setValue(0.0, juce::dontSendNotification);
    volumeSlider.setValue(1.0, juce::dontSendNotification);
    warmthSlider.setValue(0.0, juce::dontSendNotification);

    juce::File file(entry.filePath);
    waveformPeaks = computeWaveformPeaks01(file, 200);

    repaint();
}

std::vector<float> EditComponent::computeWaveformPeaks01(const juce::File& file, int numPoints)
{
    std::vector<float> peaks;
    peaks.resize((size_t) juce::jmax(0, numPoints), 0.0f);

    if (!file.existsAsFile() || numPoints <= 0)
        return peaks;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return peaks;

    const auto totalSamples = (int64_t) reader->lengthInSamples;
    const auto samplesPerPoint = juce::jmax<int64_t>(1, totalSamples / (int64_t) numPoints);

    juce::AudioBuffer<float> buffer(1, 8192);

    for (int i = 0; i < numPoints; ++i)
    {
        const int64_t start = (int64_t) i * samplesPerPoint;
        const int64_t end = juce::jmin(totalSamples, start + samplesPerPoint);

        float peak = 0.0f;
        int64_t pos = start;

        while (pos < end)
        {
            const int toRead = (int) juce::jmin<int64_t>(buffer.getNumSamples(), end - pos);
            buffer.clear();

            reader->read(&buffer, 0, toRead, pos, true, false);
            peak = juce::jmax(peak, buffer.getMagnitude(0, 0, toRead));

            pos += toRead;
        }

        peaks[(size_t) i] = juce::jlimit(0.0f, 1.0f, peak);
    }

    return peaks;
}

void EditComponent::paint(juce::Graphics& g)
{
    juce::ColourGradient gradient(
        juce::Colour(40, 20, 60), 0, 0,
        juce::Colour(70, 30, 50), (float)getWidth(), (float)getHeight(),
        false);
    gradient.addColour(0.3, juce::Colour(50, 20, 70));
    gradient.addColour(0.7, juce::Colour(80, 40, 40));

    g.setGradientFill(gradient);
    g.fillAll();

    // Layout boundaries
    auto area = getLocalBounds();
    area.removeFromTop(64); // headerBar

    auto topArea = area.removeFromTop(100); // 3 columns
    
    // Draw borders for columns
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    int colWidth = getWidth() / 3;
    g.drawLine((float)colWidth, (float)topArea.getY(), (float)colWidth, (float)topArea.getBottom(), 2.0f);
    g.drawLine((float)(colWidth * 2), (float)topArea.getY(), (float)(colWidth * 2), (float)topArea.getBottom(), 2.0f);
    g.drawLine(0.0f, (float)topArea.getBottom(), (float)getWidth(), (float)topArea.getBottom(), 2.0f);
    
    // Draw left sidebar border
    auto mainArea = area;
    auto sidebarArea = mainArea.removeFromLeft(120);
    g.drawLine((float)sidebarArea.getRight(), (float)sidebarArea.getY(), (float)sidebarArea.getRight(), (float)sidebarArea.getBottom(), 2.0f);

    // Waveform
    auto waveArea = mainArea.reduced(20.0f).toFloat();
    float barWidth = 3.0f;
    float gap = 2.0f;
    float step = barWidth + gap;
    int numBars = juce::jmax(1, (int)(waveArea.getWidth() / step));
    auto midY = waveArea.getCentreY();
    auto x0 = waveArea.getX();

    g.setColour(juce::Colours::lightpink.withAlpha(0.8f));

    if (!waveformPeaks.empty())
    {
        for (int i = 0; i < numBars; ++i)
        {
            float x = x0 + i * step;
            float startRatio = (float)i / (float)numBars;
            float endRatio = (float)(i + 1) / (float)numBars;
            
            int startIdx = (int)(startRatio * waveformPeaks.size());
            int endIdx = (int)(endRatio * waveformPeaks.size());
            
            float peak = 0.0f;
            if (startIdx == endIdx)
            {
                 int idx = juce::jlimit(0, (int)waveformPeaks.size() - 1, startIdx);
                 peak = waveformPeaks[(size_t)idx];
            }
            else
            {
                 for (int j = startIdx; j < endIdx && j < (int)waveformPeaks.size(); ++j)
                     peak = juce::jmax(peak, waveformPeaks[(size_t)j]);
            }
            
            float barHeight = juce::jmax(2.0f, peak * (waveArea.getHeight() * 0.9f));
            
            // Color bars before the playhead hot pink, rest light pink
            if (transportSource.isPlaying() && startRatio <= playheadPosition)
                g.setColour(juce::Colours::deeppink);
            else
                g.setColour(juce::Colours::lightpink.withAlpha(0.8f));
                
            g.fillRoundedRectangle(x, midY - barHeight * 0.5f, barWidth, barHeight, 1.5f);
        }
    }
    
    // Store wave area for hit-testing in mouseDown
    waveAreaBounds = mainArea.reduced(20);
}

void EditComponent::resized()
{
    auto area = getLocalBounds();
    headerBar.setBounds(area.removeFromTop(64));

    titleLabel.setBounds(area.removeFromTop(40));

    auto topArea = area.removeFromTop(100);
    int colW = topArea.getWidth() / 3;

    auto col1 = topArea.removeFromLeft(colW).reduced(10);
    pitchLabel.setBounds(col1.removeFromTop(30));
    pitchSlider.setBounds(col1.removeFromTop(40));

    auto col2 = topArea.removeFromLeft(colW).reduced(10);
    volumeLabel.setBounds(col2.removeFromTop(30));
    volumeSlider.setBounds(col2.removeFromTop(40));

    auto col3 = topArea.reduced(10);
    warmthLabel.setBounds(col3.removeFromTop(30));
    warmthSlider.setBounds(col3.removeFromTop(40));

    auto mainArea = area;
    auto sidebarArea = mainArea.removeFromLeft(160); // increased sidebar width
    sidebarArea.reduce(10, 20); // add padding

    int numButtons = 4;
    int slotHeight = sidebarArea.getHeight() / numButtons;
    int iconSize = 48; // much bigger icons

    auto placeBtnAndLabel = [&](juce::ImageButton& btn, juce::Label& lbl, int index) {
        auto slot = sidebarArea.withTrimmedTop(index * slotHeight).withHeight(slotHeight);
        
        // Center the icon horizontally, or put it on the left
        // Let's put the icon centered horizontally, and text below it.
        auto iconSlot = slot.removeFromTop(slot.getHeight() / 2 + 10);
        btn.setBounds(iconSlot.withSizeKeepingCentre(iconSize, iconSize));
        
        lbl.setJustificationType(juce::Justification::centredTop);
        lbl.setFont(juce::Font(16.0f, juce::Font::bold));
        lbl.setBounds(slot);
    };

    placeBtnAndLabel(pauseButton, pauseLabel, 0);
    placeBtnAndLabel(playButton, playLabel, 1);
    placeBtnAndLabel(saveButton, saveLabel, 2);
    placeBtnAndLabel(deleteButton, deleteLabel, 3);
}

void EditComponent::playPreview()
{
    stopPreview();

    juce::File file(currentRecording.filePath);
    if (!file.existsAsFile()) return;

    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr) return;

    readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
    transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
    updateTransportParameters();
    transportSource.start();
    startTimerHz(30);
}

void EditComponent::timerCallback()
{
    if (transportSource.getLengthInSeconds() > 0)
    {
        playheadPosition = (float)(transportSource.getCurrentPosition() / transportSource.getLengthInSeconds());
        repaint();
    }
}

void EditComponent::mouseDown(const juce::MouseEvent& event)
{
    if (waveAreaBounds.contains(event.getPosition()))
    {
        float ratio = (float)(event.x - waveAreaBounds.getX()) / (float)waveAreaBounds.getWidth();
        ratio = juce::jlimit(0.0f, 1.0f, ratio);
        if (transportSource.getLengthInSeconds() > 0)
        {
            transportSource.setPosition(ratio * transportSource.getLengthInSeconds());
            playheadPosition = ratio;
            repaint();
        }
    }
}

void EditComponent::pausePreview()
{
    transportSource.stop();
}

void EditComponent::stopPreview()
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    readerSource.reset();
}

void EditComponent::updateTransportParameters()
{
    if (readerSource != nullptr)
    {
        transportSource.setGain(volumeSlider.getValue());
        
        // Pitch (via Resampling)
        double semitones = pitchSlider.getValue();
        double ratio = std::pow(2.0, semitones / 12.0);
        resamplerSource.setResamplingRatio(ratio);
        
        // Reverb
        juce::Reverb::Parameters params;
        float warmth = (float)warmthSlider.getValue();
        params.roomSize = warmth * 0.8f;
        params.damping = 0.5f;
        params.wetLevel = warmth;
        params.dryLevel = 1.0f - (warmth * 0.4f);
        
        reverbSource.setParameters(params);
        // Bypass if warmth is practically 0 to save CPU
        reverbSource.setBypassed(warmth <= 0.01f);
    }
}

void EditComponent::deleteCurrentEdit()
{
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon,
        "Delete Edit",
        "Delete functionality coming soon!");
}

void EditComponent::saveEditedVersion()
{
    stopPreview();

    auto* nameWindow = new juce::AlertWindow(
        "Save Edited Version",
        "Enter a name for your edited recording:",
        juce::AlertWindow::NoIcon);

    nameWindow->addTextEditor("title",
        currentRecording.audioTitle + "_edited",
        "Title:");
    nameWindow->addButton("Save", 1);
    nameWindow->addButton("Cancel", 0);

    nameWindow->enterModalState(true,
        juce::ModalCallbackFunction::create(
            [this, nameWindow](int result)
            {
                if (result != 1)
                {
                    delete nameWindow;
                    return;
                }

                juce::String newTitle = nameWindow->getTextEditorContents("title").trim();
                if (newTitle.isEmpty())
                    newTitle = currentRecording.audioTitle + "_edited";

                auto safe = newTitle;
                for (auto ch : { '\\', '/', ':', '*', '?', '"', '<', '>', '|' })
                    safe = safe.replaceCharacter(ch, '_');

                juce::File sourceFile(currentRecording.filePath);
                if (!sourceFile.existsAsFile())
                {
                    delete nameWindow;
                    return;
                }

                auto destFile = sourceFile.getParentDirectory()
                    .getChildFile(safe + ".wav");
                int copyNum = 1;
                while (destFile.existsAsFile())
                {
                    destFile = sourceFile.getParentDirectory()
                        .getChildFile(safe + "_"
                            + juce::String(copyNum++) + ".wav");
                }

                if (!sourceFile.copyFileTo(destFile))
                {
                    delete nameWindow;
                    return;
                }

                auto createdAt = juce::Time::getCurrentTime()
                    .formatted("%Y-%m-%d %H:%M:%S");

                bool inserted = database.insert(
                    newTitle,
                    currentRecording.accountName,
                    currentRecording.category,
                    destFile.getFullPathName(),
                    currentRecording.imagePath,
                    createdAt);

                delete nameWindow;

                if (inserted)
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::InfoIcon,
                        "Saved!",
                        "\"" + newTitle + "\" saved! It will appear on your homescreen.");

                    if (onEditSaved)
                        onEditSaved();
                }
            }),
        true);
}