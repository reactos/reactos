//
//  shlexec.h
//  routines and macros shared between the different shell exec files
//


#ifndef SHLEXEC_H
#define SHLEXEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <krnlcmn.h>

#ifdef WINNT
typedef LPSTR LPSZ;
#include "wowshlp.h"

extern LPVOID lpfnWowShellExecCB;


#endif// WINNT

#define CH_GUIDFIRST TEXT('{') // '}'

// These fake ERROR_ values are used to display non-winerror.h available error
// messages. They are mapped to valid winerror.h values in _ShellExecuteError.
#define ERROR_RESTRICTED_APP ((UINT)-1)

#define SEE_MASK_CLASS (SEE_MASK_CLASSNAME|SEE_MASK_CLASSKEY)
#define _UseClassName(_mask) (((_mask)&SEE_MASK_CLASS) == SEE_MASK_CLASSNAME)
#define _UseClassKey(_mask)  (((_mask)&SEE_MASK_CLASS) == SEE_MASK_CLASSKEY)
#define _UseTitleName(_mask) (((_mask)&SEE_MASK_HASTITLE) || ((_mask)&SEE_MASK_HASLINKNAME))

#define SEE_MASK_PIDL (SEE_MASK_IDLIST|SEE_MASK_INVOKEIDLIST)
#define _UseIDList(_mask)     (((_mask)&SEE_MASK_PIDL) == SEE_MASK_IDLIST)
#define _InvokeIDList(_mask)  (((_mask)&SEE_MASK_PIDL) == SEE_MASK_INVOKEIDLIST)
#define _UseHooks(_mask)      (!(pei->fMask & SEE_MASK_NO_HOOKS))


#define SAFE_DEBUGSTR(str)    ((str) ? (str) : "<NULL>")


#define SZWHACK           TEXT("\\")
#define SZEQUALS          TEXT("=")
#define SZCONV            TEXT("ddeconv")
#define SZDDEEVENT        TEXT("ddeevent")

// the timer id for the kill this DDE window...
#define DDE_DEATH_TIMER_ID  0x00005555

// the timeout value (180 seconds) for killing a dead dde window...
#define DDE_DEATH_TIMEOUT   (1000 * 180)

//  timeout for conversation window terminating with us...
#define DDE_TERMINATETIMEOUT  (10 * 1000)

#define PEMAGIC         ((WORD)'P'+((WORD)'E'<<8))


// secret kernel api to get name of missing 16 bit component
extern int WINAPI PK16FNF(TCHAR *szBuffer);

DWORD SHProcessMessagesUntilEvent(HWND hwnd, HANDLE hEvent, DWORD dwTimeout);
void ActivateHandler(HWND hwnd, DWORD_PTR dwHotKey);
BOOL Window_IsLFNAware(HWND hwnd);


//  routines that need to be moved to CShellExecute
BOOL DoesAppWantUrl(LPCTSTR lpszFullPathToApp);
HWND _FindPopupFromExe(LPTSTR lpExe);
HINSTANCE Window_GetInstance(HWND hwnd);
BOOL RestrictedApp(LPCTSTR pszApp);
BOOL DisallowedApp(LPCTSTR pszApp);
BOOL CheckAppCompatibility(LPCTSTR pszApp, LPCTSTR *pszNewEnvString, BOOL fNoUI, HWND hwnd);
HRESULT TryShellExecuteHooks(LPSHELLEXECUTEINFO pei);
void RegGetValue(HKEY hkRoot, LPCTSTR lpKey, LPTSTR lpValue);
HRESULT
TryInProcess(
    IN LPSHELLEXECUTEINFO pei,
    IN HKEY               hkeyClass,
    IN LPCTSTR            pszClassVerb,     // "shell/open"
    IN LPCTSTR            pszVerb);          // "open"
UINT ReplaceParameters(LPTSTR lpTo, UINT cchTo, LPCTSTR lpFile,
        LPCTSTR lpFrom, LPCTSTR lpParms, int nShow, DWORD * pdwHotKey, BOOL fLFNAware,
        LPCITEMIDLIST lpID, LPITEMIDLIST *ppidlGlobal);
HRESULT
InvokeInProcExec(
    IN     IContextMenu *     pcm,
    IN OUT LPSHELLEXECUTEINFO pei,
    IN     LPCTSTR            pszDefVerb);


BOOL ShellExecuteNormal(LPSHELLEXECUTEINFO pei);
void _DisplayShellExecError(ULONG fMask, HWND hwnd, LPCTSTR pszFile, LPCTSTR pszTitle, DWORD dwErr);
BOOL InRunDllProcess(void);

// in exec2.c
int FindAssociatedExe(HWND hwnd, LPTSTR lpCommand, LPCTSTR pszDocument, HKEY hkeyProgID);

#ifdef __cplusplus
}
#endif

#endif // SHLEXEC_H
