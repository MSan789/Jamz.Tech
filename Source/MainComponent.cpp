#include "MainComponent.h"
#include "Homescreen.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize(600, 400);

    //title;
    titleLabel.setText("User Login", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    //username section;
    usernameEditor.setTextToShowWhenEmpty("Enter Username...", juce::Colours::grey);
    addAndMakeVisible(usernameEditor);

    //password section;
    passwordEditor.setTextToShowWhenEmpty("Enter password...", juce::Colours::grey);
    passwordEditor.setPasswordCharacter('*');
    addAndMakeVisible(passwordEditor);

    //button stuff;
    loginButton.addListener(this);
    createAccButton.addListener(this);

    addAndMakeVisible(loginButton);
    addAndMakeVisible(createAccButton);
    addAndMakeVisible(logoutButton);

    loadAccountsFromFile();

    //pages
    addAndMakeVisible(homeScreen);
    addAndMakeVisible(recordScreen);
    addAndMakeVisible(editScreen);
    homeScreen.setVisible(false);
    recordScreen.setVisible(false);
    editScreen.setVisible(false);

    //logout
    homeScreen.onLogoutClicked = [this]()
        {
            isLoggedIn = false;
            usernameEditor.clear();
            passwordEditor.clear();

            resized();
            repaint();
        };
    homeScreen.onRecordClicked = [this]()
        {
            showRecord();
        };
    homeScreen.onEditRequested = [this](const RecordingEntry& entry) { showEdit(entry); };
}

MainComponent::~MainComponent()
{
    loginButton.removeListener(this);
    createAccButton.removeListener(this);
}

void MainComponent::loadAccountsFromFile() {
    accounts.clear();

    if (accountsFile.existsAsFile()) {
        auto content = accountsFile.loadFileAsString();
        auto lines = juce::StringArray::fromLines(content);

        for (auto& line : lines) {
            auto parts = juce::StringArray::fromTokens(line, ":", "");
            if (parts.size() == 3) {
                Account acc;
                acc.password = parts[1].trim();
                acc.role = parts[2].trim();
                accounts[parts[0].trim()] = acc;
            }
        }
    }
}

void MainComponent::showRecord() {
    homeScreen.setVisible(false);
    recordScreen.setBounds(getLocalBounds());
    recordScreen.setVisible(true);

    recordScreen.setAccountName(currUsername);
    recordScreen.startRecording(); //for recording
    // when back is pressed on the record screen, return to home
    recordScreen.onBack = [this]() {
        recordScreen.setVisible(false);
        homeScreen.setVisible(true);
        resized();
        repaint();
        };

    resized();
}

void MainComponent::showEdit(const RecordingEntry& entry)
{
    homeScreen.setVisible(false);
    editScreen.setBounds(getLocalBounds());
    editScreen.setVisible(true);

    // Pass the selected recording into the edit page
    editScreen.setRecording(entry);

    // Back button — return to homescreen
    editScreen.onBack = [this]()
        {
            editScreen.setVisible(false);
            homeScreen.setVisible(true);
            homeScreen.loadRecordings();
            resized();
            repaint();
        };

    // When an edited version is saved, reload the homescreen cards
    editScreen.onEditSaved = [this]()
        {
            homeScreen.loadRecordings();
        };

    resized();
}



