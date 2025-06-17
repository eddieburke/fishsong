#include "Fishsong.h"
#include <iostream>
#include <fstream>
#include <sstream>
#ifdef USE_SEXYAPP
#include "SexyAppFramework/SexyAppBase.h"
#include "SexyAppFramework/Buffer.h"
#include "SexyAppFramework/Debug.h"
using namespace Sexy;
#endif
#include <cstdlib>
#include <cctype>
#include <algorithm>

// Global Configuration Definition
FishsongConfig g_fishsongConfig = { false, 1, 5.0f, 1.0f, 0, 0 };

// Logging helpers
static void LogInfo(const std::string& msg) {
#ifdef USE_SEXYAPP
    OutputDebug("%s\n", msg.c_str());
#else
    std::cout << "INFO: " << msg << std::endl;
#endif
}

static void LogError(const std::string& msg) {
#ifdef USE_SEXYAPP
    OutputDebug("ERROR: %s\n", msg.c_str());
#else
    std::cerr << "ERROR: " << msg << std::endl;
#endif
}

// Case-insensitive string comparison
int StrCaseCmp(const char* s1, const char* s2) {
    while(*s1 && (std::tolower(static_cast<unsigned char>(*s1)) == std::tolower(static_cast<unsigned char>(*s2)))) {
        s1++; s2++;
    }
    return std::tolower(static_cast<unsigned char>(*s1)) - std::tolower(static_cast<unsigned char>(*s2));
}

// Helper to get duration value from a character symbol
int CSongEvent::GetDurationValue(char symbol) {
    switch(std::tolower(symbol)) {
        case 'w': return 480; case 'h': return 240; case 'q': return 120;
        case 'e': return 60;  case 's': return 30;  case 't': return 15;
        case 'z': return 7;
        default:  return 0;
    }
}

// Helper to convert a note letter (c, d, e...) to its MIDI base value
int NoteLetterToValue(char note) {
    switch (std::tolower(note)) {
        case 'c': return 0; case 'd': return 2; case 'e': return 4;
        case 'f': return 5; case 'g': return 7; case 'a': return 9;
        case 'b': return 11;
        default:  return -1;
    }
}

