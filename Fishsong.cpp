#include "Fishsong.h"
#include <iostream> // For std::cout, std::cerr (placeholder for SexyApp logging)
#include <fstream>  // For std::ifstream
#include <sstream>  // For std::istringstream
#include <cstdlib>  // For std::atoi, std::atof
#include <cctype>   // For std::tolower, std::isdigit
// #include <cstring> // Not strictly needed for the custom strcasecmp or other parts
#include <algorithm>// For std::max, std::min
// #include <cmath>   // Not used

// Global Configuration Definition
FishsongConfig g_fishsongConfig = { false, 1, 5.0f, 1.0f, 0, 0 };

// Helper function for case-insensitive string comparison
// (Definition provided here as it's a utility)
int strcasecmp(const char* s1, const char* s2) {
    while(*s1 && (std::tolower(static_cast<unsigned char>(*s1)) == std::tolower(static_cast<unsigned char>(*s2)))) {
        s1++;
        s2++;
    }
    return std::tolower(static_cast<unsigned char>(*s1)) - std::tolower(static_cast<unsigned char>(*s2));
}

// Get duration value from symbol using switch case
int CSongEvent::GetDurationValue(char symbol) {
    switch(std::tolower(symbol)) { // No need for static_cast for char argument to tolower
        case 'w': return 480;    // whole note
        case 'h': return 240;    // half note
        case 'q': return 120;    // quarter note
        case 'e': return 60;     // eighth note
        case 's': return 30;     // sixteenth note
        case 't': return 15;     // 32nd note
        case 'z': return 7;      // 64th note
        default:  return 0;      // invalid symbol
    }
}

// Helper: Convert a note letter to its base value
int NoteLetterToValue(char note) { // This could be a static member of CSongEvent or remain a free function
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
    // Using std::string::npos for comparisons is standard
    std::string::size_type start = s.find_first_not_of(" \t");
    if (start == std::string::npos) { // String is empty or all whitespace
        return "";
    }
    
    std::string::size_type end = s.find_last_not_of(" \t");
    // 'end' will be valid if 'start' was valid and string wasn't all whitespace.
    // 'end' will be >= 'start'.
    return s.substr(start, end - start + 1);
}

// CSongEvent Implementation
CSongEvent::CSongEvent() : noteValue(0), duration(0), volume(1.0f), title(""), isChordMember(false) {}

bool CSongEvent::ProcessSongEvent(const std::string &eventStr) {
    std::string noteToken, durationToken, volumeToken;
    
    std::istringstream iss(eventStr);
    // Try to parse by comma first
    if (std::getline(iss, noteToken, ',')) {
        // If noteToken was read, proceed to read others, even if they might be empty
        std::getline(iss, durationToken, ',');
        std::getline(iss, volumeToken, ','); // Read up to next comma or end of stream
    } else {
        // If no commas (getline failed to find a comma for noteToken or string was empty)
        // try space-delimited parsing
        iss.clear(); // Clear error flags (like eof if first getline read the whole string)
        iss.str(eventStr); // Reset stream with the original string
        iss >> noteToken >> durationToken >> volumeToken; // Extracts space-separated tokens
    }
    
    noteToken = TrimString(noteToken);
    durationToken = TrimString(durationToken);
    volumeToken = TrimString(volumeToken);
    
    if (noteToken.empty()) return false;
    
    isChordMember = (durationToken == "0");
    
    if (std::tolower(noteToken[0]) == 'r') {
        noteValue = -1; // Rest
    } else {
        char noteLetter = noteToken[0];
        int baseValue = NoteLetterToValue(noteLetter);
        if (baseValue < 0) return false; // Invalid note letter
        
        size_t pos = 1;
        int accidental = 0;
        
        if (pos < noteToken.length()) {
            if (noteToken[pos] == 'b') {
                accidental = -1;
                pos++;
            } else if (noteToken[pos] == '#' || noteToken[pos] == 's') {
                accidental = 1;
                pos++;
            }
        }
        
        int octave = 3; // Default octave
        if (pos < noteToken.length() && std::isdigit(noteToken[pos])) {
            // Be careful with substr if pos is at the end.
            // atoi handles empty string as 0, which might be acceptable or not.
            // If noteToken.substr(pos) is empty, atoi returns 0.
            octave = std::atoi(noteToken.substr(pos).c_str());
        }
        
        noteValue = (octave + 1) * 12 + baseValue + accidental;
        noteValue += g_fishsongConfig.shift;
        noteValue += g_fishsongConfig.localShift;
    }
    
    if (!isChordMember) {
        duration = ParseDuration(durationToken);
    } else {
        duration = 0; // Explicitly set duration for chord members
    }
    
    if (!volumeToken.empty()) {
        volume = static_cast<float>(std::atof(volumeToken.c_str()));
        if (volume < 0.0f) volume = 1.0f; // Default to 1.0 if negative (or use g_fishsongConfig.volume)
    } else {
        volume = g_fishsongConfig.volume; // Use global default if no volume specified
    }
    
    title = noteToken; // Store original note token as title
    
    return true;
}

