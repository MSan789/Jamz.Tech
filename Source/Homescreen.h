/*
  ==============================================================================

    Homescreen.h
    Created: 28 Feb 2026 10:02:12pm
    Author:  Tanishi Kumar

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "AudioCards.h"
#include "LocalDatabase.h"
#include "HeaderBar.h"
#include "ClusterMapComponent.h"
#include <vector>
#include <set>
#include <map>

class Homescreen : public juce::Component, public juce::Timer {
    public:
    Homescreen();
    ~Homescreen() override;
    
    void timerCallback() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
    void setUsername(const juce::String& username);
    void setRole(const juce::String& newRole);

    void loadRecordings();

    void setRecordButtonVisible(bool shouldShow);

    juce::String currentRole;
    std::function<void()> onLogoutClicked;
    std::function<void()> onCreateGuestClicked;
    std::function<void()> onRecordClicked;
    std::function<void(int)> onEditClicked;
    std::function<void(const RecordingEntry&)> onEditRequested;
    
    void handleUpload();
    void promptForNextUpload(juce::Array<juce::File> files, int index);
    
    private:
    HeaderBar headerBar;
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::String currUser;
    
    //top of the homescreen
    
    juce::Label greetingLabel;
    juce::Label appName;
    juce::TextEditor searchBar;
    
    //sidebar
    juce::Component categoriesPanel;
    juce::Label categoriesTitle;
    juce::TextButton favoritesButton { "Favorites" };
    juce::TextButton createdSoundsButton { "Created Sounds" };
    juce::TextButton editedSoundsButton { "Edited Sounds" };
    
    bool showingCreatedSounds = false;
    bool showingEditedSounds = false;

    void applyFilter();

    bool showingFavoritesFilter = false;
    std::map<juce::String, std::set<int>> userFavorites;
       
    LocalDatabase database;
    std::vector<RecordingEntry> recordings;
    std::vector<std::unique_ptr<AudioCards>> audioCards;

    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transportSource;
    juce::Viewport viewport;
    juce::Viewport mapViewport;
    juce::Component cardsContainer;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    
    ClusterMapComponent clusterMap;
    juce::TextButton listViewTab { "List" };
    juce::TextButton mapViewTab { "Map" };
    bool showingMap = false;
    juce::String currentSearchQuery;

    void playRecording(const RecordingEntry& entry);
    void updateNowPlaying(const RecordingEntry& entry);
    std::vector<float> computeWaveformPeaks01(const juce::File& file, int numPoints);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Homescreen);
};
