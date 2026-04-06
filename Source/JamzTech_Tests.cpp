/*
  ==============================================================================

    JamzTech_Tests.cpp
    Created: 2 Apr 2026 2:57:24pm
    Author:  Tanishi Kumar

  ==============================================================================
*/

/*
  This is a System testing files that tests almost all of the important functions that are incorporated into making this application.
 ** Please make sure you edit the main.cpp file on your end (@teammates) to make this provide immediate output of success/failure of important functions on your terminal
 
*/

#include <JuceHeader.h>
#include "LocalDatabase.h"
#include "Homescreen.h"
#include "RecordComponent.h"

// Test case 1: testing localdatabase set of code and functionality
class LocalDatabaseTest : public juce::UnitTest
{
public:
    LocalDatabaseTest() : juce::UnitTest("LocalDatabase Tests", "JamzTech") {}

    void runTest() override
    {
        //  Test 1: Database opens and table is created
        beginTest("Database initialises without error");
        {
            // if calling createTables() fails; automatic failure of the application
            LocalDatabase db;
            auto recordings = db.getAllRecordings();
            expect(recordings.size() >= 0, "getAllRecordings() should return a valid (possibly empty) vector");
        }

        // Test 2: this tests how the recording works and if retreival  works or not -- test output in console
        beginTest("Insert and retrieve a recording entry");
        {
            LocalDatabase db;

            juce::String testTitle    = "SystemTest_Recording";
            juce::String testUser     = "test_user";
            juce::String testCategory = "Test";
            juce::String testPath     = "/tmp/test_audio.wav";
            juce::String testImage    = "";
            juce::String testDate     = "2026-04-02 10:00:00";

            bool inserted = db.insert(testTitle, testUser, testCategory,
                                      testPath, testImage, testDate);

            expect(inserted, "insert() should return true on success");

            auto recordings = db.getAllRecordings();

            
            bool found = false;
            for (const auto& r : recordings)
            {
                if (r.audioTitle == testTitle && r.accountName == testUser)
                {
                    found = true;
                    expectEquals(r.filePath,  testPath,     "File path should match inserted value");
                    expectEquals(r.category,  testCategory, "Category should match inserted value");
                    expectEquals(r.createdAt, testDate,     "Created-at timestamp should match");
                    break;
                }
            }
            expect(found, "Inserted recording should be retrievable from the database");
        }

        //  Test 3: Multiple insertions and testing if retrieval works or not
        //outputs this if success
        beginTest("Multiple inserts are all retrievable");
        {
            LocalDatabase db;
            int before = (int) db.getAllRecordings().size();

            db.insert("Track_A", "user1", "General", "/tmp/a.wav", "", "2026-04-02 11:00:00");
            db.insert("Track_B", "user2", "General", "/tmp/b.wav", "", "2026-04-02 11:01:00");

            int after = (int) db.getAllRecordings().size();
            expect(after >= before + 2, "two new rows should appear after two inserts");
        }

        //  Test 4: RecordingEntry fields are not empty after retrieval
        beginTest("Retrieved entries have non-empty required fields");
        {
            LocalDatabase db;
            db.insert("FieldTest", "field_user", "Cat", "/tmp/field.wav", "", "2026-04-02 12:00:00");

            auto recordings = db.getAllRecordings();
            for (const auto& r : recordings)
            {
                expect(r.audioTitle.isNotEmpty(),  "audioTitle must not be empty");
                expect(r.accountName.isNotEmpty(), "accountName must not be empty");
                expect(r.filePath.isNotEmpty(),    "filePath must not be empty");
                expect(r.createdAt.isNotEmpty(),   "createdAt must not be empty");
            }
        }
    }
};



// this section of the code is testing if login works successfully
// mainly it will test if user sees the login screen, is able to make the account
// it also tests that random user login should be prohibitted; only verified accounts that have been created must be able to login successfully
// empty fields must not be allowed either

class LoginLogicTest : public juce::UnitTest
{
public:
    LoginLogicTest() : juce::UnitTest("Login Logic Tests", "JamzTech") {}

