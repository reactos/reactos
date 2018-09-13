#include "shellprv.h"
#pragma  hdrstop

#ifdef WINNT
//
// Must have access to:
// IID_IPrinterFolder & IID_IFolderNotify interfaces
// declared in windows\inc\winprtp.h
//
#include <initguid.h>
#include <winprtp.h>
#endif

#include "printer.h"
#include "copy.h"
#include "fstreex.h"
#include "datautil.h"
#include "infotip.h"

#include "ovrlaymn.h"
#include "netview.h"

// Why aren't these in some printui.dll header?
HANDLE hFolderRegister(LPCTSTR psz1, LPCITEMIDLIST pidl);
void vFolderUnregister(HANDLE h);


//
// The many different ways of getting to printer information:
//     Win9x                       -> PRINTER_INFO_1
//     WINNT (old)                 -> PRINTER_INFO_4
//     WINNT (with PRN_FOLDERDATA) -> FOLDER_PRINTER_DATA
//
#ifdef WINNT
    #define PRINTER_INFO_TYPE PRINTER_INFO_4
    #define PRINTER_INFO_LEVEL 4
    #define TYPICAL_PRINTER_INFO_2_SIZE 900

    typedef struct _NT40_PFOLDER_PRINTER_DATA {
        LPCTSTR pName;
        LPCTSTR pComment;
        DWORD   Status;
        DWORD   Attributes;
        DWORD   cJobs;
    } NT40_FOLDER_PRINTER_DATA, *NT40_PFOLDER_PRINTER_DATA;

#else
    #define PRINTER_INFO_TYPE PRINTER_INFO_1
    #define GetPrinterName( p, index ) ((p)[index].pName)
    #define PRINTER_INFO_LEVEL 1
    #define TYPICAL_PRINTER_INFO_2_SIZE 700
#endif

LPCTSTR Printer_BuildPrinterName(CPrinterFolder *psf, LPTSTR pszFullPrinter, LPIDPRINTER pidp, UNALIGNED const TCHAR* pszPrinter);
HRESULT CPrinters_GetQueryInfo(IShellFolder2 *psf, LPCITEMIDLIST pidl, void **ppvOut);

//
// Data structure to represent the IShellFolder of the directory that contains
// our printer shortcuts
//

IShellFolder2 *g_psfPrintHood;

const struct _PRINTERCOLS
{
    UINT    uID;
    int     fmt;
    int     cxChar;
} s_printers_cols[] = {
    { IDS_PSD_PRNNAME  , LVCFMT_LEFT  , 20, },
    { IDS_PSD_QUEUESIZE, LVCFMT_CENTER, 12, },
    { IDS_PRQ_STATUS   , LVCFMT_LEFT  , 12, },
    { IDS_PSD_COMMENT  , LVCFMT_LEFT  , 25, },
#ifdef WINNT
    { IDS_PSD_LOCATION , LVCFMT_LEFT  , 20, },
    { IDS_PSD_MODEL    , LVCFMT_LEFT  , 20, },
#endif
} ;

enum
{
    PRINTERS_ICOL_NAME = 0,
    PRINTERS_ICOL_QUEUESIZE,
    PRINTERS_ICOL_STATUS,
    PRINTERS_ICOL_COMMENT,
#ifdef WINNT
    PRINTERS_ICOL_LOCATION,
    PRINTERS_ICOL_MODEL,
#endif
    PRINTERS_ICOL_MAX
} ;

const STATUSSTUFF ssPrinterStatus[] =
{
    PRINTER_STATUS_PAUSED,              IDS_PRQSTATUS_PAUSED,
    PRINTER_STATUS_ERROR,               IDS_PRQSTATUS_ERROR,
    PRINTER_STATUS_PENDING_DELETION,    IDS_PRQSTATUS_PENDING_DELETION,
    PRINTER_STATUS_PAPER_JAM,           IDS_PRQSTATUS_PAPER_JAM,
    PRINTER_STATUS_PAPER_OUT,           IDS_PRQSTATUS_PAPER_OUT,
    PRINTER_STATUS_MANUAL_FEED,         IDS_PRQSTATUS_MANUAL_FEED,
    PRINTER_STATUS_PAPER_PROBLEM,       IDS_PRQSTATUS_PAPER_PROBLEM,
    PRINTER_STATUS_OFFLINE,             IDS_PRQSTATUS_OFFLINE,
    PRINTER_STATUS_IO_ACTIVE,           IDS_PRQSTATUS_IO_ACTIVE,
    PRINTER_STATUS_BUSY,                IDS_PRQSTATUS_BUSY,
    PRINTER_STATUS_PRINTING,            IDS_PRQSTATUS_PRINTING,
    PRINTER_STATUS_OUTPUT_BIN_FULL,     IDS_PRQSTATUS_OUTPUT_BIN_FULL,
    PRINTER_STATUS_NOT_AVAILABLE,       IDS_PRQSTATUS_NOT_AVAILABLE,
    PRINTER_STATUS_WAITING,             IDS_PRQSTATUS_WAITING,
    PRINTER_STATUS_PROCESSING,          IDS_PRQSTATUS_PROCESSING,
    PRINTER_STATUS_INITIALIZING,        IDS_PRQSTATUS_INITIALIZING,
    PRINTER_STATUS_WARMING_UP,          IDS_PRQSTATUS_WARMING_UP,
    PRINTER_STATUS_TONER_LOW,           IDS_PRQSTATUS_TONER_LOW,
    PRINTER_STATUS_NO_TONER,            IDS_PRQSTATUS_NO_TONER,
    PRINTER_STATUS_PAGE_PUNT,           IDS_PRQSTATUS_PAGE_PUNT,
    PRINTER_STATUS_USER_INTERVENTION,   IDS_PRQSTATUS_USER_INTERVENTION,
    PRINTER_STATUS_OUT_OF_MEMORY,       IDS_PRQSTATUS_OUT_OF_MEMORY,
    PRINTER_STATUS_DOOR_OPEN,           IDS_PRQSTATUS_DOOR_OPEN,

    PRINTER_HACK_WORK_OFFLINE,          IDS_PRQSTATUS_WORK_OFFLINE,
    0, 0
} ;


//
// Helper function to get the printer name on NT 4.0 and NT 5.0
//
#ifdef WINNT

#ifdef PRN_FOLDERDATA

LPCTSTR GetPrinterName( PFOLDER_PRINTER_DATA pPrinter, UINT Index ) 
{
    LPCTSTR pPrinterName = NULL;

    if( g_bRunOnNT5 )
        pPrinterName = ((PFOLDER_PRINTER_DATA)(((PBYTE)pPrinter)+pPrinter->cbSize*Index))->pName;
    else
        pPrinterName = ((NT40_PFOLDER_PRINTER_DATA)pPrinter)[Index].pName;

    return pPrinterName;
}

#else

LPTSTR GetPrinterName( PRINTER_INFO_TYPE *pPrinter, UINT Index ) 
{
    return pPrinter[Index].pPrinterName;
}

#endif // PRN_FOLDERDATA

#endif // WINNT

IShellFolder2 *CPrintRoot_GetPSF()
{
    HRESULT hres;

    hres = SHCacheTrackingFolder(MAKEINTIDLIST(CSIDL_PRINTERS),
                    CSIDL_PRINTHOOD | CSIDL_FLAG_CREATE, &g_psfPrintHood);

    return g_psfPrintHood;
}

//
// CPrintRoot_GetPIDLType
//

PIDLTYPE CPrintRoot_GetPIDLType(LPCITEMIDLIST pidl)
{
#ifdef WINNT
    UNALIGNED struct _NTIDPRINTER * pidlprint = (LPIDPRINTER) pidl;
    if (pidlprint->cb >= sizeof(DWORD) + FIELD_OFFSET(IDPRINTER, dwMagic) &&
        pidlprint->dwMagic == PRINTER_MAGIC)
    {
        return HOOD_COL_PRINTER;
    }
    else
    {
        // 
        // BUGBUG: If the PRINTER_MAGIC field check fails it might still 
        // be a valid Win9x PIDL. The only reliable way I can think of 
        // to determine whether this is the case is to check if pidlprint->cb
        // points inside a W95IDPRINTER structure and also to check whether
        // the name is tighten up to the PIDL size.
        //
        // This HACK is a little ugly but have to do it, in order to support 
        // the legacy Win9x printer shortcuts under Win2k.
        //
        UNALIGNED struct _W95IDPRINTER *pidlprint95 = (struct _W95IDPRINTER *)pidl;
        int nPIDLSize = sizeof(pidlprint95->cb) + lstrlenA( pidlprint95->cName ) + 1;

        if( nPIDLSize < sizeof(struct _W95IDPRINTER) &&     // Must be inside _W95IDPRINTER
            pidlprint95->cb == nPIDLSize )                  // The PIDL size must match the ANSI name
        {
            //
            // Well it might be a Win95 printer PIDL.
            //
            return  HOOD_COL_PRINTER;
        }
        else
        {
            //
            // This PIDL is not a valid printer PIDL.
            //
            return HOOD_COL_FILE;
        }
    }
#else
    // Everything is file
    return HOOD_COL_PRINTER;
#endif
}

//---------------------------------------------------------------------------
//
// Helper functions
//

/*++

Routine Description:

    Registers a modeless, non-top level window with the shell.  When
    the user requests a window, we search for other instances of that
    window.  If we find one, we switch to it rather than creating
    a new window.

    This function is used by PRINTUI.DLL so don't change its prototype.
    Fortunately, PRINTUI treats *phClassPidl as a cookie, so we can
    stuff our UNIQUESTUBINFO there and nobody will know the difference.

Arguments:

    pszPrinter - Name of the printer resource.  Generally a fully
        qualified printer name (\\server\printer for remote print
        folders) or a server name for the folder itself.

    dwType - Type of property window.  May refer to properties, document
        defaults, or job details.  Should use the PRINTER_PIDL_TYPE_*
        flags.

    phClassPidl - Receives the newly created handle to the registered
        object.  NULL if window already exists.

    phwnd - Receives the newly created hwndStub.  The property sheet
        should use this as the parent, since subsequent calls to
        this function will set focus to the last active popup of
        hwndStub.  phwnd will be set to NULL if the window already
        exists.

Return Value:

    TRUE - Success, either the printer was registered, or a window
           already exists.

    FALSE - Call failed.

--*/

BOOL Printers_RegisterWindow(LPCTSTR pszPrinter, DWORD dwType, PHANDLE phClassPidl, HWND *phwnd)
{
    IDPRINTER idp;
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlParent;
    BOOL bReturn = TRUE;

    *phClassPidl = NULL;
    *phwnd = NULL;

    pidlParent = SHCloneSpecialIDList(NULL, CSIDL_PRINTERS, FALSE);

    Printers_FillPidl(&idp, pszPrinter);
#ifdef WINNT
    idp.dwType = dwType;
#endif

    pidl = ILCombine(pidlParent, (LPITEMIDLIST)&idp);

    if (!pidl)
    {
        bReturn = FALSE;
    }
    else
    {
        UNIQUESTUBINFO *pusi = LocalAlloc(LPTR, sizeof(UNIQUESTUBINFO));

        //
        // Create a new stub window if necessary.
        //
        BOOL fUnique = EnsureUniqueStub(pidl, STUBCLASS_PROPSHEET, NULL, pusi);

        if (!fUnique)
        {
            LocalFree(pusi);
            pusi = NULL;
        }
        else
        {
            *phwnd = pusi->hwndStub;
        }

        *phClassPidl = pusi;    // Fortunately, it's just a cookie
        ILFree(pidl);
    }

    //
    // Ensure we release the pidlParent it's not needed
    // in all cases.  ILCombine will make a copy of it.
    //
    if( pidlParent )
    {
        ILFree(pidlParent);
    }

    return bReturn;
}

VOID Printers_UnregisterWindow(HANDLE hClassPidl, HWND hwnd)

/*++

Routine Description:

    Unregister a window handle.

Arguments:

    hClassPidl - Registration handle returned from Printers_RegisterWindow.
        It's really a pointer to a UNIQUESTUBINFO structure.

Return Value:

--*/

{
    UNIQUESTUBINFO *pusi = hClassPidl;
    ASSERT(hClassPidl);

    if (pusi)
    {
        ASSERT(pusi->hwndStub == hwnd);
        FreeUniqueStub(pusi);
        LocalFree(pusi);
    }
}

void Printers_FillPidl(LPIDPRINTER pidl, LPCTSTR szName)
{
    ualstrcpyn(pidl->cName, szName, ARRAYSIZE(pidl->cName));

    pidl->cb = (USHORT)(FIELD_OFFSET(IDPRINTER, cName) + (ualstrlen(pidl->cName) + 1) * SIZEOF(TCHAR));
    *(UNALIGNED USHORT *)((LPBYTE)(pidl) + pidl->cb) = 0;
#ifdef WINNT
    pidl->uFlags = 0;
    pidl->dwType = 0;
    pidl->dwMagic = PRINTER_MAGIC;
#endif
}

//
// pidlParent can be NULL to indicate this is a local printer.
//
LPITEMIDLIST Printers_GetPidl(LPCITEMIDLIST pidlParent, LPCTSTR pszName)
{
    LPITEMIDLIST pidl = NULL;
    LPITEMIDLIST pidlAlloc = NULL;

    if (!pidlParent)
        pidlParent = pidlAlloc = SHCloneSpecialIDList(NULL, CSIDL_PRINTERS, FALSE);

    if (pidlParent)
    {
        if (!pszName)
        {
            pidl = ILClone(pidlParent);
        }
        else
        {
            IDPRINTER idp;

            Printers_FillPidl(&idp, pszName);
            pidl = ILCombine(pidlParent, (LPITEMIDLIST)&idp);
        }
    }

    if (pidlAlloc)
        ILFree(pidlAlloc);

    return pidl;
}

//---------------------------------------------------------------------------
//
// IEnumShellFolder stuff
//
// Note: there is some extra stuff in WCommonUnknown we don't need, but this
// allows us to use the Common Unknown routines
//
typedef struct
{
    IEnumIDList eidl;
    LONG cRef;
    DWORD grfFlags;
    int nLastFound;
    CPrinterFolder *ppsf;
#ifdef PRN_FOLDERDATA
    PFOLDER_PRINTER_DATA pPrinters;
#else
    PRINTER_INFO_TYPE *pPrinters;   // PRINTER_INFO_TYPE is level 4 for WINNT, 1 for WIN9x.
#endif
    DWORD dwNumPrinters;
    DWORD dwRemote;
    IEnumIDList *peunk;     // file system enumerator
} CPrintersEnum;
//

// Flags for the dwRemote field
//

#define RMF_SHOWLINKS   0x00000001  // Hoodlinks need to be shown

