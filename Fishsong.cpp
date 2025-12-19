#include "FishSong.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <algorithm>

// Disable warnings for "unsafe" CRT functions standard in VS2005
#pragma warning(disable: 4996) 

// Global song buckets
std::list<FishSong> gFishSongsNormal;
std::list<FishSong> gFishSongsRare;
std::list<FishSong> gFishSongsSanta;
std::list<FishSong> gFishSongsBeethoven;
std::list<FishSong> gFishSongsKilgore;

//-----------------------------------------------------------------------------
// Construction / Destruction
//-----------------------------------------------------------------------------
FishSong::FishSong()
{
    mIsLongVersion = false;
}

FishSong::~FishSong()
{
    mNotes.clear();
}

void FishSong_Init()
{
    FishSong_FreeAll();
}

void FishSong_FreeAll()
{
    gFishSongsNormal.clear();
    gFishSongsRare.clear();
    gFishSongsSanta.clear();
    gFishSongsBeethoven.clear();
    gFishSongsKilgore.clear();
}

//-----------------------------------------------------------------------------
// Helper: Case Insensitive Comparison (PopCap utility style)
//-----------------------------------------------------------------------------
static bool StringEquals(const std::string& aStr1, const std::string& aStr2)
{
    return stricmp(aStr1.c_str(), aStr2.c_str()) == 0;
}

static bool StringContains(const std::string& aString, const std::string& aSubString)
{
    std::string s1 = aString;
    std::string s2 = aSubString;
    std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
    std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
    return s1.find(s2) != std::string::npos;
}

//-----------------------------------------------------------------------------
// FUN_0050e860: FishSong_ParseNote
// Parses a single note string (e.g. "C#5 q") into the FishNote struct
//-----------------------------------------------------------------------------
static void FishSong_ParseNote(FishNote* pNote, char* pTokenString)
{
    char szNote[256];
    char szDur[256];
    char szExtra[256];

    // Initialize defaults based on decompilation
    pNote->mPitch = 0.0f;
    pNote->mUnused1 = 0.0f;
    pNote->mUnused2 = 0.0f;
    pNote->mSustain = 1.875f; // Magic value from 0050e860
    pNote->mDuration = 0.0f;

    // Scan for exactly two strings. If 3, it's an error (missing comma).
    int nScanned = sscanf(pTokenString, "%s%s%s", szNote, szDur, szExtra);
    
    if (nScanned != 2)
    {
        // In a real PopCap engine, this would log to a debug console
        return; 
    }

    // --- Parse Pitch ---
    int nBasePitch = 0;
    char cNote = (char)tolower(szNote[0]);

    switch (cNote)
    {
        case 'c': nBasePitch = 0; break;
        case 'd': nBasePitch = 2; break;
        case 'e': nBasePitch = 4; break;
        case 'f': nBasePitch = 5; break;
        case 'g': nBasePitch = 7; break;
        case 'a': nBasePitch = 9; break;
        case 'b': nBasePitch = 11; break;
        case 'r': nBasePitch = -10000; break; // Rest
        default: return; // Invalid note
    }

    int nOctave = 4; // Default octave
    char cAccidental = szNote[1];
    char cNext = szNote[2];

    if (cAccidental != '\0')
    {
        if (!isdigit(cAccidental))
        {
            if (cAccidental == '#') nBasePitch++;
            else if (cAccidental == 'b') nBasePitch--;
            
            // If accidental present, octave is the next char
            if (isdigit(cNext)) nOctave = cNext - '0';
        }
        else
        {
            // No accidental, cAccidental is the octave digit
            nOctave = cAccidental - '0';
        }
    }

    // Formula: Base + (Octave - 4) * 12
    pNote->mPitch = (float)(nBasePitch + (nOctave - 4) * 12);

    // --- Parse Duration ---
    // If it's a raw number
    if (isdigit(szDur[0]))
    {
        pNote->mDuration = (float)atoi(szDur);
    }
    else
    {
        char* pChar = szDur;
        while (*pChar)
        {
            int nTicks = 0;
            char cDur = (char)tolower(*pChar);

            switch (cDur)
            {
                case 'w': nTicks = 192; break; // Whole
                case 'h': nTicks = 96;  break; // Half
                case 'q': nTicks = 48;  break; // Quarter
                case 'e': nTicks = 24;  break; // Eighth
                case 's': nTicks = 12;  break; // Sixteenth
                case 't': nTicks = 6;   break; // 32nd
                case 'z': nTicks = 3;   break; // 64th
                default: break; 
            }

            // Check modifiers
            char* pNext = pChar + 1;
            char cMod = (char)tolower(*pNext);

            if (cMod == 't') // Triplet
            {
                nTicks = (nTicks * 2) / 3;
                pNext++;
            }

            // Handle Dots ('d')
            int nDotVal = nTicks;
            int nTotalTicks = nTicks;

            while (tolower(*pNext) == 'd')
            {
                nDotVal /= 2;
                nTotalTicks += nDotVal;
                pNext++;
            }

            pNote->mDuration += (float)nTotalTicks;

            // Handle Ties ('+')
            if (*pNext != '+') break;
            
            // Advance past '+' and continue loop
            pChar = pNext + 1;
        }
    }
}

