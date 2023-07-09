/*
 * EXTERN.H
 *
 * Copyright (C) 1990 Microsoft Corporation.
 *
 * External variables for the midi control panel.
 */

extern HINSTANCE        hLibInst;       // Instance handle of library
HGLOBAL         hKeyMap;                // Handle to keymap (for key edit)
extern HFONT    hFont;                  // Handle to neato dialog box font
extern HWND     hWnd,                   // 'Current' window handle
                hEdit,                  // Edit control handle
                hArrow;                 // Arrow control handle
extern RECT     rcBox;                  // Clipping/scroll rectangle
extern int      rgxPos [8],             // horizontal positions of vertical lines
                yBox,                   // rows of data y extent
                xClient,                // Window client area x pixels
                yClient,                // Window client area y pixels
                iCurPos,                // Current position on screen
                iVertPos,               // Current vertical scroll position
                iVertMax,               // Maximum veritcal scroll position
                nLines,                 // Number of lines of data
                yChar,                  // Height of character in font
                xChar,                  // Width of average character in font
                iMap;                   // Flag for GetMBData
extern char     szCurrent [],           // Name of current map
                szCurDesc [],           // Description of current map
                szMidiCtl [],           // "MIDI Control Panel"
                szNone [],              // text string '[ None ]'
                aszSourceKey[],
                aszSourceKeyName[],
                aszPatchNumber[],
                aszSourcePatch[],
                aszSourcePatchName[],
                aszSourceMnumonic[],
                aszSourceChannel[],
                aszActive[],
                szMidiHlp[];
extern BOOL     fModified,              // Has map been modified?
                fNew,                   // Is this a new map?
                fHidden;                // Is the active edit line hidden?

extern BOOL     fReadOnly;              // Are we in READ ONLY mode?

#ifdef ALLOW_SIZING

extern BOOL     fSizeChange;            // Has the size of the box changed?

#endif // ALLOW_SIZING
