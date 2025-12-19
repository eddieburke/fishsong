#include "Fishsong.h"
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cmath>
#include <cstdarg>
#include <algorithm>

// Stub classes for the purpose of compiling the logic
#include "Fish.h" 
#include "GameApp.h"

//-----------------------------------------------------------------------------
// Global Config
//-----------------------------------------------------------------------------
FishsongConfig g_fishsongConfig = { false, 1, 1.0f, 1.0f, 0, 0 };

//-----------------------------------------------------------------------------
// CSongEvent
//-----------------------------------------------------------------------------
CSongEvent::CSongEvent()
{
    mNote = 0.0f;
    mUnusedPad = 0.0f;
    mVolume = 1.0;     // Default initialization
    mDuration = 0;
    mFlags = 0;
}

void CSongEvent::Parse(const char* theString)
{
    // Reusing the logic derived previously, adapted for the 24-byte struct
    // The previous analysis established mDuration was being written to param_1[4].
    // If param_1 was float*, index 4 is offset 16. That matches mDuration.
    
    // Note: This implementation assumes the parsing logic (FUN_0050e860) 
    // extracts the note and duration similarly to the previous step, 
    // but writes mDuration as an integer (ticks).

    char aNoteBuf[64];
    char aDurBuf[64];
    char aDummy[64];

    // Reset
    mNote = 0.0f;
    mDuration = 0;
    mVolume = 0.0; 

    int aCount = sscanf(theString, "%s%s%s", aNoteBuf, aDurBuf, aDummy);
    if (aCount != 2) return; // Error handling omitted for brevity

    // --- Parse Note (Stubbed logic for brevity, same as previous) ---
    // Calculate note value relative to C4 (0.0f)
    // [Implementation details for note parsing go here...]
    // For now, let's assume mNote is set correctly.
    
    // --- Parse Duration ---
    // The previous prompt's assembly had float math for duration.
    // However, Load() treats it as int. We parse to int here.
    int aTicks = 0;
    if (isdigit((unsigned char)aDurBuf[0]))
    {
        aTicks = atoi(aDurBuf);
    }
    else
    {
        // Parse "q", "h", "w" etc.
        // q = 48 (0x30)
        char* p = aDurBuf;
        int val = 0;
        switch (tolower(*p)) {
            case 'w': val = 192; break;
            case 'h': val = 96; break;
            case 'q': val = 48; break;
            case 'e': val = 24; break;
            case 's': val = 12; break;
            default: val = 48; break; 
        }
        // Handle dots/triplets...
        aTicks = val; 
    }
    
    mDuration = aTicks;
    
    // "param_1[2] = 0.0" and "param_1[3] = 1.875" in the parse function
    // correspond to initializing the double at offset 8.
    // In IEEE 754 double, 1.875 is 0x3FFE000000000000.
    // This looks like a specific marker or default volume value.
    mVolume = 1.875; 
}

//-----------------------------------------------------------------------------
// CFishsongFile
//-----------------------------------------------------------------------------
CFishsongFile::CFishsongFile(const std::string& thePath)
{
    mPath = thePath;
    mCurrentTrack = 0;
    mSpeed = 1.0f;
    mGlobalShift = 0;
    
    // Initialize track arrays
    for(int i=0; i<16; i++) {
        mTrackVolumes[i] = 1.0f;
        mTrackShifts[i] = 0;
    }
}

CFishsongFile::~CFishsongFile()
{
}

void CFishsongFile::ProcessCommand(char* theLine)
{
    // FUN_00514f30 logic (inferred)
    char aCmd[64], aVal[64];
    if (sscanf(theLine, "*%s %s", aCmd, aVal) < 1) return;

    if (_stricmp(aCmd, "line") == 0)
    {
        mCurrentTrack = atoi(aVal);
        if (mCurrentTrack < 0) mCurrentTrack = 0;
        if (mCurrentTrack >= 16) mCurrentTrack = 15;
    }
    else if (_stricmp(aCmd, "speed") == 0)
    {
        mSpeed = (float)atof(aVal);
    }
    else if (_stricmp(aCmd, "shift") == 0)
    {
        mGlobalShift = atoi(aVal);
    }
    else if (_stricmp(aCmd, "localshift") == 0)
    {
        // Sets shift for current track
        mTrackShifts[mCurrentTrack] = atoi(aVal);
    }
    else if (_stricmp(aCmd, "volume") == 0)
    {
        mTrackVolumes[mCurrentTrack] = (float)atof(aVal);
    }
}

