#include "Fishsong.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cctype>
#include <algorithm>

//-------------------------------------------------------------------------
// global configuration instance
//-------------------------------------------------------------------------
FishsongConfig g_fishsongConfig = { false, 1, 5.0f, 1.0f, 0, 0 };

//-------------------------------------------------------------------------
// tiny helpers
//-------------------------------------------------------------------------
static void LogInfo (const std::string& m){ std::cout << "INFO:  " << m << std::endl; }
static void LogError(const std::string& m){ std::cerr << "ERROR: " << m << std::endl; }

int StrCaseCmp(const char* a,const char* b)
{
    while(*a && std::tolower((unsigned char)*a)==std::tolower((unsigned char)*b))
        { ++a; ++b; }
    return std::tolower((unsigned char)*a) - std::tolower((unsigned char)*b);
}

static std::string Trim(const std::string& s)
{
    const std::string ws(" \t\r\n");
    std::string::size_type a = s.find_first_not_of(ws);
    if(a==std::string::npos) return "";
    std::string::size_type b = s.find_last_not_of(ws);
    return s.substr(a,b-a+1);
}

static int NoteLetterToVal(char c)
{
    switch(std::tolower(c))
    {
        case 'c': return 0;  case 'd': return 2;  case 'e': return 4;
        case 'f': return 5;  case 'g': return 7;  case 'a': return 9;
        case 'b': return 11; default:  return -1;
    }
}

//=============================================================================
// CSongEvent
//=============================================================================
CSongEvent::CSongEvent() :
    noteValue(0), duration(0), volume(1.0f), title(""),
    isChordMember(false)
{}

bool CSongEvent::ProcessSongEvent(const std::string& token)
{
    std::istringstream iss(token);
    std::string noteTok, durTok, volTok;
    iss >> noteTok >> durTok >> volTok;
    if(noteTok.empty()) return false;

    isChordMember = (durTok=="0");

    // -------------------- note / rest
    if(std::tolower(noteTok[0])=='r')
    {
        noteValue = -1;
    }
    else
    {
        int base = NoteLetterToVal(noteTok[0]);
        if(base<0) return false;

        std::size_t p = 1;
        int accidental = 0;
        if(p<noteTok.size())
        {
            if(noteTok[p]=='b')                     { accidental=-1; ++p; }
            else if(noteTok[p]=='#' || noteTok[p]=='s') { accidental=+1; ++p; }
        }

        int octave = 4;
        if(p<noteTok.size() && std::isdigit((unsigned char)noteTok[p]))
            octave = std::atoi(noteTok.substr(p).c_str());

        noteValue = (octave+1)*12 + base + accidental +
                    g_fishsongConfig.shift + g_fishsongConfig.localShift;
    }

    // -------------------- duration
    duration = isChordMember ? 0 : ParseDuration(durTok);

    // -------------------- volume
    volume = g_fishsongConfig.volume;
    if(!volTok.empty())
    {
        float v = (float)std::atof(volTok.c_str());
        if(v>=0.0f) volume = v;
    }

    title = noteTok;
    return true;
}

int CSongEvent::GetDurationValue(char sym)
{
    switch(std::tolower(sym))
    {
        case 'w': return 480;  // whole
        case 'h': return 240;  // half
        case 'q': return 120;  // quarter
        case 'e': return  60;  // eighth
        case 's': return  30;  // sixteenth
        case 't': return  15;  // 32-nd (triplet eighth)
        case 'z': return   4;  // 128-th (3.75 → rounded)
        default : return   0;
    }
}

int CSongEvent::ParseDuration(const std::string& str)
{
    if(str.empty()) return 120;

    int total = 0;
    std::stringstream ss(str);
    std::string seg;
    while(std::getline(ss,seg,'+'))
    {
        seg = Trim(seg); if(seg.empty()) continue;
        int val = 0;

        if(std::isdigit((unsigned char)seg[0]))
            val = std::atoi(seg.c_str());                // raw ticks
        else
        {
            val = GetDurationValue(seg[0]);
            if(val==0) continue;

            if(seg.size()>1 && seg[1]=='d')              // dotted
            {
                val = (val*3)/2;
                if(seg.size()>2 && seg[2]=='d')          // double dot
                    val += val/2;
            }
            else if(seg.size()>1 && seg[1]=='t')         // triplet
                val = (val*2)/3;
        }
        total += val;
    }
    return total>0 ? total : 120;
}

