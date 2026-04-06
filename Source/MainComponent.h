#pragma once

#include <JuceHeader.h>
#include <unordered_map> // stores key -> value pairs, for storing user login
#include "Homescreen.h"
#include "EditComponent.h"
#include "RecordComponent.h"
#include "LocalDatabase.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::Component,
    public juce::Button::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;

    enum class UserRole
    {
        guest,
        owner
    };
    UserRole currentRole = UserRole::guest;

private:
    //==============================================================================
    // Your private member variables go here...
    bool isLoggedIn = false;
    juce::String currUsername;

    //variables
    juce::Label titleLabel;
    juce::TextEditor usernameEditor;
    juce::TextEditor passwordEditor; // NEW MODIFICATION for password input; Barua
    juce::TextButton loginButton{ "Login" };
    juce::TextButton createAccButton{ "Create New Account" };
    juce::TextButton logoutButton{ "Logout" };

    // saving user login
    struct Account
    {
        juce::String password;
        juce::String role;
    };
    std::unordered_map<juce::String, Account> accounts;
    juce::File accountsFile{ juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("accounts.txt") };
    void loadAccountsFromFile();
    void saveAccountsToFile();

    // different pages
    Homescreen homeScreen;
    RecordComponent recordScreen;
    EditComponent editScreen;


    void showHome();
    void showEdit(const RecordingEntry& entry);
    void showRecord();
    void showLogin();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};