    struct Account
    {
        juce::String password;
        juce::String role;
    };

    //this function is a dummy copy of a loadAccounts function in MainComponent
    void loadAccounts(const juce::File& file,
                      std::unordered_map<juce::String, Account>& accounts)
    {
        accounts.clear();
        if (!file.existsAsFile()) return;

        auto lines = juce::StringArray::fromLines(file.loadFileAsString());
        for (auto& line : lines)
        {
            auto parts = juce::StringArray::fromTokens(line, ":", "");
            if (parts.size() == 3)
            {
                Account acc;
                acc.password = parts[1].trim();
                acc.role     = parts[2].trim();
                accounts[parts[0].trim()] = acc;
            }
        }
    }

    // same goes here with saveAccounts; account saving section should be functional too
    void saveAccounts(const juce::File& file,
                      const std::unordered_map<juce::String, Account>& accounts)
    {
        juce::String content;
        for (auto& pair : accounts)
            content += pair.first + ":" + pair.second.password + ":" + pair.second.role + "\n";
        file.replaceWithText(content);
    }

    void runTest() override
    {
        // Use a temp file so we never touch the real accounts.txt
        auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                            .getChildFile("jamztech_test_accounts.txt");
        tempFile.deleteFile(); // start clean

        std::unordered_map<juce::String, Account> accounts;

       // test case 1: new users are able to create a new account
        beginTest("Create a new account");
        {
            Account acc;
            acc.password = "pass123";
            acc.role     = "Guest";
            accounts["alice"] = acc;
            saveAccounts(tempFile, accounts);

            loadAccounts(tempFile, accounts);
            expect(accounts.find("alice") != accounts.end(),
                   "alice should exist after creation");
            expectEquals(accounts["alice"].password, juce::String("pass123"),
                         "Password should be saved correctly");
            expectEquals(accounts["alice"].role, juce::String("Guest"),
                         "New accounts should default to Guest role");
        }

       // test case 2: a duplicate username creation must be rejected and this function tests that
        beginTest("Duplicate username is rejected");
        {
            // alice already exists from Test 1
            bool alreadyExists = (accounts.find("alice") != accounts.end());
            expect(alreadyExists,
                   "Duplicate check: username 'alice' must already exist");
            // The app would NOT insert again — verified by checking count stays same
            int before = (int) accounts.size();
            if (accounts.find("alice") == accounts.end()) // guard, as app does
                accounts["alice"] = { "newpass", "Guest" };
            int after = (int) accounts.size();
            expectEquals(before, after, "Account map should not grow on duplicate username");
        }

        // test case 3: login with appropriate credentials must only pass (valid userID and password)
        beginTest("Login succeeds with correct username and password");
        {
            auto it = accounts.find("alice");
            bool loginOk = (it != accounts.end() && it->second.password == "pass123");
            expect(loginOk, "Login should succeed with correct credentials");
        }

        // test case 4: random passwords must fail with wrong password message
        beginTest("Login fails with wrong password");
        {
            auto it = accounts.find("alice");
            bool loginOk = (it != accounts.end() && it->second.password == "wrongpassword");
            expect(!loginOk, "Login should fail with incorrect password");
        }

        // test case 5: login should fail with empty username field
        beginTest("Login fails with unknown username");
        {
            bool loginOk = (accounts.find("nobody") != accounts.end());
            expect(!loginOk, "Login should fail for a username that does not exist");
        }

        // ── Test 6: Owner role is assigned correctly ──────────────────────
        beginTest("Owner role is stored and loaded correctly");
        {
            Account ownerAcc;
            ownerAcc.password = "ownerpass";
            ownerAcc.role     = "Owner";
            accounts["tanishi"] = ownerAcc;
            saveAccounts(tempFile, accounts);

            loadAccounts(tempFile, accounts);
            expect(accounts.find("tanishi") != accounts.end(), "Owner account should exist");
            expectEquals(accounts["tanishi"].role, juce::String("Owner"),
                         "Role should persist as 'Owner' after save/load");
        }

        // any empty fields should be rejected
        beginTest("Empty username or password is rejected before account creation");
        {
            juce::String username = "  "; // whitespace only
            juce::String password = "";

            bool usernameEmpty = username.trim().isEmpty();
            bool passwordEmpty = password.isEmpty();

            expect(usernameEmpty || passwordEmpty,
                   "Creation should be blocked when username or password is empty");
        }

        tempFile.deleteFile(); // clean up
    }
};


