#include <windows.h>
#include <commctrl.h>
#include <prsht.h>
#include <aclui.h>
#include <sddl.h>
#include <ntsecapi.h>
#include "resource.h"

ULONG DbgPrint(PCH Format,...);
#define DPRINT DbgPrint

#ifndef NT_SUCCESS
#define NT_SUCCESS(status) ((LONG)(status) >= 0)
#endif

#define EnableRedrawWindow(hwnd) \
    SendMessage((hwnd), WM_SETREDRAW, TRUE, 0)

#define DisableRedrawWindow(hwnd) \
    SendMessage((hwnd), WM_SETREDRAW, FALSE, 0)

extern HINSTANCE hDllInstance;

typedef struct _ACE_LISTITEM
{
    struct _ACE_LISTITEM *Next;
    SID_NAME_USE SidNameUse;
    WCHAR *DisplayString;
    WCHAR *AccountName;
    WCHAR *DomainName;
} ACE_LISTITEM, *PACE_LISTITEM;

typedef struct _SECURITY_PAGE
{
    HWND hWnd;

    /* Main ACE List */
    HWND hWndAceList;
    PACE_LISTITEM AceListHead;

    HIMAGELIST hiUsrs;

    LPSECURITYINFO psi;
    SI_OBJECT_INFO ObjectInfo;
} SECURITY_PAGE, *PSECURITY_PAGE;

BOOL
OpenLSAPolicyHandle(IN WCHAR *SystemName,
                    IN ACCESS_MASK DesiredAccess,
                    OUT PLSA_HANDLE PolicyHandle);

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPWSTR *lpTarget,
                    ...);

/* EOF */
