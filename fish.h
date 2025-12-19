#ifndef __FISH_H__
#define __FISH_H__

#include "SexyAppFramework/Graphics.h"
#include "FishSong.h"

namespace Sexy
{

class Fish
{
public:
    float           mX, mY;
    
    // Song Playback State
    SongTrack*      mCurrentSong;
    int             mCurrentNoteIndex;
    float           mNoteTimer;         // Counts down to next note
    std::string     mCurrentLyric;
    bool            mIsSinging;
    float           mScale;             // Pulsing effect when singing

public:
    Fish(float x, float y);
    virtual ~Fish();

    void    SetSong(const std::string& theSongName);
    void    Update();
    void    Draw(Graphics* g);

private:
    void    PlayCurrentNote();
};

}

#endif // __FISH_H__
