#pragma once

//=============================================================================
// INCLUDES & FORWARD DECLARATIONS
//=============================================================================
#include <string>
#include <vector>
#include <map>

// Forward declarations to resolve circular dependencies if they were in separate files
class Fish;
class GameBoard;

//=============================================================================
// ENUMS & STRUCTS
//=============================================================================

// Enum for the special fish types based on assembly analysis
enum SpecialFishType {
    FISH_TYPE_NORMAL = -1,
    FISH_TYPE_BEETHOVEN = 1,
    FISH_TYPE_KILGORE = 4,
    FISH_TYPE_SANTA = 5,
    FISH_TYPE_SINGING_FISH = 10 // A unique ID for '1SingingFish'
};

// Placeholder for a color structure
struct Color {
    int r, g, b, a;
    Color(int _r = 255, int _g = 255, int _b = 255, int _a = 255);
};

// Global configuration structure
struct FishsongConfig {
    bool  skip;
    int   line;
    float speed;
    float volume;
    int   shift;
    int   localShift;
};

//=============================================================================
// CLASS DECLARATIONS
//=============================================================================

// CSongEvent: Represents a single note or rest
class CSongEvent {
public:
    CSongEvent();
    bool ProcessSongEvent(const std::string& token);
    void FinalizeSongStructure();
    void FishsongCopyData(const CSongEvent& root);

    // Accessors
    int GetNoteValue() const;
    int GetDuration() const;
    const std::string& GetTitle() const;
    bool IsChordMember() const;

    // Mutators
    void SetDuration(int d);
    void SetIsChordMember(bool v);

private:
    static int GetDurationValue(char sym);
    int ParseDuration(const std::string& str);

    int noteValue;
    int duration;
    float volume;
    std::string title;
    bool isChordMember;
};

// CFishsongFile: Loads and parses a "fishsong" text file
class CFishsongFile {
public:
    explicit CFishsongFile(const std::string& path);
    ~CFishsongFile();
    bool Load();
    const std::vector<CSongEvent>& GetEvents() const;
    const std::string& GetPath() const;

private:
    void ProcessCommand(const std::string& cmdLine);
    void ProcessEventWithChordHandling(const CSongEvent& ev);
    void FinalizePendingChord(int rootDuration);

    std::string mPath;
    std::vector<CSongEvent> mEvents;
    std::vector<CSongEvent> mPendingChord;
    int mCurrentTrack;
    int mWaitingForTrack;
    std::map<int, int> mTrackPos;
};

// Represents a single fish in the game
class Fish {
public:
    std::string mName;
    bool mIsSpecial;
    SpecialFishType mSpecialType;
    bool mHasCostume;
    Color mCostumeColor1, mCostumeColor2, mCostumeColor3;

    Fish(const std::string& name);
    void Sing(); // Method to make the fish play a song based on its type
};

// Manages the game state, including the list of all fish
class GameBoard {
public:
    std::vector<Fish*> mFishList;

    ~GameBoard();
    void AddFish(Fish* fish);
    void CheckAndApplySpecialFishLogic(Fish* theFish); // C++ version of FUN_0054cc70
};

//=============================================================================
// GLOBAL FUNCTION & VARIABLE DECLARATIONS
//=============================================================================

// Global config instance, defined in the .cpp file
extern FishsongConfig g_fishsongConfig;

// Main function to load and categorize all songs from the "fishsongs" directory
void InitFishsongEvents(bool force = true);

// Frees all memory used by the song lists
void ClearAllFishsongData();

// Helper for case-insensitive string comparison
int StrCaseCmp(const char* a, const char* b);
