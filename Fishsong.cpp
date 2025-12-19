#include "FishSong.h"
#include <windows.h> // For FindFirstFile/FindNextFile as per decompilation
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace Sexy;

FishSongManager* FishSongManager::gFishSongManager = NULL;

FishSongManager::FishSongManager()
{
    gFishSongManager = this;
}

FishSongManager::~FishSongManager()
{
    if (gFishSongManager == this)
        gFishSongManager = NULL;
}

// Reconstructs FUN_005166f0: Iterates directory and loads files
void FishSongManager::LoadSongs()
{
    WIN32_FIND_DATAA aFindData;
    HANDLE aHnd = FindFirstFileA("fishsongs\\*.txt", &aFindData);

    if (aHnd == INVALID_HANDLE_VALUE)
        return;

    do
    {
        std::string aFilename = aFindData.cFileName;
        
        // The original code checks for "fishsongs" directory validity and loops
        // It also handles specific naming conventions (e.g., stripping extension)
        
        std::string aSongName = aFilename;
        size_t aLastDot = aSongName.find_last_of(".");
        if (aLastDot != std::string::npos)
            aSongName = aSongName.substr(0, aLastDot);

        // Convert to lowercase for key storage
        std::transform(aSongName.begin(), aSongName.end(), aSongName.begin(), ::tolower);

        ParseSongFile("fishsongs\\" + aFilename, aSongName);

    } while (FindNextFileA(aHnd, &aFindData));

    FindClose(aHnd);
}

// Reconstructs FUN_005152f0: Reads lines, parses CSV-like format
void FishSongManager::ParseSongFile(const std::string& theFilePath, const std::string& theSongName)
{
    std::ifstream aStream(theFilePath.c_str());
    if (!aStream.is_open())
        return;

    SongTrack aTrack;
    aTrack.mName = theSongName;

    std::string aLine;
    while (std::getline(aStream, aLine))
    {
        // Skip comments or empty lines
        if (aLine.empty() || aLine[0] == '#' || aLine[0] == '\r')
            continue;

        // Simple tokenizer based on commas
        std::stringstream ss(aLine);
        std::string aSegment;
        std::vector<std::string> aTokens;

        while (std::getline(ss, aSegment, ','))
        {
            // Trim whitespace
            aSegment.erase(0, aSegment.find_first_not_of(" \t\r\n"));
            aSegment.erase(aSegment.find_last_not_of(" \t\r\n") + 1);
            aTokens.push_back(aSegment);
        }

        if (aTokens.size() >= 2)
        {
            SongNote aNote;
            aNote.mPitch = ParseNotePitch(aTokens[0]);
            aNote.mDuration = ParseDuration(aTokens[1]);
            
            if (aTokens.size() > 2)
                aNote.mLyric = aTokens[2];

            aTrack.mNotes.push_back(aNote);
        }
    }

    mSongs[theSongName] = aTrack;
}

// Reconstructs FUN_0050e860: Converts "C4", "D#", etc. to pitch
int FishSongManager::ParseNotePitch(const std::string& theNoteStr)
{
    if (theNoteStr.empty()) return 0;

    char aBase = tolower(theNoteStr[0]);
    int aBasePitch = 0;

    switch (aBase)
    {
    case 'c': aBasePitch = 0; break;
    case 'd': aBasePitch = 2; break;
    case 'e': aBasePitch = 4; break;
    case 'f': aBasePitch = 5; break;
    case 'g': aBasePitch = 7; break;
    case 'a': aBasePitch = 9; break;
    case 'b': aBasePitch = 11; break;
    case 'r': return -1000; // Rest
    default: return 0;
    }

    int aModIndex = 1;
    if (theNoteStr.length() > 1)
    {
        if (theNoteStr[1] == '#') 
        {
            aBasePitch++;
            aModIndex++;
        }
        else if (theNoteStr[1] == 'b') 
        {
            aBasePitch--;
            aModIndex++;
        }
    }

    int anOctave = 4; // Default octave
    if (theNoteStr.length() > (size_t)aModIndex)
    {
        anOctave = theNoteStr[aModIndex] - '0';
    }

    return aBasePitch + (anOctave * 12);
}

// Logic to parse duration strings like "0.5", "1", or "q" (quarter), "h" (half)
float FishSongManager::ParseDuration(const std::string& theDurStr)
{
    if (theDurStr.empty()) return 0.0f;

    // Check for standard numeric duration
    if (isdigit(theDurStr[0]) || theDurStr[0] == '.')
        return (float)atof(theDurStr.c_str()) * 100.0f; // Scale to frames

    // Check for notation chars (h=half, q=quarter, w=whole, etc)
    char c = tolower(theDurStr[0]);
    float aBase = 100.0f; // Quarter note baseline reference
    
    switch (c)
    {
    case 'w': aBase = 400.0f; break;
    case 'h': aBase = 200.0f; break;
    case 'q': aBase = 100.0f; break;
    case 'e': aBase = 50.0f; break;
    case 's': aBase = 25.0f; break;
    }

    return aBase;
}

SongTrack* FishSongManager::GetSong(const std::string& theName)
{
    auto it = mSongs.find(theName);
    if (it != mSongs.end())
        return &it->second;
    return NULL;
}

}
