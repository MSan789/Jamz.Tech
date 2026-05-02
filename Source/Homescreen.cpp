/*
  ==============================================================================

    Homescreen.cpp
    Created: 28 Feb 2026 10:02:12pm
    Author:  Tanishi Kumar

  ==============================================================================
*/

#include "Homescreen.h"
#include "AudioCards.h"


Homescreen::Homescreen() {

    //header bar
    addAndMakeVisible(headerBar);
    headerBar.onLogoutClicked = [this]()
        {
            if (onLogoutClicked)
            {
                onLogoutClicked();
            }
        };

    headerBar.onUploadClicked = [this]() { handleUpload(); };
    headerBar.onCreateGuestClicked = [this]() { if (onCreateGuestClicked) onCreateGuestClicked(); };

    //greeting:
    addAndMakeVisible(greetingLabel);
    greetingLabel.setFont(20.0f);
    greetingLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    //search bar:
    addAndMakeVisible(searchBar);
    searchBar.setFont(juce::Font(16.0f));
    searchBar.setTextToShowWhenEmpty("Search Sounds...", juce::Colours::white.withAlpha(0.5f));
    searchBar.setColour(juce::TextEditor::backgroundColourId, juce::Colours::white.withAlpha(0.05f));
    searchBar.setColour(juce::TextEditor::outlineColourId, juce::Colours::white.withAlpha(0.2f));
    searchBar.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::lightpink.withAlpha(0.8f));
    searchBar.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    searchBar.setColour(juce::CaretComponent::caretColourId, juce::Colours::lightpink);

    searchBar.onTextChange = [this]()
        {
            currentSearchQuery = searchBar.getText();
            applyFilter();
            clusterMap.setSearchQuery(currentSearchQuery);
        };

    //sidebar panel:
    addAndMakeVisible(categoriesPanel);
    addAndMakeVisible(categoriesTitle);
    categoriesTitle.setText("Categories", juce::dontSendNotification);
    categoriesTitle.setColour(juce::Label::textColourId, juce::Colours::white);
    categoriesTitle.setFont(18.0f);

    // Favorites sidebar button
    addAndMakeVisible(favoritesButton);
    favoritesButton.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 50, 110));
    favoritesButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    favoritesButton.onClick = [this]()
        {
            showingFavoritesFilter = !showingFavoritesFilter;
            showingCreatedSounds = false;
            showingEditedSounds = false;
            applyFilter();
        };

    // Created Sounds sidebar button
    addAndMakeVisible(createdSoundsButton);
    createdSoundsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 50, 110));
    createdSoundsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    createdSoundsButton.onClick = [this]()
        {
            showingCreatedSounds = !showingCreatedSounds;
            showingEditedSounds = false;
            showingFavoritesFilter = false;
            applyFilter();
        };

    // Edited Sounds sidebar button
    addAndMakeVisible(editedSoundsButton);
    editedSoundsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 50, 110));
    editedSoundsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    editedSoundsButton.onClick = [this]()
        {
            showingEditedSounds = !showingEditedSounds;
            showingCreatedSounds = false;
            showingFavoritesFilter = false;
            applyFilter();
        };

    //record button:
    headerBar.onRecordClicked = [this]()
        {
            if (onRecordClicked)
                onRecordClicked();
        };

    formatManager.registerBasicFormats();
    deviceManager.initialise(0, 2, nullptr, true);
    audioSourcePlayer.setSource(&transportSource);
    deviceManager.addAudioCallback(&audioSourcePlayer);

    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&cardsContainer, false);
    viewport.setScrollBarsShown(true, false);


    //clustermap section

    addAndMakeVisible(listViewTab);
    addAndMakeVisible(mapViewTab);
    addChildComponent(clusterMap);

    listViewTab.onClick = [this]()
        {
            showingMap = false;
            clusterMap.setVisible(false);
            for (auto& card : audioCards)
                card->setVisible(true);

            resized();
            repaint();
        };

    mapViewTab.onClick = [this]()
        {
            showingMap = true;
            clusterMap.loadRecordings();
            clusterMap.setVisible(true);
            for (auto& card : audioCards)
                card->setVisible(false);

            resized();
            repaint();
        };

    clusterMap.onDotClicked = [this](const RecordingEntry& entry)
        {
            playRecording(entry);
        };

}

