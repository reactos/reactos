/*****************************************************************************\
    FILE: unithunk.h
\*****************************************************************************/

#ifndef _UNICODE_THUNK_WRAPPERS_H
#define _UNICODE_THUNK_WRAPPERS_H



// BUGBUG: Nuke after IE5 beta 1.
#define POST_IE5_BETA

// NOTES:
//    CharPrevW and lstrcpyW doesn't appear to be used in our code.
//
// The .map file will show that we link to MessageBoxW and GetNumberFormatW but
// that is okay because we only use it on NT.

//////////////////////////// IE 5 vs IE 4 /////////////////////////////////
// These are functions that IE5 exposes (normally in shlwapi), but
// if we want to be compatible with IE4, we need to have our own copy.
// If we turn on USE_IE5_UTILS, we won't work with IE4's DLLs (like shlwapi).
#ifndef USE_IE5_UTILS
#define IUnknown_Set                    UnicWrapper_IUnknown_Set
#define SHWaitForSendMessageThread      UnicWrapper_SHWaitForSendMessageThread
#define AutoCompleteFileSysInEditbox    UnicWrapper_AutoCompleteFileSysInEditbox

void            UnicWrapper_IUnknown_Set(IUnknown ** ppunk, IUnknown * punk);
DWORD UnicWrapper_SHWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout);
HRESULT AutoCompleteFileSysInEditbox(HWND hwndEdit);

#endif // USE_IE5_UTILS
//////////////////////////// IE 5 vs IE 4 /////////////////////////////////


#endif // _UNICODE_THUNK_WRAPPERS_H
