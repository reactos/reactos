//---------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation 1991-1992
//
// Put a clock in a window.
//---------------------------------------------------------------------------

#define WC_TRAYCLOCK TEXT("TrayClockWClass")

BOOL ClockCtl_Class(HINSTANCE hinst);

HWND ClockCtl_Create(HWND hwndParent, UINT uID, HINSTANCE hInst);

// Message to calculate the minimum size
#define  WM_CALCMINSIZE  (WM_USER + 100)
#define TCM_RESET        (WM_USER + 101)
#define TCM_TIMEZONEHACK (WM_USER + 102)
