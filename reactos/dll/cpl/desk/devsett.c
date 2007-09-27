/*
* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/devsett.c
 * PURPOSE:         ReactOS Display Control Panel Shell Extension Support
 */

#include "desk.h"

#include <cfgmgr32.h>

#define NDEBUG
#include <debug.h>

#define DEBUG_DEVSETTINGS

typedef struct _CDevSettings
{
    const struct IDataObjectVtbl *lpIDataObjectVtbl;
    DWORD ref;

    CLIPFORMAT cfExtInterface; /* "Desk.cpl extension interface" */
    CLIPFORMAT cfDisplayDevice; /* "Display Device" */
    CLIPFORMAT cfDisplayName; /* "Display Name" */
    CLIPFORMAT cfDisplayId; /* "Display ID" */
    CLIPFORMAT cfMonitorName; /* "Monitor Name" */
    CLIPFORMAT cfMonitorDevice; /* "Monitor Device" */
    CLIPFORMAT cfDisplayKey; /* "Display Key" */
    CLIPFORMAT cfDisplayStateFlags; /* "Display State Flags" */
    CLIPFORMAT cfPruningMode; /* "Pruning Mode" */

    PWSTR pDisplayDevice;
    PWSTR pDisplayName;
    PWSTR pDisplayKey;
    PWSTR pDisplayId;
    PWSTR pMonitorName;
    PWSTR pMonitorDevice;

    DESK_EXT_INTERFACE ExtInterface;

    DWORD StateFlags;

    union
    {
        DWORD Flags;
        struct
        {
            DWORD bModesPruned : 1;
            DWORD bKeyIsReadOnly : 1;
            DWORD bPruningOn : 1;
        };
    };
} CDevSettings, *PCDevSettings;

#define impl_to_interface(impl,iface) (struct iface *)(&(impl)->lp##iface##Vtbl)

static __inline PCDevSettings
impl_from_IDataObject(struct IDataObject *iface)
{
    return (PCDevSettings)((ULONG_PTR)iface - FIELD_OFFSET(CDevSettings,
                                                           lpIDataObjectVtbl));
}

static __inline VOID
pCDevSettings_FreeString(PWCHAR *psz)
{
    if (*psz != NULL)
    {
        LocalFree((HLOCAL)*psz);
        *psz = NULL;
    }
}

static PWSTR
pCDevSettings_AllocAndCopyString(const TCHAR *pszSrc)
{
    INT c;
    PWSTR str;

    c = _tcslen(pszSrc) + 1;
    str = (PWSTR)LocalAlloc(LMEM_FIXED,
                            c * sizeof(WCHAR));
    if (str != NULL)
    {
#ifdef UNICODE
        wcscpy(str,
               pszSrc);
#else
        MultiByteToWideChar(CP_APC,
                            0,
                            pszSrc,
                            -1,
                            str,
                            c);
#endif
    }

    return str;
}

static PWSTR
pCDevSettings_GetMonitorName(const WCHAR *pszDisplayDevice)
{
    DISPLAY_DEVICEW dd, dd2;
    PWSTR str = NULL;

    dd.cb = sizeof(dd);
    if (EnumDisplayDevicesW(pszDisplayDevice,
                            0,
                            &dd,
                            0))
    {
        dd2.cb = sizeof(dd2);
        if (EnumDisplayDevicesW(pszDisplayDevice,
                                1,
                                &dd2,
                                0))
        {
            /* There's more than one monitor connected... */
            LoadStringW(hApplet,
                        IDS_MULTIPLEMONITORS,
                        dd.DeviceString,
                        sizeof(dd.DeviceString) / sizeof(dd.DeviceString[0]));
        }
    }
    else
    {
        /* We can't enumerate a monitor, make sure this fact is reported
           to the user! */
        LoadStringW(hApplet,
                    IDS_UNKNOWNMONITOR,
                    dd.DeviceString,
                    sizeof(dd.DeviceString) / sizeof(dd.DeviceString[0]));
    }

    str = LocalAlloc(LMEM_FIXED,
                     (wcslen(dd.DeviceString) + 1) * sizeof(WCHAR));
    if (str != NULL)
    {
        wcscpy(str,
               dd.DeviceString);
    }

    return str;
}

