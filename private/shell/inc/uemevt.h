// {
//***   UEME_* -- events
// DESCRIPTION
//  UEME_UI*
//  UEME_RUN*
//  UEME_DONE*
//  UEME_ERROR*
//  UEME_DB*
// NOTES
//  since rulc.exe must process this, it can *only* contain #defines.

// UI (menu, shortcut, etc.)
#define UEME_UIMENU     1   // did a UI menu, wP=grp lParam=IDM_*
#define UEME_UIHOTKEY   2   // did a UI hotkey, lParam=GHID_*
#define UEME_UISCUT     3   // did a UI shortcut, lParam=???
#define UEME_UIQCUT     4   // did a UI qlink/isfband, lParam=???
#define UEME_UITOOLBAR  5   // did a UI toolbar button, wP=lParam=???
#if 0 // 980825 uncomment in uemevt.h, uemedat.h if build breaks (tmp hack)
#define UEME_UIASSOC    6   // did a semi-UI association, wP=-1 lP=-1
#endif

// run (spawn, invoke, etc.)
#define UEME_RUNWMCMD   12  // ran a WM_COMMAND, lParam=UEMC_*
#define _UEME_RUNPIDL1  10  // (obsolete) ran a pidl, wP=csidl, lParam=pidl
#define UEME_RUNPIDL    18  // ran a pidl, wP=isf lP=pidlItem
#define UEME_RUNINVOKE  11  // ran an Ixxx::Invoke, lParam=???
#define UEME_RUNOLECMD  13  // ran an IOleCT::Exec wP=nCmdID lP=pguidCmdGrp
#define UEME_RUNPATHA   14  // ran a path, lParam=path
#define UEME_RUNPATHW   15  // ran a path, lParam=path
#define UEME_RUNCPLA    16  // ran a cpl path, wP=index lP=path
#define UEME_RUNCPLW    17  // ran a cpl path, wP=index lP=path

#ifdef UNICODE
#define UEME_RUNPATH    UEME_RUNPATHW
#define UEME_RUNCPL     UEME_RUNCPLW
#else
#define UEME_RUNPATH    UEME_RUNPATHA
#define UEME_RUNCPL     UEME_RUNCPLA
#endif

// exit status
#define UEME_DONECANCEL 32  // cancel
#define UEME_DONEOK     30  // (NYI) ok (==0)
#define UEME_DONEFAIL   31  // (NYI) fail (!=0)

// error
// NOTES
//  for now lParam=szMsg, that's just temporary but not sure what we need yet
#define UEME_ERRORA     20  // error (generic), lParam=szMsg
#define UEME_ERRORW     21  // error (generic), lParam=szMsg

#ifdef UNICODE
#define UEME_ERROR      UEME_ERRORW
#else
#define UEME_ERROR      UEME_ERRORA
#endif

// control
#define UEME_CTLSESSION 40  // do UASetSession

// instrumented browser
#define UEME_INSTRBROWSER 50

// debug
#define UEME_DBTRACEA   90  // just a midpoint trace..., lParam=szMsg
#define UEME_DBTRACEW   91  // just a midpoint trace..., lParam=szMsg

#ifdef UNICODE
#define UEME_DBTRACE    UEME_DBTRACEW
#else
#define UEME_DBTRACE    UEME_DBTRACEA
#endif

#define UEME_DBSLEEP    92  // sleep, lParam=mSec (per Sleep API)

// all events below here (msg < UEME_USER) are reserved
// private messages start here (at UEME_USER + 0)
// BUGBUG NYI we don't support private messages for now
#define UEME_USER       256

// }
