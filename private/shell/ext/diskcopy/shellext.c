
#include "diskcopy.h"
#include "ids.h"

#pragma data_seg(".text", "CODE")
#define INITGUID
#include <initguid.h>
// {59099400-57FF-11CE-BD94-0020AF85B590}
DEFINE_GUID(CLSID_DriveMenuExt, 0x59099400L, 0x57FF, 0x11CE, 0xBD, 0x94, 0x00, 0x20, 0xAF, 0x85, 0xB5, 0x90);
#pragma data_seg()

void DoRunDllThing(int iDrive);
BOOL DriveIdIsFloppy(int iDrive);

HINSTANCE g_hinst = NULL;

UINT g_cRefThisDll = 0;         // Reference count of this DLL.

//----------------------------------------------------------------------------
#ifdef UNICODE
#define WinExec WinExecW

//
//  For UNICODE create a companion to ANSI only base WinExec api.
//

UINT WINAPI WinExecW (LPTSTR lpCmdLine, UINT uCmdShow)
{
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInformation;

    //
    // Create the process
    //
    
    memset (&StartupInfo, 0, sizeof(StartupInfo));
    
    StartupInfo.cb = sizeof(StartupInfo);
    
    StartupInfo.wShowWindow = (WORD)uCmdShow;
    
    if (CreateProcess ( NULL,
                        lpCmdLine,            // CommandLine
                        NULL,
                        NULL,
                        FALSE,
                        NORMAL_PRIORITY_CLASS,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation
                        ))
    {
        CloseHandle(ProcessInformation.hThread);
        CloseHandle(ProcessInformation.hProcess);
    }

    return(1);

}
#endif  // UNICODE


BOOL APIENTRY LibMain(HANDLE hDll, DWORD dwReason, void *lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hinst = hDll;
        DisableThreadLibraryCalls(hDll);
    	break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
    default:
        break;
    }

    return TRUE;
}


typedef struct
{
    IClassFactory      cf;
    UINT               cRef;
    LPFNCREATEINSTANCE pfnCI;           // CreateInstance callback entry
} CClassFactory;

STDMETHODIMP CClassFactory_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppv)
{
    CClassFactory *this = IToClass(CClassFactory, cf, pcf);
    if (IsEqualIID(riid, &IID_IClassFactory) || 
        IsEqualIID(riid, &IID_IUnknown))
    {
        (LPCLASSFACTORY)*ppv = &this->cf;
        this->cRef++;
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory_AddRef(IClassFactory *pcf)
{
    CClassFactory *this = IToClass(CClassFactory, cf, pcf);
    return ++this->cRef;
}

STDMETHODIMP_(ULONG) CClassFactory_Release(IClassFactory *pcf)
{
    CClassFactory *this = IToClass(CClassFactory, cf, pcf);
    if (--this->cRef > 0)
        return this->cRef;

    LocalFree((HLOCAL)this);
    return 0;
}

STDMETHODIMP CClassFactory_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    CClassFactory *this = IToClass(CClassFactory, cf, pcf);
    return this->pfnCI(punkOuter, riid, ppv);
}

STDMETHODIMP CClassFactory_LockServer(IClassFactory *pcf, BOOL fLock)
{
    return E_NOTIMPL;
}

#pragma data_seg(".text")
IClassFactoryVtbl c_vtblCClassFactory = {
    CClassFactory_QueryInterface,
    CClassFactory_AddRef,
    CClassFactory_Release,
    CClassFactory_CreateInstance,
    CClassFactory_LockServer
};
#pragma data_seg()

STDAPI CreateClassObject(REFIID riid, LPFNCREATEINSTANCE pfnCI, void **ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IClassFactory))
    {
        CClassFactory *pcf = (CClassFactory *)LocalAlloc(LPTR, sizeof(CClassFactory));
        if (pcf)
        {
            pcf->cf.lpVtbl = &c_vtblCClassFactory;
            pcf->cRef++;
            pcf->pfnCI = pfnCI;

            (IClassFactory *)*ppv = &pcf->cf;
            return S_OK;
        }
        return E_OUTOFMEMORY;
    }
    return E_NOINTERFACE;
}

typedef struct 
{
    IContextMenu    _ctm;           // 1st base class
    IShellExtInit   _sxi;           // 2nd base class
    UINT            _cRef;          // reference count

    int             iDrive;         // drive # or -1 if not on a drive
} CDriveMenuExt;

