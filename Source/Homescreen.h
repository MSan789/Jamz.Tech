#pragma once
#include <JuceHeader.h>
#include "AudioCards.h"
#include "LocalDatabase.h"
#include "HeaderBar.h"
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

    juce::String currentRole;
    std::function<void()> onLogoutClicked;
    std::function<void()> onRecordClicked;
    std::function<void(int)> onEditClicked; // Pass the index of the audio card to edit
    std::function<void(const RecordingEntry&)> onEditRequested; // New callback for edit requests, passing the index of the audio card

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
    juce::Component cardsContainer;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    void playRecording(const RecordingEntry& entry);;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Homescreen);
};