void CSongEvent::FinalizeSongStructure()
{
    if(!isChordMember && duration<1)
        duration = 1;
}

void CSongEvent::FishsongCopyData(const CSongEvent& root)
{
    noteValue   = root.noteValue;
    duration    = root.duration;
    volume      = root.volume;
    SetIsChordMember(false);
}

bool CSongEvent::CheckSongCategory(const std::string& c) const
{
    // “short” < half-note,  “long” ≥ dotted-half
    if(StrCaseCmp(c.c_str(),"short")==0) return duration < 240;
    if(StrCaseCmp(c.c_str(),"long" )==0) return duration >=300;
    return false;
}

// accessors ----------------------------------------------------------
int                 CSongEvent::GetNoteValue()  const { return noteValue; }
int                 CSongEvent::GetDuration()   const { return duration; }
float               CSongEvent::GetVolume()     const { return volume; }
const std::string&  CSongEvent::GetTitle()      const { return title; }
bool                CSongEvent::IsChordMember() const { return isChordMember; }

void CSongEvent::SetTitle(const std::string& t){ title=t; }
void CSongEvent::SetDuration(int d){ duration=d; }
void CSongEvent::SetIsChordMember(bool v){ isChordMember=v; }

//=============================================================================
// CFishsongFile  --------------------------------------------------------------
//=============================================================================
CFishsongFile::CFishsongFile(const std::string& p)
: mPath(p), mCurrentTrack(1), mWaitingForTrack(0)
{}

CFishsongFile::~CFishsongFile(){}

bool CFishsongFile::Load()
{
    std::ifstream in(mPath.c_str());
    if(!in) { LogError("File not found: "+mPath); return false; }

    mTrackPos.clear();
    mTrackPos[mCurrentTrack]=0;
    g_fishsongConfig.localShift=0;

    bool skip=false;
    std::string line;
    while(std::getline(in,line))
    {
        line = Trim(line);
        if(line.empty()||line[0]=='#') continue;

        if(line[0]=='*')
        {
            if(StrCaseCmp(line.substr(0,4).c_str(),"*off")==0){ skip=true;  continue;}
            if(StrCaseCmp(line.substr(0,3).c_str(),"*on" )==0){ skip=false; continue;}
            ProcessCommand(line);
            continue;
        }
        if(skip) continue;

        if(mWaitingForTrack>0 &&
           mCurrentTrack!=mWaitingForTrack &&
           mTrackPos[mCurrentTrack] < mTrackPos[mWaitingForTrack])
            continue;                      // still waiting

        std::stringstream ls(line);
        std::string tok;
        while(std::getline(ls,tok,','))
        {
            tok = Trim(tok); if(tok.empty()) continue;
            CSongEvent ev;
            if(!ev.ProcessSongEvent(tok))
            {
                LogError("Bad token \""+tok+"\" in line: "+line);
                continue;
            }
            ProcessEventWithChordHandling(ev);
        }
    }

    FinalizePendingChord(120);
    return true;
}

void CFishsongFile::ProcessEventWithChordHandling(const CSongEvent& ev)
{
    if(ev.IsChordMember()){ mPendingChord.push_back(ev); return;}

    if(!mPendingChord.empty())
    {
        for(std::vector<CSongEvent>::iterator it=mPendingChord.begin();
            it!=mPendingChord.end(); ++it)
        {
            it->FishsongCopyData(ev);
            it->FinalizeSongStructure();
            mEvents.push_back(*it);
        }
        mPendingChord.clear();
    }

    CSongEvent root = ev;
    root.FinalizeSongStructure();
    mEvents.push_back(root);
    mTrackPos[mCurrentTrack]+=root.GetDuration();
}

