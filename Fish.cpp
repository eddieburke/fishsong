#include "Fish.h"
#include "Board.h"

using namespace Sexy;

Fish::Fish(const std::string& name) 
    : mName(name)
    , mVariant(0)
    , mIsSingingFish(false)
    , mType(-1)
    , mHasCostume(false)
    , mIsSpecial(false)
    , mX(0), mY(0)
    , mColor1(255, 165, 0) // Default Orange
    , mColor2(255, 255, 255)
    , mColor3(0, 0, 0)
{
}

Fish::~Fish() {}

void Fish::Update(Board* theBoard)
{
    // Basic swim movement
    mX += 1.0f;
    if (mX > 800) mX = 0;
}

void Fish::Draw(Graphics* g)
{
    // Simple debug drawing
    g->SetColor(mColor1);
    g->FillRect((int)mX, (int)mY, 40, 30);
    
    // Draw Name/Type
    g->SetColor(Color::White);
    std::string label = mName;
    if (mType == 5) label += " (SANTA)";
    if (mIsSingingFish) label += " [Singing]";
    
    g->DrawString(label, (int)mX, (int)mY - 5);
}
