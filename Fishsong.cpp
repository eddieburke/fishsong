#include "Fishsong.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <map>

// Global Configuration Definition
FishsongConfig g_fishsongConfig = { false, 1, 5.0f, 1.0f, 0, 0 };

// Duration mapping for note duration symbols
static std::map<char, int> durationMap;

// Initialize the duration map
void InitDurationMap() {
    if (durationMap.empty()) {
        durationMap['w'] = 480;    // whole note
        durationMap['h'] = 240;    // half note
        durationMap['q'] = 120;    // quarter note
        durationMap['e'] = 60;     // eighth note
        durationMap['s'] = 30;     // sixteenth note
        durationMap['t'] = 20;     // triplet (1/12)
        durationMap['z'] = 15;     // 32nd note
    }
}

// Helper function for case-insensitive string comparison
int strcasecmp(const char* s1, const char* s2) {
    while(*s1 && (tolower(*s1) == tolower(*s2))) {
        s1++;
        s2++;
    }
    return tolower(*(const unsigned char*)s1) - tolower(*(const unsigned char*)s2);
}

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

// Helper function to trim whitespace
std::string TrimString(const std::string &s) {
    std::string result = s;
    // Remove leading whitespace
    std::string::size_type start = result.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    
    // Remove trailing whitespace
    std::string::size_type end = result.find_last_not_of(" \t");
    if (end != std::string::npos) {
        result = result.substr(start, end - start + 1);
    } else {
        result = result.substr(start);
    }
    
    return result;
}

// CSongEvent Implementation
CSongEvent::CSongEvent() : noteValue(0), duration(0), volume(1.0f), title(""), isChordMember(false) {}

bool CSongEvent::ProcessSongEvent(const std::string &eventStr) {
    // Initialize duration map if needed
    InitDurationMap();
    
    std::string noteToken, durationToken, volumeToken;
    
    // Try to parse by comma first
    std::istringstream iss(eventStr);
    if (std::getline(iss, noteToken, ',')) {
        std::getline(iss, durationToken, ',');
        std::getline(iss, volumeToken, ',');
    } else {
        // If no commas, try space-delimited parsing
        iss.clear();
        iss.str(eventStr);
        iss >> noteToken >> durationToken >> volumeToken;
    }
    
    // Trim whitespace
    noteToken = TrimString(noteToken);
    durationToken = TrimString(durationToken);
    volumeToken = TrimString(volumeToken);
    
    if (noteToken.empty()) return false;
    
    // Check if this is a chord member (duration 0)
    isChordMember = (durationToken == "0");
    
    // Process note
    if (std::tolower(noteToken[0]) == 'r') {
        // This is a rest
        noteValue = -1;
    } else {
        char noteLetter = noteToken[0];
        int baseValue = NoteLetterToValue(noteLetter);
        if (baseValue < 0) return false;
        
        // Check for accidentals
        size_t pos = 1;
        int accidental = 0;
        
        if (pos < noteToken.length()) {
            if (noteToken[pos] == 'b') {
                // Always treat 'b' as flat, regardless of note letter
                accidental = -1;
                pos++;
            } else if (noteToken[pos] == '#' || noteToken[pos] == 's') {
                // Sharp accidental (support both '#' and 's')
                accidental = 1;
                pos++;
            }
        }
        
        // Parse octave - default is 3
        int octave = 3;
        if (pos < noteToken.length() && std::isdigit(noteToken[pos])) {
            octave = std::atoi(noteToken.substr(pos).c_str());
        }
        
        // Calculate MIDI-style note value
        noteValue = (octave + 1) * 12 + baseValue + accidental;
        
        // Apply transposition
        noteValue += g_fishsongConfig.shift;
        noteValue += g_fishsongConfig.localShift;
    }
    
    // Process duration
    if (!isChordMember) {
        duration = ParseDuration(durationToken);
    }
    
    // Process volume
    if (!volumeToken.empty()) {
        volume = static_cast<float>(std::atof(volumeToken.c_str()));
        if (volume < 0.0f) volume = 1.0f;
    } else {
        volume = g_fishsongConfig.volume;
    }
    
    // Store original title
    title = noteToken;
    
    return true;
}