#define DMX_OFFSETOF(x)	        (PtrToUlong(&((CDriveMenuExt *)0)->x))
#define PVOID2PDMX(pv,offset)   ((CDriveMenuExt *)(((LPBYTE)pv)-offset))
#define PCM2PDMX(pctm)	        PVOID2PDMX(pctm, DMX_OFFSETOF(_ctm))
#define PSXI2PDMX(psxi)	        PVOID2PDMX(psxi, DMX_OFFSETOF(_sxi))

//
// Vtable prototype
//
extern IContextMenuVtbl     c_DriveMenuExt_CTMVtbl;
extern IShellExtInitVtbl    c_DriveMenuExt_SXIVtbl;

HRESULT CDriveMenuExt_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    CDriveMenuExt *psmx;

    *ppvOut = NULL;

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    psmx = LocalAlloc(LPTR, sizeof(CDriveMenuExt));
    if (psmx)
    {
        psmx->_ctm.lpVtbl = &c_DriveMenuExt_CTMVtbl;
        psmx->_sxi.lpVtbl = &c_DriveMenuExt_SXIVtbl;
        // psmx->_cRef = 0;

        return c_DriveMenuExt_CTMVtbl.QueryInterface(&psmx->_ctm, riid, ppvOut);
    }

    return E_OUTOFMEMORY;
}

int DriveFromDataObject(IDataObject *pdtobj)
{
    int iDrive = -1;
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (pdtobj && SUCCEEDED(pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium)))
    {
        if (DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0) == 1)
        {
            TCHAR szFile[MAX_PATH];

            DragQueryFile((HDROP)medium.hGlobal, 0, szFile, ARRAYSIZE(szFile));

            Assert(lstrlen(szFile) == 3); // we are on the "Drives" class

            iDrive = DRIVEID(szFile);
        }

        if (medium.pUnkForRelease)
            medium.pUnkForRelease->lpVtbl->Release(medium.pUnkForRelease);
        else
            GlobalFree(medium.hGlobal);
    }
    return iDrive;
}

STDMETHODIMP MenuExt_Initialize(IShellExtInit *psxi, LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    CDriveMenuExt *this = PSXI2PDMX(psxi);

    this->iDrive = DriveFromDataObject(pdtobj);
    if (this->iDrive >= 0)
    {
        if (!DriveIdIsFloppy(this->iDrive))
            this->iDrive = -1;
    }

    return S_OK;
}

STDMETHODIMP MenuExt_QueryContextMenu(IContextMenu *pcm, HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    CDriveMenuExt *this = PCM2PDMX(pcm);

    if (this->iDrive >= 0)
    {
        TCHAR szMenu[64];

        LoadString(g_hinst, IDS_DISKCOPYMENU, szMenu, ARRAYSIZE(szMenu));

        // this will end up right above "Format Disk..."
        InsertMenu(hmenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, idCmdFirst, szMenu);
        InsertMenu(hmenu, indexMenu++, MF_STRING | MF_BYPOSITION, idCmdFirst + 1, szMenu);
    }
    return (HRESULT)2;	// room for 2 menu cmds, only use one now...
}

STDMETHODIMP MenuExt_InvokeCommand(IContextMenu *pcm, LPCMINVOKECOMMANDINFO pici)
{
    CDriveMenuExt *this = PCM2PDMX(pcm);

    if (HIWORD(pici->lpVerb) == 0)
    {
        // UINT idCmd = LOWORD(pici->lpVerb);

        Assert(LOWORD(pici->lpVerb) == 0);

        DoRunDllThing(this->iDrive);

        return S_OK;
    }

    return E_INVALIDARG;
}

STDMETHODIMP MenuExt_GetCommandString(IContextMenu *pctm, UINT_PTR idCmd, UINT uType,
                                      UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    switch(uType)
    {
        case GCS_HELPTEXTA:
            return(LoadStringA(g_hinst, IDS_HELPSTRING, pszName, cchMax) ? NOERROR : E_OUTOFMEMORY);
        case GCS_VERBA:
            return(LoadStringA(g_hinst, IDS_VERBSTRING, pszName, cchMax) ? NOERROR : E_OUTOFMEMORY);
        case GCS_HELPTEXTW:
            return(LoadStringW(g_hinst, IDS_HELPSTRING, (LPWSTR)pszName, cchMax) ? NOERROR : E_OUTOFMEMORY);
        case GCS_VERBW:
            return(LoadStringW(g_hinst, IDS_VERBSTRING, (LPWSTR)pszName, cchMax) ? NOERROR : E_OUTOFMEMORY);
        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
        default:
           return S_OK;
    }
}

