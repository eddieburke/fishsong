#include "Fish.h"

using namespace Sexy;

Fish::Fish(float theX, float theY) : mX(theX), mY(theY), mCurrentSong(nullptr), mSongTick(0), mNoteIndex(0) {}

void Fish::SetSong(FishSong* theSong) {
    mCurrentSong = theSong;
    mSongTick = 0;
    mNoteIndex = 0;
}

void Fish::Update() {
    if (!mCurrentSong || mNoteIndex >= (int)mCurrentSong->mNotes.size()) return;

    mSongTick++;
    if (mSongTick >= mCurrentSong->mNotes[mNoteIndex].mDuration) {
        mSongTick = 0;
        mNoteIndex++;
        // Trigger sound logic here based on mCurrentSong->mNotes[mNoteIndex].mPitch
    }
}

void Fish::Draw(Graphics* g) {
    g->SetColor(Color(0, 255, 255));
    g->FillRect(mX, mY, 40, 20); // Simple placeholder fish
    
    if (mCurrentSong) {
        g->DrawString(mCurrentSong->mName, mX, mY - 10);
    }
}
