#include "LocalDatabase.h"

// Include the sqlite3 implementation header only in the .cpp to avoid
// increasing include depth for files that include LocalDatabase.h.
extern "C" {
    #include "./sqlite/sqlite3.h"
}

LocalDatabase::LocalDatabase()
{
	auto appFolder = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("JamzTech");

	if (!appFolder.exists())
	{
		appFolder.createDirectory();
	}

	dbFile = appFolder.getChildFile("recordings.db");

	if (open(dbFile.getFullPathName()))
	{
		createTables();
	}
	
	else
	{
		DBG("Failed to open database at: " + dbFile.getFullPathName());
	}
}

LocalDatabase::~LocalDatabase()
{
	if (db != nullptr)
	{
		sqlite3_close(db);
	}
}

bool LocalDatabase::open(const juce::String& path)
{
	if (sqlite3_open(path.toRawUTF8(), &db) != SQLITE_OK)
	{
		db = nullptr;
		return false;
	}
	
	DBG("Opened database at: " + path);
	return true;
}

bool LocalDatabase::createTables()
{
	const char* sql = "CREATE TABLE IF NOT EXISTS recordings ("
				  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
				  "audio_title TEXT NOT NULL,"
				  "account_name TEXT NOT NULL,"
				  "category TEXT,"
				  "file_path TEXT NOT NULL,"
				  "image_path TEXT,"
				  "created_at TEXT NOT NULL);";
	char* errMsg = nullptr;
	int result = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
	
	if (result != SQLITE_OK)
	{
		DBG("Error creating tables: " + juce::String(errMsg));
		sqlite3_free(errMsg);
		return false;
	}

	DBG("Tables created successfully.");
	return true;
}

bool LocalDatabase::insert(const juce::String& audioTitle,
						const juce::String& accountName,
						const juce::String& category,
						const juce::String& filePath,
						const juce::String& imagePath,
						const juce::String& createdAt)                        
{
	if (db == nullptr)
	{
		DBG("Database not open.");
		return false;
	}

	const char* sql = "INSERT INTO recordings (audio_title, account_name, category, file_path, image_path, created_at) "
		"VALUES (?, ?, ?, ?, ?, ?);";

	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		DBG("Error preparing statement: " + juce::String(sqlite3_errmsg(db)));
		return false;
	}
	
	sqlite3_bind_text(stmt, 1, audioTitle.toRawUTF8(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, accountName.toRawUTF8(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, category.toRawUTF8(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, filePath.toRawUTF8(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 5, imagePath.toRawUTF8(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 6, createdAt.toRawUTF8(), -1, SQLITE_TRANSIENT);

	if (sqlite3_step(stmt) != SQLITE_DONE)
	{
		DBG("Error inserting data: " + juce::String(sqlite3_errmsg(db)));
		sqlite3_finalize(stmt);
		return false;
	}

	sqlite3_finalize(stmt);
	DBG("Data inserted successfully.");
	return true;
}

std::vector<RecordingEntry> LocalDatabase::getAllRecordings()
{
	std::vector<RecordingEntry> recordings;

	if (db == nullptr)
	{
		DBG("Database not open.");
		return recordings;
	}

	const char* sql = "SELECT id, audio_title, account_name, category, file_path, image_path, created_at FROM recordings;";
	
	sqlite3_stmt* stmt = nullptr;
	
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		DBG("Error preparing statement: " + juce::String(sqlite3_errmsg(db)));
		return recordings;
	}

	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		RecordingEntry entry;
		entry.id = sqlite3_column_int(stmt, 0);
		entry.audioTitle = juce::String((const char*)sqlite3_column_text(stmt, 1));
		entry.accountName = juce::String((const char*)sqlite3_column_text(stmt, 2));
		entry.category = juce::String((const char*)sqlite3_column_text(stmt, 3));
		entry.filePath = juce::String((const char*)sqlite3_column_text(stmt, 4));
		
		const unsigned char* imageText = sqlite3_column_text(stmt, 5);
		const char* imagePathCStr = imageText ? (const char*)imageText : "";

		entry.imagePath = juce::String((const char*)sqlite3_column_text(stmt, 5));
		entry.createdAt = juce::String((const char*)sqlite3_column_text(stmt, 6));
		recordings.push_back(entry);
	}
	sqlite3_finalize(stmt);
	return recordings;
}
