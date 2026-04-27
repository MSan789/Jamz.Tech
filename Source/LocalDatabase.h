#pragma once
#include <JuceHeader.h>
#include <vector>

extern "C"
{
        #include "sqlite/sqlite3.h"
}

struct RecordingEntry
{
    int id = -1;
    juce::String audioTitle;
    juce::String accountName;
    juce::String category;
    juce::String filePath;
    juce::String imagePath;
    juce::String createdAt;
    
    float price = 0.0f;
};

class LocalDatabase
{
public:
    LocalDatabase();
    ~LocalDatabase();

    bool open(const juce::String& path);
    bool createTables();

    bool insert(
                const juce::String& audioTitle,
                const juce::String& accountName,
                const juce::String& category,
                const juce::String& filePath,
                const juce::String& imagePath,
                float price,
                const juce::String& createdAt);

    std::vector<RecordingEntry> getAllRecordings();

private:
    sqlite3* db = nullptr;
    juce::File dbFile;
};
