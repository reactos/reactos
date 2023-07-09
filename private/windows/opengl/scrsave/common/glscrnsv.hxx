/******************************Module*Header*******************************\
* Module Name: glscrnsv.hxx
*
* Defines and externals for screen saver common shell
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __glscrnsv_hxx__
#define __glscrnsv_hxx__

#include "sscommon.h"

#include "sswproc.hxx"
#include "palette.hxx"

// Screen saver mode types
enum {
    SS_TYPE_FULLSCREEN = 0,  // full-screen   (/s)
    SS_TYPE_PREVIEW,         // child preview (/p)
    SS_TYPE_NORMAL,          // normal window (/w)
    SS_TYPE_CONFIG           // config dialog (/c), ()
};

// Various flags
#define SS_PALETTE_TAKEOVER (1 << 0)

/**************************************************************************\
* SCRNSAVE
*
\**************************************************************************/

class SCRNSAVE {
public:
    PSSW    psswMain;   // top level window
    PSSW    psswGL;     // the window running GL (temporary)
    BOOL    bInForeground;  // TRUE if ss is in foreground (has focus)
    int     type;       // type (e.g. /s, /p, /c)
    SS_PAL  *pssPal;    // global palette
    PSSC    pssc;      // client screen saver configuration request
    SS_GL_CONFIG GLc;   // GL configuration (for old style)
    SSW_TABLE sswTable; // table of HWND/PSSW pairs
    HBRUSH  hbrBg;  // global bg brush
    int     flags;  // various flags

    SCRNSAVE( int type, LPARAM lParam );
    SCRNSAVE( int type );
    ~SCRNSAVE();
    BOOL    SetupInitialWindows();
    PSSW    CreateChildWindow( FLOATER_INFO *pFloater );
    PSSW    CreateChildWindow( PSSW psswParent, FLOATER_INFO *pFloater );
    BOOL    bInBackground() { return !bInForeground; }
    BOOL    bResSwitch;
#ifdef SS_DEBUG
    SS_TIMER timer;
    BOOL    bDoTiming;
#endif
private:
    void    Init();     // called by constructors
    LPARAM  initParam;  // param passed in at startup
    BOOL    CreateInitialWindows();
    BOOL    CreateMainWindow();
    void    CloseWindows();
};

// one and only screen saver instance
extern SCRNSAVE *gpss;

#endif // __glscrnsv_hxx__