STDMETHODIMP CPrinters_ESF_QueryInterface(IEnumIDList *pesf, REFIID riid, void **ppvObj)
{
    CPrintersEnum *this = IToClass(CPrintersEnum, eidl, pesf);

    if (IsEqualIID(riid, &IID_IEnumIDList)  ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = &this->eidl;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    this->cRef++;
    return NOERROR;
}

STDMETHODIMP_(ULONG) CPrinters_ESF_AddRef(IEnumIDList *pesf)
{
    CPrintersEnum *this = IToClass(CPrintersEnum, eidl, pesf);
    return InterlockedIncrement(&this->cRef);
}

STDMETHODIMP_(ULONG) CPrinters_ESF_Release(IEnumIDList *pesf)
{
    CPrintersEnum *this = IToClass(CPrintersEnum, eidl, pesf);
    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    if (this->pPrinters)
        LocalFree((HLOCAL)this->pPrinters);

    // Release the link (filesystem) enumerator.
    if (this->peunk)
        this->peunk->lpVtbl->Release(this->peunk);

    LocalFree((HLOCAL)this);
    return 0;
}

TCHAR const c_szNewObject[] =  TEXT("WinUtils_NewObject");
TCHAR const c_szFileColon[] =  TEXT("FILE:");
TCHAR const c_szPrinters[]  =  TEXT("Printers");
TCHAR const c_szNewLine[]   =  TEXT("\r\n");

BOOL IsHiddenPrinter(LPCTSTR pszPrinter)
{
#ifdef WINNT
    return FALSE;
#else
    return lstrcmp(pszPrinter, TEXT("Rendering Subsystem")) == 0;
#endif
}

BOOL IsAvoidAutoDefaultPrinter(LPCTSTR pszPrinter)
{
#ifdef WINNT
    return lstrcmp(pszPrinter, TEXT("Fax")) == 0;
#else
    return lstrcmp(pszPrinter, TEXT("Rendering Subsystem")) == 0;
#endif
}


STDMETHODIMP CPrinters_ESF_Next(IEnumIDList *pesf, ULONG celt, LPITEMIDLIST *ppidl, ULONG *pceltFetched)
{
    CPrintersEnum *this = IToClass(CPrintersEnum, eidl, pesf);
    IDPRINTER idp;

    if (pceltFetched)
        *pceltFetched = 0;

    // We don't do any form of folder

    if (!(this->grfFlags & SHCONTF_NONFOLDERS))
    {
        return S_FALSE;
    }

    // Are we looking for the links right now?

    if (this->dwRemote & RMF_SHOWLINKS)
    {
        // Yes, use the link (PrintHood folder) enumerator

        if (this->peunk)
        {
            HRESULT hres = this->peunk->lpVtbl->Next(this->peunk, 1, ppidl, pceltFetched);
            if (hres == S_OK)
                return S_OK;       // Added link
        }
        this->dwRemote &= ~RMF_SHOWLINKS; // Done enumerating links
    }

    // Carry on with enumerating printers now

    ASSERT(this->nLastFound >= 0 || this->nLastFound == -1);

    if (this->nLastFound == -1)
    {
        //
        // We will return the Add Printer Wizard icon as the
        // first item (when nLastFound == -1).
        //
        // We need to enumerate printers before returning the
        // Add Printer Wizard since the refresh tells us if
        // we are an administrator on the folder.
        //
        BOOL bShowAddPrinter;

#ifdef WINNT
#ifdef PRN_FOLDERDATA

        //
        // Request a refresh.  This completes synchronously.
        //
        if (this->ppsf->hFolder && !this->ppsf->bRefreshed)
        {
            bFolderRefresh(this->ppsf->hFolder, &this->ppsf->bShowAddPrinter);
            this->ppsf->bRefreshed = TRUE;
        }
        
        bShowAddPrinter = this->ppsf->bShowAddPrinter;

        if (this->ppsf->pszServer == NULL)
            bShowAddPrinter = TRUE; // always can add for local printers

        this->dwNumPrinters = Printers_FolderEnumPrinters(this->ppsf->hFolder, &this->pPrinters);
#else
        BLOCK
        {
            DWORD dwFlags;

            //
            // Always show the Add Printer Wizard.
            //
            bShowAddPrinter = TRUE;

            if (this->ppsf->pszServer)
            {
                //
                // Remote case: enum the printers on the server.
                //
                dwFlags = PRINTER_ENUM_NAME;
            }
            else
            {
                //
                // Enum both local and connections if the print folder
                // is local.
                //
                dwFlags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_FAVORITE;
            }
            this->dwNumPrinters = Printers_EnumPrinters(this->ppsf->pszServer,
                                                        dwFlags,
                                                        PRINTER_INFO_LEVEL,
                                                        &this->pPrinters);
        }
#endif  // PRN_FOLDERDATA
#else   // WINNT

        //
        // Always show the Add Printer Wizard.
        //
        bShowAddPrinter = TRUE;

        this->dwNumPrinters = Printers_EnumPrinters(NULL, PRINTER_ENUM_LOCAL,
                                                    PRINTER_INFO_LEVEL,
                                                    &this->pPrinters);
#endif  // WINNT
        //
        // Note that pPrinters may be NULL if no printers are installed.
        //

        if (bShowAddPrinter && !SHRestricted(REST_NOPRINTERADD))
        {
            //
            // Special case the Add Printer Wizard.
            //
            Printers_FillPidl(&idp, c_szNewObject);
            goto Done;
        }

        //
        // Not an admin, skip the add printer wizard and return the
        // first item.
        //
        this->nLastFound = 0;
    }

ESF_TryAgain:

    if (this->nLastFound >= (int)this->dwNumPrinters)
        return S_FALSE;

    if (IsHiddenPrinter(GetPrinterName(this->pPrinters, this->nLastFound)))
    {
        this->nLastFound++;
        goto ESF_TryAgain;
    }
    else
    {
        Printers_FillPidl(&idp, GetPrinterName(this->pPrinters, this->nLastFound));
    }

Done:
    ++this->nLastFound;

    *ppidl = ILClone((LPCITEMIDLIST)&idp);
    if (*ppidl == NULL)
        return E_OUTOFMEMORY;

    if (pceltFetched)
        *pceltFetched = 1;
    return S_OK;
}

STDMETHODIMP CPrinters_ESF_Skip(IEnumIDList *pesf, ULONG celt)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPrinters_ESF_Reset(IEnumIDList *pesf)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPrinters_ESF_Clone(IEnumIDList *pesf, IEnumIDList **lplpenum)
{
    return E_NOTIMPL;
}


IEnumIDListVtbl c_PrintersESFVtbl =
{
    CPrinters_ESF_QueryInterface, CPrinters_ESF_AddRef, CPrinters_ESF_Release,
    CPrinters_ESF_Next,
    CPrinters_ESF_Skip,
    CPrinters_ESF_Reset,
    CPrinters_ESF_Clone,
};

//---------------------------------------------------------------------------
//
// this implements IContextMenu via defcm.c for a printer object
//

BOOL Printer_WorkOnLine(LPCTSTR pszPrinter, BOOL fWorkOnLine)
{
    LPPRINTER_INFO_5 ppi5;
    BOOL bRet = FALSE;
    HANDLE hPrinter = Printer_OpenPrinterAdmin(pszPrinter);
    if (hPrinter)
    {
        ppi5 = Printer_GetPrinterInfo(hPrinter, 5);
        if (ppi5)
        {
            if (fWorkOnLine)
                ppi5->Attributes &= ~PRINTER_ATTRIBUTE_WORK_OFFLINE;
            else
                ppi5->Attributes |= PRINTER_ATTRIBUTE_WORK_OFFLINE;

            bRet = SetPrinter(hPrinter, 5, (LPBYTE)ppi5, 0);
            PrintDef_RefreshQueue(pszPrinter);

            LocalFree((HLOCAL)ppi5);
        }
        Printer_ClosePrinter(hPrinter);
    }

    return bRet;
}

TCHAR const c_szConfig[] =  TEXT("Config");

DWORD CPrinters_SpoolerVersion(CPrinterFolder *this)
{
    if (this->dwSpoolerVersion == -1)
    {
        HANDLE hServer = Printer_OpenPrinter(this->pszServer);
        
        this->dwSpoolerVersion = 0;
        if (hServer)
        {
            DWORD dwNeeded = 0, dwType = REG_DWORD;
            GetPrinterData(hServer, TEXT("MajorVersion"), &dwType, (PBYTE)&this->dwSpoolerVersion,
                                sizeof(this->dwSpoolerVersion), &dwNeeded);
            Printer_ClosePrinter(hServer);
        }
    }
    
    return this->dwSpoolerVersion;
}


BOOL IsWinIniDefaultPrinter(LPCTSTR pszPrinter)
{
    BOOL bRet = FALSE;
    TCHAR szPrinterDefault[280];

    if (GetProfileString(TEXT("Windows"), TEXT("Device"), TEXT(""), szPrinterDefault, ARRAYSIZE(szPrinterDefault)))
    {
        LPTSTR psz = StrChr( szPrinterDefault, TEXT( ',' ));
        if (psz)
        {
            *psz = 0;
            bRet = lstrcmpi(szPrinterDefault, pszPrinter) == 0;
        }
    }
    return bRet;
}

BOOL IsDefaultPrinter(LPCTSTR pszPrinter, DWORD dwAttributesHint)
{
    return (dwAttributesHint & PRINTER_ATTRIBUTE_DEFAULT) ||
            IsWinIniDefaultPrinter(pszPrinter);
}

// more win.ini uglyness
BOOL IsPrinterInstalled(LPCTSTR pszPrinter)
{
    TCHAR szScratch[2];
    return GetProfileString(TEXT("Devices"), pszPrinter, TEXT(""), szScratch, ARRAYSIZE(szScratch));
}


VOID Printer_MergeMenu(CPrinterFolder *psf, LPQCMINFO pqcm, LPCTSTR pszPrinter, BOOL fForcePause)
{
    INT idCmdFirst = pqcm->idCmdFirst;

    //
    // pszPrinter may be the share name of a printer rather than
    // the "real" printer name.  Use the real printer name instead,
    // which is returned from GetPrinter().
    //
    // These three only valid if pData != NULL.
    //
    LPCTSTR pszRealPrinterName;
    DWORD dwAttributes;
    DWORD dwStatus;
    PPRINTER_INFO_2 pData = NULL;

#ifdef PRN_FOLDERDATA
    TCHAR szFullPrinter[MAXNAMELENBUFFER];
#else
    BOOL bUsedCommonPPI2 = FALSE;

    //
    // Valid only if pData != NULL.
    //
    LPCTSTR pszPortName;
#endif

    // Insert verbs
    CDefFolderMenu_MergeMenu(HINST_THISDLL, MENU_PRINTOBJ_VERBS, 0, pqcm);

    //
    // Now this function only takes a printer name.  We will
    // query pData from psf if it is available.
    //
    if (pszPrinter)
    {
#ifdef PRN_FOLDERDATA
        if (psf && psf->hFolder)
        {
            pData = Printer_FolderGetPrinter(psf->hFolder, pszPrinter);
            if (pData)
            {
                Printer_BuildPrinterName(psf, szFullPrinter, NULL, ((PFOLDER_PRINTER_DATA)pData)->pName);

                pszRealPrinterName = szFullPrinter;

                dwStatus = ((PFOLDER_PRINTER_DATA)pData)->Status;
                dwAttributes = ((PFOLDER_PRINTER_DATA)pData)->Attributes;
            }
        }
        else
        {
            pData = Printer_GetPrinterInfoStr(pszPrinter, 2);
            if (pData)
            {
                pszRealPrinterName = pData->pPrinterName;
                dwStatus = pData->Status;
                dwAttributes = pData->Attributes;
            }
        }
#else
        if (psf)
        {
            pData = CPrinters_SF_GetPrinterInfo2(psf, pszPrinter);
            if (pData)
            {
                bUsedCommonPPI2 = TRUE;
            }
        }
        else
        {
            pData = Printer_GetPrinterInfoStr(pszPrinter, 2);
        }

        if (pData)
        {
            pszRealPrinterName = pData->pPrinterName;
            pszPortName = pData->pPortName;
            dwStatus = pData->Status;
            dwAttributes = pData->Attributes;
        }
#endif
    }

    //
    // Remove document defaults if it's a remote print folder.
    // This command should be removed from the context menu independently 
    // on whether we have mutiple selection or not - i.e. pData.
    //
    if (!psf || psf->pszServer)
    {
        DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_DOCUMENTDEFAULTS, MF_BYCOMMAND);
    }

    // disable/check/rename verbs
    if (pData)
    {
        if (IsDefaultPrinter(pszRealPrinterName, dwAttributes))
        {
            // we need to check "Set As Default"
            CheckMenuItem(pqcm->hmenu, idCmdFirst + FSIDM_SETDEFAULTPRN, MF_BYCOMMAND | MF_CHECKED);
        }

#ifndef WINNT // PRINTQ
        // network printers don't get pause/purge
        if (dwAttributes & PRINTER_ATTRIBUTE_NETWORK && !fForcePause)
        {
            DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_PAUSEPRN, MF_BYCOMMAND);
            DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_PURGEPRN, MF_BYCOMMAND);
        }

        // FILE: printers don't get pause/purge
        if (!lstrcmp(pszPortName, c_szFileColon))
        {
            EnableMenuItem(pqcm->hmenu, idCmdFirst + FSIDM_PURGEPRN, MF_BYCOMMAND|MF_GRAYED);
            EnableMenuItem(pqcm->hmenu, idCmdFirst + FSIDM_PAUSEPRN, MF_BYCOMMAND|MF_GRAYED);
        }
        // DIRECT printers don't get pause
        else if (dwAttributes & PRINTER_ATTRIBUTE_DIRECT)
        {
            EnableMenuItem(pqcm->hmenu, idCmdFirst + FSIDM_PAUSEPRN, MF_BYCOMMAND|MF_GRAYED);
        }
        // On Winnt the spooler allows the printer to be
        // paused/resumed when printing directly to the
        // printer, i.e. port.
        else // PAUSE/RESUME depends on state of printer
#endif
        {
            if (dwStatus & PRINTER_STATUS_PAUSED)
            {
                MENUITEMINFO mii;

                // we need to check "Paused" (so, if clicked again, we RESUME)
                mii.cbSize = SIZEOF(MENUITEMINFO);
                mii.fMask = MIIM_STATE | MIIM_ID;
                mii.fState = MF_CHECKED;
                mii.wID = idCmdFirst + FSIDM_RESUMEPRN;
                SetMenuItemInfo(pqcm->hmenu, idCmdFirst + FSIDM_PAUSEPRN, MF_BYCOMMAND, &mii);
            }
        }

#ifdef WINNT

        //
        // Remove default printer if it's a remote print folder.
        //
        if (!psf || psf->pszServer)
        {
            DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_SETDEFAULTPRN, MF_BYCOMMAND);
        }

        //
        // Check whether the printer is already installed. If it
        // is, remove the option to install it.
        //

        if (IsPrinterInstalled(pszRealPrinterName))
        {
            DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_NETPRN_INSTALL, MF_BYCOMMAND);
        }

        //
        // Remove work on/off-line if any of the following is met
        //  - network printer
        //  - masq printer
        //  - down level print server 
        //
        if (( dwAttributes & PRINTER_ATTRIBUTE_NETWORK ) || 
            ( CPrinters_SpoolerVersion(psf) <= 2 ) )
            goto turn_off_less_stuff;

#else // def WINNT

        // Remove work on/off-line iff all of the following
        //  - 1 user configuration exists
        //  - local printer
        //  - printer is on-line (should always be true,
        //    but we should make sure. if it ain't the
        //    user will be stuck w/ an off-line printer.)
        if (!(dwAttributes & PRINTER_ATTRIBUTE_NETWORK) &&
            !(dwAttributes & PRINTER_ATTRIBUTE_WORK_OFFLINE))
        {
            HKEY hkey;

            if (ERROR_SUCCESS ==
                RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szConfig,
                    0, KEY_QUERY_VALUE, &hkey))
            {
                DWORD dwNumSubKeys = 2; // something > 1

                RegQueryInfoKey(hkey, NULL, NULL, NULL,
                    &dwNumSubKeys, NULL, NULL, NULL,
                    NULL, NULL, NULL, NULL);

                RegCloseKey(hkey);

                if (dwNumSubKeys == 1)
                    goto turn_off_less_stuff;
            }
        }
#endif

        if (dwAttributes & PRINTER_ATTRIBUTE_WORK_OFFLINE) 
        {
            MENUITEMINFO mii;

            // we need to check "Offline" (so, if clicked again, we ONLINE)
            mii.cbSize = SIZEOF(MENUITEMINFO);
            mii.fMask = MIIM_STATE | MIIM_ID;
            mii.fState = MF_CHECKED;
            mii.wID = idCmdFirst + FSIDM_WORKONLINE;
            SetMenuItemInfo(pqcm->hmenu, idCmdFirst + FSIDM_WORKOFFLINE, MF_BYCOMMAND, &mii);
        }

#ifndef PRN_FOLDERDATA
        if (bUsedCommonPPI2)
        {
            CPrinters_SF_FreePrinterInfo2(psf);
        }
        else
#endif
        {
            LocalFree((HLOCAL)pData);
        }
    }
    else
    {
        // we have multiple printers selected
        DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_SETDEFAULTPRN, MF_BYCOMMAND);
        EnableMenuItem(pqcm->hmenu, idCmdFirst + FSIDM_PAUSEPRN, MF_BYCOMMAND|MF_GRAYED);

#ifdef WINNT
        DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_NETPRN_INSTALL, MF_BYCOMMAND);
        DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_SETDEFAULTPRN, MF_BYCOMMAND);
#endif

turn_off_less_stuff:
        DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_WORKOFFLINE, MF_BYCOMMAND);
    }
}


VOID Printer_WarnOnError(HWND hwnd, LPCTSTR pName, int idsError)
{
#ifndef WINNT
    BOOL fWarn;
    LPPRINTER_INFO_5 ppi5 = Printer_GetPrinterInfoStr(pName, 5);
    fWarn = !ppi5 || !(ppi5->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE);
    if (ppi5)
        LocalFree((HLOCAL)ppi5);
    if (fWarn)
#endif
    {
        ShellMessageBox(HINST_THISDLL, hwnd,
            MAKEINTRESOURCE(idsError),
            MAKEINTRESOURCE(IDS_PRINTERS),
            MB_OK|MB_ICONINFORMATION);
    }
}

#ifdef WINNT

//
// Three HACK functions to parse the printer name string.
//     Printer_SplitFullName
//     Printer_BuildPrinterName
//     Printer_CheckNetworkPrinterByName
//
// All string parsing functions should be localized here.
//

VOID Printer_SplitFullName(LPTSTR pszScratch, LPCTSTR pszFullName,
    LPCTSTR *ppszServer, LPCTSTR *ppszPrinter)

/*++

Routine Description:

    Splits a fully qualified printer connection name into server and
    printer name parts.

Arguments:

    pszScratch - Scratch buffer used to store output strings.  Must
        be MAXNAMELENBUFFER in size.

    pszFullName - Input name of a printer.  If it is a printer
        connection (\\server\printer), then we will split it.  If
        it is a true local printer (not a masq) then the server is
        szNULL.

    ppszServer - Receives pointer to the server string.  If it is a
        local printer, szNULL is returned.

    ppszPrinter - Receives a pointer to the printer string.  OPTIONAL

Return Value:

--*/

{     
    LPTSTR pszPrinter;

    lstrcpyn(pszScratch, pszFullName, MAXNAMELENBUFFER);

    if (pszFullName[0] != TEXT('\\') || pszFullName[1] != TEXT('\\'))
    {
        //
        // Set *ppszServer to szNULL since it's the local machine.
        //
        *ppszServer = szNULL;
        pszPrinter = pszScratch;
    }
    else
    {
        *ppszServer = pszScratch;
        pszPrinter = StrChr(*ppszServer + 2, TEXT('\\'));

        if (!pszPrinter)
        {
            //
            // We've encountered a printer called "\\server"
            // (only two backslashes in the string).  We'll treat
            // it as a local printer.  We should never hit this,
            // but the spooler doesn't enforce this.  We won't
            // format the string.  Server is local, so set to szNULL.
            //
            pszPrinter = pszScratch;
            *ppszServer = szNULL;

            ASSERT(FALSE);
        }
        else
        {
            //
            // We found the third backslash; null terminate our
            // copy and set bRemote TRUE to format the string.
            //
            *pszPrinter++ = 0;
        }
    }

    if (ppszPrinter)
    {
        *ppszPrinter = pszPrinter;
    }
}

#endif // WINNT


