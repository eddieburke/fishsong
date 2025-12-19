#ifndef __FISHSONG_H__
#define __FISHSONG_H__

#include <string>
#include <vector>
#include <map>

//=============================================================================
// FORWARD DECLARATIONS
//=============================================================================
class Fish;
class GameApp;

//=============================================================================
// DEFINITIONS
//=============================================================================
#define SONG_TICK_QUARTER 48

struct FishsongConfig 
{
    bool    mSkip;
    int     mLine;
    float   mSpeed;
    float   mVolume; // Now treated as an array in practice for tracks, but config holds base
    int     mShift;
    int     mLocalShift;
};

extern FishsongConfig g_fishsongConfig;

//=============================================================================
// CSongEvent
// Size: 24 bytes (0x18)
//=============================================================================
class CSongEvent 
{
public:
    float   mNote;          // Offset 0x00
    float   mUnusedPad;     // Offset 0x04 (Padding/Unused)
    double  mVolume;        // Offset 0x08 (8 bytes)
    int     mDuration;      // Offset 0x10
    int     mFlags;         // Offset 0x14 (Padding to 0x18)

public:
    CSongEvent();
    void    Parse(const char* theString);
};

//=============================================================================
// CFishsongFile
//=============================================================================
class CFishsongFile 
{
public:
    std::string             mPath;
    
    // Decompilation implies a vector of events. 
    // In the binary, tracks might be interleaved or stored in a way 
    // that 'mCurrentTrack' indexes into specific event lists.
    // For this recreation, we map TrackID -> Vector.
    std::map<int, std::vector<CSongEvent> > mTracks;

    // Configuration State per file load
    float   mTrackVolumes[16]; // param_1 + 0x30 array
    int     mTrackShifts[16];  // param_1 + 0x44 array
    int     mCurrentTrack;     // param_1 + 0x54
    float   mSpeed;            // param_1 + 0x3c
    int     mGlobalShift;      // param_1 + 0x40

public:
    CFishsongFile(const std::string& thePath);
    virtual ~CFishsongFile();

    // Corresponds to FUN_005152f0
    bool    Load();

private:
    void    ProcessCommand(char* theLine);
    void    LogError(const char* theFmt, ...);
};

//=============================================================================
// Game Logic Helpers
//=============================================================================
// Corresponds to FUN_0054cc70
void CheckAndApplySpecialFishLogic(Fish* theFish, GameApp* theApp);

#endif // __FISHSONG_H__
