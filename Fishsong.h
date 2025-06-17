#ifndef FISHSONG_H
#define FISHSONG_H

#include <string>
#include <vector>
#include <map>

// Global configuration structure
struct FishsongConfig
{
    bool  skip;       // If true, skip processing events
    int   line;       // Current line/track number
    float speed;      // Playback speed
    float volume;     // Global volume [0.0, 1.0]
    int   shift;      // Global semitone shift
    int   localShift; // Local (per-track) semitone shift
};

// Global config instance
extern FishsongConfig g_fishsongConfig;

// Helper function for case-insensitive string comparison
int StrCaseCmp(const char* s1, const char* s2);

// CSongEvent: Represents a single note or rest
class CSongEvent
{
public:
    CSongEvent();

    // Attempts to parse one "song event" string (e.g. "c4 q" or "r h")
    // Returns true on success, false on parse error
    bool ProcessSongEvent(const std::string &eventStr);

    // Final adjustments (e.g., minimum duration if needed)
    void FinalizeSongStructure();

    // Checks if this event qualifies for a user-defined "category"
    bool CheckSongCategory(const std::string &category) const;

    // Copy volume/duration from another event (used for chord members)
    void FishsongCopyData(const CSongEvent &other);

    // Accessors
    int GetNoteValue() const;
    int GetDuration() const;
    float GetVolume() const;
    const std::string &GetTitle() const;
    bool IsChordMember() const;

    // Modifiers
    void SetTitle(const std::string &title);
    void SetDuration(int duration);
    void SetIsChordMember(bool val); // To mark chord members as finalized

private:
    // Helper for parsing complex duration strings (e.g., "q+e", "st", "qd")
    int ParseDuration(const std::string &durStr);

    // Helper that returns the base duration (in ticks) for a note symbol ('q','h','e', etc.)
    static int GetDurationValue(char symbol);

private:
    int         noteValue;    // e.g., 60 = C4 in MIDI terms, -1 = rest
    int         duration;     // length in ticks
    float       volume;       // 0.0 to 1.0
    std::string title;        // original token (for logging, etc.)
    bool        isChordMember;// True if this note is part of a chord awaiting its main note
};

// CFishsongFile: Loads and parses a "fishsong" text file
class CFishsongFile
{
public:
    CFishsongFile(const std::string &path);
    ~CFishsongFile();

    // Attempt to open and parse the file
    bool Load();

    // Returns the list of parsed song events
    const std::vector<CSongEvent>& GetEvents() const;

private:
    // Internal subroutines used during Load()
    void ProcessCommand(const std::string &cmd);
    void ProcessEventWithChordHandling(const CSongEvent &event);
    void FinalizePendingChord(int defaultDuration);

private:
    std::string              filePath;
    std::vector<CSongEvent>  events;         // Completed events
    std::vector<CSongEvent>  pendingChord;   // Temporarily holds chord members

    int                      currentTrack;    // Which track/line we’re parsing
    int                      waitingForTrack; // Track we must wait on for synchronization
    std::map<int, int>       trackPositions;  // For each track, how many ticks have elapsed
};

// CFishsongManager: Manages playback or scheduling of events
class CFishsongManager
{
public:
    CFishsongManager();
    ~CFishsongManager();

    // Example usage: updates the manager state; returns true on success
    bool UpdateState(int someFlag, int extraParam, int &counterOut, int cmdFlag);

    // Enqueue or "dispatch" an event for playback
    void DispatchEvent(const CSongEvent &event);

    // Post-parse adjustments to event titles or categories
    void FinalizeEventTitle(CSongEvent &event);

    // Reset internal state/counter
    void ResetState();

private:
    std::vector<CSongEvent> validatedEvents;  // Might store or queue events
    int                     updateCounter;    // Simple counter for state updates
};


// ~~~~ Global Utility Functions ~~~~

// Initialize fishsong events and reset global config
void InitFishsongEvents();

// Clear internal buffers/data if needed
void ClearFishsongBuffer();

// Load a fishsong file, returning a pointer to the new CFishsongFile object. Returns NULL on failure
CFishsongFile* LoadFishsongFile(const std::string &filePath);

// Process all fishsong files in a folder (stubbed out in code, probably something similar in the actual game)
void ProcessFishsong(bool force = false);

// Parse a fishsong command, e.g. "*speed 1.5", "*shift -2", etc.
void ParseFishSongCommand(const std::string &commandStr);

#endif // FISHSONG_H