/*++

Routine Description:

    Parses an unaligned partial printer name and printer shell folder
    into a fullly qualified printer name, and pointer to aligned printer
    name.

Arguments:

    pszFullPrinter - Buffer to receive fully qualified printer name
        Must be MAXNAMELENBUFFER is size.

    pidp - Optional pass in the pidl to allow us to try to handle cases where maybe an
        old style printer pidl was passed in.

    pszPrinter - Unaligned partial (local) printer name.

    psf - PrinterShellFolder that owns the printer.

Return Value:

    LPCTSTR pointer to aligned partal (local) printer name.

--*/

LPCTSTR Printer_BuildPrinterName(CPrinterFolder *psf, LPTSTR pszFullPrinter, LPIDPRINTER pidp,
    UNALIGNED const TCHAR* pszPrinter)
{
    UINT cchLen = 0;

#ifdef WINNT
    if (psf->pszServer)
    {
        ASSERT(!pszPrinter || (ualstrlen(pszPrinter) < MAXNAMELEN));

        cchLen = wsprintf(pszFullPrinter, TEXT("%s\\"), psf->pszServer);
    }

    if (pidp)
    {
        UNALIGNED struct _NTIDPRINTER * pidlprint = (LPIDPRINTER) pidp;
        if (pidlprint->cb >= sizeof(DWORD) + FIELD_OFFSET(IDPRINTER, dwMagic) &&
            (pidlprint->dwMagic == PRINTER_MAGIC))
        {
            ualstrcpyn(&pszFullPrinter[cchLen], pidlprint->cName, MAXNAMELEN);
        }

        else
        {
            // Win95 form...
            MultiByteToWideChar(CP_ACP, 0, ((LPW95IDPRINTER)pidp)->cName, -1, &pszFullPrinter[cchLen], MAXNAMELEN);
        }
    }
#else
    // W95 side of things...
    if (pidp)
    {
        if (pidp->cb >= sizeof(DWORD) + FIELD_OFFSET(NTIDPRINTER, dwMagic) &&
            (((LPNTIDPRINTER)pidp)->dwMagic == PRINTER_MAGIC))
        {
            WideCharToMultiByte(CP_ACP, 0, ((LPNTIDPRINTER)pidp)->cName, -1,
                    pszFullPrinter, MAXNAMELEN, NULL, NULL);
        }

        else
        {
            ualstrcpyn(pszFullPrinter, pidp->cName, MAXNAMELEN);
        }
    }
#endif
    else
        ualstrcpyn(&pszFullPrinter[cchLen], pszPrinter, MAXNAMELEN);

    ASSERT(lstrlen(pszFullPrinter) < MAXNAMELENBUFFER);

    return pszFullPrinter + cchLen;
}


#ifdef WINNT
BOOL Printer_CheckNetworkPrinterByName(LPCTSTR pszPrinter, LPCTSTR* ppszLocal)

/*++

Routine Description:

    Check whether the printer is a local printer by looking at
    the name for the "\\localmachine\" prefix or no server prefix.

    This is a HACK: we should check by printer attributes, but when
    it's too costly or impossible (e.g., if the printer connection
    no longer exists), then we use this routine.

    Note: this only works for WINNT since the WINNT spooler forces
    printer connections to be prefixed with "\\server\."  Win9x
    allows the user to rename the printer connection to any arbitrary
    name.

    We determine if it's a masq  printer by looking for the
    weird format "\\localserver\\\remoteserver\printer."

Arguments:

    pszPrinter - Printer name.

    ppszLocal - Returns local name only if the printer is a local printer.
        (May be network and local if it's a masq printer.)

Return Value:

    TRUE: it's a network printer (true or masq).

    FALSE: it's a local printer.

--*/

{
    BOOL bNetwork = FALSE;
    LPCTSTR pszLocal = NULL;

    if (pszPrinter[0] == TEXT( '\\' ) && pszPrinter[1] == TEXT( '\\' ))
    {
        TCHAR szComputer[MAX_COMPUTERNAME_LENGTH+1];
        DWORD cchComputer = ARRAYSIZE(szComputer);

        bNetwork = TRUE;
        pszLocal = NULL;

        //
        // Check if it's a masq printer.  If it has the format
        // \\localserver\\\server\printer then it's a masq case.
        //
        if (GetComputerName(szComputer, &cchComputer))
        {
            if (IntlStrEqNI(&pszPrinter[2], szComputer, cchComputer) &&
                pszPrinter[cchComputer] == TEXT( '\\' ))
            {
                if( pszPrinter[cchComputer+1] == TEXT('\\') &&
                    pszPrinter[cchComputer+2] == TEXT('\\'))
                {
                    //
                    // It's a masq printer.
                    //
                    pszLocal = &pszPrinter[cchComputer+1];
                }
            }
        }
    } else {

        //
        // It's a local printer.
        //
        pszLocal = pszPrinter;
    }

    if (ppszLocal)
    {
        *ppszLocal = pszLocal;
    }
    return bNetwork;
}

#endif

#ifdef WINNT

/*++

Routine Name:

    Printer_PurgePrinter    

Routine Description:

    Purges the specified printer, and prompting the user if
    they are really sure they want to purge the deviece.   It is
    kind of an extreme action to cancel all the documents on 
    the printer.

Arguments:

    psf - pointer to shell folder
    hwnd - handle to view window
    pszFullPrinter - Fully qualified printer name.
    uAction - action to execute.

Return Value:

    TRUE: printer was purged successfully or the user choose to cancel
    the action, FALSE: an error occurred attempting to purge the device.

--*/
BOOL Printer_PurgePrinter(CPrinterFolder *psf, HWND hwnd, LPCTSTR pszFullPrinter, UINT uAction)
{
    BOOL                    bRetval     = FALSE;
    LPTSTR                  pszRet      = NULL;
    LPCTSTR                 pszPrinter  = NULL;
    LPCTSTR                 pszServer   = NULL;
    TCHAR                   szTemp[MAXNAMELENBUFFER] = {0};

    ASSERT(psf);

    //
    // We need to break up the full printer name in its components.
    // in order to construct the display name string.
    //      
    Printer_SplitFullName (szTemp, pszFullPrinter, &pszServer, &pszPrinter);

    //
    // If there is a server name then construct a friendly printer name.
    //
    if( pszServer && *pszServer )
    {
        pszRet = ShellConstructMessageString (HINST_THISDLL, 
                                              MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON), 
                                              &pszServer[2], 
                                              pszPrinter);
        pszPrinter = pszRet;
    }

    //
    // If we are referring to a local printer or shell construct message
    // sting failed then just use the full printer name in the warning
    // message.
    //
    if (!pszRet)
    {
        pszPrinter = pszFullPrinter;
    }

    //
    // Ask the user if they are sure they want to cancel all documents.
    //
    if (ShellMessageBox (HINST_THISDLL, 
                         hwnd, 
                         MAKEINTRESOURCE(IDS_SUREPURGE),
                         MAKEINTRESOURCE(IDS_PRINTERS), 
                         MB_YESNO|MB_ICONQUESTION,
                         pszPrinter) == IDYES)
    {
        bRetval = Printer_ModifyPrinter (pszFullPrinter, uAction);
    }
    else
    {
        bRetval = TRUE;
    }

    if(pszRet)
        LocalFree (pszRet);

    return bRetval;
}

#endif

HRESULT Printer_InvokeCommand(HWND hwndView, CPrinterFolder *psf,
                              LPIDPRINTER pidp, WPARAM wParam, LPARAM lParam,
                              LPBOOL pfChooseNewDefault)
{
    HRESULT hres = NOERROR;
    BOOL bNewObject = !ualstrcmp(c_szNewObject, pidp->cName);
    LPCTSTR pszPrinter;
    LPCTSTR pszFullPrinter;

    //
    // If it's a remote machine, prepend server name.
    //
    TCHAR szFullPrinter[MAXNAMELENBUFFER];

    if (bNewObject)
    {
        pszFullPrinter = pszPrinter = c_szNewObject;
    }
    else
    {
        pszPrinter = Printer_BuildPrinterName(psf, szFullPrinter, pidp, NULL);
        pszFullPrinter = szFullPrinter;
    }

    switch(wParam)
    {
    case FSIDM_OPENPRN:
        SHInvokePrinterCommand(hwndView, PRINTACTION_OPEN, pszFullPrinter, psf->pszServer, FALSE);
        break;

#ifdef WINNT
    case FSIDM_DOCUMENTDEFAULTS:
        if (!bNewObject)
        {
            SHInvokePrinterCommand(hwndView, 
                PRINTACTION_DOCUMENTDEFAULTS, pszFullPrinter, NULL, 0 );
        }
        break;

    case FSIDM_SHARING:
#endif
    case DFM_CMD_PROPERTIES:

        if (!bNewObject)
        {
            SHInvokePrinterCommand(hwndView,
                                 PRINTACTION_PROPERTIES,
                                 pszFullPrinter,
#ifdef WINNT
                                 wParam == FSIDM_SHARING ?
                                     (LPCTSTR)PRINTER_SHARING_PAGE :
                                     (LPCTSTR)lParam,
#else
                                 (LPCTSTR)lParam,
#endif
                                 FALSE);
        }
        break;

    case DFM_CMD_DELETE:
        if (!bNewObject &&
            IDYES == CallPrinterCopyHooks(hwndView, PO_DELETE,
                0, pszFullPrinter, 0, NULL, 0))
        {
            BOOL bNukedDefault = FALSE;
            BOOL fSuccess;
            DWORD dwAttributes = PRINTER_ATTRIBUTE_NETWORK;

#ifdef PRN_FOLDERDATA
            LPCTSTR pszPrinterCheck = pszFullPrinter;
            PFOLDER_PRINTER_DATA pData = Printer_FolderGetPrinter(psf->hFolder, pszFullPrinter);
            if (pData)
            {
                dwAttributes = pData->Attributes;
                pszPrinterCheck = pData->pName;
            }

            if (psf->pszServer == NULL)
            {
                // this is a local print folder then
                // we need to check if we're deleting the default printer.
                bNukedDefault = IsDefaultPrinter(pszPrinterCheck, dwAttributes);
            }

            if (pData)
                LocalFree((HLOCAL)pData);
#else
            LPPRINTER_INFO_5 pData = Printer_GetPrinterInfoStr(pszFullPrinter, 5);
            if (pData)
            {
                bNukedDefault = pData->Attributes & PRINTER_ATTRIBUTE_DEFAULT;
                LocalFree((HLOCAL)pData);
            }
#endif
            fSuccess = Printers_DeletePrinter(hwndView, pszPrinter,
                                              dwAttributes, psf->pszServer);
#ifndef PRN_FOLDERDATA
            if (fSuccess && psf)
            {
                CPrinters_SF_RemovePrinterInfo2(psf, pidp->cName);
            }
#endif
            // if so, make another one the default
            if (bNukedDefault && fSuccess && pfChooseNewDefault)
            {
                // don't choose in the middle of deletion,
                // or we might delete the "default" again.
                *pfChooseNewDefault = TRUE;
            }
        }
        break;

    case FSIDM_SETDEFAULTPRN:
        // cannot be selected for c_szNewObject
        Printer_SetAsDefault(pszFullPrinter);
        break;

    case FSIDM_PAUSEPRN:
        // cannot be selected for c_szNewObject
        // Since this command isn't available for net printers,
        // don't worry about IDS_SECURITYDENIED message on err.
        if (!Printer_ModifyPrinter(pszFullPrinter, PRINTER_CONTROL_PAUSE))
            goto WarnOnError;
        break;

    case FSIDM_RESUMEPRN:
        // cannot be selected for c_szNewObject
        // Since this command isn't available for net printers,
        // don't worry about IDS_SECURITYDENIED message on err.
        if (!Printer_ModifyPrinter(pszFullPrinter, PRINTER_CONTROL_RESUME))
            goto WarnOnError;
        break;

    case FSIDM_PURGEPRN:
        if (!bNewObject)
        {
#ifdef WINNT
            if (!Printer_PurgePrinter(psf, hwndView, pszFullPrinter, PRINTER_CONTROL_PURGE))
            {
WarnOnError:
                Printer_WarnOnError(hwndView, 
                                    pszFullPrinter, 
                                    GetLastError() == ERROR_NOT_SUPPORTED ? 
                                    IDS_PRN_OPNOTSUPPORTED : IDS_SECURITYDENIED);
            }
#else
            if (!Printer_ModifyPrinter(pszFullPrinter, PRINTER_CONTROL_PURGE))
            {
WarnOnError:
                Printer_WarnOnError(hwndView, pszFullPrinter, IDS_SECURITYDENIED);
            }
#endif
        }
        break;

#ifdef WINNT

    case FSIDM_NETPRN_INSTALL:
        SHInvokePrinterCommand(hwndView, PRINTACTION_NETINSTALL, pszFullPrinter, NULL, FALSE);
        break;
#endif

    case FSIDM_WORKONLINE:
        // cannot be selected for c_szNewObject
        if (!Printer_WorkOnLine(pszFullPrinter, TRUE))
#ifdef WINNT
            //
            // On NT this can fail if we are in the remote printers folder
            // and the client does not have access to modify the printer state.
            //
            Printer_WarnOnError(hwndView, pszFullPrinter, IDS_SECURITYDENIED);
#else
        {
            // The only reason this can really fail is due to a network port
            // not validating, but we should double check just to make sure
            LPPRINTER_INFO_5 ppi5 = Printer_GetPrinterInfoStr(pszFullPrinter, 5);
            if (ppi5)
            {
                if (ppi5->Attributes & PRINTER_ATTRIBUTE_NETWORK)
                {
                    ShellMessageBox(HINST_THISDLL, hwndView,
                        MAKEINTRESOURCE(IDS_NO_WORKONLINE),
                        MAKEINTRESOURCE(IDS_PRINTERS),
                        MB_OK|MB_ICONINFORMATION);
                }
                LocalFree(ppi5);
            }
        }
#endif
        break;

    case FSIDM_WORKOFFLINE:
        // cannot be selected for c_szNewObject
        if( !Printer_WorkOnLine(pszFullPrinter, FALSE) )
        {
#ifdef WINNT
            //
            // On NT this can fail if we are in the remote printers folder
            // and the client does not have access to modify the printer state.
            //
            Printer_WarnOnError(hwndView, pszFullPrinter, IDS_SECURITYDENIED);
#endif
        }
        break;

    case DFM_CMD_LINK:
        // GetAttributesOf returns _CANLINK,
        // let defcm handle this
        hres = S_FALSE;
        break;

    default:
        // GetAttributesOf doesn't set other SFGAO_ bits,
        // BUT accelerator keys will get unavailable menu items,
        // so we need to return failure here.
        hres = E_NOTIMPL;
        break;
    } // switch(wParam)

#ifndef WINNT

    if (hres == NOERROR && psf)
    {
        // since the PRINTER_INFO_2 state may have changed,
        // set the UPDATE_NOW bit in the cache.
        CPrinters_SF_UpdatePrinterInfo2(psf, pszFullPrinter, UPDATE_NOW);
    }
#endif

    return hres;
}

HRESULT CALLBACK CPrinters_DFMCallBack(IShellFolder *psf, HWND hwndView,
                IDataObject * pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);
    HRESULT hres = E_NOTIMPL;

    if (pdtobj)
    {
        STGMEDIUM medium;
        LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
        if (pida)
        {
            hres = NOERROR;

            switch(uMsg)
            {
            case DFM_MERGECONTEXTMENU:
                //
                //  Returning S_FALSE indicates no need to use default verbs
                //
                hres = S_FALSE;
                break;

            case DFM_MERGECONTEXTMENU_TOP:
            {
                LPQCMINFO pqcm = (LPQCMINFO)lParam;

                if (pida->cidl == 1 && !ualstrcmp(c_szNewObject,
                    ((LPIDPRINTER)IDA_GetIDListPtr(pida, 0))->cName))
                {
                    // The only selected object is the "New Printer" thing

                    // insert verbs
                    CDefFolderMenu_MergeMenu(HINST_THISDLL, MENU_GENERIC_OPEN_VERBS, 0, pqcm);
                }
                else
                {
                    LPCTSTR pszFullPrinter = NULL;
                    TCHAR szFullPrinter[MAXNAMELENBUFFER];
                    // We're dealing with printer objects

                    if (!(wParam & CMF_DEFAULTONLY))
                    {
                        LPIDPRINTER pidp;

                        if (pida->cidl == 1)
                        {
                            pidp = (LPIDPRINTER)IDA_GetIDListPtr(pida, 0);

                            if (pidp)
                            {
                                Printer_BuildPrinterName(this, szFullPrinter, pidp, NULL);
                                pszFullPrinter = szFullPrinter;
                            }
                        }
                    }

                    Printer_MergeMenu(this, pqcm, pszFullPrinter, FALSE);

                } // if (pida->cidl=...), else case

                SetMenuDefaultItem(pqcm->hmenu, 0, MF_BYPOSITION);

                break;
            } // case DFM_MERGECONTEXTMENU

            case DFM_GETHELPTEXT:
            case DFM_GETHELPTEXTW:
            {
                int idCmd = LOWORD(wParam);
                int cchMax = HIWORD(wParam);
                LPBYTE pBuf = (LPBYTE)lParam;

                // map checkmark items to the correct message
                switch (idCmd) {
                case FSIDM_RESUMEPRN:  idCmd = FSIDM_PAUSEPRN;    break;
                case FSIDM_WORKONLINE: idCmd = FSIDM_WORKOFFLINE; break;
                }

                if (uMsg == DFM_GETHELPTEXTW)
                    LoadStringW(HINST_THISDLL, idCmd + IDS_MH_FSIDM_FIRST,
                                (LPWSTR)pBuf, cchMax);
                else
                    LoadStringA(HINST_THISDLL, idCmd + IDS_MH_FSIDM_FIRST,
                                (LPSTR)pBuf, cchMax);

                break;
            }

            case DFM_INVOKECOMMAND:
            {
                BOOL fChooseNewDefault = FALSE;
                int  i;

                INSTRUMENT_INVOKECOMMAND(SHCNFI_PRINTERS_DFM_INVOKE, hwndView, wParam);

                for (i = pida->cidl - 1; i >= 0; i--)
                {
                    LPIDPRINTER pidp = (LPIDPRINTER)IDA_GetIDListPtr(pida, i);

                    hres = Printer_InvokeCommand(hwndView, this, pidp, wParam, lParam, &fChooseNewDefault);

                    if (hres != NOERROR)
                        goto Bail;
                }

                if (fChooseNewDefault)
                    Printers_ChooseNewDefault(hwndView);

                break;
            } // case DFM_INVOKECOMMAND

            default:
                hres = E_NOTIMPL;
                break;
            } // switch (uMsg)

Bail:
            HIDA_ReleaseStgMedium(pida, &medium);
        } // if (medium.hGlobal)
    } // if (ptdobj)
    else
    {
        // on operation on the background -- we don't do anything.
    }

    return hres;
}