static PWSTR
pCDevSettings_GetMonitorDevice(const WCHAR *pszDisplayDevice)
{
    DISPLAY_DEVICEW dd;
    PWSTR str = NULL;

    dd.cb = sizeof(dd);
    if (EnumDisplayDevicesW(pszDisplayDevice,
                            0,
                            &dd,
                            0))
    {
        str = LocalAlloc(LMEM_FIXED,
                         (wcslen(dd.DeviceName) + 1) * sizeof(WCHAR));
        if (str != NULL)
        {
            wcscpy(str,
                   dd.DeviceName);
        }
    }

    return str;
}

static PWSTR
pCDevSettings_GetDeviceInstanceId(const WCHAR *pszDevice)
{
    DEVINST DevInst;
    CONFIGRET cr;
    ULONG BufLen;
    LPWSTR lpDevInstId = NULL;

    DPRINT1("CDevSettings::GetDeviceInstanceId(%ws) UNIMPLEMENTED!\n", pszDevice);

    cr = CM_Locate_DevNodeW(&DevInst,
                            (DEVINSTID_W)pszDevice,
                            CM_LOCATE_DEVNODE_NORMAL);
    if (cr == CR_SUCCESS)
    {
        DbgPrint("Success1\n");
        cr = CM_Get_Device_ID_Size(&BufLen,
                                   DevInst,
                                   0);
        if (cr == CR_SUCCESS)
        {
            DbgPrint("Success2\n");
            lpDevInstId = LocalAlloc(LMEM_FIXED,
                                     (BufLen + 1) * sizeof(WCHAR));

            if (lpDevInstId != NULL)
            {
                DbgPrint("Success3\n");
                cr = CM_Get_Device_IDW(DevInst,
                                       lpDevInstId,
                                       BufLen,
                                       0);

                if (cr != CR_SUCCESS)
                {
                    LocalFree((HLOCAL)lpDevInstId);
                    lpDevInstId = NULL;
                }
                DbgPrint("instance id: %ws\n", lpDevInstId);
            }
        }
    }

    return lpDevInstId;
}


static HKEY
pCDevSettings_OpenDeviceKey(PCDevSettings This,
                            BOOL ReadOnly)
{
    static const WCHAR szRegPrefix[] = L"\\Registry\\Machine\\";
    PWSTR lpRegKey;
    REGSAM Access = KEY_READ;
    HKEY hKey;

    lpRegKey = This->pDisplayKey;
    if (lpRegKey != NULL)
    {
        if (wcslen(lpRegKey) >= wcslen(szRegPrefix) &&
            !wcsnicmp(lpRegKey,
                      szRegPrefix,
                      wcslen(szRegPrefix)))
        {
            lpRegKey += wcslen(szRegPrefix);
        }

        if (!ReadOnly)
            Access |= KEY_WRITE;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         lpRegKey,
                         0,
                         Access,
                         &hKey) == ERROR_SUCCESS)
        {
            return hKey;
        }
    }

    return NULL;
}

PDEVMODEW DESK_EXT_CALLBACK
CDevSettings_EnumAllModes(PVOID Context,
                          DWORD Index)
{
    //PCDevSettings This = impl_from_IDataObject((IDataObject *)Context);
    /* FIXME: Implement */
    DPRINT1("CDevSettings::EnumAllModes(%u)\n", Index);
    return NULL;
}

PDEVMODEW DESK_EXT_CALLBACK
CDevSettings_GetCurrentMode(PVOID Context)
{
    //PCDevSettings This = impl_from_IDataObject((IDataObject *)Context);
    /* FIXME: Implement */
    DPRINT1("CDevSettings::GetCurrentMode\n");
    return NULL;
}

BOOL DESK_EXT_CALLBACK
CDevSettings_SetCurrentMode(PVOID Context,
                            const DEVMODEW *pDevMode)
{
    //PCDevSettings This = impl_from_IDataObject((IDataObject *)Context);
    /* FIXME: Implement */
    DPRINT1("CDevSettings::SetCurrentMode(0x%p)\n", pDevMode);
    return FALSE;
}

