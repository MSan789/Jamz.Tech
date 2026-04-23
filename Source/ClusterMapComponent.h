/*
  ==============================================================================

    ClusterMapComponent.h
    Created: 11 Apr 2026 8:24:32pm
    Author:  Tanishi Kumar

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "LocalDatabase.h"

class ClusterMapComponent : public juce::Component
{
    public:
    ClusterMapComponent();
    ~ClusterMapComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void loadRecordings();
    void setSearchQuery(const juce::String& query);
    std::function<void(const RecordingEntry&)> onDotClicked;
    
    private:
    struct DotEntry
    {
        RecordingEntry recording;
        float x = 0.0f;
        float y = 0.0f;
        juce::Colour colour;
    };
    
    std::vector<DotEntry> dots;
    juce::Point<float> panOffset { 0.0f, 0.0f };
    juce::Point<float> lastMousePos;
    juce::String hoveredTitle = "";
    bool isDragging = false;
    juce::String currentSearchQuery;
    
    std::map<juce::String, juce::Colour> categoryColours;
    
    void assignPositions(std::vector<DotEntry>& entries);
    juce::Colour getColourForCategory(const juce::String& category);
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    juce::Time parseCreatedAt(const juce::String& s);
    
    LocalDatabase database;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClusterMapComponent)
    
};