//---------------------------------------------------------------------------
//
// IDropTarget stuff
//

// printer thread data -- passed to ..._DropThreadInit

typedef struct {
    CIDLDropTarget *pdt;
    IStream     *pstmDataObj;
    IDataObject *pDataObj;
    DWORD        grfKeyState;
    POINTL       pt;
    DWORD        dwEffect;
} PRNTD;

#ifdef WINNT
STDMETHODIMP CPrinterFolder_HIDATestForRmPrns( LPIDA pida, int * pcRPFs, int * pcNonRPFs )
{
    UINT i;
    LPITEMIDLIST pidlRemainder = NULL;
    if ( !pida || !pcRPFs || !pcNonRPFs )
    {
        return E_INVALIDARG;
    }
        
    // check to see if any of the ID's are remote printers....
    for (i = 0; i < pida->cidl; i++)
    {
        LPITEMIDLIST pidlTo = IDA_ILClone(pida, i);
        if (pidlTo)
        {
            // *pidlRemainder will be NULL for remote print folders,
            // and non-NULL for printers under remote print folders
            if (NET_IsRemoteRegItem(pidlTo, &CLSID_Printers, &pidlRemainder)) // && (pidlRemainder->mkid.cb == 0))
            {
                (*pcRPFs)++;
            }
            else
            {
                (*pcNonRPFs)++;
            }
            ILFree(pidlTo);
        }
    }

    return NOERROR;
}
#endif

STDMETHODIMP CPrinterFolder_DragEnter(IDropTarget *pdt, IDataObject * pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdt);

    // We allow printer shares to be dropped for installing
    // But we don't want to spend the time on DragEnter finding out if it's
    // a printer share, so allow drops of any net resource or HIDA
    // REVIEW: Actually, it wouldn't take long to check the first one, but
    // sequencing through everything does seem like a pain.

    // let the base-class process it now to save away the pdwEffect
    CIDLDropTarget_DragEnter(pdt, pDataObj, grfKeyState, pt, pdwEffect);

    // are we dropping on the background ? Do we have the IDLIST clipformat ?
    if (this->dwData & DTID_HIDA)
    {
        int cRPFs = 0;
        int cNonRPFs = 0;
        
#ifdef WINNT
        STGMEDIUM medium;
        FORMATETC fmte = {g_cfNetResource, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        
        LPIDA pida = DataObj_GetHIDA(pDataObj, &medium);
        if (pida)
        {
            CPrinterFolder_HIDATestForRmPrns( pida, &cRPFs, &cNonRPFs );
            HIDA_ReleaseStgMedium(pida, &medium);
        }
#endif

        // if we have no Remote printers or we have any non "remote printers"
        // and we have no other clipformat to test...
        if ((( cRPFs == 0 ) || ( cNonRPFs != 0 )) && !( this->dwData & DTID_NETRES ))
        {
            // the Drop code below only handles drops for HIDA format on NT
            // and only if all off them are Remote Printers
            *pdwEffect &= ~DROPEFFECT_LINK;
        }
    }   

    if ((this->dwData & DTID_NETRES) || (this->dwData & DTID_HIDA))
    {
        *pdwEffect &= DROPEFFECT_LINK;
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    /*
    if (this->dwData & DTID_NETRES)
        *pdwEffect &= DROPEFFECT_LINK;
    else
        *pdwEffect = DROPEFFECT_NONE;
    */

    this->dwEffectLastReturned = *pdwEffect;

    return S_OK;
}

void _FreePrintDropData(PRNTD *pthp)
{
    if (pthp->pstmDataObj)
        pthp->pstmDataObj->lpVtbl->Release(pthp->pstmDataObj);

    if (pthp->pDataObj)
        pthp->pDataObj->lpVtbl->Release(pthp->pDataObj);

    pthp->pdt->dropt.lpVtbl->Release(&pthp->pdt->dropt);
    LocalFree((HLOCAL)pthp);
}



DWORD CALLBACK CPrinterDrop_ThreadProc(void *pv)
{
    PRNTD *pthp = (PRNTD *)pv;
    STGMEDIUM medium;
#ifdef WINNT
    LPIDA pida;
    HRESULT hres = E_FAIL;
    FORMATETC fmte = {g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
#else
    FORMATETC fmte = {g_cfNetResource, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
#endif

    CoGetInterfaceAndReleaseStream(pthp->pstmDataObj, &IID_IDataObject, (void **)&pthp->pDataObj);
    pthp->pstmDataObj = NULL;

    if (pthp->pDataObj == NULL)
    {
        _FreePrintDropData(pthp);
        return 0;
    }


#ifdef WINNT
    // First try to drop as a link to a remote print folder
    pida = DataObj_GetHIDA(pthp->pDataObj, &medium);
    if (pida)
    {
        // Make sure that if one item in the dataobject is a
        // remote print folder, that they are all remote print folders.

        // If none are, we just give up on dropping as a RPF link, and
        // fall through to checking for printer shares via the
        // NETRESOURCE clipboard format, below.
        UINT i, cRPFs = 0, cNonRPFs = 0;
        
        CPrinterFolder_HIDATestForRmPrns( pida, &cRPFs, &cNonRPFs );

        if ((cRPFs > 0) && (cNonRPFs == 0))
        {
            // All the items in the dataobject are remote print folders or
            // printers under remote printer folders
            for (i = 0; i < pida->cidl; i++)
            {
                LPITEMIDLIST pidlTo = IDA_ILClone(pida, i);
                if (pidlTo)
                {
                    LPITEMIDLIST pidlRemainder; // The part after the remote regitem
                    NET_IsRemoteRegItem(pidlTo, &CLSID_Printers, &pidlRemainder);
                    if (ILIsEmpty(pidlRemainder))
                    {
                        // This is a remote printer folder.  Drop a link to the
                        // 'PrintHood' directory

                        IShellFolder2 *psf = CPrintRoot_GetPSF();
                        if (psf)
                        {
                            IDropTarget *pdt;
                            hres = psf->lpVtbl->CreateViewObject(psf, pthp->pdt->hwndOwner,
                                                                 &IID_IDropTarget, &pdt);
                            if (SUCCEEDED(hres))
                            {
                                pthp->dwEffect = DROPEFFECT_LINK;
                                hres = SHSimulateDrop(pdt, pthp->pDataObj, pthp->grfKeyState, &pthp->pt, &pthp->dwEffect);
                                pdt->lpVtbl->Release(pdt);
                            }
                        }
                    }
                    else
                    {
                        TCHAR szPrinter[MAX_PATH];

                        SHGetNameAndFlags(pidlTo, SHGDN_FORPARSING, szPrinter, ARRAYSIZE(szPrinter), NULL);
                        //
                        // Setup if not the add printer wizard.
                        //
                        if (lstrcmpi(szPrinter, c_szNewObject))
                        {
                            LPITEMIDLIST pidl = Printers_PrinterSetup(pthp->pdt->hwndOwner, MSP_NETPRINTER, szPrinter, NULL);
                            if (pidl)
                                ILFree(pidl);
                        }
                    }
                    ILFree(pidlTo);

                    if (FAILED(hres))
                        break;
                }
            }
            HIDA_ReleaseStgMedium(pida, &medium);
            SHChangeNotifyHandleEvents();       // force update now
            goto Cleanup;
        }
        else if ((cRPFs > 0) && (cNonRPFs > 0))
        {
            // At least one, but not all, item(s) in this dataobject
            // was a remote printer folder.  Jump out now.
            goto Cleanup;
        }

        // else none of the items in the dataobject were remote print
        // folders, so fall through to the NETRESOURCE parsing
    }

    // Reset FORMATETC to NETRESOURCE clipformat for next GetData call
    fmte.cfFormat = g_cfNetResource;
#endif // WINNT

    // DragEnter only allows network resources to be DROPEFFECT_LINKed
    ASSERT(NOERROR == pthp->pDataObj->lpVtbl->QueryGetData(pthp->pDataObj, &fmte));

    if (SUCCEEDED(pthp->pDataObj->lpVtbl->GetData(pthp->pDataObj, &fmte, &medium)))
    {
        LPNETRESOURCE pnr = (LPNETRESOURCE)LocalAlloc(LPTR, 1024);
        if (pnr)
        {
            BOOL fNonPrnShare = FALSE;
            UINT iItem, cItems = SHGetNetResource(medium.hGlobal, (UINT)-1, NULL, 0);

            for (iItem = 0; iItem < cItems; iItem++)
            {
                if (SHGetNetResource(medium.hGlobal, iItem, pnr, 1024) &&
                    pnr->dwDisplayType == RESOURCEDISPLAYTYPE_SHARE &&
                    pnr->dwType == RESOURCETYPE_PRINT)
                {
                    LPITEMIDLIST pidl = Printers_GetInstalledNetPrinter(pnr->lpRemoteName);
                    if (pidl)
                    {
                        TCHAR szPrinter[MAXNAMELENBUFFER];
                        LPCIDPRINTER pidp = (LPCIDPRINTER)ILFindLastID(pidl);
                        int i;

                        // a printer connected to this share already exists,
                        // does the user really want to install another one?

                        SetForegroundWindow(pthp->pdt->hwndOwner);

                        ualstrcpyn(szPrinter, pidp->cName, ARRAYSIZE(szPrinter));
                        i = ShellMessageBox(HINST_THISDLL,
                                    pthp->pdt->hwndOwner,
                                    MAKEINTRESOURCE(IDS_REINSTALLNETPRINTER), NULL,
                                    MB_YESNO|MB_ICONINFORMATION,
                                    szPrinter, (LPTSTR)pnr->lpRemoteName);

                        ILFree(pidl);

                        if (i != IDYES)
                            continue;
                    }

                    pidl = Printers_PrinterSetup(pthp->pdt->hwndOwner,
                               MSP_NETPRINTER, pnr->lpRemoteName, NULL);

                    if (pidl)
                        ILFree(pidl);
                }
                else
                {
                    if (!fNonPrnShare)
                    {
                        // so we don't get > 1 of these messages per drop
                        fNonPrnShare = TRUE;

                        // let the user know that they can't drop non-printer
                        // shares into the printers folder
                        SetForegroundWindow(pthp->pdt->hwndOwner);
                        ShellMessageBox(HINST_THISDLL,
                            pthp->pdt->hwndOwner,
                            MAKEINTRESOURCE(IDS_CANTINSTALLRESOURCE), NULL,
                            MB_OK|MB_ICONINFORMATION,
                            (LPTSTR)pnr->lpRemoteName);
                    }
                }
            }

            LocalFree((HLOCAL)pnr);
        }
        ReleaseStgMedium(&medium);
    }

#ifdef WINNT
Cleanup:
#endif

    _FreePrintDropData(pthp);
    return 0;
}

STDMETHODIMP CPrinterFolder_Drop(IDropTarget *pdt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdt);
    HRESULT hres;

    *pdwEffect = DROPEFFECT_LINK;

    hres = CIDLDropTarget_DragDropMenu(this, DROPEFFECT_LINK, pDataObj,
        pt, pdwEffect, NULL, NULL, MENU_PRINTOBJ_NEWPRN_DD, grfKeyState);

    if (*pdwEffect)
    {
        PRNTD *pthp = (PRNTD *)LocalAlloc(LPTR, SIZEOF(*pthp));
        if (pthp)
        {
            pthp->grfKeyState = grfKeyState;
            pthp->pt          = pt;
            pthp->dwEffect    = *pdwEffect;

            CoMarshalInterThreadInterfaceInStream(&IID_IDataObject, (IUnknown *)pDataObj, &pthp->pstmDataObj);

            pthp->pdt = this;
            pthp->pdt->dropt.lpVtbl->AddRef(&pthp->pdt->dropt);

            if (SHCreateThread(CPrinterDrop_ThreadProc, pthp, CTF_COINIT, NULL))
            {
                hres = NOERROR;
            }
            else
            {
                _FreePrintDropData(pthp);
                hres = E_OUTOFMEMORY;
            }
        }
    }
    CIDLDropTarget_DragLeave(pdt);

    return hres;
}


const IDropTargetVtbl c_CPrinterDropTargetVtbl =
{
    CIDLDropTarget_QueryInterface, CIDLDropTarget_AddRef, CIDLDropTarget_Release,
    CPrinterFolder_DragEnter,
    CIDLDropTarget_DragOver,
    CIDLDropTarget_DragLeave,
    CPrinterFolder_Drop,
};


//---------------------------------------------------------------------------
//
// IDataObject stuff
//
// A printer's IDataObject is built on top of CIDL's IDataObject implementation

STDMETHODIMP CPrintersIDLData_QueryGetData(IDataObject * pdtobj, LPFORMATETC pformatetc)
{
    if ((pformatetc->cfFormat == g_cfPrinterFriendlyName) &&
        (pformatetc->tymed & TYMED_HGLOBAL))
    {
        return NOERROR; // same as S_OK
    }

    return CIDLData_QueryGetData(pdtobj, pformatetc);
}

STDMETHODIMP CPrintersIDLData_GetData(IDataObject * pdtobj, LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    HRESULT hres = E_INVALIDARG;

    // g_cfPrinterFriendlyName creates an HDROP-like structure that contains
    // friendly printer names (instead of absolute paths) for the objects
    // in pdtobj.  The handle returned from this can be used by the HDROP
    // functions DragQueryFile, DragQueryInfo, ...
    //
    if ((pformatetcIn->cfFormat == g_cfPrinterFriendlyName) &&
        (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        STGMEDIUM medium;
        UINT i, cbRequired = SIZEOF(DROPFILES) + SIZEOF(TCHAR); // dbl null terminated
        LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

        for (i = 0; i < pida->cidl; i++)
        {
            LPIDPRINTER pidp = (LPIDPRINTER)IDA_GetIDListPtr(pida, i);
            cbRequired += ualstrlen(pidp->cName ) * SIZEOF(TCHAR) + SIZEOF(TCHAR);
        }

        pmedium->pUnkForRelease = NULL; // caller should release hmem
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = GlobalAlloc(GPTR, cbRequired);
        if (pmedium->hGlobal)
        {
            LPDROPFILES pdf = (LPDROPFILES)pmedium->hGlobal;
            LPTSTR lps;

            pdf->pFiles = SIZEOF(DROPFILES);
#ifdef UNICODE
            pdf->fWide = TRUE;
#else
            ASSERT(pdf->fWide == FALSE);
#endif

            lps = (LPTSTR)((LPBYTE)pdf + pdf->pFiles);
            for (i = 0; i < pida->cidl; i++)
            {
                LPIDPRINTER pidp = (LPIDPRINTER)IDA_GetIDListPtr(pida, i);
                ualstrcpy(lps, pidp->cName);
                lps += lstrlen(lps) + 1;
            }
            ASSERT(*lps == 0);

            hres = NOERROR;
        }
        else
            hres = E_OUTOFMEMORY;

        HIDA_ReleaseStgMedium(pida, &medium);
    }
    else
    {
        hres = CIDLData_GetData(pdtobj, pformatetcIn, pmedium);
    }

    return hres;
}

const IDataObjectVtbl c_CPrintersIDLDataVtbl = {
    CIDLData_QueryInterface, CIDLData_AddRef, CIDLData_Release,
    CPrintersIDLData_GetData,
    CIDLData_GetDataHere,
    CPrintersIDLData_QueryGetData,
    CIDLData_GetCanonicalFormatEtc,
    CIDLData_SetData,
    CIDLData_EnumFormatEtc,
    CIDLData_Advise,
    CIDLData_Unadvise,
    CIDLData_EnumAdvise
};



//---------------------------------------------------------------------------
//
// IShellFolder stuff
//

//
// The new notification system uses printui.dll's Folder library
// to cache printer data and retrieve notifications.
//
#ifndef PRN_FOLDERDATA

//
// Stuff to play with the hdpaPrinterInfo
//

#define CPrinters_EnterCriticalSection(psf) EnterCriticalSection(&((psf)->csPrinterInfo))
#define CPrinters_LeaveCriticalSection(psf) LeaveCriticalSection(&((psf)->csPrinterInfo))

// CPrinters_SF_GetPrinterInfo2 -- gets a PRINTER_INFO_2 structure for pPrinter
// MUST use CPrinters_SF_FreePrinterInfo2 to free
LPPRINTER_INFO_2 CPrinters_SF_GetPrinterInfo2(CPrinterFolder *psf, LPCTSTR pPrinterName)
{
    PPrinterInfo pi;
    int i;

    CPrinters_EnterCriticalSection(psf);

    for (i = DPA_GetPtrCount(psf->hdpaPrinterInfo)-1 ; i >= 0 ; --i)
    {
        pi = DPA_GetPtr(psf->hdpaPrinterInfo, i);
        if (!lstrcmp(pi->pi2.pPrinterName, pPrinterName))
        {
            goto found;
        }
    }
    pi = NULL;

found:
    // i < 0 && pi == NULL  ||  i >= 0 && pi != NULL

    // if i<0 then we need to add a PRINTER_INFO_2 for pPrinterName
    // if UPDATE_ON_TIMER and PRINTER_POLL_INTERVAL has elapsed, force update
    if (i < 0 ||
        (pi->flags & UPDATE_NOW) ||
        (pi->flags & UPDATE_ON_TIMER &&
         GetTickCount() - pi->dwTimeUpdated > PRINTER_POLL_INTERVAL))
    {
        HANDLE hPrinter = Printer_OpenPrinter(pPrinterName);
        DWORD dwTime;

        if (!hPrinter)
        {
            goto error;
        }

        if (i < 0)
        {
            // We add a typical amount to the PRINTER_INFO_2 to cut down on
            // the number of calls into the subsystem.  We want 700 bytes
            // for the entire pi2 structure.

            pi = (PPrinterInfo)(void*)LocalAlloc(LPTR, SIZEOF_PRINTERINFO(TYPICAL_PRINTER_INFO_2_SIZE));
            if (!pi)
                goto error;
            pi->dwSize = TYPICAL_PRINTER_INFO_2_SIZE;

            #undef TYPICAL_PRINTER_INFO_2_SIZE
        }

        dwTime = GetTickCount();
TryAgain:
        SetLastError(0);
        if (!GetPrinter(hPrinter, 2, (LPBYTE)&(pi->pi2), pi->dwSize, &(pi->dwSize)))
        {
            DWORD dwSize = pi->dwSize;
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                PPrinterInfo piTmp;
                piTmp = (void*)LocalReAlloc((HLOCAL)pi, SIZEOF_PRINTERINFO(dwSize),
                        LMEM_MOVEABLE|LMEM_ZEROINIT);
                if (NULL == piTmp)
                    goto OutOfMem;
                pi = piTmp;
                pi->dwSize = dwSize;
                goto TryAgain;
            }
            else
            {
OutOfMem:
                LocalFree((HLOCAL)pi);
                pi = NULL;
            }
        }

        if (pi)
        {
            pi->dwTimeUpdated = dwTime;
            pi->flags = FALSE;
            // Always set this bit for now, since there's no way to hook the
            // FSNotify stuff into this to turn the bit on.  If this cache
            // goes global, then we can do it right...
            //if (pi->pi2.Attributes & PRINTER_ATTRIBUTE_NETWORK)
                pi->flags = UPDATE_ON_TIMER;

            if (i < 0)
            {
                // insert a new PRINTER_INFO_2
                DebugMsg(TF_PRINTER, TEXT("PRINTER_INFO_2 cache: Inserting %s (%x)"), pi->pi2.pPrinterName, pi);
                DPA_InsertPtr(psf->hdpaPrinterInfo,MAXSHORT,pi);
            }
            else
            {
                // update existing PRINTER_INFO_2
                DebugMsg(TF_PRINTER, TEXT("PRINTER_INFO_2 cache: Updating %d: %s (%x)"), i, pi->pi2.pPrinterName, pi);
                DPA_SetPtr(psf->hdpaPrinterInfo,i,pi);
            }
        }

        Printer_ClosePrinter(hPrinter);
    }
    else
    {
        DebugMsg(TF_PRINTER, TEXT("PRINTER_INFO_2 cache: Found %d: %s (%x)"), i, pi->pi2.pPrinterName, pi);
    }

error:
    if (!pi)
    {
        CPrinters_LeaveCriticalSection(psf);
        return FALSE;
    }

    // Seems to me like it's cheaper to keep the critical section locked
    // and unlocking it an the _FreePrinterInfo2 call instead of allocating
    // a block of memory, copying the PRINTER_INFO_2 structure, and then
    // freeing the memory.  (Since all calls to this only use
    // the PRINTER_INFO_2 for a short period of time.)  Plus, we will probably
    // only have one thread running through here at any time anyhow.

    return(&(pi->pi2));
}

// CPrinters_SF_FreePrinterInfo2 "frees" a printer_info_2 from
// the above function.  Since we're really cachine these and forcing serial
// access to them, all psf function really does is free a critical section,
// so we don't need to pass the printer_info_2 pointer.
// REVIEW: how do we inline things like psf?
void CPrinters_SF_FreePrinterInfo2(CPrinterFolder *psf)
{
    CPrinters_LeaveCriticalSection(psf);
}

// psf function forces the next GetPrinterInfo2 to update.
// I want this to be called when an SHCNE_UPDATEITEM occurs on a printer,
// so we can turn on the UPDATE_ON_TIMER bit.  But that's not going to happen.
// So we call this whenever the shell updates something, setting UPDATE_NOW.
void CPrinters_SF_UpdatePrinterInfo2(CPrinterFolder *psf, LPCTSTR pPrinterName, UINT flags)
{
    PPrinterInfo pi;
    int i;

    CPrinters_EnterCriticalSection(psf);

    for (i = DPA_GetPtrCount(psf->hdpaPrinterInfo)-1 ; i >= 0 ; --i)
    {
        pi = DPA_GetPtr(psf->hdpaPrinterInfo, i);
        if (!lstrcmp(pi->pi2.pPrinterName, pPrinterName))
        {
            DebugMsg(TF_PRINTER, TEXT("PRINTER_INFO_2 cache: Marking %d: %s (%x) as %x"), i, pi->pi2.pPrinterName, pi, flags);
            pi->flags |= flags;
            break;
        }
    }

    CPrinters_LeaveCriticalSection(psf);
}

// psf removes a printer_info_2 from the cache.
// Called when a printer is deleted.

void CPrinters_SF_RemovePrinterInfo2(CPrinterFolder *psf, LPCTSTR pPrinterName)
{
    PPrinterInfo pi;
    int i;

    CPrinters_EnterCriticalSection(psf);

    for (i = DPA_GetPtrCount(psf->hdpaPrinterInfo)-1 ; i >= 0 ; --i)
    {
        pi = DPA_GetPtr(psf->hdpaPrinterInfo, i);
        if (!lstrcmp(pi->pi2.pPrinterName, pPrinterName))
        {
            DebugMsg(TF_PRINTER, TEXT("PRINTER_INFO_2 cache: Removing %d: %s (%x)"), i, pi->pi2.pPrinterName, pi);
            LocalFree((HLOCAL)pi);
            DPA_DeletePtr(psf->hdpaPrinterInfo, i);
            break;
        }
    }

    CPrinters_LeaveCriticalSection(psf);
}

// When the sf is going away, we need to free all our pointers
void CPrinters_SF_FreeHDPAPrinterInfo(HDPA hdpa)
{
    PPrinterInfo pi;
    int i;

    DebugMsg(TF_PRINTER, TEXT("PRINTER_INFO_2 cache: freeing everything."));

    // Since this is only called when the shell folder is going away,
    // nobody is using the cache.  So we don't take the critical section.

    for (i = DPA_GetPtrCount(hdpa)-1 ; i >= 0 ; --i)
    {
        pi = DPA_GetPtr(hdpa, i);
        LocalFree((HLOCAL)pi);
    }
    DPA_Destroy(hdpa);
}

#endif // ndef PRN_FOLDERDATA


STDMETHODIMP CPrinters_SF_QueryInterface(IShellFolder2 *psf, REFIID riid, void **ppvObj)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);

    if (IsEqualIID(riid, &IID_IShellFolder) ||
        IsEqualIID(riid, &IID_IUnknown)     ||
        IsEqualIID(riid, &IID_IShellFolder2))
    {
        *ppvObj = &this->sf;
    }
    else if (IsEqualIID(riid, &IID_IPersistFolder) ||
             IsEqualIID(riid, &IID_IPersistFolder2) ||
             IsEqualIID(riid, &IID_IPersist))
    {
        *ppvObj = &this->pf;
    }
    else if (IsEqualIID(riid, &IID_IShellIconOverlay))
    {
        *ppvObj = &this->sio;
    }
#ifdef WINNT
    else if (IsEqualIID(riid, &IID_IRemoteComputer))
    {
        *ppvObj = &this->rc;
    }
    else if (IsEqualIID(riid, &IID_IPrinterFolder))
    {
        *ppvObj = &this->prnf;
    }
#endif
#ifdef PRN_FOLDERDATA
    else if (IsEqualIID(riid, &IID_IFolderNotify))
    {
        *ppvObj = &this->nf;
    }
#endif
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    this->cRef++;
    return NOERROR;
}

