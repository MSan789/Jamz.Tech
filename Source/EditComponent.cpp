#include "EditComponent.h"

EditComponent::EditComponent()
{
    addAndMakeVisible(backButton);
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(recordingNameLabel);
    addAndMakeVisible(filterButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(publishButton);
    addAndMakeVisible(deleteButton);

    auto styleButton = [](juce::TextButton& b, juce::Colour c)
        {
            b.setColour(juce::TextButton::buttonColourId, c);
            b.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        };

    styleButton(filterButton, juce::Colour(70, 70, 120));
    styleButton(saveButton, juce::Colour(60, 120, 80));
    styleButton(publishButton, juce::Colour(120, 90, 60));
    styleButton(deleteButton, juce::Colour(120, 50, 50));

    titleLabel.setText("Edit Sound Page", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(24.0f);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    recordingNameLabel.setJustificationType(juce::Justification::centred);
    recordingNameLabel.setFont(16.0f);
    recordingNameLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    formatManager.registerBasicFormats();
    previewDeviceManager.initialise(0, 2, nullptr, true);
    audioSourcePlayer.setSource(&transportSource);
    previewDeviceManager.addAudioCallback(&audioSourcePlayer);

    // ── Back button ───────────────────────────────────────────────────────
    // stopPreview() first, then callAsync so any open modal fully dismisses
    // before we navigate away — this is what makes it reliably land on home.
    backButton.onClick = [this]()
        {
            stopPreview();
            juce::Component::SafePointer<EditComponent> safeThis(this);
            juce::MessageManager::callAsync([safeThis]()
                {
                    if (safeThis != nullptr && safeThis->onBack)
                        safeThis->onBack();
                });
        };

    filterButton.onClick = [this]()
        {
            if (currentRecording.filePath.isEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "No Recording",
                    "Please select a recording to edit first.");
                return;
            }
            showFilterPopup();
        };

    saveButton.onClick = [this]()
        {
            if (currentRecording.filePath.isEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "No Recording",
                    "No recording is loaded to save.");
                return;
            }
            saveEditedVersion();
        };

    publishButton.onClick = [this]()
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Publish",
                "Publishing feature coming soon!");
        };

    deleteButton.onClick = [this]()
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Delete Edit",
                "Delete functionality coming soon!");
        };
}

EditComponent::~EditComponent()
{
    stopPreview();
    previewDeviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
}

void EditComponent::setRecording(const RecordingEntry& entry)
{
    currentRecording = entry;
    pitchSemitones = 0.0f;
    volumeGain = 1.0f;
    warmth = 0.0f;

    recordingNameLabel.setText("Editing: " + entry.audioTitle,
        juce::dontSendNotification);
    repaint();
}

void EditComponent::paint(juce::Graphics& g)
{
    juce::ColourGradient gradient(
        juce::Colour(28, 30, 45), 0, 0,
        juce::Colour(15, 16, 25), 0, getHeight(),
        false);
    g.setGradientFill(gradient);
    g.fillAll();
}

void EditComponent::resized()
{
    auto area = getLocalBounds();

    titleLabel.setBounds(area.removeFromTop(60));
    backButton.setBounds(10, 10, 80, 30);

    recordingNameLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(20);

    int buttonWidth = 220;
    int buttonHeight = 40;
    int spacing = 15;
    int centerX = getWidth() / 2 - buttonWidth / 2;

    filterButton.setBounds(centerX, area.removeFromTop(buttonHeight).getY(), buttonWidth, buttonHeight);
    area.removeFromTop(spacing);
    saveButton.setBounds(centerX, area.removeFromTop(buttonHeight).getY(), buttonWidth, buttonHeight);
    area.removeFromTop(spacing);
    publishButton.setBounds(centerX, area.removeFromTop(buttonHeight).getY(), buttonWidth, buttonHeight);
    area.removeFromTop(spacing);
    deleteButton.setBounds(centerX, area.removeFromTop(buttonHeight).getY(), buttonWidth, buttonHeight);
}

void EditComponent::showFilterPopup()
{
    auto* content = new FilterWindow(pitchSemitones, volumeGain, warmth);
    FilterWindow* contentPtr = content;

    auto* popup = new juce::AlertWindow(
        "Filter Sound",
        "Adjust the sliders, then choose an action.",
        juce::AlertWindow::NoIcon);

    content->setSize(380, 280);
    popup->addCustomComponent(content);

    popup->addButton("Preview", 1);
    popup->addButton("Save Settings", 2);
    popup->addButton("Cancel", 0);

    popup->enterModalState(true,
        juce::ModalCallbackFunction::create(
            [this, popup, contentPtr](int result)
            {
                float p = contentPtr->getPitch();
                float v = contentPtr->getVolume();
                float w = contentPtr->getWarmth();

                if (result == 1)
                {
                    pitchSemitones = p;
                    volumeGain = v;
                    warmth = w;
                    delete popup;
                    previewWithSettings(p, v, w);
                    showFilterPopup();
                }
                else if (result == 2)
                {
                    pitchSemitones = p;
                    volumeGain = v;
                    warmth = w;
                    delete popup;
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::InfoIcon,
                        "Settings Saved",
                        "Filter settings applied!\n"
                        "Pitch: " + juce::String(p, 1) + " st\n"
                        "Volume: " + juce::String(v, 2) + "x\n"
                        "Warmth: " + juce::String(w, 2));
                }
                else
                {
                    stopPreview();
                    delete popup;
                }
            }),
        true);
}

void EditComponent::previewWithSettings(float /*pitch*/, float volume, float /*warmthAmt*/)
{
    stopPreview();

    juce::File file(currentRecording.filePath);
    if (!file.existsAsFile())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "File Not Found",
            "Cannot preview: audio file not found at\n" + currentRecording.filePath);
        return;
    }

    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Preview Failed",
            "Could not read the audio file.");
        return;
    }

    readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
    transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
    transportSource.setGain(volume);
    transportSource.start();
}

void EditComponent::stopPreview()
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    readerSource.reset();
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
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Save Failed",
                        "Original audio file not found.");
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
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Save Failed",
                        "Could not copy audio file.");
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
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Database Error",
                        "File copied but could not be saved to the database.");
                }
            }),
        true);
}