int CSongEvent::ParseDuration(const std::string &durStr) {
    if (durStr.empty()) return 120; // Default to quarter note
    
    int totalDuration = 0;
    size_t pos = 0;
    
    // Process complex durations like "q+e", "st", "qd"
    while (pos < durStr.length()) {
        int baseDuration = 0;
        
        // Case 1: Numeric duration
        if (std::isdigit(durStr[pos])) {
            size_t numStart = pos;
            while (pos < durStr.length() && std::isdigit(durStr[pos])) pos++;
            baseDuration = std::atoi(durStr.substr(numStart, pos - numStart).c_str());
        }
        // Case 2: Symbol duration (q, h, w, etc.)
        else if (durationMap.find(durStr[pos]) != durationMap.end()) {
            baseDuration = durationMap[durStr[pos]];
            pos++;
            
            // Check for modifiers (d for dotted, t for triplet)
            if (pos < durStr.length()) {
                if (durStr[pos] == 'd') {
                    // Dotted note (1.5x duration)
                    baseDuration = static_cast<int>(baseDuration * 1.5f);
                    pos++;
                } else if (durStr[pos] == 't') {
                    // Triplet (2/3 duration)
                    baseDuration = static_cast<int>(baseDuration * 2.0f / 3.0f);
                    pos++;
                }
            }
        } else {
            // Unknown symbol, skip it
            pos++;
        }
        
        // Add to total duration
        totalDuration += baseDuration;
        
        // Skip the + if present
        if (pos < durStr.length() && durStr[pos] == '+') {
            pos++;
        }
    }
    
    // If no valid duration found, default to quarter note
    if (totalDuration <= 0) totalDuration = 120;
    
    return totalDuration;
}

void CSongEvent::FinalizeSongStructure() {
    // Ensure a minimum duration for non-chord members
    if (!isChordMember && duration < 1) {
        duration = 1;
    }
}

bool CSongEvent::CheckSongCategory(const std::string &category) const {
    if (category == "short")
        return duration < 100;
    else if (category == "long")
        return duration >= 150;
    else if (category == "beethoven")
        return (noteValue >= 60 && noteValue <= 72);
    else if (category == "kilgore")
        return volume < 0.5f;
    else if (category == "santarare")
        return (noteValue > 72);
    return false;
}

void CSongEvent::FishsongCopyData(const CSongEvent &other) {
    // This copies just the duration and volume, but keeps the note value
    // Used for chord notes when assigning them their final durations
    duration = other.duration;
    volume = other.volume;
    isChordMember = false; // No longer a chord member
}

int CSongEvent::GetNoteValue() const { return noteValue; }
int CSongEvent::GetDuration() const { return duration; }
float CSongEvent::GetVolume() const { return volume; }
const std::string &CSongEvent::GetTitle() const { return title; }
bool CSongEvent::IsChordMember() const { return isChordMember; }
void CSongEvent::SetTitle(const std::string &t) { title = t; }
void CSongEvent::SetDuration(int dur) { duration = dur; }

// CFishsongFile Implementation
CFishsongFile::CFishsongFile(const std::string &path) : filePath(path), currentTrack(1) {}

CFishsongFile::~CFishsongFile() {
    // No resources to clean up
}

