#include "Fishsong.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cctype>

// Global Configuration Definition
FishsongConfig g_fishsongConfig = { false, 0, 1.0f, 1.0f, 0, 0 };

// Helper: Convert a note letter to its base value
int NoteLetterToValue(char note) {
    switch (std::tolower(note)) {
        case 'c': return 0;
        case 'd': return 2;
        case 'e': return 4;
        case 'f': return 5;
        case 'g': return 7;
        case 'a': return 9;
        case 'b': return 11;
        default:  return -1;
    }
}

// CSongEvent Implementation
CSongEvent::CSongEvent() : noteValue(0), duration(0), volume(1.0f), title("") {}

bool CSongEvent::ProcessSongEvent(const std::string &eventStr) {
    std::istringstream iss(eventStr);
    std::string noteToken, durationToken, volumeToken;
    
    if (!std::getline(iss, noteToken, ',')) return false;
    if (!std::getline(iss, durationToken, ',')) return false;
    std::getline(iss, volumeToken, ','); // volumeToken may be empty

    // Simple whitespace trim
    auto trim = [](std::string &s) {
        std::string::size_type start = s.find_first_not_of(" \t");
        std::string::size_type end = s.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos)
            s = s.substr(start, end - start + 1);
    };
    
    trim(noteToken);
    trim(durationToken);
    trim(volumeToken);

    if (noteToken.empty()) return false;

    // Process the note
    if (std::tolower(noteToken[0]) == 'r') {
        noteValue = -1; // -1 indicates a rest
    } else {
        char noteLetter = noteToken[0];
        int baseValue = NoteLetterToValue(noteLetter);
        if (baseValue < 0) return false;
        
        int accidental = 0;
        size_t pos = 1;
        
        if (noteToken.size() > pos) {
            if (noteToken[pos] == '#') { accidental = 1; pos++; }
            else if (noteToken[pos] == 'b') { accidental = -1; pos++; }
        }
        
        // Parse octave if provided; default is 4
        int octave = 4;
        if (noteToken.size() > pos)
            octave = std::atoi(noteToken.substr(pos).c_str());
            
        noteValue = (octave + 1) * 12 + baseValue + accidental;
    }

    // Process duration
    duration = std::atoi(durationToken.c_str());
    if (duration <= 0)
        duration = 120; // default duration

    // Process volume
    if (!volumeToken.empty()) {
        volume = static_cast<float>(std::atof(volumeToken.c_str()));
        if (volume < 0.0f)
            volume = 1.0f;
    } else {
        volume = g_fishsongConfig.volume;
    }

    // Set title to the original note token
    title = noteToken;
    return true;
}

void CSongEvent::FinalizeSongStructure() {
    // Ensure a minimum duration
    if (duration < 1)
        duration = 1;
}

bool CSongEvent::CheckSongCategory(const std::string &category) const {
    if (category == "short")
        return duration < 100;
    else if (category == "long")
        return duration >= 150;
    else if (category == "beethoven")
        return (noteValue >= 60 && noteValue <= 72); // arbitrary range 
    else if (category == "kilgore")
        return volume < 0.5f;
    return false;
}

void CSongEvent::FishsongCopyData(const CSongEvent &other) {
    noteValue = other.noteValue;
    duration  = other.duration;
    volume    = other.volume;
    title     = other.title;
}

int CSongEvent::GetNoteValue() const { return noteValue; }
int CSongEvent::GetDuration() const { return duration; }
float CSongEvent::GetVolume() const { return volume; }
const std::string &CSongEvent::GetTitle() const { return title; }
void CSongEvent::SetTitle(const std::string &t) { title = t; }

// CFishsongFile Implementation
CFishsongFile::CFishsongFile(const std::string &path) : filePath(path) {}

CFishsongFile::~CFishsongFile() {
    // Nothing to do here
}

bool CFishsongFile::Load() {
    std::ifstream infile(filePath.c_str());
    if (!infile) {
        std::cerr << "File not found: " << filePath << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty())
            continue;
            
        if (line[0] == '#')
            continue;  // comment line
            
        if (line[0] == '*') {
            // Process command lines
            ParseFishSongCommand(line);
            continue;
        }
        
        CSongEvent event;
        if (event.ProcessSongEvent(line)) {
            event.FinalizeSongStructure();
            events.push_back(event);
        } else {
            std::cerr << "Invalid event line: " << line << std::endl;
        }
    }
    return true;
}

const std::vector<CSongEvent>& CFishsongFile::GetEvents() const {
    return events;
}