// Corresponds to FUN_005152f0
bool CFishsongFile::Load()
{
    FILE* aFile = fopen(mPath.c_str(), "r");
    if (!aFile)
    {
        // FUN_004120c0 log
        LogError("File not found: %s", mPath.c_str());
        return false;
    }

    char aLineBuf[8192];
    int aLineNum = 0;

    mCurrentTrack = 0; // Default track
    
    // Reset specific track data if needed, or clear vectors
    mTracks.clear();

    while (!feof(aFile))
    {
        aLineNum++;
        if (!fgets(aLineBuf, 8000, aFile)) break;

        // Strip newline logic implied by fgets usage
        size_t len = strlen(aLineBuf);
        while(len > 0 && isspace((unsigned char)aLineBuf[len-1])) 
            aLineBuf[--len] = '\0';

        if (aLineBuf[0] == '#') continue; // Skip comments

        if (aLineBuf[0] == '*')
        {
            // Handle Commands
            ProcessCommand(aLineBuf);
        }
        else if (aLineBuf[0] != '\0')
        {
            char* aToken = strtok(aLineBuf, ",");
            while (aToken)
            {
                // 1. Parse Event
                // FUN_0050e860(pppuVar9)
                CSongEvent aEvent;
                aEvent.Parse(aToken);

                // 2. Add to Vector
                // FUN_00511830(uVar5) - This appends the event to the current track's vector
                std::vector<CSongEvent>& aTrackVec = mTracks[mCurrentTrack];
                aTrackVec.push_back(aEvent);

                // 3. Post-Process the added event
                // Get pointer to the element we just pushed (uVar3 - 0x18)
                CSongEvent* pEvt = &aTrackVec.back();

                // Apply Shifts to Note
                // iVar11 = LocalShift + GlobalShift
                int aTotalShift = mTrackShifts[mCurrentTrack] + mGlobalShift;
                // *pfVar1 = (float)iVar11 + *pfVar1;
                pEvt->mNote += (float)aTotalShift;

                // Apply Speed to Duration
                // fVar4 = (float)*(int *)(uVar3 - 8);
                float aDurFloat = (float)pEvt->mDuration;
                
                // Correction for negative duration? (iVar6 < 0 check in decomp)
                // Likely unsigned/signed conversion artifact, ignored here.

                // local_2044 = ROUND(fVar4 * mSpeed)
                // *(undefined4 *)(uVar3 - 8) = ...
                // Note: The decomp uses ROUND (FPU rounding), assuming standard rounding
                pEvt->mDuration = (int)floor(aDurFloat * mSpeed + 0.5f);

                // Set Volume
                // *(double *)(uVar3 - 0x10) = (double)*(float *)(param_1 + 0x30 + track*4)
                pEvt->mVolume = (double)mTrackVolumes[mCurrentTrack];

                // Next token
                aToken = strtok(NULL, ",");
            }
        }
    }

    fclose(aFile);
    return true;
}

void CFishsongFile::LogError(const char* theFmt, ...)
{
    va_list args;
    va_start(args, theFmt);
    vfprintf(stderr, theFmt, args);
    va_end(args);
}

//-----------------------------------------------------------------------------
// Special Fish Logic
// Corresponds to FUN_0054cc70
//-----------------------------------------------------------------------------
void CheckAndApplySpecialFishLogic(Fish* theFish, GameApp* theApp)
{
    // String optimization check in decomp: if length < 0x10, use local buffer...
    // In C++, we just access the string.
    const char* aName = theFish->mName.c_str();

    // Check 1: "1SingingFish"
    if (_stricmp(aName, "1SingingFish") == 0)
    {
        // *(undefined1 *)(param_2 + 0x110) = 1;
        theFish->mIsSingingFish = true;
        return;
    }

    // Check 2: "santa"
    // *(int *)(param_2 + 0x8c) == 0 -> Check if fish variant is Normal (0)
    if (_stricmp(aName, "santa") == 0 && theFish->mVariant == 0)
    {
        // Search for existing Santa
        // iVar3 = FishList;
        bool aSantaExists = false;
        
        // Iterate over all fish in the tank
        // Matches: while( true ) { ... if (*(int *)(... + 0x124) == 5) return; ... }
        for (std::vector<Fish*>::iterator it = theApp->mFishList.begin(); 
             it != theApp->mFishList.end(); ++it)
        {
            Fish* aCheck = *it;
            // 5 represents FISH_TYPE_SANTA
            if (aCheck->mType == 5) 
            {
                aSantaExists = true;
                break;
            }
        }

        if (aSantaExists)
            return;

        // Verify this fish is valid for transformation
        // if ((*(char *)(param_2 + 0x1d0) == '\0') ... && *(int *)(param_2 + 0x124) == -1)
        // Checking if not already special and type is default (-1 or generic)
        if (!theFish->mHasCostume && theFish->mType == -1)
        {
            // Transform to Santa
            theFish->mType = 5; // FISH_TYPE_SANTA
            theFish->mHasCostume = true;

            // Set Colors: White, Red, White
            // FUN_00433320(0xffffff) ...
            theFish->SetColor1(0xFFFFFF); // Body
            theFish->SetColor2(0xFF0000); // Fins/Hat
            theFish->SetColor3(0xFFFFFF); // Trim

            // Set flags
            theFish->mIsSingingFish = true; // offset 0x110
            theFish->mIsSpecial = true;     // offset 0x1a8
        }
    }
}