VOID DESK_EXT_CALLBACK
CDevSettings_GetPruningMode(PVOID Context,
                            PBOOL pbModesPruned,
                            PBOOL pbKeyIsReadOnly,
                            PBOOL pbPruningOn)
{
    PCDevSettings This = impl_from_IDataObject((IDataObject *)Context);

    DPRINT1("CDevSettings::GetPruningMode(%p,%p,%p)\n", pbModesPruned, pbKeyIsReadOnly, pbPruningOn);

    *pbModesPruned = This->bModesPruned;
    *pbKeyIsReadOnly = This->bKeyIsReadOnly;
    *pbPruningOn = This->bPruningOn;
}

VOID DESK_EXT_CALLBACK
CDevSettings_SetPruningMode(PVOID Context,
                            BOOL PruningOn)
{
    HKEY hKey;
    DWORD dwValue;
    PCDevSettings This = impl_from_IDataObject((IDataObject *)Context);

    DPRINT1("CDevSettings::SetPruningMode(%d)\n", PruningOn);

    if (This->bModesPruned && !This->bKeyIsReadOnly &&
        PruningOn != This->bPruningOn)
    {
        This->bPruningOn = (PruningOn != FALSE);

        hKey = pCDevSettings_OpenDeviceKey(This,
                                           FALSE);
        if (hKey != NULL)
        {
            dwValue = (DWORD)This->bPruningOn;

            RegSetValueEx(hKey,
                          TEXT("PruningMode"),
                          0,
                          REG_DWORD,
                          (const BYTE *)&dwValue,
                          sizeof(dwValue));

            RegCloseKey(hKey);
        }
    }
}

static VOID
pCDevSettings_ReadHardwareInfo(HKEY hKey,
                               LPCTSTR lpValueName,
                               LPWSTR lpBuffer)
{
    DWORD type = REG_BINARY;
    DWORD size = 128 * sizeof(WCHAR);
    RegQueryValueEx(hKey,
                    lpValueName,
                    NULL,
                    &type,
                    (PBYTE)lpBuffer,
                    &size);
}

static VOID
pCDevSettings_InitializeExtInterface(PCDevSettings This)
{
    PDESK_EXT_INTERFACE Interface = &This->ExtInterface;
    HKEY hKeyDev;

    ZeroMemory(Interface,
               sizeof(*Interface));
    Interface->cbSize = sizeof(*Interface);

    /* Initialize the callback table */
    Interface->Context = impl_to_interface(This, IDataObject);
    Interface->EnumAllModes = CDevSettings_EnumAllModes;
    Interface->SetCurrentMode = CDevSettings_SetCurrentMode;
    Interface->GetCurrentMode = CDevSettings_GetCurrentMode;
    Interface->SetPruningMode = CDevSettings_SetPruningMode;
    Interface->GetPruningMode = CDevSettings_GetPruningMode;

    /* Read the HardwareInformation.* values from the registry key */
    hKeyDev = pCDevSettings_OpenDeviceKey(This,
                                          TRUE);
    if (hKeyDev != NULL)
    {
        DWORD dwType, dwMemSize = 0;
        DWORD dwSize = sizeof(dwMemSize);

        if (RegQueryValueEx(hKeyDev,
                            TEXT("HardwareInformation.MemorySize"),
                            NULL,
                            &dwType,
                            (PBYTE)&dwMemSize,
                            &dwSize) == ERROR_SUCCESS &&
            (dwType == REG_BINARY || dwType == REG_DWORD) &&
            dwSize == sizeof(dwMemSize))
        {
            dwMemSize /= 1024;

            if (dwMemSize > 1024)
            {
                dwMemSize /= 1024;
                if (dwMemSize > 1024)
                {
                    wsprintf(Interface->MemorySize,
                             _T("%u GB"),
                             dwMemSize / 1024);
                }
                else
                {
                    wsprintf(Interface->MemorySize,
                             _T("%u MB"),
                             dwMemSize);
                }
            }
            else
            {
                wsprintf(Interface->MemorySize,
                         _T("%u KB"),
                         dwMemSize);
            }
        }

        pCDevSettings_ReadHardwareInfo(hKeyDev,
                                       TEXT("HardwareInformation.ChipType"),
                                       Interface->ChipType);
        pCDevSettings_ReadHardwareInfo(hKeyDev,
                                       TEXT("HardwareInformation.DacType"),
                                       Interface->DacType);
        pCDevSettings_ReadHardwareInfo(hKeyDev,
                                       TEXT("HardwareInformation.AdapterString"),
                                       Interface->AdapterString);
        pCDevSettings_ReadHardwareInfo(hKeyDev,
                                       TEXT("HardwareInformation.BiosString"),
                                       Interface->BiosString);
        RegCloseKey(hKeyDev);
    }
}

