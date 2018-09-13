#include "anyfldr.h"	    // pch file
#include "resource.h"
#include "debug.h"

#pragma intrinsic( memcmp ) // for debug, to avoid all CRT

// {AA7C7080-860A-11CE-8424-08002B2CFF76}
const GUID CLSID_OtherFolder = {0xAA7C7080L, 0x860A, 0x11CE, 0x84, 0x24, 0x08, 0x00, 0x2B, 0x2C, 0xFF, 0x76};


#define ARRAYSIZE(a)	(sizeof(a)/sizeof(a[0]))

//
// Helper macro for implemting OLE classes in C
//
#define _IOffset(class, itf)         ((UINT)&(((class *)0)->itf))
#define IToClass(class, itf, pitf)   ((class  *)(((LPSTR)pitf)-_IOffset(class, itf)))


UINT g_cRefDll = 0;
HANDLE g_hinst = NULL;	// Handle to this DLL itself.

HRESULT OtherFolder_CreateInstance(IUnknown *, REFIID, void **);

void DoSendToOtherFolder(IDataObject *pdtobj, DWORD dwEffect);

LPSTR PathFindFileName(LPCSTR pszPath);

void DllAddRef()
{
    InterlockedIncrement(&g_cRefDll);
}

void DllRelease()
{
    InterlockedDecrement(&g_cRefDll);
}

STDAPI_(BOOL) DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
	g_hinst = hDll;

    return TRUE;
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefDll == 0 ? S_OK : S_FALSE;
}

// static class factory (no allocs!)

typedef struct {
    const IClassFactoryVtbl *cf;
    REFCLSID rclsid;
    HRESULT (*pfnCreate)(IUnknown *, REFIID, void **);
} OBJ_ENTRY;

extern const IClassFactoryVtbl c_CFVtbl;        // forward

//
// we always do a linear search here so put your most often used things first
//
const OBJ_ENTRY c_clsmap[] = {
    { &c_CFVtbl, &CLSID_OtherFolder,    OtherFolder_CreateInstance },
    { NULL, NULL, NULL }
};

STDMETHODIMP CCF_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = (void *)pcf;
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCF_AddRef(IClassFactory *pcf)
{
    return 3;
}

STDMETHODIMP_(ULONG) CCF_Release(IClassFactory *pcf)
{
    return 2;
}

STDMETHODIMP CCF_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter, REFIID riid, void **ppvObject)
{
    OBJ_ENTRY *this = IToClass(OBJ_ENTRY, cf, pcf);
    return this->pfnCreate(punkOuter, riid, ppvObject);
}

STDMETHODIMP CCF_LockServer(IClassFactory *pcf, BOOL fLock)
{
    if (fLock)
	DllAddRef();
    else
	DllRelease();
    return S_OK;
}

const IClassFactoryVtbl c_CFVtbl = {
    CCF_QueryInterface, CCF_AddRef, CCF_Release,
    CCF_CreateInstance,
    CCF_LockServer
};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IClassFactory))
    {
        const OBJ_ENTRY *pcls;
        for (pcls = c_clsmap; pcls->rclsid; pcls++)
        {
            if (IsEqualIID(rclsid, pcls->rclsid))
            {
                *ppv = (void *)&(pcls->cf);
                return NOERROR;
            }
        }
    }
    // failure
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;;
}

typedef struct
{
    IDropTarget		dt;
    IPersistFile        pf;
    IShellExtInit	sxi;
    int 		cRef;

    DWORD		grfKeyStateLast;
    DWORD		dwEffectLast;

    TCHAR		szPath[MAX_PATH];   // working dir

} COtherFolder;

STDMETHODIMP OtherFolder_QueryInterface(IDropTarget *pdropt, REFIID riid, void **ppv)
{
    COtherFolder *this = IToClass(COtherFolder, dt, pdropt);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDropTarget))
    {
	*ppv = &this->dt;
    }
    else if (IsEqualIID(riid, &IID_IPersistFile))
    {
        *ppv = &this->pf;
    }
    else if (IsEqualIID(riid, &IID_IShellExtInit))
    {
        *ppv = &this->sxi;
    }
    else
    {
	*ppv = NULL;
	return E_NOINTERFACE;
    }

    this->cRef++;
    // pdropt->lpVtbl->AddRef(pdropt);
    return S_OK;
}

