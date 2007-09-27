#ifndef __DESKCPLX__H
#define __DESKCPLX__H

#define DESK_EXT_CALLBACK CALLBACK

#define DESK_EXT_EXTINTERFACE TEXT("Desk.cpl extension interface")
#define DESK_EXT_PRUNINGMODE TEXT("Pruning Mode")
#define DESK_EXT_DISPLAYDEVICE TEXT("Display Device")
#define DESK_EXT_DISPLAYNAME TEXT("Display Name")
#define DESK_EXT_DISPLAYID TEXT("Display ID")
#define DESK_EXT_DISPLAYKEY TEXT("Display Key")
#define DESK_EXT_DISPLAYSTATEFLAGS TEXT("Display State Flags")
#define DESK_EXT_MONITORNAME TEXT("Monitor Name")
#define DESK_EXT_MONITORDEVICE TEXT("Monitor Device")

typedef PDEVMODEW (DESK_EXT_CALLBACK *PDESK_EXT_ENUMALLMODES)(PVOID Context, DWORD Index);
typedef PDEVMODEW (DESK_EXT_CALLBACK *PDESK_EXT_GETCURRENTMODE)(PVOID Context);
typedef BOOL (DESK_EXT_CALLBACK *PDESK_EXT_SETCURRENTMODE)(PVOID Context, const DEVMODEW *pDevMode);
typedef VOID (DESK_EXT_CALLBACK *PDESK_EXT_GETPRUNINGMODE)(PVOID Context, PBOOL pbModesPruned, PBOOL pbKeyIsReadOnly, PBOOL pbPruningOn);
typedef VOID (DESK_EXT_CALLBACK *PDESK_EXT_SETPRUNINGMODE)(PVOID Context, BOOL PruningOn);

typedef struct _DESK_EXT_INTERFACE
{
    /* NOTE: This structure is binary compatible to XP. The windows shell
             extensions rely on this structure to be properly filled! */
    DWORD cbSize;

    PVOID Context; /* This value is passed on to the callback routines */

    /* Callback routines called by the shell extensions */
    PDESK_EXT_ENUMALLMODES EnumAllModes;
    PDESK_EXT_SETCURRENTMODE SetCurrentMode;
    PDESK_EXT_GETCURRENTMODE GetCurrentMode;
    PDESK_EXT_SETPRUNINGMODE SetPruningMode;
    PDESK_EXT_GETPRUNINGMODE GetPruningMode;

    /* HardwareInformation.* values provided in the device registry key */
    WCHAR MemorySize[128];
    WCHAR ChipType[128];
    WCHAR DacType[128];
    WCHAR AdapterString[128];
    WCHAR BiosString[128];
} DESK_EXT_INTERFACE, *PDESK_EXT_INTERFACE;

LONG WINAPI DisplaySaveSettings(PVOID pContext, HWND hwndPropSheet);

static PDESK_EXT_INTERFACE __inline
QueryDeskCplExtInterface(IDataObject *pdo)
{
    PDESK_EXT_INTERFACE pRecvBuffer, pExtIface = NULL;
    FORMATETC fetc;
    STGMEDIUM medium;

    fetc.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(DESK_EXT_EXTINTERFACE);
    fetc.ptd = NULL;
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_HGLOBAL;

    if (SUCCEEDED(IDataObject_GetData(pdo, &fetc, &medium)) && medium.hGlobal != NULL)
    {
        /* We always receive the string in unicode! */
        pRecvBuffer = (PDESK_EXT_INTERFACE)GlobalLock(medium.hGlobal);

        if (pRecvBuffer->cbSize == sizeof(*pRecvBuffer))
        {
            pExtIface = LocalAlloc(LMEM_FIXED, sizeof(*pExtIface));
            if (pExtIface != NULL)
            {
                CopyMemory(pExtIface,
                           pRecvBuffer,
                           sizeof(*pRecvBuffer));
            }
        }

        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
    }

    return pExtIface;
}

static LPTSTR __inline
QueryDeskCplString(IDataObject *pdo, UINT cfFormat)
{
    FORMATETC fetc;
    STGMEDIUM medium;
    SIZE_T BufLen;
    LPWSTR lpRecvBuffer;
    LPTSTR lpStr = NULL;

    fetc.cfFormat = (CLIPFORMAT)cfFormat;
    fetc.ptd = NULL;
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_HGLOBAL;

    if (SUCCEEDED(IDataObject_GetData(pdo, &fetc, &medium)) && medium.hGlobal != NULL)
    {
        /* We always receive the string in unicode! */
        lpRecvBuffer = (LPWSTR)GlobalLock(medium.hGlobal);

        BufLen = wcslen(lpRecvBuffer) + 1;
        lpStr = LocalAlloc(LMEM_FIXED, BufLen * sizeof(TCHAR));
        if (lpStr != NULL)
        {
#ifdef UNICODE
            wcscpy(lpStr, lpRecvBuffer);
#else
            WideCharToMultiByte(CP_APC, 0, lpRecvBuffer, -1, lpStr, BufLen, NULL, NULL);
#endif
        }

        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
    }

    return lpStr;
}

static LONG __inline
DeskCplExtDisplaySaveSettings(PDESK_EXT_INTERFACE DeskExtInterface,
                              HWND hwndDlg)
{
    typedef LONG (WINAPI *PDISPLAYSAVESETTINGS)(PVOID, HWND);
    HMODULE hModDeskCpl;
    PDISPLAYSAVESETTINGS pDisplaySaveSettings;
    LONG lRet = DISP_CHANGE_BADPARAM;

    /* We could use GetModuleHandle() instead, but then this routine
       wouldn't work if some other application hosts the shell extension */
    hModDeskCpl = LoadLibrary(TEXT("desk.cpl"));
    if (hModDeskCpl != NULL)
    {
        pDisplaySaveSettings = (PDISPLAYSAVESETTINGS)GetProcAddress(hModDeskCpl,
                                                                    "DisplaySaveSettings");
        if (pDisplaySaveSettings != NULL)
        {
            lRet = pDisplaySaveSettings(DeskExtInterface->Context,
                                        hwndDlg);
        }

        FreeLibrary(hModDeskCpl);
    }

    return lRet;
}

#endif /* __DESKCPLX__H */
