#ifndef FISHSONG_H
#define FISHSONG_H

#include <string>
#include <vector>
#include <map>

// Forward declaration for older compilers, if needed, though typically not for std types.
// May not be strictly necessary with standard includes.

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

// Helper function for case-insensitive string comparison (often custom in older frameworks)
int strcasecmp(const char* s1, const char* s2); // Declaration moved here for clarity

// CSongEvent: Represents a single note or rest
class CSongEvent
{
public:
    CSongEvent();

    bool ProcessSongEvent(const std::string &eventStr);
    void FinalizeSongStructure();
    bool CheckSongCategory(const std::string &category) const;
    void FishsongCopyData(const CSongEvent &other);

    int GetNoteValue() const;
    int GetDuration() const;
    float GetVolume() const;
    const std::string &GetTitle() const;
    bool IsChordMember() const;

    void SetTitle(const std::string &title);
    void SetDuration(int duration);
    void SetIsChordMember(bool val); // Added for consistent chord member finalization

private:
    int ParseDuration(const std::string &durStr);
    static int GetDurationValue(char symbol); // Static as it doesn't depend on instance state

private:
    int         noteValue;
    int         duration;
    float       volume;
    std::string title;
    bool        isChordMember;
};

// CFishsongFile: Loads and parses a "fishsong" text file
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

private:
    std::string              filePath;
    std::vector<CSongEvent>  events;
    std::vector<CSongEvent>  pendingChord;

    int                      currentTrack;
    int                      waitingForTrack;
    std::map<int, int>       trackPositions;
};

// CFishsongManager: Manages playback or scheduling of events
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

// ~~~~ Global Utility Functions ~~~~

void InitFishsongEvents();
void ClearFishsongBuffer();
CFishsongFile* LoadFishsongFile(const std::string &filePath); // Returns NULL on failure
void ProcessFishsong(bool force = false); // Default argument is fine in C++03
void ParseFishSongCommand(const std::string &commandStr);

// Removed: void InitDurationMap(); // As it was unused

#endif // FISHSONG_H
