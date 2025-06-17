#ifndef FISHSONG_H
#define FISHSONG_H

#include <string>
#include <vector>
#include <map>

//=============================================================================
// Global configuration  ------------------------------------------------------
//=============================================================================
struct FishsongConfig
{
    bool  skip;        // *skip* flag from “*skip true/false”
    int   line;        // current track/line (1-based)
    float speed;       // playback speed multiplier
    float volume;      // 0.0 – 1.0
    int   shift;       // global semitone shift
    int   localShift;  // per-track semitone shift
};

// global instance
extern FishsongConfig g_fishsongConfig;

// helper
int  StrCaseCmp(const char* s1,const char* s2);

//=============================================================================
// CSongEvent  -----------------------------------------------------------------
//=============================================================================
class CSongEvent
{
public:
    CSongEvent();

    // parses token (returns false if invalid)
    bool        ProcessSongEvent(const std::string& token);

    // massage data after token parsed
    void        FinalizeSongStructure();

    // category helpers
    bool        CheckSongCategory(const std::string& c) const;

    // copy note/vol/dur from root to chord member
    void        FishsongCopyData(const CSongEvent& root);

    // accessors
    int                 GetNoteValue()  const;
    int                 GetDuration()   const;
    float               GetVolume()     const;
    const std::string&  GetTitle()      const;
    bool                IsChordMember() const;

    // mutators
    void        SetTitle(const std::string& t);
    void        SetDuration(int d);
    void        SetIsChordMember(bool v);

private:
    static int  GetDurationValue(char sym);   // w h q e s t z
    int         ParseDuration(const std::string& str);

private:
    int         noteValue;      // MIDI (-1 == rest)
    int         duration;       // ticks (1/480 note)
    float       volume;         // 0-1
    std::string title;          // raw token
    bool        isChordMember;  // true if duration token == “0”
};

//=============================================================================
// CFishsongFile  --------------------------------------------------------------
//=============================================================================
class CFishsongFile
{
public:
    explicit CFishsongFile(const std::string& path);
    ~CFishsongFile();

    bool                    Load();             // parse file
    const std::vector<CSongEvent>& GetEvents() const;

private:
    // helpers
    void    ProcessCommand(const std::string& cmdLine);
    void    ProcessEventWithChordHandling(const CSongEvent& ev);
    void    FinalizePendingChord(int rootDuration);

private:
    std::string                 mPath;
    std::vector<CSongEvent>     mEvents;
    std::vector<CSongEvent>     mPendingChord;

    int                         mCurrentTrack;      // 1-based
    int                         mWaitingForTrack;   // >0 if “*rest n” active
    std::map<int,int>           mTrackPos;          // track → cumulative ticks
};

//=============================================================================
// CFishsongManager  -----------------------------------------------------------
//=============================================================================
class CFishsongManager
{
public:
    CFishsongManager();
    ~CFishsongManager();

    bool    UpdateState(int flag,int extra,int& counterOut,int cmdFlag);
    void    DispatchEvent(const CSongEvent& ev);
    void    FinalizeEventTitle(CSongEvent& ev);
    void    ResetState();

private:
    std::vector<CSongEvent> mValidated;
    int                     mUpdateCounter;
};

//=============================================================================
// Global convenience fns  -----------------------------------------------------
//=============================================================================
void            InitFishsongEvents();
void            ClearFishsongBuffer();
CFishsongFile*  LoadFishsongFile(const std::string& path);
void            ProcessFishsong(bool force = false);
void            ParseFishSongCommand(const std::string& cmd);

#endif // FISHSONG_H
