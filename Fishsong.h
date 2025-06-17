#ifndef FISHSONG_H
#define FISHSONG_H

#include <string>
#include <vector>
#include <map>

//================================================================================
// Global Configuration & Utilities
//================================================================================

// Global configuration structure for fishsong parsing
struct FishsongConfig
{
    bool  skip;       // If true, skip processing all events
    int   line;       // Current line/track number being parsed
    float speed;      // Playback speed multiplier
    float volume;     // Global volume [0.0, 1.0]
    int   shift;      // Global semitone shift for all notes
    int   localShift; // Local (per-track) semitone shift
};

// Global instance of the configuration
extern FishsongConfig g_fishsongConfig;

// Helper function for case-insensitive string comparison
int StrCaseCmp(const char* s1, const char* s2);


//================================================================================
// Core Parser Classes
//================================================================================

// Represents a single musical event (a note or a rest).
class CSongEvent
{
public:
    CSongEvent();

    // Parses a single event string (e.g. "c4 q" or "r h") and populates the object.
    bool ProcessSongEvent(const std::string &eventStr);
    void FinalizeSongStructure();
    bool CheckSongCategory(const std::string &category) const;
    void FishsongCopyData(const CSongEvent &other);

    // Accessors & Modifiers
    int GetNoteValue() const;
    int GetDuration() const;
    float GetVolume() const;
    const std::string &GetTitle() const;
    bool IsChordMember() const;
    void SetTitle(const std::string &title);
    void SetDuration(int duration);
    void SetIsChordMember(bool val);

private:
    int ParseDuration(const std::string &durStr);
    static int GetDurationValue(char symbol);

    int         noteValue;
    int         duration;
    float       volume;
    std::string title;
    bool        isChordMember;
};

// Loads, parses, and contains all events from a single fishsong file.
class CFishsongFile
{
public:
    CFishsongFile(const std::string &path);
    ~CFishsongFile();

    bool Load();
    const std::vector<CSongEvent>& GetEvents() const;

private:
    void ProcessCommand(const std::string &cmd);
    void ProcessEventWithChordHandling(const CSongEvent &event);
    void FinalizePendingChord(int defaultDuration);

    std::string              filePath;
    std::vector<CSongEvent>  events;
    std::vector<CSongEvent>  pendingChord;
    int                      currentTrack;
    int                      waitingForTrack;
    std::map<int, int>       trackPositions;
};


//================================================================================
// Manager & Global Functions
//================================================================================

// Manages the playback or scheduling of song events.
class CFishsongManager
{
public:
    CFishsongManager();
    ~CFishsongManager();

    bool UpdateState(int someFlag, int extraParam, int &counterOut, int cmdFlag);
    void DispatchEvent(const CSongEvent &event);
    void FinalizeEventTitle(CSongEvent &event);
    void ResetState();

private:
    std::vector<CSongEvent> validatedEvents;
    int                     updateCounter;
};

// Initializes or resets the global fishsong configuration.
void InitFishsongEvents();

// Clears any global state (if necessary).
void ClearFishsongBuffer();

// Factory function to load a fishsong file. Returns NULL on failure.
CFishsongFile* LoadFishsongFile(const std::string &filePath);

// High-level function to process songs (implementation is application-specific).
void ProcessFishsong(bool force = false);

// Parses a global command string like "speed 9".
void ParseFishSongCommand(const std::string &commandStr);

#endif // FISHSONG_H