STDMETHODIMP_(ULONG) CPrinters_SF_Release(IShellFolder2 *psf)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);
    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

#ifdef PRN_FOLDERDATA
    if (this->hFolder)
    {
        if (g_bRunOnNT5)
            UnregisterPrintNotify(this->pszServer, &this->nf, &this->hFolder);
        else
            vFolderUnregister(this->hFolder);
    }
#else
    DeleteCriticalSection(&this->csPrinterInfo);
    CPrinters_SF_FreeHDPAPrinterInfo(this->hdpaPrinterInfo);
#endif

    //
    // The pidl must be freed here!! (after unregister from PRUNTUI.DLL),
    // because if you move this code before the call to vFolderUnregister Notify
    // serious race condition occurs. Have in mind that the interface which
    // is used for communication with PRINTUI is part this class and 
    // and uses the pidl in its ProcessNotify( ... ) member
    //
    ILFree(this->pidl);

    if (this->pszServer)
        LocalFree(this->pszServer);

    LocalFree((HLOCAL)this);
    return 0;
}

STDMETHODIMP_(ULONG) CPrinters_SF_AddRef(IShellFolder2 *psf)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);
    return InterlockedIncrement(&this->cRef);
}




STDMETHODIMP CPrinters_SF_ParseDisplayName(IShellFolder2 *psf, HWND hwnd,
        LPBC pbc, LPOLESTR lpszDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    *ppidl = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CPrinters_SF_GetAttributesOf(IShellFolder2 *psf, UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);
    HRESULT hres = NOERROR;
    ULONG rgfOut = SFGAO_CANLINK|SFGAO_CANDELETE|SFGAO_CANRENAME|SFGAO_HASPROPSHEET|SFGAO_DROPTARGET;
    ULONG rgfIn = *prgfInOut;

    UINT i;

    if ((cidl != 0) && HOOD_COL_FILE == CPrintRoot_GetPIDLType(apidl[0]))
    {
        IShellFolder2 *psf = CPrintRoot_GetPSF();
        if (psf)
            return psf->lpVtbl->GetAttributesOf(psf, cidl, apidl, prgfInOut);
        return E_INVALIDARG;
    }

    // if c_szNewObject is selected, we support CANLINK *only*
    for (i = 0 ; i < cidl ; i++)
    {
        LPIDPRINTER pidp = (LPIDPRINTER)apidl[i];

        TCHAR szPrinter[MAXNAMELENBUFFER];
        ualstrcpyn(szPrinter, pidp->cName, ARRAYSIZE(szPrinter));

        //
        // if c_szNewObject is selected, we support CANLINK *only*
        //
        if (!lstrcmp(szPrinter, c_szNewObject))
        {
            rgfOut &= SFGAO_CANLINK;
        }
#ifdef WINNT
        else
        {
            // Don't allow renaming of printer connections on WINNT.
            // This is disallowed becase on WINNT, the printer connection
            // name _must_ be the in the format \\server\printer.  On
            // win9x, the user can rename printer connections.
            if (Printer_CheckNetworkPrinterByName(szPrinter, NULL))
            {
                rgfOut &= ~SFGAO_CANRENAME;
            }
        }
#endif
    }

    *prgfInOut &= rgfOut;

    if (cidl == 1 && (rgfIn & (SFGAO_SHARE|SFGAO_GHOSTED)))
    {
        LPIDPRINTER pidp = (LPIDPRINTER)apidl[0];
        LPVOID pData = NULL;
        DWORD dwAttributes;
        TCHAR szFullPrinter[MAXNAMELENBUFFER];
        LPCTSTR pszPrinter = Printer_BuildPrinterName(this, szFullPrinter, pidp, NULL);

#ifdef PRN_FOLDERDATA

        //
        // If we have notification code, use the hFolder to get
        // printer data instead of querying the printer directly.
        //
        if (this->hFolder)
        {
            pData = Printer_FolderGetPrinter(this->hFolder, pszPrinter);
            if (pData)
                dwAttributes = ((PFOLDER_PRINTER_DATA)pData)->Attributes;
        }
        else
#endif
        {
            pData = Printer_GetPrinterInfoStr(szFullPrinter, 5);
            if (pData)
                dwAttributes = ((PPRINTER_INFO_5)pData)->Attributes;
        }

        if (pData)
        {
            if (dwAttributes & PRINTER_ATTRIBUTE_SHARED
#ifdef WINNT
                //
                // NT appears to return all network printers with their
                // share bit on. I think this is intentional.
                //
                && (dwAttributes & PRINTER_ATTRIBUTE_NETWORK) == 0
#endif
                )
            {
                *prgfInOut |= SFGAO_SHARE;
            }
            if (dwAttributes & PRINTER_ATTRIBUTE_WORK_OFFLINE)
                *prgfInOut |= SFGAO_GHOSTED;
            else
                *prgfInOut &= ~SFGAO_GHOSTED;

            LocalFree((HLOCAL)pData);
        }
        else
        {
            // This fct used to always return E_OUTOFMEMORY if pData was NULL.  pData can be
            // NULL for other reasons than out of memory.  So this failure is not really valid.
            // However the Shell handle this failure (which is bad in the first place).
            // If we fail, we just set the attributes to 0 and go on as if nothing happenned.
            // Star Office 5.0, does not handle the E_OUTOFMEMORY properly, they handle it as
            // a failure (which is exactly what we report to them) and they stop their 
            // processing to show the Add Printer icon.  But they're stupid on one point, they
            // check for S_OK directly so I cannot return S_FALSE. (stephstm, 07/30/99)

            if (SHGetAppCompatFlags(ACF_STAROFFICE5PRINTER) && 
                (ERROR_INVALID_PRINTER_NAME == GetLastError()))
            {
                hres = S_OK;
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
        }
    }
    return hres;
}

//
// Stolen almost verbatim from netviewx.c's CNetRoot_MakeStripToLikeKinds
//
// Takes a possibly-heterogenous pidl array, and strips out the pidls that
// don't match the requested type.  (If fPrinterObjects is TRUE, we're asking
// for printers pidls, otherwise we're asking for the filesystem/link
// objects.)  The return value is TRUE if we had to allocate a new array
// in which to return the reduced set of pidls (in which case the caller
// should free the array with LocalFree()), FALSE if we are returning the
// original array of pidls (in which case no cleanup is required).
//
BOOL CPrinters_ReduceToLikeKinds(UINT *pcidl, LPCITEMIDLIST **papidl, BOOL fPrintObjects)
{
    LPITEMIDLIST *apidl = (LPITEMIDLIST*)*papidl;
    int cidl = *pcidl;

    int iidl;
    LPITEMIDLIST *apidlHomo;
    int cpidlHomo;

    for (iidl = 0; iidl < cidl; iidl++)
    {
        if ((HOOD_COL_PRINTER == CPrintRoot_GetPIDLType(apidl[iidl])) != fPrintObjects)
        {
            apidlHomo = (LPITEMIDLIST *)LocalAlloc(LPTR, SIZEOF(LPITEMIDLIST) * cidl);
            if (!apidlHomo)
                return FALSE;

            cpidlHomo = 0;
            for (iidl = 0; iidl < cidl; iidl++)
            {
                if ((HOOD_COL_PRINTER == CPrintRoot_GetPIDLType(apidl[iidl])) == fPrintObjects)
                    apidlHomo[cpidlHomo++] = apidl[iidl];
            }

            // Setup to use the stripped version of the pidl array...
            *pcidl = cpidlHomo;
            *papidl = apidlHomo;
            return TRUE;
        }
    }

    return FALSE;
}


HRESULT CPrinters_CreateDropTarget(CPrinterFolder *this, HWND hwnd, void **ppv)
{
    return CIDLDropTarget_Create2(hwnd, &c_CPrinterDropTargetVtbl, NULL, NULL, (IDropTarget **)ppv);
}

STDMETHODIMP CPrinters_SF_GetUIObjectOf(IShellFolder2 *psf, HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                        REFIID riid, UINT *prgfInOut, void **ppvOut)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);
    HRESULT hres = E_INVALIDARG;

    LPCTSTR pszPrinter = NULL;
    BOOL    fStripped = FALSE;

    TCHAR szFullPrinter[MAXNAMELENBUFFER];

    //
    // If we have a multi-select case, then we'll strip out any objects
    // not of the same type (link vs. printer) in the pidl array before
    // either deferring to the PrintHood CFSFolder implementation, or
    // falling through to the previous code, which was printers-only.
    //
    if (cidl && (HOOD_COL_FILE == CPrintRoot_GetPIDLType(apidl[0])))
    {
        // Defer to the filesystem for links
        IShellFolder2 *psfPrintRoot;
        fStripped = CPrinters_ReduceToLikeKinds(&cidl, &apidl, FALSE);
        psfPrintRoot = CPrintRoot_GetPSF();
        if (psfPrintRoot)
            hres = psfPrintRoot->lpVtbl->GetUIObjectOf(psfPrintRoot, hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
        else
            hres = E_INVALIDARG;
    }
    else
    {
        fStripped = CPrinters_ReduceToLikeKinds(&cidl, &apidl, TRUE);

        // Current code could bail at this point, but just in case will fall through...
        if (cidl > 0)
            pszPrinter = Printer_BuildPrinterName(this, szFullPrinter, (LPIDPRINTER)apidl[0], NULL);

        if (cidl == 1 && 
            (IsEqualIID(riid, &IID_IExtractIconA) || IsEqualIID(riid, &IID_IExtractIconW)))
        {
            int iIcon;
            TCHAR szBuf[MAX_PATH+20];
            LPTSTR pszModule = NULL;

            if (!lstrcmp(pszPrinter, c_szNewObject))
            {
                iIcon = IDI_NEWPRN;
            }
            else
            {
                pszModule = Printer_FindIcon(this, szFullPrinter, szBuf, ARRAYSIZE(szBuf), &iIcon);
            }

            hres = SHCreateDefExtIcon(pszModule, EIRESID(iIcon), -1, GIL_PERINSTANCE, riid, ppvOut);
        }
        else if (cidl>0 && IsEqualIID(riid, &IID_IContextMenu))
        {
            HKEY hkeyBaseProgID = NULL;
            int nCount = 0;

            if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Printers"), &hkeyBaseProgID))
                nCount++;

            hres = CDefFolderMenu_Create2(this->pidl, hwndOwner,
                cidl, apidl, (IShellFolder *)psf, CPrinters_DFMCallBack,
                nCount, &hkeyBaseProgID, (IContextMenu **)ppvOut);

            if (hkeyBaseProgID)
                RegCloseKey(hkeyBaseProgID);
        }
        else if (cidl>0 && IsEqualIID(riid, &IID_IDataObject))
        {
            hres = CIDLData_CreateFromIDArray2(&c_CPrintersIDLDataVtbl,
                        this->pidl, cidl, apidl, (IDataObject * *)ppvOut);
        }
        else if (cidl==1 && IsEqualIID(riid, &IID_IDropTarget))
        {
            LPIDPRINTER pidp = (LPIDPRINTER)apidl[0];

            // Only allow drag and drop operations to the local print
            // folder.
            if (this->pszServer == NULL)
            {
                if (!lstrcmp(pszPrinter, c_szNewObject))
                {
                    // "NewPrinter" accepts network printer shares
                    hres = CPrinters_CreateDropTarget(this, hwndOwner, ppvOut);
                }
                else
                {
                    extern IDropTargetVtbl c_CPrintObjsDropTargetVtbl;

                    // regular printer objects accept files
                    hres = CIDLDropTarget_Create(hwndOwner, &c_CPrintObjsDropTargetVtbl,
                                (LPITEMIDLIST)pidp, (IDropTarget **)ppvOut);
                }
            }
        }
#ifdef WINNT
        else if (cidl == 1 && IsEqualIID(riid, &IID_IQueryInfo))
        {
            hres = CPrinters_GetQueryInfo(psf, apidl[0], ppvOut);
        }
#endif
        if (fStripped)
            LocalFree((HLOCAL)apidl);
    }

    return hres;
}

