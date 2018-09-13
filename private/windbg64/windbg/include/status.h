/*++ 
  BUILD Version: 0003    // Increment this if a change has global effects


  Copyright (c) 1991 Microsoft Corporation


  Module Name:
        Status bar

  Author:
        Carlos Klapp (a-caklap) 19-Sep-97
---*/



// Used as indexes into the rgnItemWidth & rgszItemText
// arrays. These are in order from left to right.
typedef enum {
    nMESSAGE_IDX_STATUSBAR,         // Generic txt message such as menu help,
                                    // or error messages, etc.
    nSRCLIN_IDX_STATUSBAR,          // Line num. & coloumn num. in src file
    nPROCID_IDX_STATUSBAR,          // Process ID
    nTHRDID_IDX_STATUSBAR,          // Thread ID
    nSRCASM_IDX_STATUSBAR,          // Src/Asm mode idicator      
    nOVRTYPE_IDX_STATUSBAR,         // Insert/Overtype indicator
    nCAPSLCK_IDX_STATUSBAR,         // Caps lock indicator
    nNUMLCK_IDX_STATUSBAR,          // Num lock indicator
    nMAX_IDX_STATUSBAR,             // Max num items in enum
} nIDX_STATUSBAR_ITEMS;

// Init/Term functions
void WindbgCreate_StatusBar(HWND hwndParent);
void Terminate_StatusBar();

void Show_StatusBar(BOOL bShow);

void WM_SIZE_StatusBar(WPARAM wParam, LPARAM lParam); 

HWND GetHwnd_StatusBar();

// Some of the items are owner draw.
void OwnerDrawItem_StatusBar(LPDRAWITEMSTRUCT lpDrawItem);




void SetLineColumn_StatusBar(int newLine, int newColumn);

void SetPidTid_StatusBar(const LPPD lppd, const LPTD lptd);

// Old code
void CDECL SetMessageText_StatusBar(int newTextId, WORD msgType, ...);


//Status bar operations
//
// TRUE - considered on, and the text is displayed in black
// FALSE - considered off, and the text is displayed in dark gray
//
BOOL GetNumLock_StatusBar();
BOOL SetNumLock_StatusBar(BOOL newValue);

BOOL GetCapsLock_StatusBar();
BOOL SetCapsLock_StatusBar(BOOL newValue);

BOOL GetSrcMode_StatusBar();
BOOL SetSrcMode_StatusBar(BOOL bSrcMode);

BOOL GetOverType_StatusBar();
BOOL SetOverType_StatusBar(BOOL bOverType);

