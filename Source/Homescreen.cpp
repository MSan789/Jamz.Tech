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

    //sidebar panel:
    addAndMakeVisible(categoriesPanel);
    addAndMakeVisible(categoriesTitle);
    categoriesTitle.setText("Categories", juce::dontSendNotification);
    categoriesTitle.setColour(juce::Label::textColourId, juce::Colours::white);
    categoriesTitle.setFont(18.0f);

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
}

Homescreen::~Homescreen()
{
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr); audioSourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    transportSource.setSource(nullptr);
    readerSource.reset();
}

void Homescreen::paint(juce::Graphics& g) {

    juce::ColourGradient gradient(
        juce::Colour(28, 30, 45),
        0, 0,
        juce::Colour(15, 16, 25),
        0, getHeight(),
        false);

    g.setGradientFill(gradient);
    g.fillAll();

    g.setColour(juce::Colour(32, 34, 50));
    g.fillRect(categoriesPanel.getBounds());
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
            recordings.push_back(entry);
    }

    for (size_t i = 0; i < recordings.size(); ++i)
    {
        auto card = std::make_unique<AudioCards>();
        card->setRecording(recordings[i]);

        card->onPlayClicked = [this](const RecordingEntry& entry)
            {
                playRecording(entry);
            };

        card->onEditClicked = [this](const RecordingEntry& entry)
            {
                if (onEditRequested)
                    onEditRequested(entry);
            };
        cardsContainer.addAndMakeVisible(*card);
        audioCards.push_back(std::move(card));
    }

    resized();
    repaint();
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

    DBG("Now playing: " + entry.audioTitle);
}

void Homescreen::resized() {
    auto area = getLocalBounds();
    // make header taller to fit the larger logo
    headerBar.setBounds(area.removeFromTop(64));
    area = area.reduced(20);

    // ===== Top Bar =====
    auto topBar = area.removeFromTop(100);

    // LEFT: Greeting
    auto leftArea = topBar.removeFromLeft(300);
    greetingLabel.setBounds(leftArea);

    // RIGHT: Search + Logout
    auto rightArea = topBar.removeFromRight(380);
    searchBar.setBounds(rightArea.reduced(5));

    //sidebar:
    auto sidebarArea = area.removeFromLeft(220);
    categoriesPanel.setBounds(sidebarArea);
    categoriesTitle.setBounds(sidebarArea.removeFromTop(40).reduced(10));
    area.removeFromLeft(20);


    /*int cardHeight = 120;
    int spacing = 20;

    for (auto& card : audioCards) {
        card->setBounds(area.removeFromTop(cardHeight));
        area.removeFromTop(spacing);
    }*/

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

void Homescreen::setUsername(const juce::String& username) {
    currUser = username;
    greetingLabel.setText("Good Morning, " + currUser, juce::dontSendNotification);
}

void Homescreen::setRole(const juce::String& newRole)
{
    currentRole = newRole;
}