// Helper to trim whitespace from both ends of a string
std::string TrimString(const std::string &s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// --- CSongEvent Implementation ---
CSongEvent::CSongEvent() : noteValue(0), duration(0), volume(1.0f), title(""), isChordMember(false) {}

bool CSongEvent::ProcessSongEvent(const std::string &eventStr) {
    std::istringstream iss(eventStr);
    std::string noteToken, durationToken, volumeToken;
    iss >> noteToken >> durationToken >> volumeToken;
    
    if (noteToken.empty()) return false;
    
    isChordMember = (durationToken == "0");
    
    if (std::tolower(noteToken[0]) == 'r') {
        noteValue = -1; // Rest
    } else {
        char noteLetter = noteToken[0];
        int baseValue = NoteLetterToValue(noteLetter);
        if (baseValue < 0) return false;
        
        size_t pos = 1;
        int accidental = 0;
        if (pos < noteToken.length()) {
            if (noteToken[pos] == 'b') { accidental = -1; pos++; }
            else if (noteToken[pos] == '#' || noteToken[pos] == 's') { accidental = 1; pos++; }
        }
        
        int octave = 4; // Default octave if not specified
        if (pos < noteToken.length() && std::isdigit(noteToken[pos])) {
            octave = std::atoi(noteToken.substr(pos).c_str());
        }
        
        noteValue = (octave + 1) * 12 + baseValue + accidental;
        noteValue += g_fishsongConfig.shift;
        noteValue += g_fishsongConfig.localShift;
    }
    
    duration = isChordMember ? 0 : ParseDuration(durationToken);
    
    if (!volumeToken.empty()) {
        volume = static_cast<float>(std::atof(volumeToken.c_str()));
        if (volume < 0.0f) volume = g_fishsongConfig.volume;
    } else {
        volume = g_fishsongConfig.volume;
    }
    
    title = noteToken;
    return true;
}

int CSongEvent::ParseDuration(const std::string &durStr) {
    if (durStr.empty()) return 120;

    int totalDuration = 0;
    std::stringstream ss(durStr);
    std::string segment;
    while(std::getline(ss, segment, '+')) {
        segment = TrimString(segment);
        if(segment.empty()) continue;

        int baseDuration = 0;
        char symbol = segment[0];
        if (std::isdigit(symbol)) {
            baseDuration = std::atoi(segment.c_str());
        } else {
            baseDuration = GetDurationValue(symbol);
            if (segment.length() > 1) {
                if (segment[1] == 'd') { baseDuration = static_cast<int>(baseDuration * 1.5f); }
                else if (segment[1] == 't') { baseDuration = static_cast<int>(baseDuration * 2.0f / 3.0f); }
            }
        }
        totalDuration += baseDuration;
    }
    
    return (totalDuration <= 0) ? 120 : totalDuration;
}

void CSongEvent::FinalizeSongStructure() {
    if (!isChordMember && duration < 1) {
        duration = 1;
    }
}

void CSongEvent::FishsongCopyData(const CSongEvent &other) {
    duration = other.duration;
    volume = other.volume;
    SetIsChordMember(false);
}

// --- Trivial Accessors/Modifiers ---
bool CSongEvent::CheckSongCategory(const std::string &category) const {
    if (StrCaseCmp(category.c_str(), "short") == 0) return duration < 100;
    if (StrCaseCmp(category.c_str(), "long") == 0) return duration >= 150;
    if (StrCaseCmp(category.c_str(), "beethoven") == 0) return (noteValue >= 60 && noteValue <= 72);
    if (StrCaseCmp(category.c_str(), "kilgore") == 0) return volume < 0.5f;
    if (StrCaseCmp(category.c_str(), "santarare") == 0) return (noteValue > 72);
    return false;
}
int CSongEvent::GetNoteValue() const { return noteValue; }
int CSongEvent::GetDuration() const { return duration; }
float CSongEvent::GetVolume() const { return volume; }
const std::string &CSongEvent::GetTitle() const { return title; }
bool CSongEvent::IsChordMember() const { return isChordMember; }
void CSongEvent::SetTitle(const std::string &t) { title = t; }
void CSongEvent::SetDuration(int dur) { duration = dur; }
void CSongEvent::SetIsChordMember(bool val) { isChordMember = val; }

// --- CFishsongFile Implementation ---
CFishsongFile::CFishsongFile(const std::string &path) : filePath(path), currentTrack(1), waitingForTrack(0) {}
CFishsongFile::~CFishsongFile() {}

bool CFishsongFile::Load() {
#ifdef USE_SEXYAPP
    Buffer fileData;
    if (!gSexyAppBase->ReadBufferFromFile(filePath, &fileData)) { LogError("File not found: " + filePath); return false; }
    std::istringstream infile(std::string((char*)fileData.GetDataPtr(), fileData.GetDataLen()));
#else
    std::ifstream infile(filePath.c_str());
    if (!infile.is_open()) { LogError("File not found or could not be opened: " + filePath); return false; }
#endif
    
    std::string line;
    bool skipParsing = false;
    trackPositions[currentTrack] = 0;
    g_fishsongConfig.localShift = 0; 
    
    while (std::getline(infile, line)) {
        line = TrimString(line);
        if (line.empty() || line[0] == '#') continue;
        
        if (line[0] == '*') {
            if (line.length() >= 4 && StrCaseCmp(line.substr(0,4).c_str(), "*off") == 0) { skipParsing = true; }
            else if (line.length() >= 3 && StrCaseCmp(line.substr(0,3).c_str(), "*on") == 0) { skipParsing = false; }
            else { ProcessCommand(line); }
            continue;
        }
        
        if (skipParsing) continue;
        
        if (waitingForTrack > 0) {
            auto it = trackPositions.find(waitingForTrack);
            if (it == trackPositions.end() || trackPositions.at(currentTrack) > it->second) {
                continue; // Wait: skip this line and check again on the next.
            }
            waitingForTrack = 0; // Cleared to wait, can now proceed.
        }
        
        std::stringstream line_stream(line);
        std::string event_string;
        while(std::getline(line_stream, event_string, ',')) {
            event_string = TrimString(event_string);
            if (event_string.empty()) continue;

            CSongEvent event;
            if (event.ProcessSongEvent(event_string)) {
                ProcessEventWithChordHandling(event);
            } else {
                LogError("Invalid event token '" + event_string + "' in line: " + line);
            }
        }
    }
    
    FinalizePendingChord(120);
    return true;
}

void CFishsongFile::ProcessEventWithChordHandling(const CSongEvent &event) {
    if (event.IsChordMember()) {
        pendingChord.push_back(event);
    } else {
        if (!pendingChord.empty()) {
            for (auto & chord_note : pendingChord) {
                chord_note.FishsongCopyData(event);
                chord_note.FinalizeSongStructure();
                events.push_back(chord_note);
            }
            pendingChord.clear();
        }
        CSongEvent currentEvent = event;
        currentEvent.FinalizeSongStructure();
        events.push_back(currentEvent);
        trackPositions[currentTrack] += currentEvent.GetDuration();
    }
}

void CFishsongFile::FinalizePendingChord(int defaultDuration) {
    if (pendingChord.empty()) return;
    for (auto & finalEvent : pendingChord) {
        finalEvent.SetDuration(defaultDuration);
        finalEvent.SetIsChordMember(false);
        finalEvent.FinalizeSongStructure();
        events.push_back(finalEvent);
    }
    pendingChord.clear();
    trackPositions[currentTrack] += defaultDuration;
}

void CFishsongFile::ProcessCommand(const std::string &cmd) {
    std::string command = cmd.substr(1);
    command = TrimString(command);

    std::istringstream iss(command);
    std::string commandName, commandValue;
    iss >> commandName;
    std::getline(iss, commandValue);
    commandValue = TrimString(commandValue);

    if (commandName.empty()) return;
    
    if (StrCaseCmp(commandName.c_str(), "line") == 0) {
        int lineNum = std::atoi(commandValue.c_str());
        if (lineNum > 0) {
            FinalizePendingChord(120);
            currentTrack = lineNum;
            g_fishsongConfig.line = lineNum;
            g_fishsongConfig.localShift = 0;
            if (trackPositions.find(currentTrack) == trackPositions.end()) {
                trackPositions[currentTrack] = 0;
            }
        }
    } else if (StrCaseCmp(commandName.c_str(), "rest") == 0) {
        waitingForTrack = std::atoi(commandValue.c_str());
    } else {
        ParseFishSongCommand(commandName + " " + commandValue);
    }
}

const std::vector<CSongEvent>& CFishsongFile::GetEvents() const {
    return events;
}

// --- Unchanged Manager and Global Functions ---
CFishsongManager::CFishsongManager() : updateCounter(0) {}
CFishsongManager::~CFishsongManager() {}
bool CFishsongManager::UpdateState(int, int, int &counterOut, int cmdFlag) {
    counterOut = ++updateCounter;
    if (cmdFlag >= 0) LogInfo("Manager updated (counter = " + std::to_string(updateCounter) + ")");
    return true;
}
void CFishsongManager::DispatchEvent(const CSongEvent &event) {
    validatedEvents.push_back(event);
    LogInfo("Dispatched event: " + event.GetTitle() + ", dur: " + std::to_string(event.GetDuration()));
}
void CFishsongManager::FinalizeEventTitle(CSongEvent &event) {
    if (event.CheckSongCategory("long")) event.SetTitle(event.GetTitle() + " (extended)");
    else if (event.CheckSongCategory("santarare")) event.SetTitle(event.GetTitle() + " (holiday)");
}
void CFishsongManager::ResetState() {
    validatedEvents.clear();
    updateCounter = 0;
    LogInfo("Manager state reset.");
}
void InitFishsongEvents() {
    LogInfo("Fishsong events initialized.");
    g_fishsongConfig = { false, 1, 5.0f, 1.0f, 0, 0 };
}
void ClearFishsongBuffer() { LogInfo("Fishsong buffer cleared."); }
CFishsongFile* LoadFishsongFile(const std::string &filePath) {
    auto* file = new CFishsongFile(filePath);
    if (!file || !file->Load()) {
        delete file;
        return nullptr;
    }
    return file;
}
void ProcessFishsong(bool force) {
    static bool initialized = false;
    if (force || !initialized) {
        initialized = true;
        LogInfo("Processing fishsong files...");
    }
}
void ParseFishSongCommand(const std::string &commandStr) {
    std::istringstream iss(commandStr);
    std::string commandName, commandValue;
    iss >> commandName;
    std::getline(iss, commandValue);
    commandValue = TrimString(commandValue);
    if (commandName.empty()) return;
    if (StrCaseCmp(commandName.c_str(), "speed") == 0) g_fishsongConfig.speed = std::max(0.1f, static_cast<float>(atof(commandValue.c_str())));
    else if (StrCaseCmp(commandName.c_str(), "volume") == 0) g_fishsongConfig.volume = std::max(0.0f, std::min(1.0f, static_cast<float>(atof(commandValue.c_str()))));
    else if (StrCaseCmp(commandName.c_str(), "shift") == 0) g_fishsongConfig.shift = std::atoi(commandValue.c_str());
    else if (StrCaseCmp(commandName.c_str(), "localshift") == 0) g_fishsongConfig.localShift = std::atoi(commandValue.c_str());
    else if (StrCaseCmp(commandName.c_str(), "on") == 0) g_fishsongConfig.skip = false;
    else if (StrCaseCmp(commandName.c_str(), "off") == 0) g_fishsongConfig.skip = true;
    else if (StrCaseCmp(commandName.c_str(), "attrib") == 0) LogInfo("Global Attribute Set: " + commandValue);
    else LogError("Unrecognized global command: " + commandName);
}
