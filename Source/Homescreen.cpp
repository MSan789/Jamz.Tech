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

    //greeting:
    addAndMakeVisible(greetingLabel);
    greetingLabel.setFont(20.0f);
    greetingLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    //search bar:
    addAndMakeVisible(searchBar);
    searchBar.setTextToShowWhenEmpty("Search Sounds..", juce::Colours::lightpink);
    searchBar.setColour(juce::TextEditor::backgroundColourId, juce::Colour(35, 35, 50));
    searchBar.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    
    searchBar.onTextChange = [this]()
    {
        currentSearchQuery = searchBar.getText();
        loadRecordings();
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
    favoritesButton.onClick = [this]() { openFavoritesOverlay(); };

    // Favorites overlay setup
    favoritesTitle.setText("Favorited Sounds", juce::dontSendNotification);
    favoritesTitle.setFont(20.0f);
    favoritesTitle.setColour(juce::Label::textColourId, juce::Colours::white);
    favoritesTitle.setJustificationType(juce::Justification::centred);

    closeFavoritesButton.setColour(juce::TextButton::buttonColourId, juce::Colour(90, 40, 40));
    closeFavoritesButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    closeFavoritesButton.onClick = [this]() { closeFavoritesOverlay(); };

    favoritesOverlay.addAndMakeVisible(favoritesTitle);
    favoritesOverlay.addAndMakeVisible(closeFavoritesButton);
    favoritesOverlay.addAndMakeVisible(favoritesViewport);
    favoritesViewport.setViewedComponent(&favoritesContainer, false);
    favoritesViewport.setScrollBarsShown(true, false);
    addChildComponent(favoritesOverlay);
    favoritesOverlay.setInterceptsMouseClicks(true, true);

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
        for(auto& card : audioCards)
            card->setVisible(true);
        
        resized();
        repaint();
    };
    
    mapViewTab.onClick = [this]()
    {
        showingMap = true;
        clusterMap.loadRecordings();
        clusterMap.setVisible(true);
        for(auto& card : audioCards)
            card->setVisible(false);
        
        resized();
        repaint();
    };

    clusterMap.onDotClicked = [this] (const RecordingEntry& entry)
    {
        playRecording(entry);
    };
    
}

