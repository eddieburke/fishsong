#ifndef __FISH_H__
#define __FISH_H__

#include <string>

class Fish 
{
public:
    std::string mName;          // 0xB4
    int         mVariant;       // 0x8C (0 = Guppy/Normal)
    bool        mIsSingingFish; // 0x110
    int         mType;          // 0x124 (-1 = Default, 5 = Santa)
    bool        mHasCostume;    // 0x1D0
    bool        mIsSpecial;     // 0x1A8

public:
    Fish() : mVariant(0), mIsSingingFish(false), mType(-1), mHasCostume(false), mIsSpecial(false) {}
    
    // Stub methods for FUN_00433320
    void SetColor1(int c) {}
    void SetColor2(int c) {}
    void SetColor3(int c) {}
};

#endif