//-----------------------------------------------------------------------------
// FUN_005152f0: FishSong_ParseFile
// Reads the file line by line
//-----------------------------------------------------------------------------
static bool FishSong_ParseFile(const std::string& aFilename, FishSong& outSong)
{
    FILE* fp = fopen(aFilename.c_str(), "r");
    if (!fp) return false;

    char szLineBuffer[4096];
    outSong.mNotes.clear();

    while (fgets(szLineBuffer, sizeof(szLineBuffer), fp))
    {
        // Basic trimming and comment check
        if (szLineBuffer[0] == '#' || szLineBuffer[0] == '\r' || szLineBuffer[0] == '\n') 
            continue;

        // Tokenize by comma
        char* pToken = strtok(szLineBuffer, ",");
        while (pToken != NULL)
        {
            // Skip whitespace (simple implementation)
            while (isspace(*pToken)) pToken++;

            FishNote aNote;
            FishSong_ParseNote(&aNote, pToken);

            // Only add if it's a valid note or valid rest
            if (aNote.mDuration > 0.0f || aNote.mPitch == -10000.0f)
            {
                outSong.mNotes.push_back(aNote);
            }

            pToken = strtok(NULL, ",");
        }
    }

    fclose(fp);
    return !outSong.mNotes.empty();
}

//-----------------------------------------------------------------------------
// FUN_005166f0: FishSong_LoadAll
// Main entry point for loading songs
//-----------------------------------------------------------------------------
void FishSong_LoadAll()
{
    FishSong_Init();

    // In VS2005/Windows, we use FindFirstFile
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("fishsongs\\*.txt", &findData);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    // Error logging file
    FILE* pErrorFile = NULL; 

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        std::string sFilename = findData.cFileName;
        std::string sFullPath = "fishsongs\\" + sFilename;
        std::string sShortName = sFilename;
        
        bool bIsLong = false;

        // Detect _long.txt or _short.txt suffix
        size_t nUnderscore = sShortName.find('_');
        if (nUnderscore != std::string::npos)
        {
            std::string sSuffix = sShortName.substr(nUnderscore);
            if (StringEquals(sSuffix, "_long.txt"))
                bIsLong = true;
            else if (StringEquals(sSuffix, "_short.txt"))
                bIsLong = false;
            
            // Strip suffix for categorization
            sShortName = sShortName.substr(0, nUnderscore);
        }
        else
        {
            // Strip extension
            size_t nDot = sShortName.find('.');
            if (nDot != std::string::npos)
                sShortName = sShortName.substr(0, nDot);
        }

        FishSong aSong;
        aSong.mName = sShortName;
        aSong.mIsLongVersion = bIsLong;

        if (FishSong_ParseFile(sFullPath, aSong))
        {
            // Categorization logic based on name
            if (StringEquals(sShortName, "kilgore"))
            {
                gFishSongsKilgore.push_back(aSong);
            }
            else if (StringEquals(sShortName, "santa") || StringEquals(sShortName, "santarare"))
            {
                gFishSongsSanta.push_back(aSong);
            }
            else if (StringEquals(sShortName, "beethoven") || StringEquals(sShortName, "beethovenrare"))
            {
                gFishSongsBeethoven.push_back(aSong);
            }
            else
            {
                if (StringContains(sShortName, "rare"))
                    gFishSongsRare.push_back(aSong);
                else
                    gFishSongsNormal.push_back(aSong);
            }
        }
        else
        {
            // Mimic error logging behavior from decomp
            if (!pErrorFile) pErrorFile = fopen("fishsongerror.txt", "w");
            if (pErrorFile) fprintf(pErrorFile, "%s - Error parsing file\n", sFilename.c_str());
        }

    } while (FindNextFileA(hFind, &findData));

    if (pErrorFile) fclose(pErrorFile);
    FindClose(hFind);
}