bool CFishsongFile::Load() {
    std::ifstream infile(filePath.c_str());
    if (!infile) {
        std::cerr << "File not found: " << filePath << std::endl;
        return false;
    }
    
    std::string line;
    bool skipParsing = false;
    int waitingForTrack = 0;
    
    // Initialize track positions
    trackPositions[currentTrack] = 0;
    
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        
        // Skip comment lines
        if (line[0] == '#') continue;
        
        // Process commands
        if (line[0] == '*') {
            // Check for on/off parsing commands first
            if (line.find("*off") == 0) {
                skipParsing = true;
                continue;
            } else if (line.find("*on") == 0) {
                skipParsing = false;
                continue;
            }
            
            // Process other commands
            ProcessCommand(line);
            continue;
        }
        
        // Skip if parsing is disabled
        if (skipParsing) continue;
        
        // Check if we need to wait for another track
        if (waitingForTrack > 0) {
            if (trackPositions.find(waitingForTrack) != trackPositions.end() && 
                trackPositions[waitingForTrack] >= trackPositions[currentTrack]) {
                // We've caught up to the track we were waiting for
                waitingForTrack = 0;
            } else {
                // Still waiting for the other track
                continue;
            }
        }
        
        // Process the event line
        CSongEvent event;
        if (event.ProcessSongEvent(line)) {
            // Handle chord building logic
            if (event.IsChordMember()) {
                // Add to pending chord
                pendingChord.push_back(event);
            } else {
                // Is this a rest?
                if (event.GetNoteValue() == -1) {
                    // For a rest, don't finalize chord yet, just add the rest
                    event.FinalizeSongStructure();
                    events.push_back(event);
                    
                    // Update track position
                    trackPositions[currentTrack] += event.GetDuration();
                } else {
                    // This is a regular note - finalize any pending chord
                    if (!pendingChord.empty()) {
                        for (size_t i = 0; i < pendingChord.size(); i++) {
                            // Copy duration and volume from the current note
                            pendingChord[i].FishsongCopyData(event);
                            pendingChord[i].FinalizeSongStructure();
                            events.push_back(pendingChord[i]);
                        }
                        pendingChord.clear();
                    }
                    
                    // Add the current note
                    event.FinalizeSongStructure();
                    events.push_back(event);
                    
                    // Update track position
                    trackPositions[currentTrack] += event.GetDuration();
                }
            }
        } else {
            std::cerr << "Invalid event line: " << line << std::endl;
        }
    }
    
    // Handle any pending chord notes at the end
    if (!pendingChord.empty()) {
        for (size_t i = 0; i < pendingChord.size(); i++) {
            // Use the default duration (quarter note)
            pendingChord[i].SetDuration(120);
            pendingChord[i].FinalizeSongStructure();
            events.push_back(pendingChord[i]);
        }
        pendingChord.clear();
    }
    
    return true;
}

void CFishsongFile::ProcessCommand(const std::string &cmd) {
    // Cleanup String
    std::string command = cmd;
    if (!command.empty() && command[0] == '*') {
        command.erase(0, 1);
    }
    
    std::istringstream iss(command);
    std::string commandName, commandValue;
    iss >> commandName >> commandValue;
    
    if (commandName.empty()) return;
    
    // Handle file-specific commands
    if (strcasecmp(commandName.c_str(), "line") == 0) {
        int lineNum = std::atoi(commandValue.c_str());
        if (lineNum > 0) {
            // Update current track
            currentTrack = lineNum;
            g_fishsongConfig.line = lineNum;
            
            // Initialize track position if not already done
            if (trackPositions.find(currentTrack) == trackPositions.end()) {
                trackPositions[currentTrack] = 0;
            }
        }
    } else if (strcasecmp(commandName.c_str(), "rest") == 0) {
        int trackToWaitFor = std::atoi(commandValue.c_str());
        if (trackToWaitFor > 0 && trackToWaitFor != currentTrack) {
            // We need to wait for another track to catch up
            if (trackPositions.find(trackToWaitFor) == trackPositions.end()) {
                // If the track doesn't exist yet, initialize it at 0
                trackPositions[trackToWaitFor] = 0;
            }
            
            if (trackPositions[trackToWaitFor] < trackPositions[currentTrack]) {
                // Only wait if this track is ahead
                std::cout << "Track " << currentTrack << " waiting for track " << trackToWaitFor << std::endl;
                waitingForTrack = trackToWaitFor;
                pendingChord.clear(); // Clear any pending chord when waiting
            }
        }
    } else {
        // Forward other commands to the global parser
        ParseFishSongCommand(cmd);
    }
}

const std::vector<CSongEvent>& CFishsongFile::GetEvents() const {
    return events;
}

// CFishsongManager Implementation
CFishsongManager::CFishsongManager() : updateCounter(0) {}

CFishsongManager::~CFishsongManager() {
    // Nothing to see here
}

