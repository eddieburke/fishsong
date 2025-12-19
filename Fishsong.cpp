#include "FishSong.h"
#include "SexyAppFramework/SexyAppBase.h"
#include "SexyAppFramework/PakInterface.h"
#include <fstream>
#include <sstream>

using namespace Sexy;

FishSong::FishSong() : mIsLong(false) {}

// Logic replicated from FUN_0050e860
void FishSong::ParseNote(const std::string& theNoteStr, const std::string& theDurationStr) {
    Note aNote;
    aNote.mIsRest = false;
    int aStep = 0;
    int anOctave = 4;

    if (theNoteStr.empty()) return;

    // 1. Parse Note Step
    char aNoteChar = tolower(theNoteStr[0]);
    switch (aNoteChar) {
        case 'c': aStep = 0; break;
        case 'd': aStep = 2; break;
        case 'e': aStep = 4; break;
        case 'f': aStep = 5; break;
        case 'g': aStep = 7; break;
        case 'a': aStep = 9; break;
        case 'b': aStep = 11; break;
        case 'r': aNote.mIsRest = true; aStep = -1000; break;
        default: return; // Invalid note
    }

    // 2. Parse Accidentals and Octave
    size_t idx = 1;
    while (idx < theNoteStr.length()) {
        if (theNoteStr[idx] == '#') aStep++;
        else if (theNoteStr[idx] == 'b') aStep--;
        else if (isdigit(theNoteStr[idx])) {
            anOctave = theNoteStr[idx] - '0';
        }
        idx++;
    }
    aNote.mPitch = (float)(aStep + (anOctave * 12));

    // 3. Parse Duration (Logic from FUN_0050e860)
    int aDuration = 0;
    if (!theDurationStr.empty() && isdigit(theDurationStr[0])) {
        aDuration = atoi(theDurationStr.c_str());
    } else {
        int aBase = 0;
        char aDurChar = tolower(theDurationStr[0]);
        switch (aDurChar) {
            case 'w': aBase = 192; break; // Whole
            case 'h': aBase = 96;  break; // Half
            case 'q': aBase = 48;  break; // Quarter
            case 'e': aBase = 24;  break; // Eighth
            case 's': aBase = 12;  break; // Sixteenth
            case 't': aBase = 6;   break; // Thirty-second
            case 'z': aBase = 3;   break; // Sixty-fourth
        }
        
        // Handle Triplets ('t' suffix) or Dots
        float aMult = 1.0f;
        if (theDurationStr.find('t', 1) != std::string::npos) aMult = 2.0f/3.0f;
        
        aDuration = (int)(aBase * aMult);
        if (theDurationStr.find('.') != std::string::npos) aDuration += aDuration / 2;
    }

    aNote.mDuration = aDuration;
    mNotes.push_back(aNote);
}

FishSongManager::FishSongManager() {}
FishSongManager::~FishSongManager() {
    for (auto const& [name, song] : mSongs) delete song;
}

// Replicates FUN_005166f0 using SexyAppFramework helpers
void FishSongManager::LoadAllSongs(const std::string& theDirectory) {
    auto aFileList = gSexyAppBase->GetPakInterface()->GetFileList(theDirectory + "/*.txt");
    
    for (const auto& aFileName : aFileList) {
        FishSong* aSong = new FishSong();
        aSong->mName = aFileName;
        
        if (aFileName.find("long") != std::string::npos) aSong->mIsLong = true;

        // Open and parse file (Logic from FUN_005152f0)
        std::string aPath = theDirectory + "/" + aFileName;
        PFile* aFile = gSexyAppBase->GetPakInterface()->FOpen(aPath, "r");
        if (aFile) {
            char aLine[4096];
            while (gSexyAppBase->GetPakInterface()->FGets(aLine, 4096, aFile)) {
                if (aLine[0] == '#' || aLine[0] == '\n') continue;
                
                std::stringstream ss(aLine);
                std::string n, d;
                while (std::getline(ss, n, ',') && std::getline(ss, d, ',')) {
                    aSong->ParseNote(n, d);
                }
            }
            gSexyAppBase->GetPakInterface()->FClose(aFile);
        }
        mSongs[aFileName] = aSong;
    }
}
