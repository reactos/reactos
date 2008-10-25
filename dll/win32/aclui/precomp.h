#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#define _ACLUI_
#include <windows.h>
#include <ndk/ntndk.h>
#include <commctrl.h>
#include <oleauto.h>
#include <objsel.h>
#include <prsht.h>
#include <aclui.h>
#include <sddl.h>
#include <ntsecapi.h>
#ifdef SUPPORT_UXTHEME
#include <uxtheme.h>
#include <tmschema.h>
#endif
#include "resource.h"

#define EnableRedrawWindow(hwnd) \
    SendMessage((hwnd), WM_SETREDRAW, TRUE, 0)

#define DisableRedrawWindow(hwnd) \
    SendMessage((hwnd), WM_SETREDRAW, FALSE, 0)

extern HINSTANCE hDllInstance;

typedef struct _ACE_ENTRY
{
    struct _ACE_ENTRY *Next;
} ACE_ENTRY, *PACE_ENTRY;

typedef struct _PRINCIPAL_LISTITEM
{
    struct _PRINCIPAL_LISTITEM *Next;
    struct _SIDREQRESULT *SidReqResult;
    PACE_ENTRY ACEs;
    LPWSTR DisplayString;
} PRINCIPAL_LISTITEM, *PPRINCIPAL_LISTITEM;

typedef struct _SECURITY_PAGE
{
    HWND hWnd;
    HWND hBtnAdd;
    HWND hBtnRemove;
    HWND hBtnAdvanced;
    HWND hAceCheckList;
    HWND hPermissionsForLabel;

    /* Main Principals List */
    HWND hWndPrincipalsList;
    PPRINCIPAL_LISTITEM PrincipalsListHead;

    PSID OwnerSid;
    BOOL OwnerDefaulted;

    INT ControlsMargin;

    INT SpecialPermCheckIndex;

    HIMAGELIST hiPrincipals;

    LPSECURITYINFO psi;
    SI_OBJECT_INFO ObjectInfo;
    IDsObjectPicker *pDsObjectPicker;

    SI_ACCESS DefaultAccess;

    HANDLE SidCacheMgr;
    LONG SidLookupsPending;
    UINT Initializing : 1;

    LPCWSTR ServerName;
} SECURITY_PAGE, *PSECURITY_PAGE;

/* MISC ***********************************************************************/

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPWSTR *lpTarget,
                    ...);

LPARAM
ListViewGetSelectedItemData(IN HWND hwnd);

BOOL
ListViewSelectItem(IN HWND hwnd,
                   IN INT Index);

HRESULT
InitializeObjectPicker(IN PCWSTR ServerName,
                       IN PSI_OBJECT_INFO ObjectInfo,
                       OUT IDsObjectPicker **pDsObjectPicker);

VOID
FreeObjectPicker(IN IDsObjectPicker *pDsObjectPicker);

typedef BOOL (*POBJPICK_SELECTED_SID)(IN IDsObjectPicker *pDsObjectPicker,
                                      IN HWND hwndParent  OPTIONAL,
                                      IN PSID pSid,
                                      IN PVOID Context  OPTIONAL);

HRESULT
InvokeObjectPickerDialog(IN IDsObjectPicker *pDsObjectPicker,
                         IN HWND hwndParent  OPTIONAL,
                         IN POBJPICK_SELECTED_SID SelectedSidCallback,
                         IN PVOID Context  OPTIONAL);

/* CHECKLIST CONTROL **********************************************************/

#define CIS_DENYDISABLED        (0x8)
#define CIS_DENYENABLED (0x0)
#define CIS_ALLOWDISABLED       (0x4)
#define CIS_ALLOWENABLED        (0x0)
#define CIS_ALLOW       (0x2)
#define CIS_DENY        (0x1)
#define CIS_NONE        (0x0)
#define CIS_DISABLED    (CIS_ALLOWDISABLED | CIS_DENYDISABLED)

#define CLB_ALLOW       (0x1)
#define CLB_DENY        (0x0)

#define CIS_MASK (CIS_DISABLED | CIS_ALLOW | CIS_DENY)

#define CLM_ADDITEM     (WM_USER + 1)
#define CLM_DELITEM     (WM_USER + 2)
#define CLM_GETITEMCOUNT        (WM_USER + 3)
#define CLM_CLEAR       (WM_USER + 4)
#define CLM_SETCHECKBOXCOLUMN   (WM_USER + 5)
#define CLM_GETCHECKBOXCOLUMN   (WM_USER + 6)
#define CLM_CLEARCHECKBOXES     (WM_USER + 7)
#define CLM_SETITEMSTATE        (WM_USER + 8)
#define CLM_ENABLEQUICKSEARCH   (WM_USER + 9)
#define CLM_SETQUICKSEARCH_TIMEOUT_RESET        (WM_USER + 10)
#define CLM_SETQUICKSEARCH_TIMEOUT_SETFOCUS     (WM_USER + 11)
#define CLM_FINDITEMBYACCESSMASK    (WM_USER + 12)

#define CLN_CHANGINGITEMCHECKBOX        (101)

typedef struct _NMCHANGEITEMCHECKBOX
{
    NMHDR nmhdr;
    DWORD OldState;
    DWORD NewState;
    DWORD CheckBox;
    BOOL Checked;
} NMCHANGEITEMCHECKBOX, *PNMCHANGEITEMCHECKBOX;

BOOL
RegisterCheckListControl(HINSTANCE hInstance);

VOID
UnregisterCheckListControl(HINSTANCE hInstance);

/* DLLMAIN ********************************************************************/

BOOL
WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved);

/* SIDCACHE *******************************************************************/

typedef struct _SIDREQRESULT
{
    LONG RefCount;
    SID_NAME_USE SidNameUse;
    LPWSTR AccountName;
    LPWSTR DomainName;
} SIDREQRESULT, *PSIDREQRESULT;

typedef VOID (*PSIDREQCOMPLETIONPROC)(IN HANDLE SidCacheMgr,
                                      IN PSID Sid,
                                      IN PSIDREQRESULT SidRequestResult,
                                      IN PVOID Context);

HANDLE
CreateSidCacheMgr(IN HANDLE Heap,
                  IN LPCWSTR SystemName);

VOID
DestroySidCacheMgr(IN HANDLE SidCacheMgr);

VOID
DequeueSidLookup(IN HANDLE SidCacheMgr,
                 IN PSID pSid);

BOOL
LookupSidCache(IN HANDLE SidCacheMgr,
               IN PSID pSid,
               IN PSIDREQCOMPLETIONPROC CompletionProc,
               IN PVOID Context);

VOID
ReferenceSidReqResult(IN HANDLE SidCacheMgr,
                      IN PSIDREQRESULT ReqResult);

VOID
DereferenceSidReqResult(IN HANDLE SidCacheMgr,
                        IN PSIDREQRESULT ReqResult);

/* EOF */