static HRESULT
pCDevSettings_Initialize(PCDevSettings This,
                         PDISPLAY_DEVICE_ENTRY DisplayDeviceInfo)
{
    HKEY hKey;

    This->Flags = 0;
    This->StateFlags = DisplayDeviceInfo->DeviceStateFlags;
    DPRINT1("This->StateFlags: %x\n", This->StateFlags);

    /* Register clipboard formats */
    This->cfExtInterface = RegisterClipboardFormat(DESK_EXT_EXTINTERFACE);
    This->cfDisplayDevice = RegisterClipboardFormat(DESK_EXT_DISPLAYDEVICE);
    This->cfDisplayName = RegisterClipboardFormat(DESK_EXT_DISPLAYNAME);
    This->cfDisplayId = RegisterClipboardFormat(DESK_EXT_DISPLAYID);
    This->cfDisplayKey = RegisterClipboardFormat(DESK_EXT_DISPLAYKEY);
    This->cfDisplayStateFlags = RegisterClipboardFormat(DESK_EXT_DISPLAYSTATEFLAGS);
    This->cfMonitorName = RegisterClipboardFormat(DESK_EXT_MONITORNAME);
    This->cfMonitorDevice = RegisterClipboardFormat(DESK_EXT_MONITORDEVICE);
    This->cfPruningMode = RegisterClipboardFormat(DESK_EXT_PRUNINGMODE);

    /* Copy the device name */
    This->pDisplayDevice = pCDevSettings_AllocAndCopyString(DisplayDeviceInfo->DeviceName);
    DPRINT1("This->pDisplayDevice: %ws\n", This->pDisplayDevice);
    This->pDisplayName = pCDevSettings_AllocAndCopyString(DisplayDeviceInfo->DeviceDescription);
    DPRINT1("This->pDisplayName: %ws\n", This->pDisplayName);
    This->pDisplayKey = pCDevSettings_AllocAndCopyString(DisplayDeviceInfo->DeviceKey);
    DPRINT1("This->pDisplayKey: %ws\n", This->pDisplayKey);
    This->pDisplayId = pCDevSettings_GetDeviceInstanceId(DisplayDeviceInfo->DeviceID);
    DPRINT1("This->pDisplayId: %ws\n", This->pDisplayId);
    This->pMonitorName = pCDevSettings_GetMonitorName(This->pDisplayDevice);
    DPRINT1("This->pMonitorName: %ws\n", This->pMonitorName);
    This->pMonitorDevice = pCDevSettings_GetMonitorDevice(This->pDisplayDevice);
    DPRINT1("This->pMonitorDevice: %ws\n", This->pMonitorDevice);

    /* Check pruning mode */
    This->bModesPruned = ((DisplayDeviceInfo->DeviceStateFlags & DISPLAY_DEVICE_MODESPRUNED) != 0);
    hKey = pCDevSettings_OpenDeviceKey(This,
                                       FALSE);
    if (hKey == NULL)
    {
        hKey = pCDevSettings_OpenDeviceKey(This,
                                           FALSE);
        This->bKeyIsReadOnly = TRUE;
    }
    if (hKey != NULL)
    {
        DWORD dw = 0;
        DWORD dwType, dwSize;

        dwSize = sizeof(dw);
        if (RegQueryValueEx(hKey,
                            TEXT("PruningMode"),
                            NULL,
                            &dwType,
                            (PBYTE)&dw,
                            &dwSize) == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD && dwSize == sizeof(dw))
                This->bPruningOn = (dw != 0);
        }

        RegCloseKey(hKey);
    }

    /* Initialize the shell extension interface */
    pCDevSettings_InitializeExtInterface(This);

    return S_OK;
}

static VOID
pCDevSettings_Free(PCDevSettings This)
{
    pCDevSettings_FreeString(&This->pDisplayDevice);
    pCDevSettings_FreeString(&This->pDisplayName);
    pCDevSettings_FreeString(&This->pDisplayKey);
    pCDevSettings_FreeString(&This->pDisplayId);
    pCDevSettings_FreeString(&This->pMonitorName);
    pCDevSettings_FreeString(&This->pMonitorDevice);
}

