#ifndef _RESOLVE_H_
#define _RESOLVE_H_

typedef struct {
    // input parameters
    DWORD               dwMatch;        // must macth attributes
    const WIN32_FIND_DATA *pfd;         // original find data

    DWORD               dwTimeLimit;    // don't go past this

    BOOL                bContinue;      // keep going

    LPCTSTR             pszSearchOrigin;// path where current search originated, to help avoid dup searchs
    LPCTSTR             pszSearchOriginFirst;// path where search originated, to help avoid dup searchs
    HWND                hDlg;           // dialog to talk to
    HANDLE              hThread;        // search thread

    int                 iScore;         // score for current item
    WIN32_FIND_DATA     fdFound;        // results

    WIN32_FIND_DATA     fd;             // to save stack space
    UINT                uFlags;         // SLR_ flags

#ifdef WINNT
    struct CTracker     *ptracker;	// Implements ObjectID-based link tracking
                                        // Flags from the TrkMendRestrictions enumeration
    DWORD               TrackerRestrictions;
    DWORD               fifFlags;       // FIF_ flags (lnktrack.h)
    UINT_PTR            idtDelayedShow; // timer for delayed-show
#endif

} RESOLVE_SEARCH_DATA;

EXTERN_C VOID
DoDownLevelSearch(RESOLVE_SEARCH_DATA *prs,
                  TCHAR szFolderPath[],
                  int iStopScore);


#define LNKTRACK_HINTED_UPLEVELS 4  // directory levels to search upwards from last know object locn
#define LNKTRACK_DESKTOP_DOWNLEVELS 4 // infinite downlevels
#define LNKTRACK_ROOT_DOWNLEVELS 4  // levels down from root of fixed disks
#define LNKTRACK_HINTED_DOWNLEVELS 4 // levels down at each level on way up during hinted uplevels

#endif
