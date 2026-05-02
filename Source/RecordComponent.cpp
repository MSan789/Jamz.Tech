#include "RecordComponent.h"

RecordComponent::RecordComponent()
{
    addAndMakeVisible(headerBar);
    headerBar.onLogoClicked = [this]() {
        stopRecording();
        stopPreview();
        if (onBack) onBack();
    };

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Record", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(32.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(statusLabel);
    statusLabel.setText("Ready to record", juce::dontSendNotification);
    statusLabel.setFont(16.0f);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    recordImage = juce::ImageCache::getFromMemory(BinaryData::record_png, BinaryData::record_pngSize);
    pauseImage = juce::ImageCache::getFromMemory(BinaryData::pause_png, BinaryData::pause_pngSize);
    playImage = juce::ImageCache::getFromMemory(BinaryData::play_png, BinaryData::play_pngSize);
    saveImage = juce::ImageCache::getFromMemory(BinaryData::save_png, BinaryData::save_pngSize);
    deleteImage = juce::ImageCache::getFromMemory(BinaryData::delete_png, BinaryData::delete_pngSize);

    auto setupBtn = [this](juce::ImageButton& btn, juce::Label& lbl, const juce::String& text, juce::Image& img) {
        btn.setImages(false, true, true, img, 1.0f, juce::Colour(), img, 1.0f, juce::Colour(), img, 1.0f, juce::Colour());
        addAndMakeVisible(btn);
        
        lbl.setText(text, juce::dontSendNotification);
        lbl.setColour(juce::Label::textColourId, juce::Colours::white);
        lbl.setFont(juce::Font(16.0f, juce::Font::bold));
        lbl.setJustificationType(juce::Justification::centredTop);
        addAndMakeVisible(lbl);
    };

    setupBtn(recordButton, recordLabel, "Record", recordImage);
    setupBtn(pauseButton, pauseLabel, "Pause", pauseImage);
    setupBtn(playButton, playLabel, "Play", playImage);
    setupBtn(deleteButton, deleteLabel, "Delete", deleteImage);
    setupBtn(saveButton, saveLabel, "Save", saveImage);

    recordButton.onClick = [this]() { stopPreview(); startRecording(); };
    pauseButton.onClick = [this]() { 
        isRecording = false; 
        pausePreview(); 
        statusLabel.setText("Paused", juce::dontSendNotification); 
    };
    playButton.onClick = [this]() { stopRecording(); playPreview(); };
    deleteButton.onClick = [this]() { deleteCurrentRecording(); };
    saveButton.onClick = [this]() { promptForTitleAndSave(); };

    backgroundThread.startThread();
    
    juce::String err = deviceManager.initialise(1, 0, nullptr, true, juce::String(), nullptr);
    if (err.isNotEmpty())
    {
        juce::Logger::writeToLog("RecordComponent audio init error: " + err);
    }

    deviceManager.addAudioCallback(this);

    formatManager.registerBasicFormats();
    audioSourcePlayer.setSource(&transportSource);
    deviceManager.addAudioCallback(&audioSourcePlayer);

    startTimerHz(60);
}

RecordComponent::~RecordComponent()
{
    stopTimer();
    stopRecording();
    stopPreview();
    
    deviceManager.removeAudioCallback(this);
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
}

void RecordComponent::setAccountName(const juce::String& name)
{
    currentAccountName = name;
}

void RecordComponent::timerCallback()
{
    if (transportSource.isPlaying() && transportSource.getLengthInSeconds() > 0)
    {
        playheadPosition = (float)(transportSource.getCurrentPosition() / transportSource.getLengthInSeconds());
    }
    repaint();
}

void RecordComponent::mouseDown(const juce::MouseEvent& event)
{
    if (playbackWaveArea.contains(event.getPosition()) && transportSource.getLengthInSeconds() > 0)
    {
        float ratio = (float)(event.x - playbackWaveArea.getX()) / (float)playbackWaveArea.getWidth();
        ratio = juce::jlimit(0.0f, 1.0f, ratio);
        transportSource.setPosition(ratio * transportSource.getLengthInSeconds());
        playheadPosition = ratio;
        repaint();
    }
}