static HRESULT STDMETHODCALLTYPE
CDevSettings_QueryInterface(IDataObject* iface,
                            REFIID riid,
                            void** ppvObject)
{
    PCDevSettings This = impl_from_IDataObject(iface);

    *ppvObject = NULL;

    if (IsEqualGUID(riid,
                    &IID_IUnknown) ||
        IsEqualGUID(riid,
                    &IID_IDataObject))
    {
        *ppvObject = (PVOID)impl_to_interface(This, IDataObject);
        return S_OK;
    }
    else
    {
        DPRINT1("CDevSettings::QueryInterface: Queried unknown interface\n");
    }

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
CDevSettings_AddRef(IDataObject* iface)
{
    PCDevSettings This = impl_from_IDataObject(iface);
    return (ULONG)InterlockedIncrement((PLONG)&This->ref);
}

static ULONG STDMETHODCALLTYPE
CDevSettings_Release(IDataObject* iface)
{
    ULONG refs;
    PCDevSettings This = impl_from_IDataObject(iface);
    refs = (ULONG)InterlockedDecrement((PLONG)&This->ref);
    if (refs == 0)
        pCDevSettings_Free(This);

    return refs;
}

static HRESULT STDMETHODCALLTYPE
CDevSettings_GetData(IDataObject* iface,
                     FORMATETC* pformatetcIn,
                     STGMEDIUM* pmedium)
{
    static const WCHAR szEmpty[] = {0};
    HRESULT hr;
    PCWSTR pszRet = NULL;
    PWSTR pszBuf;
    PCDevSettings This = impl_from_IDataObject(iface);

    ZeroMemory(pmedium,
               sizeof(STGMEDIUM));

    hr = IDataObject_QueryGetData(iface,
                                  pformatetcIn);
    if (SUCCEEDED(hr))
    {
        /* Return the reqested data back to the shell extension */

        if (pformatetcIn->cfFormat == This->cfDisplayDevice)
        {
            pszRet = This->pDisplayDevice;
            DPRINT1("CDevSettings::GetData returns display device %ws\n", pszRet);
        }
        else if (pformatetcIn->cfFormat == This->cfDisplayName)
        {
            pszRet = This->pDisplayName;
            DPRINT1("CDevSettings::GetData returns display name %ws\n", pszRet);
        }
        else if (pformatetcIn->cfFormat == This->cfDisplayKey)
        {
            pszRet = This->pDisplayKey;
            DPRINT1("CDevSettings::GetData returns display key %ws\n", pszRet);
        }
        else if (pformatetcIn->cfFormat == This->cfDisplayId)
        {
            pszRet = This->pDisplayId;
            DPRINT1("CDevSettings::GetData returns display id %ws\n", pszRet);
        }
        else if (pformatetcIn->cfFormat == This->cfMonitorName)
        {
            pszRet = This->pMonitorName;
            DPRINT1("CDevSettings::GetData returns monitor name %ws\n", pszRet);
        }
        else if (pformatetcIn->cfFormat == This->cfMonitorDevice)
        {
            pszRet = This->pMonitorDevice;
            DPRINT1("CDevSettings::GetData returns monitor device %ws\n", pszRet);
        }
        else if (pformatetcIn->cfFormat == This->cfExtInterface)
        {
            PDESK_EXT_INTERFACE pIface;

            pIface = GlobalAlloc(GPTR,
                                 sizeof(*pIface));
            if (pIface != NULL)
            {
                CopyMemory(pIface,
                           &This->ExtInterface,
                           sizeof(This->ExtInterface));

                DPRINT1("CDevSettings::GetData returns the desk.cpl extension interface\n");

                pmedium->tymed = TYMED_HGLOBAL;
                pmedium->hGlobal = pIface;

                return S_OK;
            }
            else
                return E_OUTOFMEMORY;
        }
        else if (pformatetcIn->cfFormat == This->cfDisplayStateFlags)
        {
            PDWORD pdw;

            pdw = GlobalAlloc(GPTR,
                              sizeof(*pdw));
            if (pdw != NULL)
            {
                *pdw = This->StateFlags;

                DPRINT1("CDevSettings::GetData returns the display state flags %x\n", This->StateFlags);

                pmedium->tymed = TYMED_HGLOBAL;
                pmedium->hGlobal = pdw;

                return S_OK;
            }
            else
                return E_OUTOFMEMORY;
        }
        else if (pformatetcIn->cfFormat == This->cfPruningMode)
        {
            PBYTE pb;

            pb = GlobalAlloc(GPTR,
                             sizeof(*pb));
            if (pb != NULL)
            {
                *pb = (This->bModesPruned && This->bPruningOn);

                pmedium->tymed = TYMED_HGLOBAL;
                pmedium->hGlobal = pb;

                return S_OK;
            }
            else
                return E_OUTOFMEMORY;
        }

        /* NOTE: This only returns null-terminated strings! */
        if (pszRet == NULL)
            pszRet = szEmpty;

        pszBuf = GlobalAlloc(GPTR,
                             (_tcslen(pszRet) + 1) * sizeof(WCHAR));
        if (pszBuf != NULL)
        {
            _tcscpy(pszBuf,
                    pszRet);

            pmedium->tymed = TYMED_HGLOBAL;
            pmedium->hGlobal = pszBuf;

            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE
CDevSettings_GetDataHere(IDataObject* iface,
                         FORMATETC* pformatetc,
                         STGMEDIUM* pmedium)
{
    ZeroMemory(pformatetc,
               sizeof(*pformatetc));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
CDevSettings_QueryGetData(IDataObject* iface,
                          FORMATETC* pformatetc)
{
#if DEBUG
    TCHAR szFormatName[255];
#endif
    PCDevSettings This = impl_from_IDataObject(iface);

    if (pformatetc->dwAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    if (pformatetc->lindex != -1)
        return DV_E_LINDEX;

    if (!(pformatetc->tymed & TYMED_HGLOBAL))
        return DV_E_TYMED;

    /* Check if the requested data can be provided */
    if (pformatetc->cfFormat == This->cfExtInterface ||
        pformatetc->cfFormat == This->cfDisplayDevice ||
        pformatetc->cfFormat == This->cfDisplayName ||
        pformatetc->cfFormat == This->cfDisplayId ||
        pformatetc->cfFormat == This->cfDisplayKey ||
        pformatetc->cfFormat == This->cfDisplayStateFlags ||
        pformatetc->cfFormat == This->cfMonitorDevice ||
        pformatetc->cfFormat == This->cfMonitorName ||
        pformatetc->cfFormat == This->cfPruningMode)
    {
        return S_OK;
    }
#if DEBUG
    else
    {
        if (GetClipboardFormatName(pformatetc->cfFormat,
                                   szFormatName,
                                   sizeof(szFormatName) / sizeof(szFormatName[0])))
        {
            DPRINT1("CDevSettings::QueryGetData(\"%ws\")\n", szFormatName);
        }
        else
        {
            DPRINT1("CDevSettings::QueryGetData(Format %u)\n", (unsigned int)pformatetc->cfFormat);
        }
    }
#endif

    return DV_E_FORMATETC;
}

static HRESULT STDMETHODCALLTYPE
CDevSettings_GetCanonicalFormatEtc(IDataObject* iface,
                                   FORMATETC* pformatectIn,
                                   FORMATETC* pformatetcOut)
{
    HRESULT hr;

    DPRINT1("CDevSettings::GetCanonicalFormatEtc\n");

    hr = IDataObject_QueryGetData(iface,
                                  pformatectIn);
    if (SUCCEEDED(hr))
    {
        CopyMemory(pformatetcOut,
                   pformatectIn,
                   sizeof(FORMATETC));

        /* Make sure the data is target device independent */
        if (pformatectIn->ptd == NULL)
            hr = DATA_S_SAMEFORMATETC;
        else
        {
            pformatetcOut->ptd = NULL;
            hr = S_OK;
        }
    }
    else
    {
        ZeroMemory(pformatetcOut,
                   sizeof(FORMATETC));
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE
CDevSettings_SetData(IDataObject* iface,
                     FORMATETC* pformatetc,
                     STGMEDIUM* pmedium,
                     BOOL fRelease)
{
    DPRINT1("CDevSettings::SetData UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

static __inline VOID
pCDevSettings_FillFormatEtc(FORMATETC *pFormatEtc,
                            CLIPFORMAT cf)
{
    pFormatEtc->cfFormat = cf;
    pFormatEtc->ptd = NULL;
    pFormatEtc->dwAspect = DVASPECT_CONTENT;
    pFormatEtc->lindex = -1;
    pFormatEtc->tymed = TYMED_HGLOBAL;
}

static HRESULT STDMETHODCALLTYPE
CDevSettings_EnumFormatEtc(IDataObject* iface,
                           DWORD dwDirection,
                           IEnumFORMATETC** ppenumFormatEtc)
{
    HRESULT hr;
    FORMATETC fetc[9];
    PCDevSettings This = impl_from_IDataObject(iface);

    *ppenumFormatEtc = NULL;

    if (dwDirection == DATADIR_GET)
    {
        pCDevSettings_FillFormatEtc(&fetc[0],
                                    This->cfExtInterface);
        pCDevSettings_FillFormatEtc(&fetc[1],
                                    This->cfDisplayDevice);
        pCDevSettings_FillFormatEtc(&fetc[2],
                                    This->cfDisplayName);
        pCDevSettings_FillFormatEtc(&fetc[3],
                                    This->cfDisplayId);
        pCDevSettings_FillFormatEtc(&fetc[4],
                                    This->cfDisplayKey);
        pCDevSettings_FillFormatEtc(&fetc[5],
                                    This->cfDisplayStateFlags);
        pCDevSettings_FillFormatEtc(&fetc[6],
                                    This->cfMonitorName);
        pCDevSettings_FillFormatEtc(&fetc[7],
                                    This->cfMonitorDevice);
        pCDevSettings_FillFormatEtc(&fetc[8],
                                    This->cfPruningMode);

        hr = SHCreateStdEnumFmtEtc(sizeof(fetc) / sizeof(fetc[0]),
                                   fetc,
                                   ppenumFormatEtc);
    }
    else
        hr = E_NOTIMPL;

    return hr;
}

static HRESULT STDMETHODCALLTYPE
CDevSettings_DAdvise(IDataObject* iface,
                     FORMATETC* pformatetc,
                     DWORD advf,
                     IAdviseSink* pAdvSink,
                     DWORD* pdwConnection)
{
    *pdwConnection = 0;
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE
CDevSettings_DUnadvise(IDataObject* iface,
                       DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE
CDevSettings_EnumDAdvise(IDataObject* iface,
                         IEnumSTATDATA** ppenumAdvise)
{
    *ppenumAdvise = NULL;
    return OLE_E_ADVISENOTSUPPORTED;
}

static const struct IDataObjectVtbl vtblIDataObject = {
    CDevSettings_QueryInterface,
    CDevSettings_AddRef,
    CDevSettings_Release,
    CDevSettings_GetData,
    CDevSettings_GetDataHere,
    CDevSettings_QueryGetData,
    CDevSettings_GetCanonicalFormatEtc,
    CDevSettings_SetData,
    CDevSettings_EnumFormatEtc,
    CDevSettings_DAdvise,
    CDevSettings_DUnadvise,
    CDevSettings_EnumDAdvise,
};

IDataObject *
CreateDevSettings(PDISPLAY_DEVICE_ENTRY DisplayDeviceInfo)
{
    PCDevSettings This;

    This = HeapAlloc(GetProcessHeap(),
                     0,
                     sizeof(*This));
    if (This != NULL)
    {
        This->lpIDataObjectVtbl = &vtblIDataObject;
        This->ref = 1;

        if (SUCCEEDED(pCDevSettings_Initialize(This,
                                               DisplayDeviceInfo)))
        {
            return impl_to_interface(This, IDataObject);
        }

        CDevSettings_Release(impl_to_interface(This, IDataObject));
    }

    return NULL;
}

LONG WINAPI
DisplaySaveSettings(PVOID pContext,
                    HWND hwndPropSheet)
{
    //PCDevSettings This = impl_from_IDataObject((IDataObject *)Context);
    DPRINT("DisplaySaveSettings() UNIMPLEMENTED!\n");
    return DISP_CHANGE_BADPARAM;
}