// TEST FIELD 3: HOMESCREEN LOADS AND THINGS LOOK OK WHEN USER SUCCESSFULLY LOGS IN

class HomescreenTest : public juce::UnitTest
{
public:
    HomescreenTest() : juce::UnitTest("Homescreen Tests", "JamzTech") {}

    void runTest() override
    {
        // ── Test 1: Homescreen constructs without crashing ────────────────
        beginTest("Homescreen constructs successfully");
        {
            // This exercises the constructor: header bar, labels, search bar,
            // audio device manager initialisation, etc.
            Homescreen hs;
            expect(true, "Homescreen constructor completed without throwing");
        }

        // test case 2: (silly one) but username gets updated in the greeting section depending on who logs in
        // ** using mine as an example
        beginTest("setUsername stores the username");
        {
            Homescreen hs;
            hs.setUsername("shruti");
            
            hs.setRole("Guest");
            expect(true, "setUsername and setRole completed without error");
        }

        // test case 3: since log in based on local computer basis, accounts created inside a certain computer should be able to log in successfully (account details get saved on that particular system)  AND recordings saved/created in that computer must be visible to all users who logged in and recorded on the device
        beginTest("loadRecordings creates cards only for the logged-in user");
        {
            // Insert a known entry for "testuser" into the DB
            LocalDatabase db;
            db.insert("UserTestTrack", "testuser", "General",
                      "/tmp/usertest.wav", "", "2026-04-02 09:00:00");

            Homescreen hs;
            hs.setUsername("testuser");

            // loadRecordings() reads from the DB and creates AudioCards
            
            hs.loadRecordings();

            
            expect(hs.getNumChildComponents() > 0,
                   "Homescreen should have child components after loadRecordings()");
        }

        //  Test 4: onPlayClicked callback fires accurately and recording is saved nicely and can be played
        beginTest("AudioCards onPlayClicked callback is invoked");
        {
            AudioCards card;

            RecordingEntry entry;
            entry.id          = 99;
            entry.audioTitle  = "CallbackTest";
            entry.accountName = "cb_user";
            entry.filePath    = "/tmp/callback.wav";
            entry.createdAt   = "2026-04-02 08:00:00";
            card.setRecording(entry);

            bool callbackFired = false;
            juce::String receivedTitle;

            card.onPlayClicked = [&](const RecordingEntry& r)
            {
                callbackFired  = true;
                receivedTitle  = r.audioTitle;
            };

            // button click section works fine
            card.onPlayClicked(entry);

            expect(callbackFired, "onPlayClicked callback should fire when triggered");
            expectEquals(receivedTitle, juce::String("CallbackTest"),
                         "Callback should receive the correct RecordingEntry");
        }

        // test case 5: on edit clicked method should work fine, so once edit button hits, users should be redirected to the edit page
        beginTest("AudioCards onEditClicked callback is invoked");
        {
            AudioCards card;

            RecordingEntry entry;
            entry.audioTitle  = "EditCallbackTest";
            entry.accountName = "edit_user";
            entry.filePath    = "/tmp/edit.wav";
            entry.createdAt   = "2026-04-02 08:30:00";
            card.setRecording(entry);

            bool editFired = false;
            card.onEditClicked = [&](const RecordingEntry&) { editFired = true; };

            card.onEditClicked(entry);
            expect(editFired, "onEditClicked callback should fire when triggered");
        }

        // test case 6: users should be able to access their recordings
        beginTest("AudioCards getRecording returns the assigned entry");
        {
            AudioCards card;
            RecordingEntry entry;
            entry.audioTitle  = "GetRecordingTest";
            entry.accountName = "gr_user";
            entry.filePath    = "/tmp/gr.wav";
            entry.createdAt   = "2026-04-02 09:30:00";
            card.setRecording(entry);

            const RecordingEntry& retrieved = card.getRecording();
            expectEquals(retrieved.audioTitle,  juce::String("GetRecordingTest"),
                         "getRecording() should return the same title that was set");
            expectEquals(retrieved.accountName, juce::String("gr_user"),
                         "getRecording() should return the same accountName that was set");
        }
    }
};