void MainComponent::saveAccountsToFile() {
    juce::String content;
    for (auto& pair : accounts) {
        content += pair.first + ":" + pair.second.password + ":" + pair.second.role + "\n";
    }
    accountsFile.replaceWithText(content);
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    juce::ColourGradient gradient(juce::Colour::fromRGB(20, 30, 48), 0, 0, juce::Colour::fromRGB(36, 59, 85), 0, getHeight(), false);
    g.setGradientFill(gradient);
    g.fillAll();


    if (isLoggedIn) {
        auto cardArea = getLocalBounds().reduced(80);
        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 30));
        g.fillRoundedRectangle(cardArea.toFloat(), 20.0f);

        //welcoming mah users
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(28.0f));
        g.drawText("Welcome, " + currUsername + "!", cardArea.removeFromTop(60), juce::Justification::centred, true);

        g.setFont(juce::Font(juce::FontOptions(15.0f).withStyle("Italics")));

        g.drawText("Hello", cardArea.removeFromTop(80), juce::Justification::centred, true);
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    int width = 250;
    int height = 30;
    int center = getWidth() / 2 - width / 2;

    if (!isLoggedIn)
    {
        titleLabel.setBounds(center, 80, width, 30);
        usernameEditor.setBounds(center, 130, width, height);
        passwordEditor.setBounds(center, 170, width, height); // new addition per password feature; Barua
        loginButton.setBounds(center, 210, width, height); //changed from 180 to 210 to move down; Barua
        createAccButton.setBounds(center, 250, width, height); //from 220 to 250 to move down; Barua

        logoutButton.setVisible(false);

        titleLabel.setVisible(true);
        usernameEditor.setVisible(true);
        passwordEditor.setVisible(true); //per password feature;
        loginButton.setVisible(true);
        createAccButton.setVisible(true);

        homeScreen.setVisible(false);
    }

    else {
        //logoutButton.setBounds(center, 250, width, height);
        titleLabel.setVisible(false);
        usernameEditor.setVisible(false);
        passwordEditor.setVisible(false); //per password feature;
        loginButton.setVisible(false);
        createAccButton.setVisible(false);

        logoutButton.setBounds(10, 10, 100, 30);
        logoutButton.setVisible(true);

        homeScreen.setBounds(getLocalBounds());
        homeScreen.setVisible(true);
    }
}

void MainComponent::buttonClicked(juce::Button* button) {
    if (button == &loginButton) {
        juce::String username = usernameEditor.getText().trim();
        juce::String password = passwordEditor.getText();

        if (username.isEmpty() || password.isEmpty()) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Login failed", "Enter both username and password. There are 2 boxes brotha");
            return;
        }

        loadAccountsFromFile();

        auto check = accounts.find(username);
        if (check != accounts.end() && check->second.password == password)
        {
            currUsername = username;
            isLoggedIn = true;

            // Determine user role
            juce::String role = check->second.role;
            if (role == "Owner")
            {
                currentRole = UserRole::owner;
            }
            else
            {
                currentRole = UserRole::guest;
            }

            homeScreen.setUsername(currUsername);
            homeScreen.setRole(role);
            homeScreen.loadRecordings();

            repaint();
            resized();
        }

        else {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Login failed", "Invalid username or password.");
        }

        if (check != accounts.end() && check->second.password == password)
        {
            currUsername = username;
            isLoggedIn = true;

            currentRole = (check->second.role == "Owner") ? UserRole::owner : UserRole::guest;

            homeScreen.setUsername(currUsername);
            homeScreen.setRole(check->second.role);

            homeScreen.setRecordButtonVisible(currentRole == UserRole::owner);

            repaint();
            resized();
        }

    }

    if (button == &logoutButton) {
        isLoggedIn = false;
        usernameEditor.clear();
        passwordEditor.clear(); //per password feature, to clear out after logout; Barua
        repaint();
        resized();
    }

    if (button == &createAccButton) {
        juce::String username = usernameEditor.getText().trim();
        juce::String password = passwordEditor.getText();

        // check if user entered only one
        if (username.isEmpty() || password.isEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Unable to create account",
                "Please enter both a username and a password. Is your password as tall as Burj Khalifa and you ignoring it mah frieend"
            );
            return;
        }

        // check if username already exists
        if (accounts.find(username) != accounts.end())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Cannot create account",
                "Username already exists! You are unlucky :("
            );
            return;
        }

        Account acc;
        acc.password = password;
        acc.role = "Owner";
        accounts[username] = acc;
        saveAccountsToFile();

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Account created",
            "Account successfully created! You can now log in."
        );


        usernameEditor.clear();
        passwordEditor.clear();

    }
}