Homescreen::~Homescreen()
{
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    transportSource.setSource(nullptr);
    readerSource.reset();
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

    // Favorites overlay background
    if (showingFavorites)
    {
        // dim the rest of the screen
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillAll();

        auto ob = favoritesOverlay.getBounds().toFloat();
        //g.setColour(juce::Colour(28, 20, 45).withAlpha(0.97f));
        g.setColour(juce::Colour(28, 20, 45));
        g.fillRoundedRectangle(ob, 16.0f);
        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.drawRoundedRectangle(ob, 16.0f, 1.5f);
    }
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
        if (entry.accountName == currUser)
        {
            if (currentSearchQuery.isEmpty() || entry.audioTitle.containsIgnoreCase(currentSearchQuery))
            {
                recordings.push_back(entry);
            }
        }
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

        card->onPlayClicked = [this](const RecordingEntry& entry)
            {
                updateNowPlaying(entry);
                playRecording(entry);
            };

        card->onEditClicked = [this](const RecordingEntry& entry)
            {
                if (onEditRequested)
                    onEditRequested(entry);
            };

        card->onFavoriteToggled = [this](const RecordingEntry& entry, bool fav)
        {
            if (fav)
                favoritedIds.insert(entry.id);
            else
                favoritedIds.erase(entry.id);

            
            if (showingFavorites)
                openFavoritesOverlay();
        };

        // Restore favorite state if it was set before reload
        if (favoritedIds.count(recordings[i].id))
            card->setFavorite(true);

        cardsContainer.addAndMakeVisible(*card);
        audioCards.push_back(std::move(card));
    }

    resized();
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
    area.removeFromLeft(20);

    // ===== Content Area =====
    if (showingMap)
    {
        clusterMap.setBounds(area);
    }
    else
    {
        viewport.setBounds(area);

        int cardHeight = 120;
        int spacing = 20;
        int totalHeight = (int)audioCards.size() * (cardHeight + spacing);

        cardsContainer.setBounds(0, 0, area.getWidth(), totalHeight);

        int y = 0;
        for (auto& card : audioCards)
        {
            card->setBounds(0, y, area.getWidth(), cardHeight);
            y += cardHeight + spacing;
        }
    }

    // ===== Favorites Overlay =====
    if (showingFavorites)
    {
        //auto overlayBounds = getLocalBounds().reduced(60, 40);
        //favoritesOverlay.setBounds(overlayBounds);

        //auto oa = favoritesOverlay.getLocalBounds().reduced(12);
        auto overlayBounds = getLocalBounds();
        favoritesOverlay.setBounds(overlayBounds);

        auto oa = favoritesOverlay.getLocalBounds().reduced(60, 40);
        closeFavoritesButton.setBounds(oa.removeFromTop(32).removeFromRight(100));
        //favoritesTitle.setBounds(favoritesOverlay.getLocalBounds().removeFromTop(44));
        auto headerArea = favoritesOverlay.getLocalBounds().removeFromTop(80);

        favoritesTitle.setBounds(headerArea.removeFromTop(40));
        closeFavoritesButton.setBounds(headerArea.removeFromRight(120).reduced(10));
        oa.removeFromTop(4);
        favoritesViewport.setBounds(oa);

        int cardH = 120, spacing = 16;
        int totalH = (int)favCards.size() * (cardH + spacing);
        favoritesContainer.setBounds(0, 0, oa.getWidth(), juce::jmax(totalH, 1));
        int fy = 0;
        for (auto& fc : favCards)
        {
            fc->setBounds(0, fy, oa.getWidth(), cardH);
            fy += cardH + spacing;
        }
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

void Homescreen::openFavoritesOverlay()
{
    // Rebuild fav cards from currently favorited IDs
    for (auto& fc : favCards)
        favoritesContainer.removeChildComponent(fc.get());
    favCards.clear();

    auto all = database.getAllRecordings();

    for (auto& entry : all)
    {
        if (entry.accountName != currUser)
            continue;

        if (favoritedIds.count(entry.id) == 0)
            continue;

        auto card = std::make_unique<AudioCards>();
        card->setRecording(entry);
        card->setFavorite(true);

        juce::File file(entry.filePath);
        if (file.existsAsFile())
        {
            auto peaks = computeWaveformPeaks01(file, 150);
            card->setWaveformPeaks(peaks);
        }

        card->onPlayClicked = [this](const RecordingEntry& e)
        {
            playRecording(e);
        };

        card->onEditClicked = [this](const RecordingEntry& e)
        {
            closeFavoritesOverlay();
            if (onEditRequested) onEditRequested(e);
        };

        card->onFavoriteToggled = [this](const RecordingEntry& e, bool fav)
        {
            if (fav) favoritedIds.insert(e.id);
            else     favoritedIds.erase(e.id);

            // sync main list
            for (auto& mc : audioCards)
                if (mc->getRecording().id == e.id)
                    mc->setFavorite(fav);

            openFavoritesOverlay(); // refresh overlay
        };

        favoritesContainer.addAndMakeVisible(*card);
        favCards.push_back(std::move(card));
    }

    showingFavorites = true;
    favoritesOverlay.setVisible(true);
    favoritesOverlay.toFront(true);
    resized();
    repaint();
    
    // Hide main UI behind overlay
    viewport.setVisible(false);
    cardsContainer.setVisible(false);
    listViewTab.setVisible(false);
    mapViewTab.setVisible(false);
    searchBar.setVisible(false);
    headerBar.setVisible(false);
    greetingLabel.setVisible(false);
    categoriesPanel.setVisible(false);
    categoriesTitle.setVisible(false);
    favoritesButton.setVisible(false);
}

void Homescreen::closeFavoritesOverlay()
{
    showingFavorites = false;
    favoritesOverlay.setVisible(false);
    // Restore main UI
    viewport.setVisible(true);
    cardsContainer.setVisible(true);
    listViewTab.setVisible(true);
    mapViewTab.setVisible(true);
    searchBar.setVisible(true);
    headerBar.setVisible(true);
    greetingLabel.setVisible(true);
    categoriesPanel.setVisible(true);
    categoriesTitle.setVisible(true);
    favoritesButton.setVisible(true);
    resized();
    repaint();
}