Homescreen::~Homescreen()
{
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
    transportSource.setSource(nullptr);
    readerSource.reset();
    stopTimer();
}

void Homescreen::paint(juce::Graphics& g) {

    // Rich vibrant gradient (Purple, Pink, Orange hues)
    juce::ColourGradient gradient(juce::Colour(40, 20, 60), 0, 0,
        juce::Colour(70, 30, 50), (float)getWidth(), (float)getHeight(),
        false);
    gradient.addColour(0.3, juce::Colour(50, 20, 70));
    gradient.addColour(0.7, juce::Colour(80, 40, 40));

    g.setGradientFill(gradient);
    g.fillAll();

    // Subtle Grid Overlay
    g.setColour(juce::Colours::white.withAlpha(0.03f));
    int gridSize = 50;
    for (int x = 0; x < getWidth(); x += gridSize)
        g.drawLine((float)x, 0.0f, (float)x, (float)getHeight(), 1.0f);
    for (int y = 0; y < getHeight(); y += gridSize)
        g.drawLine(0.0f, (float)y, (float)getWidth(), (float)y, 1.0f);

    // Sidebar panel: Frosted dark glass
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.fillRoundedRectangle(categoriesPanel.getBounds().toFloat(), 12.0f);
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(categoriesPanel.getBounds().toFloat(), 12.0f, 1.0f);
}


void Homescreen::handleUpload()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Audio Files to Upload",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.mp3"
    );

    auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectMultipleItems;

    fileChooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& chooser)
        {
            juce::Array<juce::File> results = chooser.getResults();
            if (results.size() > 0)
            {
                promptForNextUpload(results, 0);
            }
        });
}

void Homescreen::promptForNextUpload(juce::Array<juce::File> files, int index)
{
    if (index >= files.size())
    {
        loadRecordings();
        return;
    }

    juce::File sourceFile = files[index];

    auto* titleWindow = new juce::AlertWindow(
        "Upload Audio (" + juce::String(index + 1) + " of " + juce::String(files.size()) + ")",
        "Please enter a title and price for: " + sourceFile.getFileName(),
        juce::AlertWindow::NoIcon);

    titleWindow->addTextEditor("title", sourceFile.getFileNameWithoutExtension(), "Title:");
    titleWindow->addTextEditor("price", "Free", "Price ($):");
    titleWindow->addButton("Upload", 1);
    titleWindow->addButton("Skip", 0);

    juce::Component::SafePointer<Homescreen> safeThis(this);
    titleWindow->enterModalState(true,
        juce::ModalCallbackFunction::create([safeThis, titleWindow, files, index, sourceFile](int result)
            {
                if (safeThis == nullptr)
                {
                    delete titleWindow;
                    return;
                }

                auto* self = safeThis.getComponent();

                if (result == 1) // Upload
                {
                    juce::String title = titleWindow->getTextEditorContents("title").trim();
                    juce::String price = titleWindow->getTextEditorContents("price").trim();

                    if (title.isEmpty()) title = sourceFile.getFileNameWithoutExtension();
                    if (price.isEmpty()) price = "Free";

                    auto safe = title;
                    for (auto ch : { '\\', '/', ':', '*', '?', '"', '<', '>', '|' })
                        safe = safe.replaceCharacter(ch, '_');

                    auto recordingsFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("JamzRecordings");

                    if (!recordingsFolder.exists())
                        recordingsFolder.createDirectory();

                    auto destFile = recordingsFolder.getChildFile(safe + sourceFile.getFileExtension());

                    int copyNumber = 1;
                    while (destFile.existsAsFile())
                    {
                        destFile = recordingsFolder.getChildFile(
                            safe + "_" + juce::String(copyNumber) + sourceFile.getFileExtension());
                        ++copyNumber;
                    }

                    bool copySuccess = sourceFile.copyFileTo(destFile);

                    if (copySuccess)
                    {
                        auto createdAt = juce::Time::getCurrentTime().formatted("%Y-%m-%d %H:%M:%S");

                        self->database.insert(
                            title,
                            self->currUser,
                            "Uploads",
                            destFile.getFullPathName(),
                            "",
                            createdAt,
                            price
                        );
                    }
                }

                delete titleWindow;

                // Proceed to the next file regardless of whether the user uploaded or skipped
                self->promptForNextUpload(files, index + 1);
            }),
        true);
}