STDMETHODIMP_(ULONG) OtherFolder_AddRef(IDropTarget *pdropt)
{
    COtherFolder *this = IToClass(COtherFolder, dt, pdropt);
    this->cRef++;
    return this->cRef;
}

STDMETHODIMP_(ULONG) OtherFolder_Release(IDropTarget *pdropt)
{
    COtherFolder *this = IToClass(COtherFolder, dt, pdropt);

    this->cRef--;
    if (this->cRef > 0)
	return this->cRef;

    LocalFree((HLOCAL)this);

    DllRelease();

    return 0;
}

STDMETHODIMP OtherFolder_DragEnter(IDropTarget *pdropt, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    COtherFolder *this = IToClass(COtherFolder, dt, pdropt);

    DebugMsg(DM_TRACE, "OtherFolder_DragEnter");
    this->grfKeyStateLast = grfKeyState;
    this->dwEffectLast = *pdwEffect;
    return S_OK;
}

STDMETHODIMP OtherFolder_DragOver(IDropTarget *pdropt, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    COtherFolder *this = IToClass(COtherFolder, dt, pdropt);

    this->grfKeyStateLast = grfKeyState;

    *pdwEffect = this->dwEffectLast;
    return S_OK;
}

STDMETHODIMP OtherFolder_DragLeave(IDropTarget *pdropt)
{
    COtherFolder *this = IToClass(COtherFolder, dt, pdropt);
    return S_OK;
}

STDMETHODIMP OtherFolder_Drop(IDropTarget *pdropt, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    COtherFolder *this = IToClass(COtherFolder, dt, pdropt);

    DoSendToOtherFolder(pdtobj, this->dwEffectLast);
    return S_OK;
}

const IDropTargetVtbl c_OtherFolder_DTVtbl =
{
    OtherFolder_QueryInterface, OtherFolder_AddRef, OtherFolder_Release,
    OtherFolder_DragEnter,
    OtherFolder_DragOver,
    OtherFolder_DragLeave,
    OtherFolder_Drop,
};


STDMETHODIMP OtherFolder_PF_QueryInterface(IPersistFile *ppf, REFIID riid, void **ppv)
{
    COtherFolder *this = IToClass(COtherFolder, pf, ppf);
    return OtherFolder_QueryInterface(&this->dt, riid, ppv);
}

STDMETHODIMP_(ULONG) OtherFolder_PF_AddRef(IPersistFile *ppf)
{
    COtherFolder *this = IToClass(COtherFolder, pf, ppf);
    return ++this->cRef;
}

STDMETHODIMP_(ULONG) OtherFolder_PF_Release(IPersistFile *ppf)
{
    COtherFolder *this = IToClass(COtherFolder, pf, ppf);
    return OtherFolder_Release(&this->dt);
}

STDMETHODIMP OtherFolder_GetClassID(IPersistFile *ppf, CLSID *pClassID)
{
    *pClassID = CLSID_OtherFolder;
    return S_OK;
}

STDMETHODIMP OtherFolder_IsDirty(IPersistFile *psf)
{
    return S_FALSE;
}

STDMETHODIMP OtherFolder_Load(IPersistFile *psf, LPCOLESTR pwszFile, DWORD grfMode)
{
    DebugMsg(DM_TRACE, "OtherFolder_Load");

    return S_OK;
}

STDMETHODIMP OtherFolder_Save(IPersistFile *psf, LPCOLESTR pwszFile, BOOL fRemember)
{
    return S_OK;
}

STDMETHODIMP OtherFolder_SaveCompleted(IPersistFile *psf, LPCOLESTR pwszFile)
{
    return S_OK;
}

STDMETHODIMP OtherFolder_GetCurFile(IPersistFile *psf, LPOLESTR *ppszFileName)
{
    *ppszFileName = NULL;
    return S_OK;
}

const IPersistFileVtbl c_OtherFolder_PFVtbl = {
    OtherFolder_PF_QueryInterface,
    OtherFolder_PF_AddRef,
    OtherFolder_PF_Release,
    OtherFolder_GetClassID,
    OtherFolder_IsDirty,
    OtherFolder_Load,
    OtherFolder_Save,
    OtherFolder_SaveCompleted,
    OtherFolder_GetCurFile
};