void RecordComponent::paint(juce::Graphics& g)
{
    juce::ColourGradient gradient(
        juce::Colour(40, 20, 60), 0, 0,
        juce::Colour(70, 30, 50), (float)getWidth(), (float)getHeight(),
        false);
    gradient.addColour(0.3, juce::Colour(50, 20, 70));
    gradient.addColour(0.7, juce::Colour(80, 40, 40));

    g.setGradientFill(gradient);
    g.fillAll();

    auto area = getLocalBounds();
    area.removeFromTop(64); // headerBar
    area.removeFromTop(50); // title
    area.removeFromTop(40); // status
    
    auto mainArea = area;
    auto sidebarArea = mainArea.removeFromLeft(160);
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawLine((float)sidebarArea.getRight(), (float)sidebarArea.getY(), (float)sidebarArea.getRight(), (float)sidebarArea.getBottom(), 2.0f);

    auto waveArea = mainArea.reduced(20.0f).toFloat();
    playbackWaveArea = mainArea.reduced(20); // store for seek hit-testing

    std::vector<float> peaksCopy;
    {
        const juce::ScopedLock sl(waveformLock);
        peaksCopy = recentPeaks;
    }

    if (!peaksCopy.empty())
    {
        float midY = waveArea.getCentreY();
        float x0 = waveArea.getX();
        float barWidth = 3.0f;
        float gap = 2.0f;
        float step = barWidth + gap;
        int numBars = juce::jmax(1, (int)(waveArea.getWidth() / step));

        for (int i = 0; i < numBars; ++i)
        {
            float x = x0 + i * step;
            
            float startRatio = (float)i / (float)numBars;
            float endRatio = (float)(i + 1) / (float)numBars;
            
            int startIdx = (int)(startRatio * peaksCopy.size());
            int endIdx = (int)(endRatio * peaksCopy.size());
            
            float peak = 0.0f;
            if (startIdx == endIdx)
            {
                 int idx = juce::jlimit(0, (int)peaksCopy.size() - 1, startIdx);
                 peak = peaksCopy[(size_t)idx];
            }
            else
            {
                 for (int j = startIdx; j < endIdx && j < (int)peaksCopy.size(); ++j)
                     peak = juce::jmax(peak, peaksCopy[(size_t)j]);
            }
            
            float barHeight = juce::jmax(2.0f, peak * (waveArea.getHeight() * 0.9f));
            
            // Show playback progress in hot pink
            if (transportSource.isPlaying() && startRatio <= playheadPosition)
                g.setColour(juce::Colours::deeppink);
            else
                g.setColour(juce::Colours::lightpink.withAlpha(0.8f));
                
            g.fillRoundedRectangle(x, midY - barHeight * 0.5f, barWidth, barHeight, 1.5f);
        }
    }
    else
    {
        g.setColour(juce::Colours::lightgrey);
        g.drawText("Waveform will appear when input is detected", waveArea.toNearestInt(), juce::Justification::centred);
    }
}

void RecordComponent::resized()
{
    auto area = getLocalBounds();
    headerBar.setBounds(area.removeFromTop(64));
    titleLabel.setBounds(area.removeFromTop(50));
    statusLabel.setBounds(area.removeFromTop(40));

    auto mainArea = area;
    auto sidebarArea = mainArea.removeFromLeft(160);
    sidebarArea.reduce(10, 20);

    int numButtons = 5;
    int slotHeight = sidebarArea.getHeight() / numButtons;
    int iconSize = 48;

    auto placeBtnAndLabel = [&](juce::ImageButton& btn, juce::Label& lbl, int index) {
        auto slot = sidebarArea.withTrimmedTop(index * slotHeight).withHeight(slotHeight);
        
        auto iconSlot = slot.removeFromTop(slot.getHeight() / 2 + 10);
        btn.setBounds(iconSlot.withSizeKeepingCentre(iconSize, iconSize));
        
        lbl.setJustificationType(juce::Justification::centredTop);
        lbl.setFont(juce::Font(16.0f, juce::Font::bold));
        lbl.setBounds(slot);
    };

    placeBtnAndLabel(recordButton, recordLabel, 0);
    placeBtnAndLabel(pauseButton, pauseLabel, 1);
    placeBtnAndLabel(playButton, playLabel, 2);
    placeBtnAndLabel(deleteButton, deleteLabel, 3);
    placeBtnAndLabel(saveButton, saveLabel, 4);
}

void RecordComponent::startRecording()
{
    if (sampleRate <= 0.0)
    {
        statusLabel.setText("Error: Audio device not ready (sampleRate=0)", juce::dontSendNotification);
        return;
    }

    if (activeWriter != nullptr) 
    {
        // Resuming
        isRecording = true;
        statusLabel.setText("Recording...", juce::dontSendNotification);
        return;
    }

    // Starting new
    auto recordingsFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("JamzRecordings");

    if (!recordingsFolder.exists())
        recordingsFolder.createDirectory();

    auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    currentRecordingFile = recordingsFolder.getChildFile("recording_" + timestamp + ".wav");

    std::unique_ptr<juce::FileOutputStream> fileStream(currentRecordingFile.createOutputStream());

    if (fileStream == nullptr)
    {
        statusLabel.setText("Error: Failed to create file", juce::dontSendNotification);
        return;
    }

    juce::WavAudioFormat wavFormat;
    auto* writer = wavFormat.createWriterFor(fileStream.get(),
        sampleRate,
        1,
        16,
        {},
        0);

    if (writer != nullptr)
    {
        fileStream.release(); // writer takes ownership
        auto newThreadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(writer, backgroundThread, 32768);

        {
            const juce::ScopedLock sl(writerLock);
            threadedWriter = std::move(newThreadedWriter);
            activeWriter = threadedWriter.get();
        }

        recordingStartTime = juce::Time::getCurrentTime(); 
        currentAudioTitle = "Recording_" + timestamp;

        isRecording = true;
        statusLabel.setText("Recording...", juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("Error: Could not create audio file", juce::dontSendNotification);
    }
}

void RecordComponent::stopRecording()
{
    isRecording = false;

    {
        const juce::ScopedLock sl(writerLock);
        activeWriter = nullptr;
    }

    threadedWriter.reset();
}

void RecordComponent::playPreview()
{
    if (currentRecordingFile.existsAsFile())
    {
        // Switch to playback mode
        deviceManager.initialise(0, 2, nullptr, true, juce::String(), nullptr);

        auto* reader = formatManager.createReaderFor(currentRecordingFile);
        if (reader != nullptr)
        {
            readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
            transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
            transportSource.start();
            statusLabel.setText("Playing preview...", juce::dontSendNotification);
        }
    }
}

void RecordComponent::pausePreview()
{
    transportSource.stop();
}

void RecordComponent::stopPreview()
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    readerSource.reset();

    // Re-initialize recording mode so waveform updates live again
    deviceManager.initialise(1, 0, nullptr, true, juce::String(), nullptr);
}

