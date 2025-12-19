#ifndef __FISH_H__
#define __FISH_H__

#include <string>

namespace Sexy {

class Board;

class Fish {
public:
    // Memory offsets from FUN_0054cc70
    int         mVariant;       // 0x8C
    std::string mName;          // 0xB4
    bool        mIsSinging;     // 0x110
    int         mType;          // 0x124
    bool        mIsSpecial;     // 0x1A8
    bool        mHasCostume;    // 0x1D0
    int         mColor1, mColor2, mColor3; // 0x1D8, 0x1E8, 0x1F8 (approx)

public:
    Fish(const std::string& theName);
    void Update();
    
    // The C++ version of FUN_0054cc70
    static void CheckSpecialLogic(Board* theBoard, Fish* theFish);
};

}

#endif