STDMETHODIMP_(UINT) MenuExt_CTM_AddRef(IContextMenu *pctm)
{
    CDriveMenuExt *this = PCM2PDMX(pctm);
    g_cRefThisDll++;
    return ++this->_cRef;
}


STDMETHODIMP_(UINT) MenuExt_SXI_AddRef(IShellExtInit *psxi)
{
    CDriveMenuExt *this = PSXI2PDMX(psxi);
    g_cRefThisDll++;
    return ++this->_cRef;
}

STDMETHODIMP_(UINT) MenuExt_Release(IContextMenu *pctm)
{
    CDriveMenuExt *this = PCM2PDMX(pctm);
    if (--this->_cRef)
        return this->_cRef;

    LocalFree((HLOCAL)this);

    g_cRefThisDll--;

    return 0;
}

STDMETHODIMP_(UINT) MenuExt_SXI_Release(IShellExtInit *psxi)
{
    CDriveMenuExt *this = PSXI2PDMX(psxi);
    return MenuExt_Release(&this->_ctm);
}

STDMETHODIMP MenuExt_CTM_QueryInterface(IContextMenu *pctm, REFIID riid, void **ppvOut)
{
    CDriveMenuExt *this = PCM2PDMX(pctm);
    if (IsEqualIID(riid, &IID_IContextMenu) || IsEqualIID(riid, &IID_IUnknown))
    {
        (IContextMenu *)*ppvOut = &this->_ctm;
        this->_cRef++;
        return S_OK;
    }
    if (IsEqualIID(riid, &IID_IShellExtInit))
    {
        (IShellExtInit *)*ppvOut = &this->_sxi;
        this->_cRef++;
        return S_OK;
    }
    *ppvOut = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP MenuExt_SXI_QueryInterface(IShellExtInit *psxi, REFIID riid, void **ppv)
{
    CDriveMenuExt *this = PSXI2PDMX(psxi);
    return MenuExt_CTM_QueryInterface(&this->_ctm, riid, ppv);
}


#pragma data_seg(".text")
IContextMenuVtbl c_DriveMenuExt_CTMVtbl = {
    MenuExt_CTM_QueryInterface,
    MenuExt_CTM_AddRef,
    MenuExt_Release,
    MenuExt_QueryContextMenu,
    MenuExt_InvokeCommand,
    MenuExt_GetCommandString,
};

IShellExtInitVtbl c_DriveMenuExt_SXIVtbl = {
    MenuExt_SXI_QueryInterface,
    MenuExt_SXI_AddRef,
    MenuExt_SXI_Release,
    MenuExt_Initialize
};
#pragma data_seg()

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(rclsid, &CLSID_DriveMenuExt))
        return CreateClassObject(riid, CDriveMenuExt_CreateInstance, ppv);

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefThisDll == 0 ? S_OK : S_FALSE;
}


TCHAR const c_szExecTemplate[] = TEXT("rundll32.exe %s,DiskCopyRunDll %d");

void DoRunDllThing(int iDrive)
{
    TCHAR szModule[MAX_PATH];
    TCHAR szExec[MAX_PATH + ARRAYSIZE(c_szExecTemplate) + 5];

    GetModuleFileName(g_hinst, szModule, ARRAYSIZE(szModule));

    wsprintf(szExec, c_szExecTemplate, szModule, iDrive);

    WinExec(szExec, SW_SHOWNORMAL);
}

// allow command lines to do diskcopy, use the syntax:
// rundll32.dll diskcopy.dll,DiskCopyRunDll

void WINAPI DiskCopyRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    int iDrive = StrToIntA(pszCmdLine);

    SHCopyDisk(NULL, iDrive, iDrive, 0);
}

void WINAPI DiskCopyRunDllW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR pwszCmdLine, int nCmdShow)
{
    int iDrive = StrToIntW(pwszCmdLine);

    SHCopyDisk(NULL, iDrive, iDrive, 0);
}
