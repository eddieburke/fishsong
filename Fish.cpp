#include "Fish.h"
#include "Board.h"
#include <algorithm>

namespace Sexy {

Fish::Fish(const std::string& theName) : mName(theName) {
    mVariant = 0; mIsSinging = false; mType = -1;
    mIsSpecial = false; mHasCostume = false;
}

// Corresponds to FUN_0054cc70
void Fish::CheckSpecialLogic(Board* theBoard, Fish* theFish) {
    // 1. Check for Singing Fish ID
    if (_stricmp(theFish->mName.c_str(), "1SingingFish") == 0) {
        theFish->mIsSinging = true; // Offset 0x110
        return;
    }

    // 2. Check for Santa
    if (_stricmp(theFish->mName.c_str(), "santa") == 0 && theFish->mVariant == 0) {
        // Iterate board's fish list to see if Santa (Type 5) already exists
        bool aSantaFound = false;
        for (size_t i = 0; i < theBoard->mFishList.size(); ++i) {
            if (theBoard->mFishList[i]->mType == 5) {
                aSantaFound = true;
                break;
            }
        }

        if (aSantaFound) return;

        // Transformation Logic
        if (!theFish->mHasCostume && theFish->mType == -1) {
            theFish->mType = 5;         // Type 5 = Santa
            theFish->mHasCostume = true; // 0x1D0
            theFish->mIsSinging = true;  // 0x110
            theFish->mIsSpecial = true;  // 0x1A8
            
            // Set Santa Colors (Matches FUN_00433320 calls)
            theFish->mColor1 = 0xFFFFFF; // White
            theFish->mColor2 = 0xFF0000; // Red
            theFish->mColor3 = 0xFFFFFF; // White
        }
    }
}

}
