/******************************Module*Header*******************************\
* Module Name: sswproc.hxx
*
* Defines and externals for screen saver common shell
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __sswproc_hxx__
#define __sswproc_hxx__

#include "sscommon.h"

#include "sswindow.hxx"

// Window proc messages

enum {
    SS_WM_PALETTE = WM_USER,
    SS_WM_INITGL,
    SS_WM_START,
    SS_WM_CLOSING,
    SS_WM_IDLE
};

// message parameters

enum {
    SS_IDLE_OFF = 0,
    SS_IDLE_ON
};

/**************************************************************************\
* SSW_TABLE
*
\**************************************************************************/

typedef struct {
    HWND hwnd;
    PSSW pssw;
} SSW_TABLE_ENTRY;

#define SS_MAX_WINDOWS 10

class SSW_TABLE{
public:
    SSW_TABLE();
    void    Register( HWND hwnd, PSSW pssw );
    PSSW    PsswFromHwnd( HWND hwnd );
    BOOL    Remove( HWND hwnd );
private:
    SSW_TABLE_ENTRY sswTable[SS_MAX_WINDOWS];
    int     nEntries;
};

// Generic ss window proc
extern LRESULT SS_ScreenSaverProc(HWND, UINT, WPARAM, LPARAM);

#endif // __sswproc_hxx__
