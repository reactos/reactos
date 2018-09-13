//////////////////////////////////////////////////////////////////////////////
//
//  KBDDLL.H -
//
//      Windows Keyboard Select Dynalink Include File
//
//////////////////////////////////////////////////////////////////////////////

// Key combinations

#define ALT_SHIFT_COMBO     1
#define CTRL_SHIFT_COMBO    2

// Return values

#define KEY_PRIMARY_KL      1
#define KEY_ALTERNATE_KL    2


// Function prototypes

LRESULT CALLBACK KbdGetMsgProc  (INT, WPARAM, LPARAM);

BOOL APIENTRY    InitKbdHook    (HWND, WORD);