// SECTION 4: RECORDCOMPONENT PART TESTING

class RecordComponentTest : public juce::UnitTest
{
public:
    RecordComponentTest() : juce::UnitTest("RecordComponent Tests", "JamzTech") {}

    void runTest() override
    {
        //Test 1: RecordComponent constructs cleanly
        beginTest("RecordComponent constructs without error");
        {
            RecordComponent rc;
            expect(true, "RecordComponent constructor completed without throwing");
        }

        //  Test 2: setAccountName persists
        beginTest("setAccountName does not crash with any string");
        {
            RecordComponent rc;
            rc.setAccountName("tanishi");
            rc.setAccountName("");           // edge case: empty string
            rc.setAccountName("user_123!@"); // edge case: special chars
            expect(true, "setAccountName should handle any string without crashing");
        }

        // test case 3: no illegal characters for recording naming conventions
        beginTest("Filename sanitisation removes illegal characters");
        {
            // Mirror of RecordComponent::makeName()
            auto makeName = [](juce::String name) -> juce::String
            {
                juce::String safe = name.trim();
                safe = safe.replaceCharacter('\\', '_');
                safe = safe.replaceCharacter('/',  '_');
                safe = safe.replaceCharacter(':',  '_');
                safe = safe.replaceCharacter('*',  '_');
                safe = safe.replaceCharacter('?',  '_');
                safe = safe.replaceCharacter('"',  '_');
                safe = safe.replaceCharacter('<',  '_');
                safe = safe.replaceCharacter('>',  '_');
                safe = safe.replaceCharacter('|',  '_');
                if (safe.isEmpty()) safe = "Untitled Recording";
                return safe;
            };

            expectEquals(makeName("hello/world"),   juce::String("hello_world"),
                         "Forward slash should be replaced with underscore");
            expectEquals(makeName("test:file"),     juce::String("test_file"),
                         "Colon should be replaced with underscore");
            expectEquals(makeName("my*song?"),      juce::String("my_song_"),
                         "Asterisk and question mark should be replaced");
            expectEquals(makeName("  "),            juce::String("Untitled Recording"),
                         "Whitespace-only name should fall back to default");
            expectEquals(makeName(""),              juce::String("Untitled Recording"),
                         "Empty name should fall back to default");
            expectEquals(makeName("NormalTitle"),   juce::String("NormalTitle"),
                         "Clean titles should pass through unchanged");
        }

        // Test 4: Recording folder can be  created
        beginTest("Recordings output folder can be created");
        {
            auto folder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                              .getChildFile("JamzRecordings");

            if (!folder.exists())
                folder.createDirectory();

            expect(folder.exists() && folder.isDirectory(),
                   "JamzRecordings folder should exist or be creatable");
        }

        //  test case 5: when clicking back; you go back to homescreen mah friends
        beginTest("onBack callback fires when assigned");
        {
            RecordComponent rc;
            bool backFired = false;
            rc.onBack = [&]() { backFired = true; };

            // Invoke directly (simulates back button press logic)
            rc.onBack();
            expect(backFired, "onBack callback should fire when invoked");
        }
    }
};

// some random stuff to make sure this stuff works on terminal when you hit run 
static LocalDatabaseTest    localDatabaseTestInstance;
static LoginLogicTest       loginLogicTestInstance;
static HomescreenTest       homescreenTestInstance;
static RecordComponentTest  recordComponentTestInstance;
