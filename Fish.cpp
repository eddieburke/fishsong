#include "Fish.h"
#include "FishSong.h"
#include "SexyAppFramework/Font.h"
#include "SexyAppFramework/Color.h"

using namespace Sexy;

Fish::Fish(float x, float y)
{
    mX = x;
    mY = y;
    mCurrentSong = NULL;
    mCurrentNoteIndex = 0;
    mNoteTimer = 0;
    mIsSinging = false;
    mScale = 1.0f;
}

Fish::~Fish()
{
}

void Fish::SetSong(const std::string& theSongName)
{
    if (FishSongManager::gFishSongManager)
    {
        mCurrentSong = FishSongManager::gFishSongManager->GetSong(theSongName);
        mCurrentNoteIndex = 0;
        mNoteTimer = 0;
        mIsSinging = (mCurrentSong != NULL);
        mCurrentLyric = "";
    }
}

void Fish::Update()
{
    // Generic fish animation logic (bobbing)
    mY += sin(mNoteTimer * 0.1f) * 0.5f;

    // Scale logic return to normal
    if (mScale > 1.0f)
        mScale -= 0.05f;

    if (!mIsSinging || !mCurrentSong)
        return;

    // Advance Timer
    mNoteTimer -= 1.0f; // Assuming Update called once per frame (10ms usually)

    if (mNoteTimer <= 0)
    {
        // Move to next note
        mCurrentNoteIndex++;

        if (mCurrentNoteIndex >= (int)mCurrentSong->mNotes.size())
        {
            // Song Over
            mIsSinging = false;
            mCurrentLyric = "";
            return;
        }

        PlayCurrentNote();
    }
}

void Fish::PlayCurrentNote()
{
    if (!mCurrentSong) return;

    SongNote& aNote = mCurrentSong->mNotes[mCurrentNoteIndex];
    
    // Set timer for how long this note lasts
    mNoteTimer = aNote.mDuration;

    // Set Lyric
    if (!aNote.mLyric.empty())
    {
        mCurrentLyric = aNote.mLyric;
    }

    // "Play" the sound
    if (aNote.mPitch != -1000) // If not a rest
    {
        // Visual pop effect
        mScale = 1.3f; 

        // In a real SexyApp implementation, you would access the SoundManager here.
        // Example:
        // float aFreq = pow(1.05946f, aNote.mPitch - 60); 
        // gSexyAppBase->PlaySample(SOUND_FISH_VOICE, 0, aFreq);
    }
}

void Fish::Draw(Graphics* g)
{
    // Save state
    g->PushState();

    // Draw The Fish (Simple Placeholder Circle/Oval if no Image)
    g->SetColor(Color(255, 128, 0));
    g->FillRect((int)mX - 20, (int)mY - 15, (int)(40 * mScale), (int)(30 * mScale));

    // Draw Lyrics
    if (mIsSinging && !mCurrentLyric.empty())
    {
        g->SetColor(Color::White);
        // Assuming a font is available, otherwise this would just be logic
        // g->SetFont(FONT_DEFAULT); 
        g->DrawString(mCurrentLyric, (int)mX, (int)mY - 30);
    }

    g->PopState();
}

}