void RecordComponent::deleteCurrentRecording()
{
    stopRecording();
    stopPreview();

    if (currentRecordingFile.existsAsFile())
    {
        currentRecordingFile.deleteFile();
    }

    {
        const juce::ScopedLock sl(waveformLock);
        recentPeaks.clear();
    }

    statusLabel.setText("Recording deleted", juce::dontSendNotification);
}

void RecordComponent::promptForTitleAndSave()
{
    stopRecording();
    stopPreview();

    if (!currentRecordingFile.existsAsFile())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "No Recording",
            "There is no recording to save.");
        return;
    }

    auto* titleWindow = new juce::AlertWindow(
        "Save Recording",
        "Please enter a title and price for this recording.",
        juce::AlertWindow::NoIcon);

    titleWindow->addTextEditor("title", currentAudioTitle, "Title:");
    titleWindow->addTextEditor("price", "Free", "Price ($):");
    titleWindow->addButton("Save", 1);
    titleWindow->addButton("Cancel", 0);

    juce::Component::SafePointer<RecordComponent> safeThis(this);
    titleWindow->enterModalState(true,
        juce::ModalCallbackFunction::create([safeThis, titleWindow](int result)
        {
            if (safeThis == nullptr)
            {
                delete titleWindow;
                return;
            }

            auto* self = safeThis.getComponent();

            if (result == 1)
            {
                juce::String title = titleWindow->getTextEditorContents("title").trim();
                juce::String price = titleWindow->getTextEditorContents("price").trim();

                if (title.isEmpty()) title = self->currentAudioTitle;
                if (price.isEmpty()) price = "Free";

                title = self->makeName(title);
                auto createdAt = self->recordingStartTime.formatted("%Y-%m-%d %H:%M:%S");
                auto renamedFile = self->currentRecordingFile.getSiblingFile(title + ".wav");

                int copyNumber = 1;
                while (renamedFile.existsAsFile())
                {
                    renamedFile = self->currentRecordingFile.getSiblingFile(
                        title + "_" + juce::String(copyNumber) + ".wav");
                    ++copyNumber;
                }

                bool renameSuccess = self->currentRecordingFile.moveFileTo(renamedFile);
                if (renameSuccess) self->currentRecordingFile = renamedFile;

                bool inserted = self->database.insert(
                    title,
                    self->currentAccountName,
                    self->currentCategory,
                    self->currentRecordingFile.getFullPathName(),
                    "",
                    createdAt,
                    price
                );

                if (inserted && renameSuccess)
                    self->statusLabel.setText("Recording saved!", juce::dontSendNotification);
                else
                    self->statusLabel.setText("Save failed", juce::dontSendNotification);
            }
            else
            {
                // Cancelled, maybe clean up? For now, we leave the file.
                self->statusLabel.setText("Save cancelled", juce::dontSendNotification);
            }

            delete titleWindow;
        }),
        true);
}

juce::String RecordComponent::makeName(const juce::String& name)
{
    juce::String safe = name.trim();
    for (auto ch : { '\\', '/', ':', '*', '?', '"', '<', '>', '|' })
        safe = safe.replaceCharacter(ch, '_');
    if (safe.isEmpty()) safe = "Untitled";
    return safe;
}

void RecordComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
}

void RecordComponent::audioDeviceStopped()
{
    sampleRate = 0.0;
}

void RecordComponent::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);
    juce::ignoreUnused(outputChannelData);
    juce::ignoreUnused(numOutputChannels);

    if (isRecording && threadedWriter != nullptr && numInputChannels > 0 && inputChannelData[0] != nullptr)
    {
        const float* channels[] = { inputChannelData[0] };
        threadedWriter->write(channels, numSamples);
    }

    if (numInputChannels > 0 && inputChannelData[0] != nullptr)
    {
        float peak = 0.0f;
        auto* in = inputChannelData[0];
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax(peak, std::abs(in[i]));

        const juce::ScopedLock sl(waveformLock);
        recentPeaks.push_back(juce::jlimit(0.0f, 1.0f, peak));
        if ((int) recentPeaks.size() > maxRecentPeaks)
            recentPeaks.erase(recentPeaks.begin(), recentPeaks.begin() + ((int) recentPeaks.size() - maxRecentPeaks));
    }
}
