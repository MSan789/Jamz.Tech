#pragma once
#include <JuceHeader.h>
#include "AudioCards.h"
#include "LocalDatabase.h"
#include "HeaderBar.h"
#include "ClusterMapComponent.h"
#include <vector>

class Homescreen : public juce::Component {
    public:
    Homescreen();
    ~Homescreen() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
    void setUsername(const juce::String& username);
    void setRole(const juce::String& newRole);

    void loadRecordings();

	void setRecordButtonVisible(bool shouldShow);

    juce::String currentRole;
    std::function<void()> onLogoutClicked;
    std::function<void()> onRecordClicked;
    std::function<void(int)> onEditClicked; 
    std::function<void(const RecordingEntry&)> onEditRequested; 
    
    private:
    HeaderBar headerBar;

    juce::String currUser;
    
    //top of the homescreen
    
    juce::Label greetingLabel;
    juce::Label appName;
    juce::TextEditor searchBar;
    
    //sidebar
    juce::Component categoriesPanel;
    juce::Label categoriesTitle;
       
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

    void playRecording(const RecordingEntry& entry);;
    void updateNowPlaying(const RecordingEntry& entry);
    std::vector<float> computeWaveformPeaks01(const juce::File& file, int numPoints);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Homescreen);
};
