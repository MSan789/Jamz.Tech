/*
  ==============================================================================

    ClusterMapComponent.cpp
    Created: 11 Apr 2026 8:24:01pm
    Author:  Tanishi Kumar

  ==============================================================================
*/

#include "ClusterMapComponent.h"


ClusterMapComponent::ClusterMapComponent()
{
    setOpaque(false);
}

ClusterMapComponent::~ClusterMapComponent()
{
    
}

juce::Colour ClusterMapComponent::getColourForCategory(const juce::String& category)
{
    if(categoryColours.find(category) == categoryColours.end())
    {
        static const juce::Colour palette[] = {
            juce::Colour(130, 100, 220), //purple
            juce::Colour(80, 180, 160), //teal
            juce::Colour(220, 120, 80), //orange
            juce::Colour(90, 150, 220), //blue
            juce::Colour(210, 90, 130), //pink
            juce::Colour(160, 200, 80), //green
        };
        
        int idx = (int)categoryColours.size() % 6;
        categoryColours[category] = palette[idx];
    }
    return categoryColours[category];
    
}


juce::Time ClusterMapComponent::parseCreatedAt(const juce::String& s)
{
        int year   = s.substring(0, 4).getIntValue();
        int month  = s.substring(5, 7).getIntValue();
        int day    = s.substring(8, 10).getIntValue();
        int hour   = s.substring(11, 13).getIntValue();
        int minute = s.substring(14, 16).getIntValue();
        int second = s.substring(17, 19).getIntValue();
        return juce::Time(year, month - 1, day, hour, minute, second, 0, false);
}

void ClusterMapComponent::assignPositions(std::vector<DotEntry>& entries)
{
    
    if(entries.empty()) return;
    
    juce::Time oldest, newest;
    bool firstDate = true;
    
    for(auto& e : entries)
    {
        juce::Time t = parseCreatedAt(e.recording.createdAt);
        if(firstDate)
        {
            oldest = newest = t;
            firstDate = false;
        }
        else
        {
            if (t < oldest) oldest = t;
            if (t > newest) newest = t;
        }
    }
    
    double dateRange = (double)(newest.toMilliseconds() - oldest.toMilliseconds());
    if(dateRange == 0.0) dateRange = 1.0;
    
    juce::String minTitle = entries[0].recording.audioTitle.toLowerCase();
    juce::String maxTitle = minTitle;
    
    for(auto& e : entries)
    {
        juce::String t = e.recording.audioTitle.toLowerCase();
        if(t < minTitle) minTitle = t;
        if (t > maxTitle) maxTitle = t;
    }
    
    std::map<juce::String, std::vector<int>> groups;
    for (int i = 0; i < (int)entries.size(); ++i)
        groups[entries[i].recording.category].push_back(i);
    
    float canvasW = (float)getWidth() > 0 ? (float)getWidth() : 800.0f;
    float canvasH = (float)getHeight() > 0 ? (float)getHeight() : 600.0f;
    
    float padX = 150.0f;
    float padY = 150.0f;
    
    float usableW = canvasW - padX * 2.0f;
    float usableH = canvasH - padY * 2.0f;
    
    juce::Random rng(42);
    float jitter = 30.0f;
    
    for(auto& [cat, indices] : groups)
    {
        for(int idx : indices)
        {
            auto& entry = entries[idx];
            juce::String title = entry.recording.audioTitle.toLowerCase();
           
            /*if(minTitle != maxTitle)
            {
                float minVal = (float)(minTitle[0] - 'a');
                float maxVal = (float)(maxTitle[0] - 'a');
                float val = (float)(title[0]       - 'a');
                if(maxVal != minVal)
                    xNorm = (val - minVal) / (maxVal - minVal);
            }*/
            float xNorm = 0.5f;
            if (minTitle != maxTitle)
            {
                // compare up to 4 characters for better spread
                float val = 0.0f, minVal = 0.0f, maxVal = 0.0f;
                for (int c = 0; c < 4; ++c)
                {
                    float weight = std::pow(26.0f, 3.0f - c);
                    val    += (title.length()    > c ? (float)(title[c]    - 'a') : 0.0f) * weight;
                    minVal += (minTitle.length() > c ? (float)(minTitle[c] - 'a') : 0.0f) * weight;
                    maxVal += (maxTitle.length() > c ? (float)(maxTitle[c] - 'a') : 0.0f) * weight;
                }
                if (maxVal != minVal)
                    xNorm = (val - minVal) / (maxVal - minVal);
            }
            
            juce::Time t = parseCreatedAt(entry.recording.createdAt);
            double ms = (double)(t.toMilliseconds() - oldest.toMilliseconds());
            float yNorm = 1.0f - (float)(ms / dateRange);
            
            /*entry.x = padX + xNorm * usableW + rng.nextFloat() * jitter * 2.0f - jitter;
            entry.y = padY + yNorm * usableH + rng.nextFloat() * jitter * 2.0f - jitter;
            entry.x = juce::jlimit(padX, canvasW - padX, entry.x);
            entry.y = juce::jlimit(padY, canasH - padY, entry.y);
            entry.colour = getColourForCategory(cat);*/
            
            int catIndex = 0;
            for (auto& [c, ignored] : groups)
            {
                if (c == cat) break;
                ++catIndex;
            }
            int totalCats = (int)groups.size();
            float catOffsetX = (float)catIndex / (float)juce::jmax(1, totalCats - 1) * usableW * 0.4f;
            float catOffsetY = (float)catIndex / (float)juce::jmax(1, totalCats - 1) * usableH * 0.4f;

            entry.x = padX + xNorm * usableW * 0.6f + catOffsetX + rng.nextFloat() * jitter * 2.0f - jitter;
            entry.y = padY + yNorm * usableH * 0.6f + catOffsetY + rng.nextFloat() * jitter * 2.0f - jitter;
            entry.x = juce::jlimit(padX, canvasW - padX, entry.x);
            entry.y = juce::jlimit(padY, canvasH - padY, entry.y);
            entry.colour = getColourForCategory(cat);
        }
    }
}

