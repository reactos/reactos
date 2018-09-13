#ifndef _CABSH_H
#define _CABSH_H

// Define structure to be used at head of state stream that is
// not dependent on 16 or 32 bits...
typedef struct _CABSHOLD       // Cabinet Stream header
{
    DWORD   dwSize;       // Offset to where the View streamed additional info

    // First stuff from the window placement
    DWORD  flags;
    DWORD  showCmd;
    POINTL ptMinPosition;
    POINTL ptMaxPosition;
    RECTL  rcNormalPosition;

    // Stuff from Folder Settings;
    DWORD   ViewMode;       // View mode (FOLDERVIEWMODE values)
    DWORD   fFlags;         // View options (FOLDERFLAGS bits)
    DWORD   TreeSplit;      // Position of split in pixels (BUGBUG?)

    // Hot Key
    DWORD   dwHotkey;        // Hotkey

    WINVIEW wv;
} CABSHOLD;

typedef struct _CABSH       // Cabinet Stream header
{
    DWORD   dwSize;       // Offset to where the View streamed additional info

    // First stuff from the window placement
    DWORD  flags;
    DWORD  showCmd;
    POINTL ptMinPosition;
    POINTL ptMaxPosition;
    RECTL  rcNormalPosition;

    // Stuff from Folder Settings;
    DWORD   ViewMode;       // View mode (FOLDERVIEWMODE values)
    DWORD   fFlags;         // View options (FOLDERFLAGS bits)
    DWORD   TreeSplit;      // Position of split in pixels (BUGBUG?)

    // Hot Key
    DWORD   dwHotkey;        // Hotkey

    WINVIEW wv;

    DWORD   fMask;          // Flags specifying which fields are valid
    SHELLVIEWID vid;        // extended view id
    DWORD   dwVersionId;    // CABSH_VER below
    DWORD   dwRevCount;     // rev count of default settings when the folder was saved to the stream
} CABSH;

#define CABSHM_VIEWID  0x00000001
#define CABSHM_VERSION 0x00000002
#define CABSHM_REVCOUNT 0x00000004

#define CABSH_VER 1 // change this version whenever we want to change defaults
#define CABSH_WIN95_VER 0 // this was the pre-ie4 version number

#endif  // _CABSH_H

