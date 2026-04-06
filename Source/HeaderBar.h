#pragma once
#include <JuceHeader.h>

class HeaderBar : public juce::Component
{
public:
	HeaderBar();
	~HeaderBar();

	std::function<void()> onLogoutClicked; 
	std::function<void()> onRecordClicked;
	void paint(juce::Graphics & g) override; // header background
 	void resized() override; // header positioning and layout
	void setRole(const juce::String& newRole);

private:
	juce::String currentRole;

    juce::Image logo;
    juce::Label titleLabel;
    
    //account 
    juce::ImageButton accountButton;
    juce::Image accountImageDefault;
    juce::Image accountImageOwner;

    //record
    juce::ImageButton recordButton;
    juce::Image recordImage;

    void updateAccountImage();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};