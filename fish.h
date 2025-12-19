#ifndef __FISH_H__
#define __FISH_H__

#include <string>
#include "SexyAppFramework/Color.h"
#include "SexyAppFramework/Graphics.h"

namespace Sexy
{

class Board;

class Fish 
{
public:
    std::string mName;          
    int         mVariant;       // 0 = Guppy/Normal
    bool        mIsSingingFish; 
    int         mType;          // -1 = Default, 5 = Santa, 1 = Beethoven, etc.
    bool        mHasCostume;    
    bool        mIsSpecial;     

    // Visuals (Stubbed)
    float       mX, mY;
    Color       mColor1, mColor2, mColor3;

public:
    Fish(const std::string& name);
    virtual ~Fish();

    void Update(Board* theBoard);
    void Draw(Graphics* g);

    // Helpers for the decompiled logic
    void SetColor1(int c) { mColor1 = Color(c); }
    void SetColor2(int c) { mColor2 = Color(c); }
    void SetColor3(int c) { mColor3 = Color(c); }
};

}

#endif // __FISH_H__