int CSongEvent::ParseDuration(const std::string &durStr) {
    if (durStr.empty()) return 120; // Default to quarter note

    int totalDuration = 0;
    size_t pos = 0;
    std::string currentDurStr = durStr; // Use a copy for potential modifications or easier parsing

    while (pos < currentDurStr.length()) {
        int baseDuration = 0;
        
        // Skip leading whitespace
        while (pos < currentDurStr.length() && (currentDurStr[pos] == ' ' || currentDurStr[pos] == '\t')) {
            pos++;
        }
        if (pos >= currentDurStr.length()) break; // End of string

        // Case 1: Numeric duration
        if (std::isdigit(currentDurStr[pos])) {
            size_t numStart = pos;
            while (pos < currentDurStr.length() && std::isdigit(currentDurStr[pos])) pos++;
            baseDuration = std::atoi(currentDurStr.substr(numStart, pos - numStart).c_str());
        }
        // Case 2: Symbol duration
        else { // Not a digit, try symbol
            baseDuration = GetDurationValue(currentDurStr[pos]);
            if (baseDuration > 0) {
                pos++;
                if (pos < currentDurStr.length()) {
                    if (currentDurStr[pos] == 'd') {
                        baseDuration = static_cast<int>(baseDuration * 1.5f);
                        pos++;
                    } else if (currentDurStr[pos] == 't') {
                        baseDuration = static_cast<int>(baseDuration * 2.0f / 3.0f);
                        pos++;
                    }
                }
            } else {
                pos++; // Unknown symbol, skip it
            }
        }
        
        totalDuration += baseDuration;
        
        // Skip whitespace before potential '+'
        while (pos < currentDurStr.length() && (currentDurStr[pos] == ' ' || currentDurStr[pos] == '\t')) {
            pos++;
        }
        
        if (pos < currentDurStr.length() && currentDurStr[pos] == '+') {
            pos++; // Skip '+'
            // Skip whitespace after '+' (handled by loop start)
        } else {
            // If no '+', we might be done with this part or the string
            // The loop condition (pos < currentDurStr.length()) will handle termination
        }
    }
    
    return (totalDuration <= 0) ? 120 : totalDuration;
}

void CSongEvent::FinalizeSongStructure() {
    if (!isChordMember && duration < 1) {
        duration = 1; // Minimum duration for non-chord members
    }
}

bool CSongEvent::CheckSongCategory(const std::string &category) const {
    // Using strcasecmp for case-insensitivity, good for older systems
    if (strcasecmp(category.c_str(), "short") == 0)
        return duration < 100;
    else if (strcasecmp(category.c_str(), "long") == 0)
        return duration >= 150;
    else if (strcasecmp(category.c_str(), "beethoven") == 0)
        return (noteValue >= 60 && noteValue <= 72); // C4 to C5
    else if (strcasecmp(category.c_str(), "kilgore") == 0)
        return volume < 0.5f;
    else if (strcasecmp(category.c_str(), "santarare") == 0)
        return (noteValue > 72); // Notes above C5
    return false;
}