STDMETHODIMP OtherFolder_SXI_QueryInterface(IShellExtInit *psxi, REFIID riid, LPVOID *ppv)
{
    COtherFolder *this = IToClass(COtherFolder, sxi, psxi);
    return OtherFolder_QueryInterface(&this->dt, riid, ppv);
}

STDMETHODIMP_(ULONG) OtherFolder_SXI_AddRef(IShellExtInit *psxi)
{
    COtherFolder *this = IToClass(COtherFolder, sxi, psxi);
    return ++this->cRef;
}

STDMETHODIMP_(ULONG) OtherFolder_SXI_Release(IShellExtInit *psxi)
{
    COtherFolder *this = IToClass(COtherFolder, sxi, psxi);
    return OtherFolder_Release(&this->dt);
}

STDMETHODIMP OtherFolder_SXI_Initialize(IShellExtInit *psxi, LPCITEMIDLIST pidl, IDataObject *pdtobj, HKEY hkeyProgID)
{
    COtherFolder *this = IToClass(COtherFolder, sxi, psxi);

    DebugMsg(DM_TRACE, "OtherFolder_SXI_Initialize");

    return S_OK;
}

IShellExtInitVtbl c_OtherFolder_SXIVtbl = {
    OtherFolder_SXI_QueryInterface, OtherFolder_SXI_AddRef, OtherFolder_SXI_Release,
    OtherFolder_SXI_Initialize
};


HRESULT OtherFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hres;
    COtherFolder *this;

    *ppv = NULL;		// assume error

    DebugMsg(DM_TRACE, "OtherFolder_CreateInstance");

    if (punkOuter)
	return CLASS_E_NOAGGREGATION;

    this = (COtherFolder *)LocalAlloc(LPTR, sizeof(COtherFolder));
    if (this)
    {
    	this->dt.lpVtbl = &c_OtherFolder_DTVtbl;
    	this->pf.lpVtbl = &c_OtherFolder_PFVtbl;
    	this->sxi.lpVtbl = &c_OtherFolder_SXIVtbl;

    	this->cRef = 1;

    	DllAddRef();

        hres = OtherFolder_QueryInterface(&this->dt, riid, ppv);
        OtherFolder_Release(&this->dt);
    }
    else
        hres = E_OUTOFMEMORY;

    return hres;
}

// Procedure for uninstalling this DLL (given an INF file)
void CALLBACK Uninstall(HWND hwndStub, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    const static char szFmt[] = "rundll.exe setupx.dll,InstallHinfSection DefaultUninstall 132 %s";
    char szCmd[sizeof(szFmt) + MAX_PATH];
    char szTitle[100];
    char szMessage[256];

    DebugMsg(DM_TRACE, "Uninstall: '%s'", lpszCmdLine);

    if (!lpszCmdLine || lstrlen(lpszCmdLine)>=MAX_PATH)
	return;

    LoadString(g_hinst, IDS_THISDLL, szTitle, sizeof(szTitle));
    LoadString(g_hinst, IDS_SUREUNINST, szMessage, sizeof(szMessage));

    if (MessageBox(HWND_DESKTOP, szMessage, szTitle, MB_YESNO | MB_ICONSTOP) != IDYES)
	return;
    
    wsprintf(szCmd, szFmt, lpszCmdLine);

    // Note that I use START.EXE, to minimize the chance that this DLL will still be loaded
    // when SETUPX tries to delete it
    ShellExecute(hwndStub, NULL, "start.exe", szCmd, NULL, SW_SHOWMINIMIZED);
}


typedef struct {
    HWND hDlg;
    DWORD dwEffect;
    HANDLE hMRU;
    HWND hwndTo;	// IDC_TO, combobox
    TCHAR szFiles[1];	// variable size data, double null string in HDROP

} OTHERFOLDERDATA;

