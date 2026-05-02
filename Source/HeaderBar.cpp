#include "HeaderBar.h"

HeaderBar::HeaderBar()
{
	// ui elements 
		logoImage = juce::ImageCache::getFromMemory(BinaryData::jam_png, BinaryData::jam_pngSize);
        addAndMakeVisible(logoButton);
        logoButton.setImages(false, true, true, logoImage, 1.0f, juce::Colour(), logoImage, 1.0f, juce::Colour(), logoImage, 1.0f, juce::Colour());
        
        // load account images 
        accountImageDefault = juce::ImageCache::getFromMemory(BinaryData::guest_png, BinaryData::guest_pngSize);
        accountImageOwner = juce::ImageCache::getFromMemory(BinaryData::owner_png, BinaryData::owner_pngSize);

        // load record image
        recordImage = juce::ImageCache::getFromMemory(BinaryData::record_png, BinaryData::record_pngSize);

        // account button next to logo, uses an image that changes based on role
        addAndMakeVisible(accountButton);
        accountButton.setTooltip("Account");
        addAndMakeVisible(recordButton);
        recordButton.setTooltip("Record");
		recordButton.setVisible(true);
        addAndMakeVisible(uploadButton);
        uploadImage = juce::ImageCache::getFromMemory(BinaryData::upload_png, BinaryData::upload_pngSize);
        uploadButton.setImages(false, true, true, uploadImage, 1.0f, juce::Colour(), uploadImage, 1.0f, juce::Colour(), uploadImage, 1.0f, juce::Colour());
        uploadButton.setTooltip("Upload");
        
		addAndMakeVisible(titleLabel);

        // make sure the account image matches initial role
        updateAccountImage();

        logoButton.onClick = [this]()
        {
            if (onLogoClicked)
                onLogoClicked();
        };

        accountButton.onClick = [this]() 
		{
				// display dropdown
				juce::PopupMenu menu;
				menu.addSectionHeader("Account");
				menu.addItem(1, "Create Guest Account");
				menu.addItem(2, "Logout");
				
				// menu functionality
				menu.showMenuAsync(
					juce::PopupMenu::Options().withTargetComponent(&accountButton),
					[this](int result)
					{
						if (result == 1)
						{
							if (onCreateGuestClicked)
							{
								onCreateGuestClicked(); 
							}
						}
						if (result == 2)
						{
							if (onLogoutClicked)
							{
								onLogoutClicked();
							}

						}
					}
				);
		};

		recordButton.onClick = [this]() 
		{
			if (onRecordClicked)
				onRecordClicked();
		};

        uploadButton.onClick = [this]()
        {
            if (onUploadClicked)
                onUploadClicked();
        };

        // set the record button image and click handler
        if (recordImage.isValid())
        {
            recordButton.setImages(
                /*resizeButtonNowToFitThisImage*/ true,
                /*rescaleImagesWhenButtonSizeChanges*/ true,
                /*preserveImageProportions*/ true,
                recordImage, 1.0f, juce::Colour(),
                recordImage, 1.0f, juce::Colour(),
                recordImage, 1.0f, juce::Colour(),
                0.0f);
        }
		recordImage = juce::ImageCache::getFromMemory(BinaryData::record_png, BinaryData::record_pngSize);

		titleLabel.setText("Jamz.Tech", juce::dontSendNotification);
		titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
		titleLabel.setFont(22.0f);
		titleLabel.setJustificationType(juce::Justification::centred);
}

HeaderBar::~HeaderBar() {}

void HeaderBar::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::black.withAlpha(0.25f)); // translucent glass header
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(0, (float)getHeight() - 1.0f, (float)getWidth(), (float)getHeight() - 1.0f, 1.5f);

	// draw logo
	if (logoImage.isValid()) {
		// draw a larger logo (48x48)
        g.setOpacity(1.0f);
        // Draw the logo a few times to boost its opacity if the original PNG is translucent
        for (int i = 0; i < 3; ++i)
		    g.drawImageWithin(logoImage, 8, 6, 48, 48, juce::RectanglePlacement::centred);
	}
}

void HeaderBar::setRole(const juce::String& newRole)
{
	currentRole = newRole; 
    uploadButton.setVisible(currentRole == "Owner");
    updateAccountImage();
    resized();
}

void HeaderBar::updateAccountImage()
{
    // choose image based on role; fall back to default
    juce::Image img;
    if (currentRole == "Owner")
        img = accountImageOwner.isValid() ? accountImageOwner : accountImageDefault;
    else
        img = accountImageDefault.isValid() ? accountImageDefault : accountImageOwner;

    if (img.isValid())
    {
        // set the same image for normal/over/down states using the full-argument overload
        accountButton.setImages(
            /*resizeButtonNowToFitThisImage*/ true,
            /*rescaleImagesWhenButtonSizeChanges*/ true,
            /*preserveImageProportions*/ true,
            img, 1.0f, juce::Colour(),
            img, 1.0f, juce::Colour(),
            img, 1.0f, juce::Colour(),
            0.0f);
    }
}

// establish layout 
void HeaderBar::resized()
{
	auto bounds = getLocalBounds().reduced(8, 4); 

    // give more space on the right so the account button can show its full text
    auto rightArea = bounds.removeFromRight(240);
    // place account button at the far right (wider so its text isn't elided)
    accountButton.setBounds(rightArea.removeFromRight(120).reduced(2));
    // record button sits to the left of the account button and stays compact
    recordButton.setBounds(rightArea.removeFromRight(34).reduced(2));
    
    if (uploadButton.isVisible())
    {
        rightArea.removeFromRight(8); // space between upload and record
        uploadButton.setBounds(rightArea.removeFromRight(34).reduced(2)); // Use icon-sized bounds
    }

	auto leftArea = bounds.removeFromLeft(150);
	logoButton.setBounds(8, 6, 48, 48);
	leftArea.removeFromLeft(54); // space for larger logo

	titleLabel.setBounds(bounds);
}

// record button visibility control
void HeaderBar::setRecordButtonVisible(bool shouldShow)
{
	recordButton.setVisible(shouldShow);
	resized();
}