void CSongEvent::FishsongCopyData(const CSongEvent &other) {
    duration = other.duration;
    volume = other.volume;
    SetIsChordMember(false); // Explicitly mark as not a chord member after copying real duration/volume
}

int CSongEvent::GetNoteValue() const { return noteValue; }
int CSongEvent::GetDuration() const { return duration; }
float CSongEvent::GetVolume() const { return volume; }
const std::string &CSongEvent::GetTitle() const { return title; }
bool CSongEvent::IsChordMember() const { return isChordMember; }
void CSongEvent::SetTitle(const std::string &t) { title = t; }
void CSongEvent::SetDuration(int dur) { duration = dur; }
void CSongEvent::SetIsChordMember(bool val) { isChordMember = val; }


// CFishsongFile Implementation
CFishsongFile::CFishsongFile(const std::string &path) : filePath(path), currentTrack(1), waitingForTrack(0) {
    // trackPositions will be populated as tracks are encountered
}

CFishsongFile::~CFishsongFile() {
    // No explicit dynamic memory to clean up within this class instance
}

bool CFishsongFile::Load() {
    std::ifstream infile(filePath.c_str());
    if (!infile.is_open()) { // Check with is_open() for robustness
        std::cerr << "File not found or could not be opened: " << filePath << std::endl;
        return false;
    }
    
    std::string line;
    bool skipParsing = false;
    
    // Initialize current track's position and global localShift
    trackPositions[currentTrack] = 0;
    g_fishsongConfig.localShift = 0; 
    
    while (std::getline(infile, line)) {
        line = TrimString(line); // Trim line once at the beginning
        if (line.empty()) continue;
        
        if (line[0] == '#') continue; // Skip comment lines
        
        if (line[0] == '*') {
            if (line.length() > 3 && strcasecmp(line.substr(0,4).c_str(), "*off") == 0) { // Be safer with substr
                 skipParsing = true;
                 continue;
            } else if (line.length() > 2 && strcasecmp(line.substr(0,3).c_str(), "*on") == 0) {
                 skipParsing = false;
                 continue;
            }
            ProcessCommand(line);
            continue;
        }
        
        if (skipParsing) continue;
        
        if (waitingForTrack > 0) {
            // Check if the track we are waiting for exists and has caught up
            std::map<int, int>::iterator it = trackPositions.find(waitingForTrack);
            if (it != trackPositions.end() && it->second >= trackPositions[currentTrack]) {
                waitingForTrack = 0; // Caught up
            } else {
                // Still waiting, or waitingForTrack doesn't exist yet (implies it hasn't started)
                // If it doesn't exist, we must wait for it to appear and then catch up.
                // This event line is skipped for now.
                // To prevent infinite loops with poorly formed files, one might add a timeout or max wait count.
                // For this code, we assume files are well-formed or this is the intended behavior.
                continue; 
            }
        }
        
        CSongEvent event;
        if (event.ProcessSongEvent(line)) {
            ProcessEventWithChordHandling(event);
        } else {
            std::cerr << "Invalid event line: " << line << std::endl;
        }
    }
    
    FinalizePendingChord(120); // Default to quarter note for any remaining chord
    
    // infile.close(); // ifstream closes automatically on destruction
    return true;
}

