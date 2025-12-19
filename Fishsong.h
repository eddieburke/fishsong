#ifndef __FISHSONG_H__
#define __FISHSONG_H__

#include <string>
#include <vector>
#include <list>

//-----------------------------------------------------------------------------
// Structure definitions matching the binary memory layout
//-----------------------------------------------------------------------------
struct FishNote
{
    float mPitch;       // Offset 0x00: Pitch relative to C4 (0.0 = C4). -10000.0 = Rest
    float mUnused1;     // Offset 0x04: Padding/Unused
    float mUnused2;     // Offset 0x08: Padding/Unused
    float mSustain;     // Offset 0x0C: Default set to 1.875 in binary
    float mDuration;    // Offset 0x10: Duration in ticks
};

class FishSong
{
public:
    std::string             mName;
    std::vector<FishNote>   mNotes;
    bool                    mIsLongVersion;

public:
    FishSong();
    virtual ~FishSong();
};

//-----------------------------------------------------------------------------
// Global Song Lists
//-----------------------------------------------------------------------------
extern std::list<FishSong> gFishSongsNormal;
extern std::list<FishSong> gFishSongsRare;
extern std::list<FishSong> gFishSongsSanta;
extern std::list<FishSong> gFishSongsBeethoven;
extern std::list<FishSong> gFishSongsKilgore;

//-----------------------------------------------------------------------------
// Public Interface
//-----------------------------------------------------------------------------
void FishSong_Init();
void FishSong_LoadAll();
void FishSong_FreeAll();

#endif // __FISHSONG_H__