void Homescreen::applyFilter()
{
    int y = 0;
    int cardHeight = 120;
    int spacing = 20;

    for (auto& card : audioCards)
    {
        const auto& entry = card->getRecording();
        bool matchesSearch = currentSearchQuery.isEmpty() || entry.audioTitle.containsIgnoreCase(currentSearchQuery);

        bool matchesCreated = !showingCreatedSounds || (entry.accountName == currUser);
        bool matchesEdited = !showingEditedSounds || (entry.audioTitle.endsWithIgnoreCase("_edited") && entry.accountName == currUser);
        bool matchesFavorite = !showingFavoritesFilter || (userFavorites[currUser].count(entry.id) > 0);

        if (showingEditedSounds && !matchesEdited) matchesSearch = false;
        if (showingCreatedSounds && !matchesCreated) matchesSearch = false;
        if (showingFavoritesFilter && !matchesFavorite) matchesSearch = false;

        card->setVisible(matchesSearch);

        if (matchesSearch)
        {
            card->setBounds(0, y, cardsContainer.getWidth(), cardHeight);
            y += cardHeight + spacing;
        }
    }
    cardsContainer.setSize(cardsContainer.getWidth(), juce::jmax(y, 1));
}

void Homescreen::loadRecordings()
{
    for (auto& card : audioCards)
        cardsContainer.removeChildComponent(card.get());
    audioCards.clear();
    recordings.clear();

    auto allRecordings = database.getAllRecordings();

    for (const auto& entry : allRecordings)
    {
        recordings.push_back(entry);
    }

    for (size_t i = 0; i < recordings.size(); ++i)
    {
        auto card = std::make_unique<AudioCards>();
        card->setRecording(recordings[i]);
        card->setIsPlaying(false);

        juce::File file(recordings[i].filePath);
        if (file.existsAsFile())
        {
            auto peaks = computeWaveformPeaks01(file, 150);
            card->setWaveformPeaks(peaks);
        }

        card->onPlayClicked = [this, &card](const RecordingEntry& entry)
            {
                if (readerSource != nullptr && transportSource.isPlaying() && card->getRecording().id == entry.id)
                {
                    transportSource.stop();
                }
                else if (readerSource != nullptr && !transportSource.isPlaying() && card->getRecording().id == entry.id)
                {
                    transportSource.start();
                }
                else
                {
                    updateNowPlaying(entry);
                    playRecording(entry);
                }
            };

        card->onEditClicked = [this](const RecordingEntry& entry)
            {
                if (onEditRequested)
                    onEditRequested(entry);
            };

        card->onSeekRequested = [this](float ratio)
            {
                if (transportSource.getTotalLength() > 0)
                {
                    double newPosition = ratio * transportSource.getLengthInSeconds();
                    transportSource.setPosition(newPosition);
                }
            };

        card->onFavoriteToggled = [this](const RecordingEntry& entry, bool fav)
            {
                if (fav)
                    userFavorites[currUser].insert(entry.id);
                else
                    userFavorites[currUser].erase(entry.id);

                if (showingFavoritesFilter)
                    applyFilter();
            };

        // Restore favorite state if it was set before reload
        if (userFavorites[currUser].count(recordings[i].id))
            card->setFavorite(true);

        cardsContainer.addAndMakeVisible(*card);
        audioCards.push_back(std::move(card));
    }

    applyFilter();
    repaint();
}

void Homescreen::updateNowPlaying(const RecordingEntry& entry)
{
    for (auto& card : audioCards)
        card->setIsPlaying(card->getRecording().id == entry.id);
}

