#ifndef __FISHSONG_H__
#define __FISHSONG_H__

#include <string>
#include <vector>
#include <map>

namespace Sexy
{

// Represents a single musical note and optional lyric
struct SongNote
{
    int         mPitch;     // Semi-tone offset (e.g., 0 = C, 1 = C#, etc.)
    float       mDuration;  // Duration in updates/frames
    std::string mLyric;     // Text to display, if any
};

// Represents a full song loaded from a text file
struct SongTrack
{
    std::string             mName;
    std::vector<SongNote>   mNotes;
};

class FishSongManager
{
public:
    static FishSongManager* gFishSongManager;

    // Maps song names (e.g., "beethoven", "santa") to track data
    std::map<std::string, SongTrack> mSongs;

public:
    FishSongManager();
    virtual ~FishSongManager();

    void    LoadSongs(); // Corresponds to FUN_005166f0
    
    // Helpers
    SongTrack* GetSong(const std::string& theName);

private:
    void    ParseSongFile(const std::string& theFilePath, const std::string& theSongName);
    int     ParseNotePitch(const std::string& theNoteStr); // Logic from FUN_0050e860
    float   ParseDuration(const std::string& theDurStr);
};

}

#endif // __FISHSONG_H__