#ifdef PRN_FOLDERDATA

BOOL CPrinters_RegisterNotify(CPrinterFolder *this, BOOL bRefresh)
{
    BOOL bResult;

    if (this->hFolder == NULL)
    {
        if (g_bRunOnNT5)
        {
            bResult = SUCCEEDED(RegisterPrintNotify(this->pszServer, &this->nf, &this->hFolder));
        }
        else
        {
            bResult = ((this->hFolder = hFolderRegister(this->pszServer ? this->pszServer : TEXT(""), this->pidl)) != NULL);
        }
    }
    else
        bResult = TRUE;

    // If everything went OK than - proceed to refresh
    if (bResult)
    {
        if (this->hFolder && bRefresh)
        {
            if (!this->bRefreshed)
            {
                bFolderRefresh(this->hFolder, &this->bShowAddPrinter);
                this->bRefreshed = TRUE;    // Disable refresh further from here
            }
        }
    }
    
    return bResult;
}

#else

#define CPrinters_RegisterNotify(p, b)    (TRUE)

#endif



STDMETHODIMP CPrinters_SF_EnumObjects(IShellFolder2 *psf, HWND hwndOwner, DWORD grfFlags, IEnumIDList **ppenum)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);
    CPrintersEnum *pesf;
    IShellFolder2 *psfPrintHood;

    *ppenum = NULL;

    CPrinters_RegisterNotify(this, FALSE);
    
    pesf = (CPrintersEnum *)LocalAlloc(LPTR, SIZEOF(CPrintersEnum));
    if (!pesf)
        return E_OUTOFMEMORY;

    //
    // Always try to enum links.
    //

    psfPrintHood = CPrintRoot_GetPSF();

    // By default we always do standard (printer) enumeration

    pesf->dwRemote = 0;

    // Only add links (from the PrintHood directory) to the enumeration
    // if this is the local print folder

#ifdef WINNT
    if (this->pszServer == NULL)
    {
        if (psfPrintHood)
            psfPrintHood->lpVtbl->EnumObjects(psfPrintHood, NULL, grfFlags, &(pesf->peunk));

        if (pesf->peunk)
        {
            // If this went OK, we will also enumerate links

            pesf->dwRemote |= RMF_SHOWLINKS;
        }
    }
#endif

    pesf->eidl.lpVtbl = &c_PrintersESFVtbl;
    pesf->cRef = 1;
    pesf->grfFlags = grfFlags;
    pesf->nLastFound = -1;

    pesf->ppsf = this;

    *ppenum = (IEnumIDList *)&pesf->eidl;
    return NOERROR;
}

/*++

Routine Name:

    CPrinters_SF_GetStatusString

Routine Description:

    Returns the printer status string in the privided 
    buffer.

Arguments:

    pData - pointer to printer data, i.e. cache data
    pBuff - pointer to buffer where to return status string.
    uSize - size in characters of status buffer.

Return Value:

    pointer to printer status string.

--*/

#ifdef PRN_FOLDERDATA
LPCTSTR CPrinters_SF_GetStatusString(PFOLDER_PRINTER_DATA pData, LPTSTR pBuff, UINT uSize)
#else
LPCTSTR CPrinters_SF_GetStatusString(LPPRINTER_INFO_2 pData, LPTSTR pBuff, UINT uSize)
#endif
{
    LPCTSTR pszReturn = pBuff;
    DWORD dwStatus = pData->Status;

    ASSERT(pData);
    ASSERT(pBuff);
    ASSERT(uSize);

    //
    // Null initialize status buffer.
    //
    *pBuff = 0;

    // HACK: Use this free bit for "Work Offline"
    // 99/03/30 #308785 vtan: compare the strings displayed. Adjust
    // for this hack from CPrinters_SF_GetDetailsOf().
    if (pData->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE)
        dwStatus |= PRINTER_HACK_WORK_OFFLINE;

    //
    // If there is queue status value then convert the status id to a 
    // readable status string.
    //
    if (dwStatus)
    {
        Printer_BitsToString(dwStatus, IDS_PRQSTATUS_SEPARATOR, ssPrinterStatus, pBuff, uSize);
    }
#ifdef WINNT
    else
    {
        //
        // If we do not have queue status string then the status of the queue 
        // is 0 and assumed ready, display ready rather than an empty string.
        //
        if (!pData->pStatus)
        {
            LoadString(HINST_THISDLL, IDS_PRN_INFOTIP_READY, pBuff, uSize);
        }
        else
        {
            //
            // If we do not have a queue status value then we assume we 
            // must have a queue status string.  Queue status strings
            // are cooked up string from printui to indicate pending 
            // connection status. i.e. opening|retrying|unable to connect|etc.
            //
            pszReturn = pData->pStatus;
        }
    }
#endif

    return pszReturn;
}

/*++

Routine Name:

    CPrinters_SF_GetCompareDisplayName

Routine Description:

    Compares the printers display name for column sorting 
    support.

Arguments:

    pName1 - pointer to unalligned printer name.
    pName2 - pointer to unalligned printer name.

Return Value:

    -1 = pName1 less than pName2 
     0 = pName1 equal to pName2 
     1 = pName1 greather than pName2 

--*/

INT CPrinters_SF_GetCompareDisplayName(UNALIGNED LPCTSTR pName1, UNALIGNED LPCTSTR pName2)
{

#ifdef WINNT

    LPCTSTR pszServer                   = NULL;
    LPCTSTR pszPrinter                  = NULL;
    LPTSTR  pszRet1                     = NULL;
    LPTSTR  pszRet2                     = NULL;
    TCHAR   szTemp[MAXNAMELENBUFFER]    = {0};
    INT     iResult                     = 0;

    //
    // We need to break up the full printer name in its components.
    // in order to construct the display name string.
    //
    Printer_SplitFullName(szTemp, pName1, &pszServer, &pszPrinter);

    pszRet1 = ShellConstructMessageString(HINST_THISDLL, 
                                          MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON), 
                                          &pszServer[2], 
                                          pszPrinter);

    if (pszRet1)
    {
        Printer_SplitFullName(szTemp, pName2, &pszServer, &pszPrinter);

        pszRet2 = ShellConstructMessageString(HINST_THISDLL, 
                                              MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON), 
                                              &pszServer[2], 
                                              pszPrinter);

        if (pszRet2)
        {
            pName1 = pszRet1;
            pName2 = pszRet2;
        }
    }

    iResult = ualstrcmpi(pName1, pName2);
    
    if (pszRet1)
        LocalFree(pszRet1);

    if (pszRet2)
        LocalFree(pszRet2);

    return iResult;

#else

    return lstrcmpi(pName1, pName2);

#endif
}

/*++

Routine Name:

    CPrinters_SF_CompareData

Routine Description:

    Compares printer column  data using the 
    column index as a guide indicating which data to compare.

Arguments:

    psf   - pointer to the containter shell folder.
    pidp1 - pointer to unalligned printer name.
    pidp1 - pointer to unalligned printer name.
    iCol  - column index shich to sort on.

Return Value:

    -1 = pName1 less than pName2 
     0 = pName1 equal to pName2 
     1 = pName1 greather than pName2 

--*/

INT CPrinters_SF_CompareData(IShellFolder2 *psf, UNALIGNED IDPRINTER *pidp1, UNALIGNED IDPRINTER *pidp2, LPARAM iCol)
{
    LPCTSTR pName1              = NULL;
    LPCTSTR pName2              = NULL;
    INT     iResult             = 0;
    TCHAR   szTemp1[MAX_PATH]   = {0};
    TCHAR   szTemp2[MAX_PATH]   = {0};
    TCHAR   szName1[MAX_PATH];
    TCHAR   szName2[MAX_PATH];
    BOOL    bDoStringCompare    = TRUE;
#ifdef PRN_FOLDERDATA
    PFOLDER_PRINTER_DATA pData1; 
    PFOLDER_PRINTER_DATA pData2;
#else
    LPPRINTER_INFO_2 pData1;
    LPPRINTER_INFO_2 pData2;
#endif

    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);

    // since the pidp's are UNALIGNED we need to copy the strings out.
    ualstrcpyn(szName1, pidp1->cName, ARRAYSIZE(szName1));
    ualstrcpyn(szName2, pidp2->cName, ARRAYSIZE(szName2));

    // There is no reason to hit the cache for the printer name.
    if ((iCol & SHCIDS_COLUMNMASK) == PRINTERS_ICOL_NAME)
    {
        return CPrinters_SF_GetCompareDisplayName(szName1, szName2);
    }

#ifdef PRN_FOLDERDATA
    pData1 = Printer_FolderGetPrinter(this->hFolder, szName1);
    pData2 = Printer_FolderGetPrinter(this->hFolder, szName2);
#else
    pData1 = CPrinters_SF_GetPrinterInfo2(this, szName1);
    pData2 = CPrinters_SF_GetPrinterInfo2(this, szName2);
#endif

    ASSERT(this);
    
    if (pData1 && pData2)
    {
        switch (iCol & SHCIDS_COLUMNMASK)
        {
        case PRINTERS_ICOL_QUEUESIZE:
            iResult = pData1->cJobs - pData2->cJobs;
            bDoStringCompare = FALSE;
            break;

        case PRINTERS_ICOL_STATUS:
            pName1 = CPrinters_SF_GetStatusString(pData1, szTemp1, ARRAYSIZE(szTemp1));
            pName2 = CPrinters_SF_GetStatusString(pData2, szTemp2, ARRAYSIZE(szTemp1));
            break;

        case PRINTERS_ICOL_COMMENT:
            pName1 = pData1->pComment;
            pName2 = pData2->pComment;
            break;

#ifdef WINNT
        case PRINTERS_ICOL_LOCATION:
            pName1 = pData1->pLocation;
            pName2 = pData2->pLocation;
            break;

        case PRINTERS_ICOL_MODEL:
            pName1 = pData1->pDriverName;
            pName2 = pData2->pDriverName;
            break;
#endif
        default:
            ASSERT(FALSE);
            bDoStringCompare = FALSE;
            break;
        }

        if( bDoStringCompare )
        {
            if (!pName1)
                pName1 = TEXT("");

            if (!pName2)
                pName2 = TEXT("");
                
            TraceMsg(TF_GENERAL, "CPrinters_SF_CompareData %ws %ws", pName1, pName2);

            iResult = lstrcmpi(pName1, pName2);
        }
    }

    //
    // Release the printer data.
    //
#ifdef PRN_FOLDERDATA
    if (pData1)
        LocalFree((HLOCAL)pData1);

    if(pData2)
        LocalFree((HLOCAL)pData2);
#else
    CPrinters_SF_FreePrinterInfo2(this);
#endif

    return iResult;
}

STDMETHODIMP CPrinters_SF_BindToObject(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CPrinters_SF_BindToStorage(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CPrinters_SF_CompareIDs(IShellFolder2 *psf, LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    UNALIGNED IDPRINTER *pidp1 = (UNALIGNED IDPRINTER*)pidl1;
    UNALIGNED IDPRINTER *pidp2 = (UNALIGNED IDPRINTER*)pidl2;

    PIDLTYPE ColateType1 = CPrintRoot_GetPIDLType(pidl1);
    PIDLTYPE ColateType2 = CPrintRoot_GetPIDLType(pidl2);

    if (ColateType1 == ColateType2) 
    {
        // pidls are of same type.

        if (ColateType1 == HOOD_COL_FILE) 
        {
            // pidls are both of type file, so pass on to the IShellFolder
            // interface for the hoods custom directory.

            psf = CPrintRoot_GetPSF();
            if (psf)
            {
                return psf->lpVtbl->CompareIDs(psf, iCol, pidl1, pidl2);
            }
        }
        else
        {
            // pidls are same and not files, so much be printers
            INT i;
#ifdef WINNT
            if (pidp1->dwType != pidp2->dwType)
            {
                return (pidp1->dwType < pidp2->dwType) ?
                       ResultFromShort(-1) :
                       ResultFromShort(1);
            }
#endif
            i = ualstrcmpi(pidp1->cName, pidp2->cName);

            if (i != 0)
            {
                //
                // c_szNewObject is "less" than everything else
                // This implies that when the list is sorted 
                // either accending or decending the add printer
                // wizard object will always appear at the extream
                // ends of the list, i.e. the top or bottom.
                //
                // !!BUGBUG!! 
                // The printers folder should be converted to a 
                // delegate folder like nethood.  The the new printer
                // object would be an IShellFolder and the other printer
                // another IShellFolder.  
                //
                if (!ualstrcmp(pidp1->cName, c_szNewObject))
                    i = -1;
                else if (!ualstrcmp(pidp2->cName, c_szNewObject))
                    i = 1;
                else 
                {
                    INT     iDataCompareResult;
                    //
                    // Both of the names are not the add printer wizard
                    // object then compare further i.e. using the cached 
                    // column data.
                    //

                    // 99/03/24 #308785 vtan: Make the compare data call.
                    // If that fails use the name compare result which is
                    // known to be non-zero.

                    iDataCompareResult = CPrinters_SF_CompareData(psf, pidp1, pidp2, iCol);
                    if (iDataCompareResult != 0)
                        i = iDataCompareResult;
                }
            }
            return ResultFromShort(i);
        }
    }
    else {

        // pidls are not of same type, so have already been correctly
        // collated (consequently, sorting is first by type and
        // then by subfield).

        return ResultFromShort((( (INT)(ColateType2 - ColateType1)) > 0) ? -1 : 1);
    }

    return E_FAIL;
}


//===========================================================================
//
// To be called back from within CDefFolderMenu
//
HRESULT CALLBACK CPrinters_DFMCallBackBG(IShellFolder *psf, HWND hwndOwner,
                IDataObject * pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);
    HRESULT hres = NOERROR;
    LPQCMINFO pqcm;
    UINT idCmdBase;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        pqcm = (LPQCMINFO)lParam;
        idCmdBase = pqcm->idCmdFirst; // must be called before merge
        CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DRIVES_PRINTERS, POPUP_PRINTERS_POPUPMERGE, pqcm);
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));
        break;

    case DFM_INVOKECOMMAND:
        INSTRUMENT_INVOKECOMMAND(SHCNFI_PRINTERSBG_DFM_INVOKE, hwndOwner, wParam);
        switch(wParam)
        {
        case FSIDM_CONNECT_PRN:
            SHNetConnectionDialog(hwndOwner, NULL, RESOURCETYPE_PRINT);
            break;

        case FSIDM_DISCONNECT_PRN:
            WNetDisconnectDialog(hwndOwner, RESOURCETYPE_PRINT);
            break;

        case FSIDM_SORTBYNAME:
            ShellFolderView_ReArrange(hwndOwner, 0);
            break;
#ifdef WINNT
        case FSIDM_SERVERPROPERTIES:

            SHInvokePrinterCommand(hwndOwner,
                                 PRINTACTION_SERVERPROPERTIES,
                                 this->pszServer ? this->pszServer : TEXT(""),
                                 NULL, 0 );
            break;
#endif
        default:
            // This is one of view menu items, use the default code.
            hres = S_FALSE;
            break;
        }
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }

    return hres;
}

HRESULT CPrinters_SD_Create(CPrinterFolder *ppsf, void ** ppvOut);

