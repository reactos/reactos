#ifndef __PRECOMP_H
#define __PRECOMP_H

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <tchar.h>
#include "resource.h"

/* console.c */

BOOL
RegisterMMCWndClasses(VOID);

VOID
UnregisterMMCWndClasses(VOID);

HWND
CreateConsoleWindow(IN LPCTSTR lpFileName  OPTIONAL);

/* misc.c */

INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID);

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPTSTR *lpTarget,
                    ...);

/* mmc.c */

extern HINSTANCE hAppInstance;
extern HANDLE hAppHeap;

#endif /* __PRECOMP_H */