void CFishsongFile::ProcessEventWithChordHandling(const CSongEvent &event) {
    if (event.IsChordMember()) {
        pendingChord.push_back(event);
    } else {
        // If there was a pending chord, finalize it using the current event's properties (if it's not a rest)
        // or a default if it is a rest.
        if (!pendingChord.empty()) {
            if (event.GetNoteValue() != -1) { // Current event is a note, use its duration/volume for chord
                for (size_t i = 0; i < pendingChord.size(); ++i) {
                    CSongEvent chordEvent = pendingChord[i];
                    chordEvent.FishsongCopyData(event); // This sets isChordMember to false
                    chordEvent.FinalizeSongStructure();
                    events.push_back(chordEvent);
                }
            } else { // Current event is a rest, finalize chord with default duration
                FinalizePendingChord(120); // Or some other sensible default like last note's duration
            }
            pendingChord.clear();
        }

        // Add the current event (note or rest)
        CSongEvent currentEvent = event; // Make a mutable copy
        currentEvent.FinalizeSongStructure();
        events.push_back(currentEvent);
        
        // Update track position for the current track
        // Ensure currentTrack actually exists in map (should be by now)
        if (trackPositions.find(currentTrack) != trackPositions.end()) {
             trackPositions[currentTrack] += currentEvent.GetDuration();
        } else {
            // This case should ideally not happen if currentTrack is always initialized
            std::cerr << "Error: currentTrack " << currentTrack << " not in trackPositions map." << std::endl;
            trackPositions[currentTrack] = currentEvent.GetDuration(); // Initialize if missing
        }
    }
}

void CFishsongFile::FinalizePendingChord(int defaultDuration) {
    if (!pendingChord.empty()) {
        for (size_t i = 0; i < pendingChord.size(); ++i) {
            CSongEvent finalEvent = pendingChord[i];
            finalEvent.SetDuration(defaultDuration);
            finalEvent.SetIsChordMember(false); // Mark as finalized
            // Volume would remain what it was parsed as, or use g_fishsongConfig.volume if not set
            // If chord members should inherit a global/last volume, that logic needs to be added
            finalEvent.FinalizeSongStructure();
            events.push_back(finalEvent);
        }
        pendingChord.clear();
    }
}

void CFishsongFile::ProcessCommand(const std::string &cmd) {
    std::string command = cmd;
    if (!command.empty() && command[0] == '*') {
        command.erase(0, 1); // Remove '*'
    }
    command = TrimString(command); // Trim after removing '*'

    std::istringstream iss(command);
    std::string commandName, commandValue; // commandValue might contain the rest of the line
    
    iss >> commandName; // Get the command itself
    // For commands like "attrib", we want the rest of the line as commandValue
    if (strcasecmp(commandName.c_str(), "attrib") == 0) {
        // Read the first part of the value
        iss >> commandValue;
        // Read the rest of the line into commandValue
        std::string remainingValue;
        std::getline(iss, remainingValue); // Reads rest of line after first token
        commandValue += remainingValue;
        commandValue = TrimString(commandValue); // Trim combined value
    } else {
         // For other commands, just get the next token as value
        iss >> commandValue;
        commandValue = TrimString(commandValue);
    }

    if (commandName.empty()) return;
    
    if (strcasecmp(commandName.c_str(), "line") == 0) {
        int lineNum = std::atoi(commandValue.c_str());
        if (lineNum > 0) {
            FinalizePendingChord(120); // Finalize before switching
            currentTrack = lineNum;
            g_fishsongConfig.line = lineNum;
            g_fishsongConfig.localShift = 0; // Reset localShift for the new track
            
            if (trackPositions.find(currentTrack) == trackPositions.end()) {
                trackPositions[currentTrack] = 0; // Initialize new track's position
            }
        }
    } else if (strcasecmp(commandName.c_str(), "rest") == 0) { // 'rest' was original name, could be 'sync' or 'wait'
        int trackToWaitFor = std::atoi(commandValue.c_str());
        if (trackToWaitFor > 0 && trackToWaitFor != currentTrack) {
            // Initialize the track we are waiting for if it doesn't exist yet
            if (trackPositions.find(trackToWaitFor) == trackPositions.end()) {
                trackPositions[trackToWaitFor] = 0;
            }
            
            // Only set waitingForTrack if the other track is actually behind or at the same position.
            // If trackPositions[currentTrack] > trackPositions[trackToWaitFor], we are ahead and need to wait.
            if (trackPositions[currentTrack] > trackPositions[trackToWaitFor]) {
                 std::cout << "Track " << currentTrack 
                           << " (pos " << trackPositions[currentTrack] 
                           << ") waiting for track " << trackToWaitFor 
                           << " (pos " << trackPositions[trackToWaitFor] << ")" << std::endl;
                waitingForTrack = trackToWaitFor;
                // Do NOT clear pendingChord here, it should be processed when sync is achieved or file ends.
            } else {
                // We are already behind or at the same position as the target track, no need to wait.
                std::cout << "Track " << currentTrack 
                           << " (pos " << trackPositions[currentTrack] 
                           << ") does not need to wait for track " << trackToWaitFor 
                           << " (pos " << trackPositions[trackToWaitFor] << ")" << std::endl;
                waitingForTrack = 0; // Ensure not waiting
            }
        }
    } else {
        // Reconstruct the command string for ParseFishSongCommand if it expects "name value"
        // The original global ParseFishSongCommand takes the full string *after* the '*'.
        ParseFishSongCommand(command); // Pass the already trimmed command string (without '*')
    }
}