bool CFishsongManager::UpdateState(int someFlag, int extraParam, int &counterOut, int cmdFlag) {
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
    // For example, if an event qualifies as "long", append " (extended)".
    if (event.CheckSongCategory("long"))
        event.SetTitle(event.GetTitle() + " (extended)");
    else if (event.CheckSongCategory("santarare"))
        event.SetTitle(event.GetTitle() + " (holiday)");
}

void CFishsongManager::ResetState() {
    validatedEvents.clear();
    updateCounter = 0;
    std::cout << "Manager state reset." << std::endl;
}

// ~~~~~~~
// Global Utility Functions
// ~~~~~~~

void InitFishsongEvents() {
    std::cout << "Fishsong events initialized." << std::endl;
    // Reset global configuration
    g_fishsongConfig.skip = false;
    g_fishsongConfig.line = 1;
    g_fishsongConfig.speed = 5.0f;
    g_fishsongConfig.volume = 1.0f;
    g_fishsongConfig.shift = 0;
    g_fishsongConfig.localShift = 0;
    
    // Initialize the duration map
    InitDurationMap();
}

void ClearFishsongBuffer() {
    // In a real system, free any allocated buffers.
    std::cout << "Fishsong buffer cleared." << std::endl;
}

CFishsongFile* LoadFishsongFile(const std::string &filePath) {
    CFishsongFile* file = new CFishsongFile(filePath);
    if (!file->Load()) {
        delete file;
        return NULL; // Use NULL instead of nullptr for VS2003 compatibility
    }
    return file;
}

void ProcessFishsong(bool force) {
    static bool initialized = false;
    if (force || !initialized) {
        initialized = true;
        std::cout << "Processing fishsong files..." << std::endl;
        
        // Get all files from fishsong folder
        std::string folderPath = "fishsongs/";
        std::vector<std::string> files;
        
        // TODO: In Sexyapp framework, I'm sure there's a proper way to do it, but I don't care
        
        CFishsongManager manager;
        for (size_t i = 0; i < files.size(); i++) {
            std::cout << "Loading file: " << files[i] << std::endl;
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
    // Cleanup the string
    std::string cmd = commandStr;
    if (!cmd.empty() && cmd[0] == '*')
        cmd.erase(0, 1);
    
    std::istringstream iss(cmd);
    std::string commandName, commandValue;
    iss >> commandName >> commandValue;
    
    if (commandName.empty())
        return;
    
    // Commands.
    if (strcasecmp(commandName.c_str(), "skip") == 0) {
        g_fishsongConfig.skip = (strcasecmp(commandValue.c_str(), "true") == 0);
    } else if (strcasecmp(commandName.c_str(), "speed") == 0) {
        float speed = static_cast<float>(std::atof(commandValue.c_str()));
        
        if (speed == 0.0f) {
            g_fishsongConfig.speed = 5.0f; 
        } else {
            g_fishsongConfig.speed = speed;
        }
    } else if (strcasecmp(commandName.c_str(), "volume") == 0) {
        float volume = static_cast<float>(std::atof(commandValue.c_str()));
        // Clamp volume to 0.0-1.0 range
        g_fishsongConfig.volume = std::max(0.0f, std::min(1.0f, volume));
    } else if (strcasecmp(commandName.c_str(), "shift") == 0) {
        g_fishsongConfig.shift = std::atoi(commandValue.c_str());
    } else if (strcasecmp(commandName.c_str(), "localshift") == 0) {
        g_fishsongConfig.localShift = std::atoi(commandValue.c_str());
    } else if (strcasecmp(commandName.c_str(), "on") == 0) {
        g_fishsongConfig.skip = false;
    } else if (strcasecmp(commandName.c_str(), "off") == 0) {
        g_fishsongConfig.skip = true;
    } else if (strcasecmp(commandName.c_str(), "attrib") == 0) {
        // Process attribute settings - for logging purposes
        std::string attrValue;
        std::getline(iss, attrValue);
        std::cout << "Attribute: " << commandValue << attrValue << std::endl;
    } else {
        std::cerr << "Unrecognized command: " << commandName << std::endl;
    }
    
    std::cout << "Parsed command: " << commandName << " = " << commandValue << std::endl;
}
