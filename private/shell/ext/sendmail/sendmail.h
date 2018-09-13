#pragma warning(disable:4001)

#define STRICT
#define CONST_VTABLE

// gotta redirect to shlwapi, o.w. fail (quietly!) on w95
#define lstrcmpW    StrCmpW
#define lstrcmpiW   StrCmpIW
#define lstrcpyW    StrCpyW
#define lstrcpynW   StrCpyNW
#define lstrcatW    StrCatW
// lstrlen is o.k.

/////////////////////////////////////////////////////////////////////////
//
//  ATL / OLE HACKHACK
//
//  Include <w95wraps.h> before anything else that messes with names.
//  Although everybody gets the wrong name, at least it's *consistently*
//  the wrong name, so everything links.
//
//  NOTE:  This means that while debugging you will see functions like
//  CWindowImplBase__DefWindowProcWrapW when you expected to see
//  CWindowImplBase__DefWindowProc.
//
#ifdef UNICODE
#define POST_IE5_BETA // turn on post-split iedev stuff
#endif
#include <w95wraps.h>

#include <windows.h>
#include <windowsx.h>

#include <intshcut.h>
#include <wininet.h> 
#include <shellapi.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shsemip.h>
#include <shellp.h>
#include <shlwapi.h>
#include <ccstock.h>
#include <crtfree.h>
#include <debug.h>

#include "comdll.h"


BOOL _SHGetSpecialFolderPath(HWND hwndOwner, LPTSTR pszPath, int nFolder, BOOL fCreate);

//
// Trace/dump/break flags specific to shell32\.
//   (Standard flags defined in shellp.h)
//

// Break flags
#define BF_ONLOADED         0x00000010      // Stop when loaded

// Trace flags

// Dump flags

// stuff for COM objects. every object needs to have a CLSID and Create function

extern const GUID CLSID_MailRecipient;
extern const GUID CLSID_DesktopShortcut;
STDAPI MailRecipient_CreateInstance(IUnknown *, REFIID, void **);
STDAPI MailRecipient_RegUnReg(BOOL bReg, HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszModule);
STDAPI DesktopShortcut_RegUnReg(BOOL bReg, HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszModule);



typedef HRESULT (STDMETHODCALLTYPE *LPDROPPROC)(IDataObject *pdtobj, DWORD grfKeyState, DWORD dwEffect);

typedef struct
{
    IDropTarget         dt;
    IShellExtInit       sxi;
    IPersistFile        pf;
    int                 cRef;
    DWORD               grfKeyStateLast;
    DWORD               dwEffectLast;
    TCHAR               szPath[MAX_PATH];   // working dir

    LPDROPPROC pfnDrop;     // IDropTarget::Drop handler

} CDropHandler;
STDAPI DropHandler_CreateInstance(LPDROPPROC pfnDrop, IUnknown *punkOuter, REFIID riid, void **ppv);

BOOL PathYetAnotherMakeUniqueNameT(LPTSTR  pszUniqueName,
                                  LPCTSTR pszPath,
                                  LPCTSTR pszShort,
                                  LPCTSTR pszFileSpec);
int AnsiToUnicode(LPCSTR pstr, LPWSTR pwstr, int cch);
int UnicodeToAnsi(LPCWSTR pwstr, LPSTR pstr, int cch);


#define MRPARAM_DOC          0x00000001
#define MRPARAM_DELETEFILE   0x00000002 // The temporary file was created by us and need to be deleted
#define MRPARAM_USECODEPAGE  0x00000004
typedef struct {
    DWORD dwFlags;    // Attributes passed to the MAPI apis
    LPTSTR pszFiles;  // All files names separated by NULL;
    LPTSTR pszTitle;  // All files names of URLs separated by ';', this will 
                      // be the title;
    int nFiles;       // The number of files being sent.
    UINT uiCodePage;  // Code page 
} MRPARAM;

#define DEFAULTICON             TEXT("DefaultIcon")
#define NEVERSHOWEXT            TEXT("NeverShowExt")
#define SHELLEXT_DROPHANDLER    TEXT("shellex\\DropHandler")

HRESULT GetDropTargetPath(LPTSTR pszPath, int id, LPCTSTR pszExt);
HRESULT _CreateSendToFilesFromDataObj(IDataObject *pdtobj, DWORD grfKeyState, MRPARAM * pmp);

BOOL CleanupPMP(MRPARAM * pmp);