const char c_szOtherFldMRU[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\OtherFolder";

LPSTR PathFindFileName(LPCSTR pPath)
{
    LPCSTR pT;

    for (pT = pPath; *pPath; pPath = AnsiNext(pPath)) {
        if ((pPath[0] == '\\' || pPath[0] == ':') && pPath[1] && (pPath[1] != '\\'))
            pT = pPath + 1;
    }

    return (LPSTR)pT;   // const -> non const
}

void OtherFolderInitDlg(HWND hDlg, OTHERFOLDERDATA *pofd)
{
    LPSTR psz;
    HWND hwndLB = GetDlgItem(hDlg, IDC_FROM);

    pofd->hDlg = hDlg;
    pofd->hwndTo = GetDlgItem(hDlg, IDC_TO);

    CheckRadioButton(hDlg, IDC_MOVE, IDC_COPY, IDC_COPY);

    for (psz = pofd->szFiles; *psz; psz += lstrlen(psz) + 1)
    {
        ListBox_AddString(hwndLB, PathFindFileName(psz));
    }

    SendMessage(hDlg, WM_SETICON, 1, (LPARAM)LoadIcon(GetWindowInstance(hDlg), MAKEINTRESOURCE(IDI_OTHERFLD)));

    {
        MRUINFO mi = {
            sizeof(MRUINFO), 20,
            MRU_CACHEWRITE, HKEY_CURRENT_USER,
            c_szOtherFldMRU, NULL
        };

        pofd->hMRU = CreateMRUList(&mi);

	if (pofd->hMRU)
	{
            int i;
            char szCommand[MAX_PATH];

            for (i = 0; EnumMRUList(pofd->hMRU, i, szCommand, sizeof(szCommand)) > 0; i++)
            {
                ComboBox_AddString(pofd->hwndTo, szCommand);
            }
	}

	ComboBox_SetCurSel(pofd->hwndTo, 0);
        ComboBox_LimitText(pofd->hwndTo, MAX_PATH-1);
    }
    SetForegroundWindow(hDlg);
}


int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch(uMsg) {
    case BFFM_INITIALIZED:
        // lpData is the path string
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        break;
    }
    return 0;
}

void DoBrowse(OTHERFOLDERDATA *pofd)
{
    LPITEMIDLIST pidl;
    TCHAR szPath[MAX_PATH];
    BROWSEINFO bi = {
	pofd->hDlg, NULL, NULL,
	"Pick a target folder",
	BIF_RETURNONLYFSDIRS,
	BrowseCallback,
	(LPARAM)szPath
    };

    ComboBox_GetText(pofd->hwndTo, szPath, ARRAYSIZE(szPath));

    pidl = SHBrowseForFolder(&bi);
    if (pidl)
    {
        IMalloc *pmalloc;

	if (SHGetPathFromIDList(pidl, szPath))
	{
	    ComboBox_SetText(pofd->hwndTo, szPath);
	}

	SHGetMalloc(&pmalloc);
	pmalloc->lpVtbl->Free(pmalloc, pidl);
	pmalloc->lpVtbl->Release(pmalloc);
    }
    SetFocus(pofd->hwndTo);
}

#if 1
#define PathIsUNC(psz) (*(WORD *)(psz) == 0x5C5C)	// check for double backslash '\\'

BOOL ValidateUNC(HWND hwnd, LPCSTR pszFile)
{
    if (PathIsUNC(pszFile) && (GetFileAttributes(pszFile) == (DWORD)-1))
    {
	DWORD dwResult;
	char szAccessName[MAX_PATH];
	UINT cbAccessName = sizeof(szAccessName);
	NETRESOURCE rc = {0, RESOURCETYPE_DISK, 0, 0, NULL, (LPSTR)pszFile, NULL, NULL};

	if (WNetUseConnection(hwnd, &rc, NULL, NULL,
		CONNECT_TEMPORARY | CONNECT_INTERACTIVE, szAccessName, &cbAccessName, &dwResult))
	    return FALSE;
    }

    return TRUE;
}
#else
#define ValidateUNC(w, p)	(TRUE)
#endif

