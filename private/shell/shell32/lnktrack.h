
/*
 * File:     lnktrack.h
 *
 * Purpose:  This file provides definitions and prototypes
 *           useful for link-tracking.
 */

#ifndef _LNKTRACK_H_
#define _LNKTRACK_H_

// Only if the file we found scores more than this number do we 
// even show the user this result, any thing less than this would
// be too shameful of us to show the user. 
#define MIN_SHOW_USER_SCORE     10

// magic score that stops searches and causes us not to warn
// whe the link is actually found

#define MIN_NO_UI_SCORE         40

// If no User Interface will be provided during the search,
// then do not search more than 3 seconds.

#define NOUI_SEARCH_TIMEOUT     (3 * 1000)

// If a User Interface will be provided during the search,
// then search as much as 2 minutes.

#define UI_SEARCH_TIMEOUT       (120 * 1000)


// Function prototypes.

EXTERN_C HRESULT TimeoutExpired( DWORD dwTickCountDeadline );
EXTERN_C DWORD DeltaTickCount( DWORD dwTickCountDeadline );

EXTERN_C int FindInFolder(HWND hwnd, UINT uFlags, LPCTSTR pszPath, WIN32_FIND_DATA *pfd, LPCTSTR pszCurFile
#ifdef WINNT
                 , struct CTracker *pTracker, DWORD TrackerRestrictions,
                   UINT fifFlags
#endif
                 );

//
//  Flags for FindInFilder.fifFlags
//
//  FIF_NODDRIVE
//      The drive referred to by the shortcut does not exist.
//      Let pTracker search for it, but do not perform an old-style
//      ("downlevel") search of our own.
//
#define FIF_NODRIVE     0x0001

#endif // !_LNKTRACK_H_
