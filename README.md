# fishsong
An attempt at reverse engineering Insaniquarium's fishsong format. Started with trying to do it just from the file format itsself and educated guesses, then went to using Ghidra but couldn't locate the logic for note duration parsing. 

A lot of help from LLMs to put together the bits and pieces into something that should theoretically work.

## SexyAppFramework Integration

This project can be built with PopCap's SexyAppFramework. Define `USE_SEXYAPP`
and provide the framework headers and libraries when compiling. When enabled,
file I/O uses `Sexy::SexyAppBase::ReadBufferFromFile` and logging goes through
`OutputDebug`.

### Building on Linux
Install mingw-w64 to provide Windows headers:

    sudo apt-get install mingw-w64

Compile with:

    x86_64-w64-mingw32-g++ -std=c++17 -I../SexyAppFramework/source -DUSE_SEXYAPP -c Fishsong.cpp

