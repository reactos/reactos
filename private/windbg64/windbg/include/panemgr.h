/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation


Module Name:

    panemgr.h

Abstract:

    Header file for pane manager windows

Author:

    William Heaton (v-willhe)
    Griffith WWm. Kadnier (v-griffk) 10-Mar-1993
    
Environment:

    Win32, User Mode

--*/

#if ! defined( _PANEMGR_H )
#define _PANEMGR_H

#define ID_PANE_BUTTON 0x101    // Button Panel (+/-)
#define ID_PANE_LEFT   0x102    // Name Panel
#define ID_PANE_RIGHT  0x103    // Value Panel
#define ID_PANE_SIZER  0x104    // Sizer (Between Left|Right pane)
#define ID_PANE_SCROLL 0x105    // Scrollbar

/*
 *  Panemgr structure
 */

typedef struct INFODEF *PINFO;
typedef struct INFODEF   INFO;

struct INFODEF {
    LONG    Length;       // Length of the Status Area
    PFLAGS  Flags;        // Panel flags
    DWORD   PerCent;      // Pane Percent
    char    Text[1];      // Text Area (Watch Window)
};


#define EDITMAX  260


typedef struct tagPANEINFO PANEINFO;
typedef struct tagPANEINFO *PPANEINFO;

struct tagPANEINFO {
    UINT  CtrlId;        // ID_PANE_BUTTON, ID_PANE_LEFT, or ID_PANE_RIGHT
    UINT  ItemId;        // Item Index in pane
    BOOL  ReadOnly;      // Is the Item ReadOnly?
    BOOL  NewText;       // Has the Item changed?
    PSTR  pBuffer;       // The Address of the Buffer (READ-ONLY) for item
    PSTR  pFormat;       // The format strings (if any)
};


typedef struct tagPANEMGR {
    WORD    Type;           // Window Type (Watch,Local,Register,Floating)
    int     iView;          // View Number

    WORD       LineHeight;  // Height of a line in a pane
    WORD       CharWidth;   // Width of the widest character
    HFONT      hFont;       // Font for window
    COLOR_ITEM ColorItem;   // Color Information for this window
    HBRUSH     hbrBackground; // Background brush

    WORD    PaneLines;      // Number of lines in pane
    WORD    PanePerCent;    // Percentage of left pane to right pane

    LONG    nXoffLeft;      // Position of Left Horz. Scroll bar
    LONG    nXoffRight;     // Position of Right Horz. Scroll bar
    LONG    nCaretPos;      // Position of Caret (-1) if not up

    UINT    nCtrlId;        // Pane control w\current focus
    HWND    hWndFocus;      // Pane with the current focus
    HWND    hWndLeft;       // Handle to Left Pane
    HWND    hWndSizer;      // Handle to Sizer Bar
    HWND    hWndRight;      // Handle to Right Pane
    HWND    hWndScroll;     // Handle to Scroll bar
    HWND    hWndButton;     // Handle to Plus/Minus Pane

    WNDPROC fnEditProc;     // Low-Level Edit Proc
    BOOL    LeftOk;         // Has Left Pane  Changed?
    BOOL    RightOk;        // Has Right Pane Changed?
    BOOL    ScrollBarUp;    // Is the Scroll bar visiable?

    BOOL    ReadOnly;       // Is the current line readonly?
    BOOL    Edit;           // Is the current line being edited?
    BOOL    OverType;       // Are we in OverType mode?

    WORD    CurPos;         // Current Index into Line
    WORD    CurLen;         // Length of current line

    WORD    SelPos;         // Selection Position
    int     SelLen;         // Selection Length

    WORD    CurIdx;         // Index of Current Line
    WORD    TopIdx;         // Index of Top Line (Button,Left,Right)
    WORD    MaxIdx;         // Count of indexs available

    int     X;              // for caretpos()
    int     Y;              // same

    PFLAGS  bFlags;          // Flags for pane

    CHAR    EditBuf[EDITMAX]; // Current Edit Buffer
} PANE, *PPANE;


void OpenPanedWindow(int Type, LPWININFO lpWinInfo, int Preference, BOOL bUserActivated);
LONG_PTR CreatePane( HWND hWnd, int iView, int Type);
void PaneKeyboardHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL PaneCloseEdit( PPANE p);
void SyncPanes(PPANE p, WORD Index);
void PaneInvalidateCurrent(HWND hWnd, PPANE p, SHORT Idx);
void PaneInvalidateItem( HWND hWnd, PPANE p, SHORT item);
void PaneInvalidateRow( PPANE p );
void PaneSwitchFocus(PPANE p, HWND hWnd, BOOL fPrev);
void CheckPaneScrollBar( PPANE p, WORD Count);
void PaneResetIdx( PPANE p, SHORT Idx);
void PaneSetIdx( PPANE p, SHORT NewIdx);
void PaneSetCaret( PPANE p, LONG cx, BOOL Scroll);

PLONG GetPaneStatus( int ViewNumber);
void  SetPaneStatus( int ViewNumber, PLONG ptr);
void  FreePaneStatus( int ViewNumber, PLONG ptr);

void DrawPaneItem( HWND hWnd, PPANE p, LPDRAWITEMSTRUCT lpDis );
void InvertButton( PPANE p );
void ScrollPanes( PPANE p,WPARAM wParam, LPARAM lParam);
int  PaneCaretNum( PPANE p);

#endif // _PANEMGR_H