void ClusterMapComponent::loadRecordings()
{
    dots.clear();
    categoryColours.clear();

    auto all = database.getAllRecordings();

    for (auto& entry : all)
    {
        DotEntry d;
        d.recording = entry;
        dots.push_back(d);
    }

    assignPositions(dots);
    repaint();
}

void ClusterMapComponent::setSearchQuery(const juce::String& query)
{
    currentSearchQuery = query;
    repaint();
}

void ClusterMapComponent::paint(juce::Graphics& g)
{
    // background
    //g.fillAll(juce::Colour(15, 16, 25));
    g.fillAll(juce::Colours::black.withAlpha(0.2f));
    
    // y axis :
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("Newest", 5, 100, 60, 14, juce::Justification::left);
    g.drawText("Oldest", 5, getHeight()- 30, 60, 14, juce::Justification::left);
    
    // x axis:
    g.drawText("A", 100, getHeight() - 20, 20, 14, juce::Justification::centred);
    g.drawText("Z", getWidth() - 40, getHeight() - 20, 20, 14, juce::Justification::centred);

    
    // axis lines:
    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    g.drawLine(100, 100, 100,  getHeight() - 30, 1.0f);
    g.drawLine(100, getHeight() - 30, getWidth() - 40, getHeight() - 30, 1.0f);
    
    
    // legend top-left
    int legendY = 10;
    for (auto& [cat, colour] : categoryColours)
    {
        g.setColour(colour);
        g.fillEllipse(10, legendY, 12, 12);
        g.setColour(juce::Colours::white);
        g.setFont(13.0f);
        g.drawText(cat.isEmpty() ? "General" : cat,
                   28, legendY, 120, 14,
                   juce::Justification::left);
        legendY += 20;
    }

    // draw dots
    float dotRadius = 8.0f;

    for (auto& dot : dots)
    {
        float px = dot.x + panOffset.x;
        float py = dot.y + panOffset.y;

        // glow
        g.setColour(dot.colour.withAlpha(0.25f));
        g.fillEllipse(px - dotRadius * 1.8f,
                      py - dotRadius * 1.8f,
                      dotRadius * 3.6f,
                      dotRadius * 3.6f);

        // core dot
        g.setColour(dot.colour);
        g.fillEllipse(px - dotRadius, py - dotRadius,
                      dotRadius * 2.0f, dotRadius * 2.0f);

        // label
        if (dot.recording.audioTitle == hoveredTitle)
        {
            g.setColour(juce::Colours::white);
            g.setFont(12.0f);
            g.drawText(dot.recording.audioTitle,
                       (int)(px - 50), (int)(py + dotRadius + 4),
                       100, 16,
                       juce::Justification::centred);
        }
    }

    // empty state
    if (dots.empty())
    {
        g.setColour(juce::Colours::grey);
        g.setFont(18.0f);
        g.drawText("No recordings found.",
                   getLocalBounds(),
                   juce::Justification::centred);
    }
}

void ClusterMapComponent::resized() {
    assignPositions(dots);
    repaint();
}

void ClusterMapComponent::mouseDown(const juce::MouseEvent& e)
{
    // check if a dot was clicked
    float dotRadius = 6.0f;
    for (auto& dot : dots)
    {
        float px = dot.x + panOffset.x;
        float py = dot.y + panOffset.y;
        float dx = e.position.x - px;
        float dy = e.position.y - py;
        if (dx * dx + dy * dy <= dotRadius * dotRadius * 2.0f)
        {
            if (onDotClicked)
                onDotClicked(dot.recording);
            return;
        }
    }

    // otherwise start panning
    isDragging = true;
    lastMousePos = e.position;
}

void ClusterMapComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging) return;

    panOffset.x += e.position.x - lastMousePos.x;
    panOffset.y += e.position.y - lastMousePos.y;
    lastMousePos = e.position;
    repaint();
}

void ClusterMapComponent::mouseUp(const juce::MouseEvent& e)
{
    isDragging = false;
}

void ClusterMapComponent::mouseMove(const juce::MouseEvent& e)
{
    float dotRadius = 6.0f;
    hoveredTitle = "";
    
    for(auto& dot : dots)
    {
        float px = dot.x + panOffset.x;
                float py = dot.y + panOffset.y;
                float dx = e.position.x - px;
                float dy = e.position.y - py;

                if (dx * dx + dy * dy <= dotRadius * dotRadius * 3.0f)
                {
                    hoveredTitle = dot.recording.audioTitle;
                    break;
                }
    }
    repaint();
}