void CFishsongFile::FinalizePendingChord(int rootDur)
{
    if(mPendingChord.empty()) return;
    for(std::vector<CSongEvent>::iterator it=mPendingChord.begin();
        it!=mPendingChord.end(); ++it)
    {
        if(rootDur>0) it->SetDuration(rootDur);
        it->SetIsChordMember(false);
        it->FinalizeSongStructure();
        mEvents.push_back(*it);
    }
    mTrackPos[mCurrentTrack]+=rootDur;
    mPendingChord.clear();
}

void CFishsongFile::ProcessCommand(const std::string& raw)
{
    std::string cmd = Trim(raw.substr(1));     // drop '*'
    std::istringstream iss(cmd);
    std::string name,value; iss>>name; std::getline(iss,value); value=Trim(value);
    if(name.empty()) return;

    if(StrCaseCmp(name.c_str(),"line")==0)
    {
        int n=std::atoi(value.c_str()); if(n<=0) return;
        FinalizePendingChord(120);
        mCurrentTrack=n; g_fishsongConfig.line=n; g_fishsongConfig.localShift=0;
        if(mTrackPos.find(n)==mTrackPos.end()) mTrackPos[n]=0;
        return;
    }
    if(StrCaseCmp(name.c_str(),"rest")==0)
    {
        mWaitingForTrack=std::atoi(value.c_str());
        return;
    }
    ParseFishSongCommand(cmd);    // anything else – global
}

const std::vector<CSongEvent>& CFishsongFile::GetEvents() const { return mEvents; }

//=============================================================================
// CFishsongManager (stub)
//=============================================================================
CFishsongManager::CFishsongManager():mUpdateCounter(0){}
CFishsongManager::~CFishsongManager(){}

bool CFishsongManager::UpdateState(int,int,int& out,int flag)
{ out=++mUpdateCounter; if(flag>=0) LogInfo("Manager update"); return true; }

void CFishsongManager::DispatchEvent(const CSongEvent& ev)
{ mValidated.push_back(ev); LogInfo("Dispatch "+ev.GetTitle()); }

void CFishsongManager::FinalizeEventTitle(CSongEvent& ev)
{ if(ev.CheckSongCategory("long")) ev.SetTitle(ev.GetTitle()+" (extended)"); }

void CFishsongManager::ResetState(){ mValidated.clear(); mUpdateCounter=0; }

//=============================================================================
// Global helpers
//=============================================================================
void InitFishsongEvents(){ g_fishsongConfig=FishsongConfig{false,1,5.0f,1.0f,0,0}; }
void ClearFishsongBuffer(){ LogInfo("Fishsong buffer cleared"); }

CFishsongFile* LoadFishsongFile(const std::string& p)
{
    CFishsongFile* f=new CFishsongFile(p);
    if(!f||!f->Load()){ delete f; return 0; }
    return f;
}

void ProcessFishsong(bool force)
{
    static bool once=false;
    if(force||!once){ once=true; LogInfo("ProcessFishsong stub"); }
}

void ParseFishSongCommand(const std::string& cmdLine)
{
    std::istringstream iss(cmdLine);
    std::string n,v; iss>>n; std::getline(iss,v); v=Trim(v);
    if(n.empty()) return;

    if(StrCaseCmp(n.c_str(),"speed")==0)
        g_fishsongConfig.speed  = std::max(0.1f,(float)std::atof(v.c_str()));
    else if(StrCaseCmp(n.c_str(),"volume")==0)
        g_fishsongConfig.volume = std::max(0.0f,std::min(1.0f,(float)std::atof(v.c_str())));
    else if(StrCaseCmp(n.c_str(),"shift")==0)
        g_fishsongConfig.shift  = std::atoi(v.c_str());
    else if(StrCaseCmp(n.c_str(),"localshift")==0)
        g_fishsongConfig.localShift = std::atoi(v.c_str());
    else if(StrCaseCmp(n.c_str(),"attrib")==0)
        LogInfo("Global attrib: "+v);
    else
        LogError("Unknown global cmd: "+n);
}
