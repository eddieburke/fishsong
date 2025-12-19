#ifndef __FISHSONG_H__
#define __FISHSONG_H__

#include <string>
#include <vector>
#include <map>

// Forward declarations
class Fish;
class GameApp;

//=============================================================================
// CSongEvent
// Exactly 24 bytes (0x18) to match vector stride in FUN_00511830
//=============================================================================
class CSongEvent 
{
public:
    float   mNote;          // 0x00
    int     mPad04;         // 0x04
    double  mVolume;        // 0x08
    int     mDuration;      // 0x10
    int     mPad14;         // 0x14

public:
    CSongEvent();
    void    Parse(const char* theString); // FUN_0050e860
};

//=============================================================================
// CFishsongFile
//=============================================================================
class CFishsongFile 
{
public:
    std::string mPath;
    
    // Config state matching FUN_005152f0 offsets
    float   mTrackVolumes[16]; // 0x30
    float   mSpeed;            // 0x3C
    int     mGlobalShift;      // 0x40
    int     mTrackShifts[16];  // 0x44
    int     mCurrentTrack;     // 0x54
    bool    mIsSkipping;       // 0x50

    // In the original, this might be a pointer to a struct containing vectors
    std::map<int, std::vector<CSongEvent> > mTracks;

public:
    CFishsongFile(const std::string& thePath);
    virtual ~CFishsongFile();

    bool    Load(); // FUN_005152f0
    
private:
    void    ProcessCommand(char* theLine); // FUN_00514f30
    void    LogError(const char* theFmt, ...);
};

//=============================================================================
// Fish Logic Helpers
//=============================================================================
void CheckAndApplySpecialFishLogic(Fish* theFish, GameApp* theApp); // FUN_0054cc70

#endif