void DoTransfer(OTHERFOLDERDATA *pofd)
{
    LPTSTR pszSpec;
    TCHAR szTemp[MAX_PATH], szDest[MAX_PATH];
    SHFILEOPSTRUCT fo = {pofd->hDlg, FO_COPY, pofd->szFiles, szDest, FOF_ALLOWUNDO};

    // HACK, set current directory so GetFullPathName can do it's stuff
    // any other thread in the shell could change this (cur dir is per
    // process state) but that is unlikely

    lstrcpyn(szTemp, pofd->szFiles, ARRAYSIZE(szTemp));

    // wack off file spec, careful with a root
    pszSpec = PathFindFileName(szTemp);
    if (pszSpec != szTemp)
        *pszSpec = 0;

    SetCurrentDirectory(szTemp);

    ComboBox_GetText(pofd->hwndTo, szTemp, ARRAYSIZE(szTemp) - 1);

    GetFullPathName(szTemp, ARRAYSIZE(szDest), szDest, NULL);

    if (!ValidateUNC(pofd->hDlg, szDest))
        return;

    szDest[lstrlen(szDest) + 1] = 0;     // double NULL terminate

    // BUGBUG: validate desitionation

    if (pofd->hMRU)
        AddMRUString(pofd->hMRU, szDest);

    if (IsDlgButtonChecked(pofd->hDlg, IDC_MOVE))
        fo.wFunc = FO_MOVE;

    SHFileOperation(&fo);
}

BOOL CALLBACK OtherFolderDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    OTHERFOLDERDATA *pofd = (OTHERFOLDERDATA *)GetWindowLong(hDlg, DWL_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
	SetWindowLong(hDlg, DWL_USER, lParam);
	OtherFolderInitDlg(hDlg, (OTHERFOLDERDATA *)lParam);
	break;

    case WM_DESTROY:
        if (pofd && pofd->hMRU)
	    FreeMRUList(pofd->hMRU);
	break;

    case WM_COMMAND:
	switch (GET_WM_COMMAND_ID(wParam, lParam)) {

	case IDC_BROWSE:
	    DoBrowse(pofd);
	    break;

	case IDC_MOVE:
	case IDC_COPY:
            CheckRadioButton(hDlg, IDC_MOVE, IDC_COPY, GET_WM_COMMAND_ID(wParam, lParam));
	    break;

	case IDCANCEL:
	    EndDialog(hDlg, IDCANCEL);
	    break;

	case IDOK:
	    DoTransfer(pofd);
	    EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
	    break;
	}
	break;

    default:
	return FALSE;
    }
    return TRUE;
}

void OFReleaseStgMedium(STGMEDIUM *pmedium)
{
    if (pmedium->pUnkForRelease)
    {
	pmedium->pUnkForRelease->lpVtbl->Release(pmedium->pUnkForRelease);
    }
    else
    {
	switch (pmedium->tymed)
	{
	case TYMED_HGLOBAL:
	    GlobalFree(pmedium->hGlobal);
	    break;

	case TYMED_ISTORAGE: // depends on pstm/pstg overlap in union
	case TYMED_ISTREAM:
	    pmedium->pstm->lpVtbl->Release(pmedium->pstm);
	    break;
	}
    }
}


DWORD CALLBACK SendToOtherFolderhreadProc(LPVOID pv)
{
    OTHERFOLDERDATA *pofd = (OTHERFOLDERDATA *)pv;

    DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, OtherFolderDlgProc, (LPARAM)pofd);

    GlobalFree(pofd);

    DllRelease();

    return 0;
}


void DoSendToOtherFolder(IDataObject *pdtobj, DWORD dwEffect)
{
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (SUCCEEDED(pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium)))
    {
	DROPFILES *pdrop = GlobalLock(medium.hGlobal);
	if (pdrop)
	{
	    // copy data for thread (can't pass object across threads, need
            // state in memory that is not on the stack)

	    DWORD dwSizeStrings = GlobalSize(medium.hGlobal) - pdrop->pFiles;
	    OTHERFOLDERDATA *pofd = GlobalAlloc(GPTR, dwSizeStrings + sizeof(*pofd));
	    if (pofd)
	    {
		HANDLE hThread;
		DWORD idThread;

                pofd->dwEffect = dwEffect;

	        CopyMemory(pofd->szFiles, (LPBYTE)pdrop + pdrop->pFiles, dwSizeStrings);

                DllAddRef();
		hThread = CreateThread(NULL, 0, SendToOtherFolderhreadProc, pofd, 0, &idThread);
		if (hThread)
		{
		    CloseHandle(hThread);
		}
		else
                {
		    GlobalFree(pofd);
                    DllRelease();
                }
	    }

	    GlobalUnlock(medium.hGlobal);
	}

	OFReleaseStgMedium(&medium);
    }
}
