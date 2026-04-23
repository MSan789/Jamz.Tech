#include "HeaderBar.h"

HeaderBar::HeaderBar()
{
	// ui elements 
		// logo image top left, take to home bar when clicked
		logo = juce::ImageCache::getFromMemory(BinaryData::jam_png, BinaryData::jam_pngSize);
        
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
		addAndMakeVisible(titleLabel);

        // make sure the account image matches initial role
        updateAccountImage();

        accountButton.onClick = [this]() 
		{
				// display dropdown
				juce::PopupMenu menu;
				menu.addSectionHeader("Account");
				menu.addItem(1, "Saved Sounds");
				menu.addItem(2, "Edited Sounds");
				if (currentRole == "Owner")
				{
					menu.addItem(3, "Created Sounds");
				}
				menu.addItem(4, "Create Guest Account");
				menu.addItem(5, "Logout");
				
				// menu functionality
				menu.showMenuAsync(
					juce::PopupMenu::Options().withTargetComponent(&accountButton),
					[this](int result)
					{
						if (result == 1)
						{
							juce::AlertWindow::showMessageBoxAsync(
								juce::AlertWindow::InfoIcon,
								"Saved Sounds",
								"Open Saved Sounds page here."
							);
						}
						if (result == 2)
						{
							juce::AlertWindow::showMessageBoxAsync(
								juce::AlertWindow::InfoIcon,
								"Edited Sounds",
								"Open Edited Sounds page here."
							);
						}
						if (result == 3)
						{
							if (currentRole != "Owner")
							{
								juce::AlertWindow::showMessageBoxAsync(
									juce::AlertWindow::WarningIcon,
									"Access denied",
									"Guests cannot record sounds."
								);
								return; 
							}
							else
							{
								juce::AlertWindow::showMessageBoxAsync(
									juce::AlertWindow::InfoIcon,
									"Created Sounds",
									"Open Created Sounds page here."
								);
							}
						}
						if (result == 4)
						{
							if (onCreateGuestClicked)
							{
								onCreateGuestClicked(); 
							}
						}
						if (result == 5)
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

		titleLabel.setText("Jamz.Tech", juce::dontSendNotification);
		titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
		titleLabel.setFont(22.0f);
		titleLabel.setJustificationType(juce::Justification::centred);
}

HeaderBar::~HeaderBar() {}

void HeaderBar::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::black); // header background color

	// draw logo 
	if (logo.isValid()) {
		// draw a larger logo (48x48)
		g.drawImageWithin(logo, 8, 6, 48, 48, juce::RectanglePlacement::centred);
	}
}

void HeaderBar::setRole(const juce::String& newRole)
{
	currentRole = newRole; 
    updateAccountImage();
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
    auto rightArea = bounds.removeFromRight(160);
    // place account button at the far right (wider so its text isn't elided)
    accountButton.setBounds(rightArea.removeFromRight(120).reduced(2));
    // record button sits to the left of the account button and stays compact
    recordButton.setBounds(rightArea.removeFromRight(34).reduced(2));

	auto leftArea = bounds.removeFromLeft(150);
	leftArea.removeFromLeft(54); // space for larger logo

	titleLabel.setBounds(bounds);
}

// record button visibility control
void HeaderBar::setRecordButtonVisible(bool shouldShow)
{
	recordButton.setVisible(shouldShow);
	resized();
}