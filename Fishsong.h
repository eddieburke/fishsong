#ifndef FISHSONG_H
#define FISHSONG_H

#include <string>
#include <vector>
#include <map>

// Global configuration structure
struct FishsongConfig {
    bool skip;           // Skip processing flag
    int line;            // Current line/track being processed
    float speed;         // Playback speed
    float volume;        // Default volume
    int shift;           // Global transposition shift
    int localShift;      // Track-specific transposition shift
};

// Global configuration instance
extern FishsongConfig g_fishsongConfig;

// Song event class - represents a single note or rest
class CSongEvent {
public:
    CSongEvent();
    
    // Core functionality
    bool ProcessSongEvent(const std::string &eventStr);
    void FinalizeSongStructure();
    bool CheckSongCategory(const std::string &category) const;
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
    
private:
    int noteValue;       // MIDI-style note value, -1 for rest
    int duration;        // Duration in ticks
    float volume;        // Note volume (0.0 - 1.0)
    std::string title;   // Original note string
    bool isChordMember;  // Whether this note is part of a chord
    
    // Helper for parsing duration strings
    int ParseDuration(const std::string &durStr);
};

// Sorta Placeholder: Loader for fishsong from file
class CFishsongFile {
public:
    CFishsongFile(const std::string &path);
    ~CFishsongFile();
    
    bool Load();
    const std::vector<CSongEvent>& GetEvents() const;
    
private:
    std::string filePath;
    std::vector<CSongEvent> events;
    std::vector<CSongEvent> pendingChord;
    int currentTrack;
    std::map<int, int> trackPositions;
    
    void ProcessCommand(const std::string &cmd);
};

// Fishsong manager class - handles playback and events
class CFishsongManager {
public:
    CFishsongManager();
    ~CFishsongManager();
    
    bool UpdateState(int someFlag, int extraParam, int &counterOut, int cmdFlag);
    void DispatchEvent(const CSongEvent &event);
    void FinalizeEventTitle(CSongEvent &event);
    void ResetState();
    
private:
    std::vector<CSongEvent> validatedEvents;
    int updateCounter;
};

// Global utility functions
void InitFishsongEvents();
void ClearFishsongBuffer();
CFishsongFile* LoadFishsongFile(const std::string &filePath);
void ProcessFishsong(bool force);
void ParseFishSongCommand(const std::string &commandStr);

// Initialize duration map
void InitDurationMap();

#endif // FISHSONG_H