// CFishsongManager Implementation
CFishsongManager::CFishsongManager() : updateCounter(0) {}

CFishsongManager::~CFishsongManager() {
    // Cleanup, if needed
}

bool CFishsongManager::UpdateState(int someFlag, int extraParam, int &counterOut, int cmdFlag) {
    // If cmdFlag is negative, perform alternate handling
    if (cmdFlag < 0) {
        counterOut = ++updateCounter;
        return true;
    }
    
    counterOut = ++updateCounter;
    std::cout << "Manager updated (counter = " << updateCounter << ")." << std::endl;
    return true;
}

void CFishsongManager::DispatchEvent(const CSongEvent &event) {
    validatedEvents.push_back(event);
    std::cout << "Dispatched event: " << event.GetTitle() << std::endl;
}

void CFishsongManager::FinalizeEventTitle(CSongEvent &event) {
    // If an event qualifies as "long", append " (extended)"
    if (event.CheckSongCategory("long"))
        event.SetTitle(event.GetTitle() + " (extended)");
}

void CFishsongManager::ResetState() {
    validatedEvents.clear();
    updateCounter = 0;
    std::cout << "Manager state reset." << std::endl;
}

// ----
// Global Utility Functions
// -----
void InitFishsongEvents() {
    std::cout << "Fishsong events initialized." << std::endl;
}

void ClearFishsongBuffer() {
    std::cout << "Fishsong buffer cleared." << std::endl;
}

CFishsongFile* LoadFishsongFile(const std::string &filePath) {
    CFishsongFile* file = new CFishsongFile(filePath);
    if (!file->Load()) {
        delete file;
        return 0;
    }
    return file;
}

void ProcessFishsong(bool force) {
    static bool initialized = false;
    if (force || !initialized) {
        initialized = true;
        std::cout << "Processing fishsong files..." << std::endl;
        
        // For this example, use a fixed list of filenames
        std::vector<std::string> files;
        files.push_back("fishsong1.txt");
        files.push_back("fishsong2.txt");
        
        CFishsongManager manager;
        for (size_t i = 0; i < files.size(); i++) {
            CFishsongFile* fsFile = LoadFishsongFile(files[i]);
            if (fsFile) {
                const std::vector<CSongEvent> &events = fsFile->GetEvents();
                for (size_t j = 0; j < events.size(); j++) {
                    if (g_fishsongConfig.skip)
                        continue;
                        
                    manager.DispatchEvent(events[j]);
                    CSongEvent eventCopy = events[j];
                    manager.FinalizeEventTitle(eventCopy);
                    std::cout << "Finalized event: " << eventCopy.GetTitle() << std::endl;
                }
                delete fsFile;
            }
        }
        
        int counter;
        manager.UpdateState(0, 0, counter, 0);
    }
}

void ParseFishSongCommand(const std::string &commandStr) {
    // Remove the leading '*' and any extra whitespace
    std::string cmd = commandStr;
    if (!cmd.empty() && cmd[0] == '*')
        cmd.erase(0, 1);
        
    std::istringstream iss(cmd);
    std::string commandName, commandValue;
    iss >> commandName >> commandValue;
    
    if (commandName.empty())
        return;
        
    // Process supported commands
    if (strcasecmp(commandName.c_str(), "skip") == 0) {
        g_fishsongConfig.skip = (strcasecmp(commandValue.c_str(), "true") == 0);
    } else if (strcasecmp(commandName.c_str(), "line") == 0) {
        g_fishsongConfig.line = std::atoi(commandValue.c_str()) - 1;
    } else if (strcasecmp(commandName.c_str(), "speed") == 0) {
        g_fishsongConfig.speed = static_cast<float>(std::atof(commandValue.c_str()));
    } else if (strcasecmp(commandName.c_str(), "volume") == 0) {
        g_fishsongConfig.volume = static_cast<float>(std::atof(commandValue.c_str()));
    } else if (strcasecmp(commandName.c_str(), "shift") == 0) {
        g_fishsongConfig.shift = std::atoi(commandValue.c_str());
    } else if (strcasecmp(commandName.c_str(), "localshift") == 0) {
        g_fishsongConfig.localShift = std::atoi(commandValue.c_str());
    } else if (strcasecmp(commandName.c_str(), "on") == 0) {
        g_fishsongConfig.skip = false;
    } else if (strcasecmp(commandName.c_str(), "off") == 0) {
        g_fishsongConfig.skip = true;
    } else {
        std::cerr << "Unrecognized command: " << commandName << std::endl;
    }
    
    std::cout << "Parsed command: " << commandName << " = " << commandValue << std::endl;
}
