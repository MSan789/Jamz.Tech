#pragma once
#include <JuceHeader.h>

class HeaderBar : public juce::Component
{
public:
	HeaderBar();
	~HeaderBar();

	std::function<void()> onLogoutClicked; 
	std::function<void()> onRecordClicked;
    std::function<void()> onUploadClicked;
	std::function<void()> onCreateGuestClicked;
	void paint(juce::Graphics & g) override; // header background
 	void resized() override; // header positioning and layout
	void setRole(const juce::String& newRole);
	void setRecordButtonVisible(bool shouldShow);
    std::function<void()> onLogoClicked;

private:
	juce::String currentRole;
    
    juce::ImageButton uploadButton;
    juce::Image uploadImage;
    
    juce::ImageButton logoButton;
    juce::Image logoImage;
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