const std::vector<CSongEvent>& CFishsongFile::GetEvents() const {
    return events;
}

// CFishsongManager Implementation
CFishsongManager::CFishsongManager() : updateCounter(0) {}

CFishsongManager::~CFishsongManager() {}

bool CFishsongManager::UpdateState(int /*someFlag*/, int /*extraParam*/, int &counterOut, int cmdFlag) {
    // Parameters someFlag and extraParam are not used in the provided snippet
    counterOut = ++updateCounter;
    if (cmdFlag >= 0) { // Only print if not the "negative cmdFlag" case
        std::cout << "Manager updated (counter = " << updateCounter << ")." << std::endl;
    }
    return true;
}

void CFishsongManager::DispatchEvent(const CSongEvent &event) {
    validatedEvents.push_back(event);
    std::cout << "Dispatched event: " << event.GetTitle() << std::endl;
}

void CFishsongManager::FinalizeEventTitle(CSongEvent &event) {
    if (event.CheckSongCategory("long"))
        event.SetTitle(event.GetTitle() + " (extended)");
    else if (event.CheckSongCategory("santarare"))
        event.SetTitle(event.GetTitle() + " (holiday)");
    // Other categories don't modify title in this example
}

void CFishsongManager::ResetState() {
    validatedEvents.clear();
    updateCounter = 0;
    std::cout << "Manager state reset." << std::endl;
}

// Global Utility Functions
void InitFishsongEvents() {
    std::cout << "Fishsong events initialized." << std::endl;
    g_fishsongConfig.skip = false;
    g_fishsongConfig.line = 1;
    g_fishsongConfig.speed = 5.0f;
    g_fishsongConfig.volume = 1.0f;
    g_fishsongConfig.shift = 0;
    g_fishsongConfig.localShift = 0;
}

void ClearFishsongBuffer() {
    std::cout << "Fishsong buffer cleared." << std::endl;
}

CFishsongFile* LoadFishsongFile(const std::string &filePath) {
    CFishsongFile* file = new CFishsongFile(filePath);
    if (!file->Load()) {
        delete file;
        return NULL; // Use NULL for VS2003 compatibility
    }
    return file;
}

void ProcessFishsong(bool force) {
    static bool initialized = false;
    if (force || !initialized) {
        initialized = true;
        std::cout << "Processing fishsong files..." << std::endl;
        
        // std::string folderPath = "fishsongs/"; // Path for files
        std::vector<std::string> files; // Files vector remains empty as per original stub
        
        // TODO: In Sexyapp framework, file iteration would be different.
        // For now, this loop won't execute if 'files' is empty.
        
        CFishsongManager manager; // Create one manager for all files in this processing pass
        for (size_t i = 0; i < files.size(); ++i) {
            std::cout << "Loading file: " << files[i] << std::endl;
            CFishsongFile* fsFile = LoadFishsongFile(files[i]); // Assuming files[i] is full path or relative
            if (fsFile) {
                const std::vector<CSongEvent> &songEvents = fsFile->GetEvents();
                for (size_t j = 0; j < songEvents.size(); ++j) {
                    if (g_fishsongConfig.skip) continue;
                    
                    // It seems the intent is to dispatch, then finalize a copy for logging/further processing
                    manager.DispatchEvent(songEvents[j]); 
                    
                    CSongEvent eventCopy = songEvents[j]; // Make a copy to modify for title
                    manager.FinalizeEventTitle(eventCopy);
                    std::cout << "Finalized event (for logging/display): " << eventCopy.GetTitle() << std::endl;
                }
                delete fsFile;
            }
        }
        
        int counter;
        manager.UpdateState(0, 0, counter, 0); // Example call
    }
}