STDMETHODIMP CPrinters_SF_CreateViewObject(IShellFolder2 *psf, HWND hwnd, REFIID riid, void **ppvOut)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);

    if (IsEqualIID(riid, &IID_IShellView))
    {
        SFV_CREATE sSFV;
        HRESULT hres;

        sSFV.cbSize   = sizeof(sSFV);
        sSFV.pshf     = (IShellFolder *)psf;
        sSFV.psvOuter = NULL;
        sSFV.psfvcb   = Printer_CreateSFVCB(psf, this, this->pidl);

        hres = SHCreateShellFolderView(&sSFV, (IShellView**)ppvOut);

        if (sSFV.psfvcb)
            sSFV.psfvcb->lpVtbl->Release(sSFV.psfvcb);

        return hres;
    }
    else if (IsEqualIID(riid, &IID_IDropTarget))
    {
        return CPrinters_CreateDropTarget(this, hwnd, ppvOut);
    }
    else if (IsEqualIID(riid, &IID_IShellDetails))
    {
        return CPrinters_SD_Create(this, ppvOut);
    }
    else if (IsEqualIID(riid, &IID_IContextMenu))
    {
        return CDefFolderMenu_Create2(NULL, hwnd,
                0, NULL, (IShellFolder *)psf, CPrinters_DFMCallBackBG,
                0, NULL, (IContextMenu **)ppvOut);
    }

    *ppvOut = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP CPrinters_SF_GetDisplayNameOf(IShellFolder2 *psf,
    LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
{
    LPIDPRINTER pidc = (LPIDPRINTER)pidl;
#ifdef UNICODE
    BOOL bPrinterOnServerFormat = FALSE;
    LPTSTR pszServer;
    TCHAR szBuffer[MAXNAMELENBUFFER];
    TCHAR szTemp[MAXNAMELENBUFFER];
    LPTSTR pszTemp;
    LPTSTR pszPrinter = szBuffer;
#endif

    CPrinterFolder *this = IToClass(CPrinterFolder,
                                         sf, psf);


    if (pidl && HOOD_COL_FILE == CPrintRoot_GetPIDLType(pidl))
    {
        IShellFolder2 *psf = CPrintRoot_GetPSF();

        return psf->lpVtbl->GetDisplayNameOf(psf, pidl, uFlags, lpName);
    }

    // BUGBUG: I should do this with a flag instead of a string
    if (ualstrcmpi(pidc->cName, c_szNewObject))
    {
        UINT uOffset = 0;

#if defined(WINNT) && !defined(PRN_FOLDERDATA)

        //
        // If remoted, then strip off server prefix.  We only need to
        // do this for EnumPrinters, since the notifications strips
        // them off for us.
        //
        if (this->szServer != NULL)
        {
            ASSERT(pidc->cName[0] == TEXT('\\') &&
                   pidc->cName[1] == TEXT('\\') &&
                   IntlStrEqNI(this->pszServer, pidc->cName, lstrlen(this->szServer)));

            uOffset += (lstrlen(this->pszServer) + 1) * sizeof(TCHAR);
        }
#endif

#ifdef UNICODE

#ifdef ALIGNMENT_SCENARIO
        ualstrcpyn(szBuffer, pidc->cName+uOffset, ARRAYSIZE(szBuffer));
        pszPrinter = szBuffer;
#else
        pszPrinter = pidc->cName+uOffset;
#endif

        if (uFlags & SHGDN_INFOLDER)
        {
            //
            // Show just the printer name, not fully qualified.
            // Note: this assumes that the cName is not fully
            // qualified for local printers in the local printer
            // folder.
            //

            //
            // If it's a connection then format as "printer on server."
            //
            Printer_SplitFullName(szTemp, pszPrinter, &pszServer, &pszTemp);

            if (pszServer[0])
            {
                bPrinterOnServerFormat = TRUE;
                pszPrinter = pszTemp;
            }

        }
        else                        // SHGDN_NORMAL
        {
          if (!(uFlags & SHGDN_FORPARSING)) // SHGDN_NORMAL | SHGDN_NORMAL
          {
            //
            // If it's a RPF then extract the server name from psf.
            // Note in the case of masq connections, we still do this
            // (for gateway services: sharing a masq printer).
            //
            if (this->pszServer)
            {
                pszServer = this->pszServer;
                bPrinterOnServerFormat = TRUE;
            }
            else
            {
                //
                // If it's a connection then format as "printer on server."
                //
                Printer_SplitFullName(szTemp, pszPrinter, &pszServer, &pszTemp);

                if (pszServer[0])
                {
                    bPrinterOnServerFormat = TRUE;
                    pszPrinter = pszTemp;
                }
            }
          }
          else                      // SHGDN_NORMAL | SHGDN_FORPARSING
          {
            //
            // Fully qualify the printer name if it's not
            // the add printer wizard.
            //
            if (lstrcmpi(c_szNewObject, pszPrinter))
            {
                Printer_BuildPrinterName(this, szTemp, pidc, NULL);
                pszPrinter = szTemp;
            }

          }
        }
#else

        lpName->uOffset = FIELD_OFFSET(IDPRINTER, cName) + uOffset;
        lpName->uType = STRRET_OFFSET;

#endif // ndef UNICODE

    }
    else
    {
#ifdef UNICODE
        LoadString(HINST_THISDLL, IDS_NEWPRN, szBuffer, ARRAYSIZE(szBuffer));

        //
        // Use "Add Printer Wizard on \\server" description only if not
        // remote and if not in folder view (e.g., on the desktop).
        //
        if (this->pszServer && (uFlags == SHGDN_NORMAL))
        {
            bPrinterOnServerFormat = TRUE;
            pszServer = this->pszServer;
            pszPrinter = szBuffer;

        } 
        else if( uFlags & SHGDN_FORPARSING ) 
        {
            //
            // Return the raw add printer wizard object.
            //
            pszPrinter = (LPTSTR)c_szNewObject;
        }
#else
        lpName->uType = STRRET_CSTR;
        LoadString(HINST_THISDLL, IDS_NEWPRN, lpName->cStr, ARRAYSIZE(lpName->cStr));
#endif
    }

#ifdef UNICODE
    if (bPrinterOnServerFormat)
    {
        //
        // When bRemote is set, we want to translate the name to
        // "printer on server."  Note: we should not have a rename problem
        // since renaming connections is disallowed.
        //
        // pszServer and pszPrinter must be initialize if bRemote is TRUE.
        // Also skip the leading backslashes for the server name.
        //
        LPTSTR pszRet;

        ASSERT(pszServer[0] == TEXT('\\') && pszServer[1] == TEXT('\\'));

        pszRet = ShellConstructMessageString(HINST_THISDLL,
                     MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON),
                     &pszServer[2], pszPrinter);
        if (pszRet)
        {
            HRESULT hres = StringToStrRet(pszRet, lpName);
            LocalFree(pszRet);
            return hres;
        }
        return E_FAIL;
    }
    else
    {
        //
        // Convert to STRET_OLESTR.
        //
        lpName->uType = STRRET_WSTR;
        return SHStrDup(pszPrinter, &lpName->pOleStr);
    }
#endif

    return(NOERROR);
}


HRESULT Printer_SetNameOf(CPrinterFolder *psf, HWND hwndOwner, LPCTSTR pOldName, LPTSTR pNewName, LPITEMIDLIST *ppidlOut)
{
    HRESULT hres = E_OUTOFMEMORY;
    HANDLE hPrinter;
    LPCTSTR pFullOldName;

    TCHAR szFullPrinter[MAXNAMELENBUFFER];

    if (psf)
    {
        Printer_BuildPrinterName(psf, szFullPrinter, NULL, pOldName);
        pFullOldName = szFullPrinter;
    }

    hPrinter = Printer_OpenPrinterAdmin(pFullOldName);

    if (hPrinter)
    {
        LPPRINTER_INFO_2 pPrinter = Printer_GetPrinterInfo(hPrinter, 2);
        if (pPrinter)
        {
            int nTmp;

            if (0 != (nTmp = Printer_IllegalName(pNewName)))
            {
                // BUGBUG: We need to impl ::SetSite() and pass it to UI APIs
                //         to go modal if we display UI.
                ShellMessageBox(HINST_THISDLL, hwndOwner, MAKEINTRESOURCE(nTmp),
                    MAKEINTRESOURCE(IDS_PRINTERS),
                    MB_OK|MB_ICONEXCLAMATION);
            }
            else if (IDYES != CallPrinterCopyHooks(hwndOwner, PO_RENAME, 0,
                        pNewName, 0, pFullOldName, 0))
            {
                // user canceled a shared printer name change, bail.
                hres = E_FAIL;
            }
            else
            {
                pPrinter->pPrinterName = pNewName;
                if (SetPrinter(hPrinter, 2, (LPBYTE)pPrinter, 0))
                {
                    hres = NOERROR;

#ifndef WINNT
                    // It looks like we need to generate a SHCNE_RENAMEITEM event
                    Printer_SHChangeNotifyRename((LPTSTR)pFullOldName, pNewName);

                    // And we need to rename any open queue view
                    PrintDef_UpdateName(pFullOldName, pNewName);
                    PrintDef_RefreshQueue(pNewName);
#endif
                    // return the new pidl if requested
                    if (ppidlOut)
                    {
                        USHORT cb = (USHORT)(FIELD_OFFSET(IDPRINTER, cName) + (lstrlen(pNewName) + 1) * SIZEOF(TCHAR));
                        LPIDPRINTER pidp = (LPIDPRINTER)SHAlloc(cb + SIZEOF(USHORT));
                        if (pidp)
                        {
                            Printers_FillPidl(pidp, pNewName);
                            *ppidlOut = (LPITEMIDLIST)pidp;
                        }
                        else
                        {
                            hres = E_OUTOFMEMORY;
                        }
                    } // if (ppidlOut)
                } // if (SetPrinter...
            }

            LocalFree((HLOCAL)pPrinter);
        }
        Printer_ClosePrinter(hPrinter);
    }

    return hres;
}

STDMETHODIMP CPrinters_SF_SetNameOf(IShellFolder2 *psf, HWND hwndOwner,
    LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD dwReserved, LPITEMIDLIST *ppidlOut)
{
    LPIDPRINTER pidc = (LPIDPRINTER)pidl;
    TCHAR szNewName[MAX_PATH];

    CPrinterFolder *this = IToClass(CPrinterFolder, sf, psf);
    ASSERT(ualstrcmp(pidc->cName, c_szNewObject));

    if (HOOD_COL_FILE == CPrintRoot_GetPIDLType(pidl))
    {
        IShellFolder2 *psf = CPrintRoot_GetPSF();
        return psf->lpVtbl->SetNameOf(psf, hwndOwner, pidl, lpszName, dwReserved, ppidlOut);
    }

    SHUnicodeToTChar(lpszName, szNewName, ARRAYSIZE(szNewName));
    PathRemoveBlanks(szNewName);

    return Printer_SetNameOf(this, hwndOwner, pidc->cName, szNewName, ppidlOut);
}

STDMETHODIMP CPrinters_SF_GetDefaultSearchGUID(IShellFolder2 *psf, LPGUID pGuid)
{
    *pGuid = SRCID_SFindPrinter;
    return S_OK;
}

STDMETHODIMP CPrinters_SF_EnumSearches(IShellFolder2 *psf, LPENUMEXTRASEARCH *ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CPrinters_SF_GetDefaultColumn(IShellFolder2 *psf, DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPrinters_SF_GetDefaultColumnState(IShellFolder2 *psf, UINT iColumn, DWORD *pdwState)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPrinters_SF_GetDetailsEx(IShellFolder2 *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPrinters_SF_GetDetailsOf(IShellFolder2 *psf, LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPrinters_SF_MapColumnToSCID(IShellFolder2 *psf, UINT iCol, SHCOLUMNID *pscid)
{
    return E_NOTIMPL;
}


IShellFolder2Vtbl c_PrintersSFVtbl =
{
    CPrinters_SF_QueryInterface, CPrinters_SF_AddRef, CPrinters_SF_Release,
    CPrinters_SF_ParseDisplayName,
    CPrinters_SF_EnumObjects,
    CPrinters_SF_BindToObject,
    CPrinters_SF_BindToStorage,
    CPrinters_SF_CompareIDs,
    CPrinters_SF_CreateViewObject,
    CPrinters_SF_GetAttributesOf,
    CPrinters_SF_GetUIObjectOf,
    CPrinters_SF_GetDisplayNameOf,
    CPrinters_SF_SetNameOf,
    CPrinters_SF_GetDefaultSearchGUID,
    CPrinters_SF_EnumSearches,
    CPrinters_SF_GetDefaultColumn,
    CPrinters_SF_GetDefaultColumnState,
    CPrinters_SF_GetDetailsEx,
    CPrinters_SF_GetDetailsOf,
    CPrinters_SF_MapColumnToSCID,
};


#ifdef WINNT


//
// Routine to tack on the specified string with some formating 
// to the existing infotip string.  If there was a previous string,
// then we also insert a newline.
//
STDMETHODIMP _FormatInfoTip(LPTSTR *ppszText, UINT idFmt, STRRET *pStrRet)
{
    HRESULT hres;
    TCHAR szFmt[MAX_PATH], szBuf[MAX_PATH];

    LoadString(HINST_THISDLL, idFmt, szFmt, ARRAYSIZE(szFmt));

    hres = StrRetToBuf(pStrRet, NULL, szBuf, ARRAYSIZE(szBuf));

    // If no string was returned, then nothing to do, and we should
    // return success (which "hres" already is).
    //
    if (SUCCEEDED(hres) && szBuf[0])
    {
        //
        // Note: This calculation only works because we assume
        // all the format strings will contain string specifiers.
        //
        UINT uLen = (*ppszText ? lstrlen( *ppszText ) : 0) +
               lstrlen( szFmt ) + lstrlen( szBuf ) +
               (*ppszText ? lstrlen( c_szNewLine ) : 0) + 1;

        LPTSTR  pszText = (TCHAR *) SHAlloc( uLen * SIZEOF(TCHAR) );
        if( pszText )
        {
            *pszText = TEXT('\0');

            if( *ppszText )
            {
                lstrcat( pszText, *ppszText );
                lstrcat( pszText, c_szNewLine );
            }

            uLen = lstrlen( pszText );

            wsprintf( pszText+uLen, szFmt, szBuf );

            if( *ppszText )
                SHFree( *ppszText );

            *ppszText = pszText;

            hres = NOERROR;
        }
    }

    return hres;
}

HRESULT CPrinters_GetQueryInfo(IShellFolder2 *psf, LPCITEMIDLIST pidl, void **ppvOut)
{
    HRESULT hres;
    LPTSTR pszText = NULL;
    TCHAR szPrinter[MAXNAMELENBUFFER];
    SHELLDETAILS dt = {0};
    IShellDetails   *psd;

    *ppvOut = NULL;

    ualstrcpyn(szPrinter, ((LPIDPRINTER)pidl)->cName, ARRAYSIZE(szPrinter));
    if (!lstrcmp(c_szNewObject, szPrinter))
        return E_FAIL;

    hres = psf->lpVtbl->CreateViewObject(psf, NULL, &IID_IShellDetails, (void **)&psd);
    if (SUCCEEDED(hres))
    {
        // Get the printer status string.
        hres = psd->lpVtbl->GetDetailsOf( psd, pidl, PRINTERS_ICOL_STATUS, &dt );

        if (SUCCEEDED(hres))
        {
            hres = _FormatInfoTip(&pszText, IDS_PRN_INFOTIP_STATUS_FMT, &dt.str);
        }
        // Get the printer queue size string.
        if (SUCCEEDED(hres))
        {
            hres = psd->lpVtbl->GetDetailsOf( psd, pidl, PRINTERS_ICOL_QUEUESIZE, &dt );

            if (SUCCEEDED(hres))
            {
                hres = _FormatInfoTip(&pszText, IDS_PRN_INFOTIP_QUEUESIZE_FMT, &dt.str);
            }
        }
        // Get the printer location string.
        if (SUCCEEDED(hres))
        {
            hres = psd->lpVtbl->GetDetailsOf( psd, pidl, PRINTERS_ICOL_LOCATION, &dt );

            if (SUCCEEDED(hres))
            {
                hres = _FormatInfoTip(&pszText, IDS_PRN_INFOTIP_LOCATION_FMT, &dt.str);
            }
        }
        psd->lpVtbl->Release( psd );
    }

    if (pszText)
    {
        hres = CreateInfoTipFromText(pszText, &IID_IQueryInfo, ppvOut);
        SHFree(pszText);
    }
    else
        hres = E_FAIL;
    return hres;
}

BOOL Printer_CheckShowFolder(LPCTSTR pszMachine)
{
    HANDLE hServer = Printer_OpenPrinter(pszMachine);

    if (hServer)
    {
        Printer_ClosePrinter(hServer);
        return TRUE;
    }
    return FALSE;
}


#endif // def WINNT

STDMETHODIMP CPrinters_PF_QueryInterface(IPersistFolder2 *ppf, REFIID riid, void **ppvObj)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, pf, ppf);
    return CPrinters_SF_QueryInterface(&this->sf, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CPrinters_PF_AddRef(IPersistFolder2 *ppf)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, pf, ppf);
    return CPrinters_SF_AddRef(&this->sf);
}

STDMETHODIMP_(ULONG) CPrinters_PF_Release(IPersistFolder2 *ppf)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, pf, ppf);
    return CPrinters_SF_Release(&this->sf);
}

STDMETHODIMP CPrinters_PF_GetClassID(IPersistFolder2 *ppf, LPCLSID lpClassID)
{
    *lpClassID = CLSID_Printers;
    return NOERROR;
}

STDMETHODIMP CPrinters_PF_Initialize(IPersistFolder2 *ppf, LPCITEMIDLIST pidl)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, pf, ppf);
    ASSERT(this->pidl == NULL);
    this->pidl = ILClone(pidl);
    return this->pidl ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CPrinters_PF_GetCurFolder(IPersistFolder2 *ppf, LPITEMIDLIST *ppidl)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, pf, ppf);
    return GetCurFolderImpl(this->pidl, ppidl);
}