std::vector<float> Homescreen::computeWaveformPeaks01(const juce::File& file, int numPoints)
{
    std::vector<float> peaks;
    peaks.resize((size_t)juce::jmax(0, numPoints), 0.0f);

    if (!file.existsAsFile() || numPoints <= 0)
        return peaks;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return peaks;

    const auto totalSamples = (int64_t)reader->lengthInSamples;
    const auto samplesPerPoint = juce::jmax<int64_t>(1, totalSamples / (int64_t)numPoints);

    juce::AudioBuffer<float> buffer(1, 8192);

    for (int i = 0; i < numPoints; ++i)
    {
        const int64_t start = (int64_t)i * samplesPerPoint;
        const int64_t end = juce::jmin(totalSamples, start + samplesPerPoint);

        float peak = 0.0f;
        int64_t pos = start;

        while (pos < end)
        {
            const int toRead = (int)juce::jmin<int64_t>(buffer.getNumSamples(), end - pos);
            buffer.clear();

            reader->read(&buffer, 0, toRead, pos, true, false);
            peak = juce::jmax(peak, buffer.getMagnitude(0, 0, toRead));

            pos += toRead;
        }

        peaks[(size_t)i] = juce::jlimit(0.0f, 1.0f, peak);
    }

    return peaks;
}

void Homescreen::setRecordButtonVisible(bool shouldShow)
{
    headerBar.setRecordButtonVisible(shouldShow);
}

void Homescreen::playRecording(const RecordingEntry& entry)
{
    juce::File file(entry.filePath);

    if (!file.existsAsFile())
    {
        DBG("Audio file not found: " + entry.filePath);
        return;
    }

    transportSource.stop();
    transportSource.setSource(nullptr);
    readerSource.reset();

    auto* reader = formatManager.createReaderFor(file);

    if (reader == nullptr)
    {
        DBG("Failed to create audio reader for: " + entry.filePath);
        return;
    }

    readerSource.reset(new juce::AudioFormatReaderSource(reader, true));
    transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
    transportSource.start();

    startTimerHz(30);

    // Precompute waveform peaks for the now-playing card
    auto peaks = computeWaveformPeaks01(file, 300);
    for (auto& card : audioCards)
        if (card->getRecording().id == entry.id)
            card->setWaveformPeaks(peaks);

    DBG("Now playing: " + entry.audioTitle);
}

void Homescreen::resized()
{
    auto area = getLocalBounds();
    headerBar.setBounds(area.removeFromTop(64));
    area = area.reduced(20);

    // ===== Greeting =====
    auto topBar = area.removeFromTop(50);
    greetingLabel.setBounds(topBar.removeFromLeft(300));

    // ===== Centered Search Bar =====
    auto searchRow = area.removeFromTop(50);
    int searchW = juce::jmin(600, searchRow.getWidth() - 40);
    searchBar.setBounds(searchRow.withSizeKeepingCentre(searchW, 34));

    // ===== Tab Row =====
    auto tabRow = area.removeFromTop(40);
    listViewTab.setBounds(tabRow.removeFromLeft(100).reduced(4));
    mapViewTab.setBounds(tabRow.removeFromLeft(100).reduced(4));

    // ===== Sidebar =====
    auto sidebarArea = area.removeFromLeft(220);
    categoriesPanel.setBounds(sidebarArea);
    categoriesTitle.setBounds(sidebarArea.removeFromTop(40).reduced(10));
    favoritesButton.setBounds(sidebarArea.removeFromTop(36).reduced(10, 4));
    createdSoundsButton.setBounds(sidebarArea.removeFromTop(36).reduced(10, 4));
    editedSoundsButton.setBounds(sidebarArea.removeFromTop(36).reduced(10, 4));
    area.removeFromLeft(20);

    // ===== Content Area =====
    if (showingMap)
    {
        clusterMap.setBounds(area);
    }
    else
    {
        viewport.setBounds(area);
        int estimatedHeight = juce::jmax(1, (int)audioCards.size() * 140);
        cardsContainer.setBounds(0, 0, area.getWidth(), estimatedHeight);
        applyFilter();
    }
}

void Homescreen::setUsername(const juce::String& username)
{
    currUser = username;
    greetingLabel.setText("Good Morning, " + currUser, juce::dontSendNotification);
}

void Homescreen::setRole(const juce::String& newRole)
{
    currentRole = newRole;
    headerBar.setRole(newRole);
}

void Homescreen::timerCallback()
{
    if (transportSource.isPlaying() && transportSource.getLengthInSeconds() > 0)
    {
        float ratio = (float)(transportSource.getCurrentPosition() / transportSource.getLengthInSeconds());
        for (auto& card : audioCards)
        {
            if (card->getIsPlaying())
            {
                card->setPlayheadPosition(ratio);
            }
        }
    }
}