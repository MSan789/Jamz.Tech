#pragma once
#include <JuceHeader.h>
#include <vector>

// Forward declaration to avoid including the heavy sqlite header in this header
// which can cause excessive include depth during compilation.
extern "C" {
    struct sqlite3;
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
                const juce::String& createdAt);

    std::vector<RecordingEntry> getAllRecordings();

private:
    sqlite3* db = nullptr;
    juce::File dbFile;
};