void ParseFishSongCommand(const std::string &commandStr) {
    // commandStr is expected to be the content *after* '*' and already trimmed.
    std::istringstream iss(commandStr);
    std::string commandName, commandValue; // commandValue might get more later for "attrib"

    iss >> commandName; // First token is command name
    
    // For "attrib", the value is the rest of the line
    if (strcasecmp(commandName.c_str(), "attrib") == 0) {
        // Get the first part of the attribute's value
        iss >> commandValue; 
        std::string restOfLine;
        std::getline(iss, restOfLine); // Get everything else on the line
        if (!restOfLine.empty() && (restOfLine[0] == ' ' || restOfLine[0] == '\t')) {
             // Add space only if there was one, then the rest
            commandValue += restOfLine;
        } else {
            commandValue += restOfLine; // Concatenate directly if no leading space in restOfLine
        }
        commandValue = TrimString(commandValue); // Trim the full attribute value
    } else {
        // For other commands, value is just the next token
        iss >> commandValue;
        commandValue = TrimString(commandValue); // Trim typical value
    }

    if (commandName.empty()) return;
    
    bool recognized = true;
    if (strcasecmp(commandName.c_str(), "skip") == 0) {
        g_fishsongConfig.skip = (strcasecmp(commandValue.c_str(), "true") == 0 || commandValue == "1");
    } else if (strcasecmp(commandName.c_str(), "speed") == 0) {
        float speed = static_cast<float>(std::atof(commandValue.c_str()));
        g_fishsongConfig.speed = (speed == 0.0f && commandValue != "0" && commandValue != "0.0") ? 5.0f : speed; // Default if atof fails (and not explicitly "0")
        if (g_fishsongConfig.speed <= 0.0f) g_fishsongConfig.speed = 5.0f; // Ensure positive speed
    } else if (strcasecmp(commandName.c_str(), "volume") == 0) {
        float volume = static_cast<float>(std::atof(commandValue.c_str()));
        g_fishsongConfig.volume = std::max(0.0f, std::min(1.0f, volume));
    } else if (strcasecmp(commandName.c_str(), "shift") == 0) {
        g_fishsongConfig.shift = std::atoi(commandValue.c_str());
    } else if (strcasecmp(commandName.c_str(), "localshift") == 0) {
        g_fishsongConfig.localShift = std::atoi(commandValue.c_str());
    } else if (strcasecmp(commandName.c_str(), "on") == 0) { // Global on/off, distinct from file-local *on/*off
        g_fishsongConfig.skip = false;
    } else if (strcasecmp(commandName.c_str(), "off") == 0) { // Global on/off
        g_fishsongConfig.skip = true;
    } else if (strcasecmp(commandName.c_str(), "attrib") == 0) {
        // Value is already in commandValue from specific handling above
        std::cout << "Global Attribute Set: " << commandValue << std::endl;
    } else {
        recognized = false;
        std::cerr << "Unrecognized global command: " << commandName << std::endl;
    }
    
    if (recognized) {
        std::cout << "Parsed global command: " << commandName;
        if (!commandValue.empty() || strcasecmp(commandName.c_str(), "attrib")==0 ) { // Attrib might have empty value string
            std::cout << " = " << commandValue;
        }
        std::cout << std::endl;
    }
}
