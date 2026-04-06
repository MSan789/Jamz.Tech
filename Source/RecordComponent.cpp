#include "RecordComponent.h"

RecordComponent::RecordComponent()
{
    addAndMakeVisible(backButton);
    addAndMakeVisible(recordButton);
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(statusLabel);

    titleLabel.setText("Record Page", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(24.0f);

    statusLabel.setText("Not recording", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);

    //set colors
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightcoral);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::coral);

    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(70, 70, 120));
    backButton.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 50, 80));

    backgroundThread.startThread();

    // 1 input channel, 0 output channels is enough for basic recording
    deviceManager.initialise(1, 0, nullptr, true);
    deviceManager.addAudioCallback(this);

    recordButton.onClick = [this]()
        {
            if (!isRecording)
                startRecording();
            else
                stopRecording();
        };

    backButton.onClick = [this]()
        {
            stopRecording();

            if (onBack)
                onBack();
        };
}

RecordComponent::~RecordComponent()
{
    stopRecording();
    deviceManager.removeAudioCallback(this);
    backgroundThread.stopThread(1000);
}

void RecordComponent::paint(juce::Graphics& g)
{
    juce::ColourGradient gradient(
        juce::Colour(28, 30, 45),
        0, 0,
        juce::Colour(15, 16, 25),
        0, getHeight(),
        false);

    g.setGradientFill(gradient);
    g.fillAll();

    g.setColour(juce::Colour(32, 34, 50));
    //g.fillRect(categoriesPanel.getBounds());
}


void RecordComponent::resized()
{
    auto area = getLocalBounds().reduced(20);

    titleLabel.setBounds(area.removeFromTop(50));
    statusLabel.setBounds(area.removeFromTop(40));

    auto topRow = area.removeFromTop(40);
    backButton.setBounds(topRow.removeFromLeft(100));
    topRow.removeFromLeft(10);
    recordButton.setBounds(topRow.removeFromLeft(160));
}

void RecordComponent::setAccountName(const juce::String& name)
{
    currentAccountName = name;
}

void RecordComponent::startRecording()
{
    if (isRecording || sampleRate <= 0.0)
        return;

    auto recordingsFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("JamzRecordings");

    if (!recordingsFolder.exists())
    {
        recordingsFolder.createDirectory();
    }

    auto timestamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S");
	currentRecordingFile = recordingsFolder.getChildFile("Recording_" + timestamp + ".wav");

    std::unique_ptr<juce::FileOutputStream> fileStream(currentRecordingFile.createOutputStream());

    if (fileStream == nullptr)
    {
        statusLabel.setText("Failed to create file", juce::dontSendNotification);
        return;
    }

    juce::WavAudioFormat wavFormat;
    auto* writer = wavFormat.createWriterFor(fileStream.get(),
        sampleRate,
        1,       // mono
        16,
        {},
        0);

    if (writer == nullptr)
    {
        statusLabel.setText("Failed to create WAV writer", juce::dontSendNotification);
        return;
    }

    fileStream.release();

    auto newThreadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
        writer, backgroundThread, 32768);

    {
        const juce::ScopedLock sl(writerLock);
        threadedWriter = std::move(newThreadedWriter);
        activeWriter = threadedWriter.get();
    }

	recordingStartTime = juce::Time::getCurrentTime(); 
	currentAudioTitle = "Recording_" + timestamp;

    isRecording = true;
    recordButton.setButtonText("Stop Recording");
    statusLabel.setText("Recording to: " + currentRecordingFile.getFileName(),
        juce::dontSendNotification);
}

void RecordComponent::stopRecording()
{
    if (!isRecording)
        return;

    {
        const juce::ScopedLock sl(writerLock);
        activeWriter = nullptr;
    }

    threadedWriter.reset(); // flushes remaining data
    isRecording = false;  

	recordButton.setButtonText("Start Recording");
	promptForTitleAndSave();
}

void RecordComponent::promptForTitleAndSave()
{
    auto* titleWindow = new juce::AlertWindow(
        "Enter Recording Title",
        "Please enter a title for this recording.",
        juce::AlertWindow::NoIcon);

    titleWindow->addTextEditor("title", currentAudioTitle, "Title:");
    titleWindow->addButton("Save", 1);
    titleWindow->addButton("Cancel", 0);

    titleWindow->enterModalState(true,
        juce::ModalCallbackFunction::create([this, titleWindow](int result)
        {
            juce::String title;

            if (result == 1)
                title = titleWindow->getTextEditorContents("title").trim();

            if (title.isEmpty())
                title = currentAudioTitle;

            title = makeName(title);

            auto createdAt = recordingStartTime.formatted("%Y-%m-%d %H:%M:%S");

            auto renamedFile = currentRecordingFile.getSiblingFile(title + ".wav");

            int copyNumber = 1;
            while (renamedFile.existsAsFile())
            {
                renamedFile = currentRecordingFile.getSiblingFile(
                    title + "_" + juce::String(copyNumber) + ".wav");
                ++copyNumber;
            }

            bool renameSuccess = currentRecordingFile.moveFileTo(renamedFile);

            if (renameSuccess)
                currentRecordingFile = renamedFile;

            bool inserted = database.insert(
                title,
                currentAccountName,
                currentCategory,
                currentRecordingFile.getFullPathName(),
                "",
                createdAt
            );

            if (inserted && renameSuccess)
                statusLabel.setText("Recording saved to file and database", juce::dontSendNotification);
            else if (inserted)
                statusLabel.setText("Saved in DB, but file rename failed", juce::dontSendNotification);
            else
                statusLabel.setText("Recording saved, DB insert failed", juce::dontSendNotification);

            delete titleWindow;
        }),
        true);
}

juce::String RecordComponent::makeName(const juce::String& name)
{
    juce::String safe = name.trim();

    safe = safe.replaceCharacter('\\', '_');
    safe = safe.replaceCharacter('/', '_');
    safe = safe.replaceCharacter(':', '_');
    safe = safe.replaceCharacter('*', '_');
    safe = safe.replaceCharacter('?', '_');
    safe = safe.replaceCharacter('"', '_');
    safe = safe.replaceCharacter('<', '_');
    safe = safe.replaceCharacter('>', '_');
    safe = safe.replaceCharacter('|', '_');

    if (safe.isEmpty())
        safe = "Untitled Recording";

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

    if (threadedWriter != nullptr && numInputChannels > 0 && inputChannelData[0] != nullptr)
    {
        const float* channels[] = { inputChannelData[0] };
        threadedWriter->write(channels, numSamples);
    }

    for (int i = 0; i < numOutputChannels; ++i)
        if (outputChannelData[i] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
}