IPersistFolder2Vtbl c_CPrintersPFVtbl =
{
    CPrinters_PF_QueryInterface, CPrinters_PF_AddRef, CPrinters_PF_Release,
    CPrinters_PF_GetClassID,
    CPrinters_PF_Initialize,
    CPrinters_PF_GetCurFolder
};


// IShellIconOverlay stuff
STDMETHODIMP CPrinters_SIO_QueryInterface(IShellIconOverlay *psio, REFIID riid, void **ppvObj)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sio, psio);
    return CPrinters_SF_QueryInterface(&this->sf, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CPrinters_SIO_AddRef(IShellIconOverlay *psio)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sio, psio);
    return CPrinters_SF_AddRef(&this->sf);
}

STDMETHODIMP_(ULONG) CPrinters_SIO_Release(IShellIconOverlay *psio)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sio, psio);
    return CPrinters_SF_Release(&this->sf);
}


STDMETHODIMP CPrinters_SIO_GetOverlayIndex(IShellIconOverlay *psio, LPCITEMIDLIST pidl, int *pIndex)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, sio, psio);
    HRESULT hres = E_INVALIDARG;

    if (pidl)
    {
        ULONG uAttrib = SFGAO_SHARE;

        hres = E_FAIL;      // Until proven otherwise...
        CPrinters_SF_GetAttributesOf(&this->sf, 1, &pidl, &uAttrib);
        if (uAttrib & SFGAO_SHARE)
        {
            IShellIconOverlayManager *psiom;
            if (SUCCEEDED(hres = GetIconOverlayManager(&psiom)))
            {
                hres = psiom->lpVtbl->GetReservedOverlayInfo(psiom, L"0", 0, pIndex, SIOM_OVERLAYINDEX, SIOM_RESERVED_SHARED);
                psiom->lpVtbl->Release(psiom);
            }
        }       
    }

    return hres;
}

STDMETHODIMP CPrinters_SIO_GetOverlayIconIndex(IShellIconOverlay *psio, LPCITEMIDLIST pidl, int *pIndex)
{
    return E_NOTIMPL;
}

IShellIconOverlayVtbl c_PrintersSIOVtbl =
{
    CPrinters_SIO_QueryInterface, CPrinters_SIO_AddRef, CPrinters_SIO_Release,
    CPrinters_SIO_GetOverlayIndex,
    CPrinters_SIO_GetOverlayIconIndex,
};


#ifdef WINNT
// IRemoteComputer stuff

STDMETHODIMP CPrinters_RC_QueryInterface(IRemoteComputer *prc, REFIID riid, void **ppvObj)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, rc, prc);
    return CPrinters_SF_QueryInterface(&this->sf, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CPrinters_RC_AddRef(IRemoteComputer *prc)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, rc, prc);
    return CPrinters_SF_AddRef(&this->sf);
}

STDMETHODIMP_(ULONG) CPrinters_RC_Release(IRemoteComputer *prc)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, rc, prc);
    return CPrinters_SF_Release(&this->sf);
}

STDMETHODIMP CPrinters_RC_Initialize(IRemoteComputer *prc, const WCHAR *pszMachine, BOOL bEnumerating)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, rc, prc);
    TCHAR szBuf[MAXCOMPUTERNAME];

    SHUnicodeToTChar(pszMachine, szBuf, ARRAYSIZE(szBuf));

    this->pszServer = StrDup(szBuf);

    //
    // For NT servers, we want to show the remote printer folder. Only check
    // during enumeration
    //
    if (bEnumerating && !Printer_CheckShowFolder(pszMachine))
        return E_FAIL;

    return S_OK;
}

IRemoteComputerVtbl c_PrintersRCVtbl =
{
    CPrinters_RC_QueryInterface, CPrinters_RC_AddRef, CPrinters_RC_Release,
    CPrinters_RC_Initialize,
};

// IPrinterFolder stuff
STDMETHODIMP CPrinters_FI_QueryInterface(IPrinterFolder *pfi, REFIID riid, void **ppvObj)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, prnf, pfi);
    return CPrinters_SF_QueryInterface(&this->sf, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CPrinters_FI_AddRef(IPrinterFolder *pfi)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, prnf, pfi);
    return CPrinters_SF_AddRef(&this->sf);
}

STDMETHODIMP_(ULONG) CPrinters_FI_Release(IPrinterFolder *pfi)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, prnf, pfi);
    return CPrinters_SF_Release(&this->sf);
}

STDMETHODIMP CPrinters_FI_IsPrinter(IPrinterFolder *pfi, LPCITEMIDLIST pidl)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, prnf, pfi);
    return CPrintRoot_GetPIDLType(pidl) == HOOD_COL_PRINTER;
}

IPrinterFolderVtbl c_PrintersFIVtbl =
{
    CPrinters_FI_QueryInterface, CPrinters_FI_AddRef, CPrinters_FI_Release,
    CPrinters_FI_IsPrinter,
};

//
// IFolderNotify implementation
//
STDMETHODIMP CPrinters_FN_QueryInterface(IFolderNotify *fni, REFIID riid, void **ppvObj)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, nf, fni);
    return CPrinters_SF_QueryInterface(&this->sf, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CPrinters_FN_AddRef(IFolderNotify *fni)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, nf, fni);
    return CPrinters_SF_AddRef(&this->sf);
}

STDMETHODIMP_(ULONG) CPrinters_FN_Release(IFolderNotify *fni)
{
    CPrinterFolder *this = IToClass(CPrinterFolder, nf, fni);
    return CPrinters_SF_Release(&this->sf);
}

#define COUNTOF( arr ) (sizeof(arr) / sizeof(arr[0]))
STDMETHODIMP_(BOOL) CPrinters_FN_ProcessNotify(IFolderNotify *fni, FOLDER_NOTIFY_TYPE NotifyType, LPCWSTR pszName, LPCWSTR pszNewName)
{
    static DWORD aNotifyTypes[] = { kFolderUpdate,        SHCNE_UPDATEITEM,
                                    kFolderCreate,        SHCNE_CREATE,
                                    kFolderDelete,        SHCNE_DELETE,
                                    kFolderRename,        SHCNE_RENAMEITEM,
                                    kFolderAttributes,    SHCNE_ATTRIBUTES };

    CPrinterFolder *this = IToClass(CPrinterFolder, nf, fni);
    BOOL            bReturn;
    LPITEMIDLIST    pidl, pidlNew;
    UINT            uFlags;
    int             i;

    bReturn = FALSE;
    pidl = pidlNew = NULL;
    uFlags = SHCNF_IDLIST | SHCNF_FLUSH | SHCNF_FLUSHNOWAIT;

    if (kFolderUpdateAll == NotifyType)
    {
        //
        // Clear the this->bRefreshed flag, which will force invalidating the folder cache 
        // during the next print folder enumeration, and then request the defview to update 
        // the entire printers folder content (i.e. to re-enumerate the folder).
        //
        this->bRefreshed = FALSE;
        NotifyType = kFolderUpdate;
        pszName = NULL;
    }

    for (i=0; i<ARRAYSIZE(aNotifyTypes); i+=2)
    {
        if (aNotifyTypes[i] == (DWORD)NotifyType)
        {
            pidl = Printers_GetPidl((LPITEMIDLIST)this->pidl, pszName);

            if (pszNewName)
            {
                pidlNew = Printers_GetPidl((LPITEMIDLIST)this->pidl, pszNewName);
            }

            //
            // We can get a null pidl if the printer receives a refresh,
            // and before we call Printers_GetPidl the printer is gone.
            //
            if (pidl)
            {
                SHChangeNotify(aNotifyTypes[i+1], uFlags, pidl, pidlNew);
                ILFree( pidl );
            }

            if (pidlNew)
            {
                ILFree(pidlNew);
            }

            bReturn = TRUE;
            break;
        }
    }

    return bReturn;
}

IFolderNotifyVtbl c_PrintersNFVtbl =
{
    // IUnknown
    CPrinters_FN_QueryInterface, 
    CPrinters_FN_AddRef, 
    CPrinters_FN_Release,
    // IFolderNotify
    CPrinters_FN_ProcessNotify,
};

#endif

//
// The IClassFactory callback for CLSID_Printers
//
HRESULT CPrinters_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hres;
    CPrinterFolder *ppf;

    if ((ppf = (void*)LocalAlloc(LPTR, SIZEOF(*ppf))) != NULL)
    {
        ppf->sf.lpVtbl    = &c_PrintersSFVtbl;
        ppf->pf.lpVtbl    = &c_CPrintersPFVtbl;
        ppf->sio.lpVtbl   = &c_PrintersSIOVtbl;
#ifdef WINNT
        ppf->rc.lpVtbl    = &c_PrintersRCVtbl;
        ppf->prnf.lpVtbl  = &c_PrintersFIVtbl;
#endif
        ppf->cRef = 1;

#ifndef PRN_FOLDERDATA
        InitializeCriticalSection(&ppf->csPrinterInfo);
        ppf->hdpaPrinterInfo = DPA_Create(4);
#else
        ppf->nf.lpVtbl    = &c_PrintersNFVtbl;
        ppf->bRefreshed   = FALSE;
        ppf->bShowAddPrinter = FALSE;
#endif
        ppf->dwSpoolerVersion = -1;

        hres = CPrinters_SF_QueryInterface(&ppf->sf, riid, ppv);
        CPrinters_SF_Release(&ppf->sf);
    }
    else
    {
        *ppv = NULL;
        hres = E_OUTOFMEMORY;
    }
    return hres;
}


// IShellDetails stuff

typedef struct
{
    IShellDetails sd;
    LONG cRef;
    CPrinterFolder *ppsf;
} CPrintersSD;

STDMETHODIMP CPrinters_SD_QueryInterface(IShellDetails *psd, REFIID riid, void **ppvObj)
{
    CPrintersSD *this = IToClass(CPrintersSD, sd, psd);

    if (IsEqualIID(riid, &IID_IShellDetails)  ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = &this->sd;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    this->cRef++;
    return NOERROR;
}

STDMETHODIMP_(ULONG) CPrinters_SD_AddRef(IShellDetails *psd)
{
    CPrintersSD *this = IToClass(CPrintersSD, sd, psd);
    return InterlockedIncrement(&this->cRef);
}

STDMETHODIMP_(ULONG) CPrinters_SD_Release(IShellDetails *psd)
{
    CPrintersSD *this = IToClass(CPrintersSD, sd, psd);
    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    LocalFree((HLOCAL)this);
    return 0;
}

STDMETHODIMP CPrinters_SD_GetDetailsOf(IShellDetails *psd, LPCITEMIDLIST pidl,
        UINT iColumn, LPSHELLDETAILS lpDetails)
{
    CPrintersSD *this = IToClass(CPrintersSD, sd, psd);
    LPIDPRINTER pidp = (LPIDPRINTER)pidl;
    HRESULT hres = NOERROR;
    TCHAR szTemp[MAX_PATH];
    TCHAR szPrinter[MAXNAMELENBUFFER];

    // NT 5.0 supports a few new columns.
    if (g_bRunOnNT5 && iColumn >= PRINTERS_ICOL_MAX)
        return E_NOTIMPL;

    // NT 4.0 doesn't support the location and model columns.
    if (!g_bRunOnNT5 && iColumn > PRINTERS_ICOL_COMMENT)
        return E_NOTIMPL;

    // BUGBUG This case doesn't ever seem to get called, but is technically
    // required

    if (pidl && (HOOD_COL_FILE == CPrintRoot_GetPIDLType(pidl)))
    {
        IShellFolder2 *psf = CPrintRoot_GetPSF();
        if (iColumn >= 1)
            return E_NOTIMPL;

        return psf->lpVtbl->GetDisplayNameOf(psf, pidl, SHGDN_INFOLDER, &(lpDetails->str));
    }

    lpDetails->str.uType = STRRET_CSTR;
    lpDetails->str.cStr[0] = 0;

    if (!pidp)
    {
        lpDetails->fmt = s_printers_cols[iColumn].fmt;
        lpDetails->cxChar = s_printers_cols[iColumn].cxChar;
        return ResToStrRet(s_printers_cols[iColumn].uID, &lpDetails->str);
    }

    ualstrcpyn(szPrinter, pidp->cName, ARRAYSIZE(szPrinter));

    if (iColumn == PRINTERS_ICOL_NAME)
    {
#ifdef UNICODE
        LPCTSTR pszPrinterName = szPrinter;
        TCHAR szPrinterName[MAXNAMELENBUFFER];

        //
        // If we have a valid server name and the printer is not 
        // the add printer wizard object then return a fully qualified 
        // printer name in the remote printers folder.
        //
        if( this->ppsf->pszServer && lstrcmpi( c_szNewObject, szPrinter ) )
        {
            UINT len;
            //
            // Build the name which consists of the 
            // server name plus slash plus the printer name.
            //
            lstrcpyn( szPrinterName, this->ppsf->pszServer, ARRAYSIZE(szPrinterName) );
            len = lstrlen( szPrinterName );
            lstrcpyn( &szPrinterName[len++], TEXT("\\"), ARRAYSIZE(szPrinterName) - len );
            lstrcpyn( &szPrinterName[len], pszPrinterName, ARRAYSIZE(szPrinterName) - len );
            pszPrinterName = szPrinterName;
        }
        hres = StringToStrRet(pszPrinterName, &lpDetails->str);
#else
        hres = StringToStrRet(szPrinter, &lpDetails->str);
#endif
    }
    else if (lstrcmp(c_szNewObject, szPrinter))
    {
#ifdef PRN_FOLDERDATA
        PFOLDER_PRINTER_DATA pData = Printer_FolderGetPrinter(this->ppsf->hFolder, szPrinter);
#else
        LPPRINTER_INFO_2 pData = CPrinters_SF_GetPrinterInfo2(this->ppsf, szPrinter);
#endif // ndef PRN_FOLDERDATA
        if (pData)
        {
            switch (iColumn)
            {
            case PRINTERS_ICOL_QUEUESIZE:
                wsprintf(szTemp, TEXT("%ld"), pData->cJobs);
                hres = StringToStrRet(szTemp, &lpDetails->str);
                break;

            case PRINTERS_ICOL_STATUS:
            {
                DWORD dwStatus = pData->Status;

                // HACK: Use this free bit for "Work Offline"
                if (pData->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE)
                    dwStatus |= PRINTER_HACK_WORK_OFFLINE;

                szTemp[0] = 0;
                Printer_BitsToString(dwStatus, IDS_PRQSTATUS_SEPARATOR, ssPrinterStatus, szTemp, ARRAYSIZE(szTemp));

                hres = StringToStrRet(szTemp, &lpDetails->str);

#ifdef WINNT
                //
                // If the status word is null and we have a connection status string
                // display the status string.  This only works on NT because printui.dll
                // in will generate printer connection status i.e. <opening> | <access denied> etc.
                // 

#ifdef PRN_FOLDERDATA
                if( g_bRunOnNT5 && !dwStatus )
                {
                    LPCTSTR pStr = pData->pStatus;

                    //
                    // Discard the previous status StrRet if any.
                    //
                    StrRetToStrN(szTemp, ARRAYSIZE(szTemp), &lpDetails->str, NULL);

                    //
                    // If we do not have a connection status string and the status
                    // is 0 then the printer is ready, display ready rather than an empty string.
                    //
                    if (!pStr)
                    {
                        LoadString(HINST_THISDLL, IDS_PRN_INFOTIP_READY, szTemp, ARRAYSIZE(szTemp));
                        pStr = szTemp;
                    }
                    hres = StringToStrRet(pStr, &lpDetails->str);
                }
#endif // PRN_FOLDERDATA

#endif // WINNT
                break;
            }

            case PRINTERS_ICOL_COMMENT:
                if (pData->pComment)
                {
                    LPTSTR pStr;

                    // pComment can have newlines in it because it comes from
                    // a multi-line edit box. BUT we display it here in a
                    // single line edit box. Strip out the newlines
                    // to avoid the ugly characters.
                    lstrcpyn(szTemp, pData->pComment, ARRAYSIZE(szTemp));
                    pStr = szTemp;
                    while (*pStr)
                    {
                        if (*pStr == TEXT('\r') || *pStr == TEXT('\n'))
                            *pStr = TEXT(' ');
                        pStr = CharNext(pStr);
                    }
                    hres = StringToStrRet(szTemp, &lpDetails->str);
                }
                break;

#ifdef WINNT
            case PRINTERS_ICOL_LOCATION:
                if (pData->pLocation)
                    hres = StringToStrRet(pData->pLocation, &lpDetails->str);
                break;

            case PRINTERS_ICOL_MODEL:
                if (pData->pDriverName)
                    hres = StringToStrRet(pData->pDriverName, &lpDetails->str);
                break;
#endif
            }

#ifdef PRN_FOLDERDATA
            LocalFree((HLOCAL)pData);
#else
            CPrinters_SF_FreePrinterInfo2(this->ppsf);
#endif // ndef PRN_FOLDERDATA
        }
    }

    return hres;
}


STDMETHODIMP CPrinters_SD_ColumnClick(IShellDetails *psd, UINT iColumn)
{
    return S_FALSE; // do default
}

IShellDetailsVtbl c_PrintersSDVtbl =
{
    CPrinters_SD_QueryInterface, CPrinters_SD_AddRef, CPrinters_SD_Release,
    CPrinters_SD_GetDetailsOf,
    CPrinters_SD_ColumnClick,
};

HRESULT CPrinters_SD_Create(CPrinterFolder *ppsf, void **ppvOut)
{
    CPrintersSD *psd;

    *ppvOut = NULL;

    CPrinters_RegisterNotify(ppsf, TRUE);

    psd = (CPrintersSD *)LocalAlloc(LPTR, SIZEOF(CPrintersSD));
    if (!psd)
        return E_OUTOFMEMORY;

    psd->sd.lpVtbl = &c_PrintersSDVtbl;
    psd->cRef = 1;

    psd->ppsf = ppsf;

    *ppvOut = psd;
    return NOERROR;
}
