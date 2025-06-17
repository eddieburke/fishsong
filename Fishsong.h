#ifndef FISHSONG_H
#define FISHSONG_H

#include <string>
#include <vector>
#include <map>

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

// Represents a single musical event (a note or a rest).
class CSongEvent
{
public:
    CSongEvent();

    // Parses a single event string (e.g. "c4 q" or "r h") and populates the object.
    // Returns true on success, false on failure.
    bool ProcessSongEvent(const std::string &eventStr);

    // Performs final adjustments, like ensuring a minimum duration.
    void FinalizeSongStructure();

    // Checks if the event matches a specific category (e.g., "long", "beethoven").
    bool CheckSongCategory(const std::string &category) const;

    // Copies duration and volume from another event, used to finalize chord members.
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
    void SetIsChordMember(bool val);

private:
    // Helper to parse complex duration strings (e.g., "q+e", "st", "qd", "120").
    int ParseDuration(const std::string &durStr);

    // Helper to get the base duration in ticks for a note symbol ('q','h','e', etc.).
    static int GetDurationValue(char symbol);

private:
    int         noteValue;      // MIDI-like note value (60 = C4), or -1 for a rest
    int         duration;       // Duration in abstract ticks
    float       volume;         // Volume from 0.0 to 1.0
    std::string title;          // The original note token (e.g., "c#5") for logging
    bool        isChordMember;  // True if this note is part of a chord awaiting finalization
};

// Loads, parses, and contains all events from a single fishsong file.
class CFishsongFile
{
public:
    CFishsongFile(const std::string &path);
    ~CFishsongFile();

    // Opens and parses the entire file, populating the event list.
    bool Load();

    // Returns the final list of parsed song events.
    const std::vector<CSongEvent>& GetEvents() const;

private:
    // Processes a command line (e.g., "*speed 9").
    void ProcessCommand(const std::string &cmd);
    
    // Handles event logic, including chord finalization.
    void ProcessEventWithChordHandling(const CSongEvent &event);
    
    // Finalizes any pending chord at the end of a track or file.
    void FinalizePendingChord(int defaultDuration);

private:
    std::string              filePath;
    std::vector<CSongEvent>  events;
    std::vector<CSongEvent>  pendingChord;

    int                      currentTrack;
    int                      waitingForTrack; // If > 0, track to sync with
    std::map<int, int>       trackPositions;  // Elapsed ticks for each track to handle synchronization
};

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

// Global Utility Functions
void InitFishsongEvents();
void ClearFishsongBuffer();
CFishsongFile* LoadFishsongFile(const std::string &filePath);
void ProcessFishsong(bool force = false);
void ParseFishSongCommand(const std::string &commandStr);

#endif // FISHSONG_H
