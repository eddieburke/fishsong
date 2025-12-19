#ifndef __FISH_H__
#define __FISH_H__

#include "SexyAppFramework/Graphics.h"
#include "FishSong.h"

namespace Sexy {
    class Fish {
    public:
        float mX, mY;
        FishSong* mCurrentSong;
        int mSongTick;
        int mNoteIndex;

        Fish(float theX, float theY);
        void SetSong(FishSong* theSong);
        void Update();
        void Draw(Graphics* g);
    };
}

#endif
