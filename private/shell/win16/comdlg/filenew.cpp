/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    filenew.cpp

Abstract:

    This module implements the Win32 explorer fileopen dialogs.

Revision History:

--*/



//
//  Include Files.
//

#define _SHELL32_
#define _COMCTL32_
#define _INC_OLE

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlapip.h>
#include <shell2.h>
#include <shellp.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <ole2.h>
#include <shlobj.h>
#include "privcomd.h"
#include "cdids.h"
#include "isz.h"
#include "fileopen.h"

#define INITGUID
#include <initguid.h>
#include <coguid.h>
#include <shlguid.h>
#include <shguidp.h>
#include <oleguid.h>

#include <help.h>




//
//  Constant Declarations.
//

#define IDOI_SHARE           1

#define IDC_TOOLBAR          1              // toolbar control ID

#define CDM_SETSAVEBUTTON    (CDM_LAST + 100)
#define CDM_FSNOTIFY         (CDM_LAST + 101)
#define CDM_SELCHANGE        (CDM_LAST + 102)

#define TIMER_FSCHANGE       100

#define NODE_DESKTOP         0
#define NODE_DRIVES          1

#define DEREFMACRO(x)        x

//
//  IShellView::MenuHelp flags.
//
#define MH_DONE              0x0001
//      MH_LONGHELP
#define MH_MERGEITEM         0x0004
#define MH_SYSITEM           0x0008
#define MH_POPUP             0x0010
#define MH_TOOLBAR           0x0020
#define MH_TOOLTIP           0x0040

//
//  IShellView::MenuHelp return values.
//
#define MH_NOTHANDLED        0
#define MH_STRINGFILLED      1
#define MH_ALLHANDLED        2

#define MYCBN_DRAW           0x8000
#define OFN_FILTERDOWN       0x10000000
#define MIN_DEFEXT_LEN       4

#define MAX_DRIVELIST_STRING_LEN  (64 + 4)




//
//  Macro Definitions.
//

#define IsServer(psz)        (IsUNC(psz) && !mystrchr((psz) + 2, CHAR_BSLASH))

#define LPIDL_GetIDList(_pida,n) \
    (LPCITEMIDLIST)(((LPBYTE)(_pida)) + (_pida)->aoffset[n])

#define RectWid(_rc)         ((_rc).right - (_rc).left)
#define RectHgt(_rc)         ((_rc).bottom - (_rc).top)

#define IsVisible(_hwnd)     (GetWindowLong(_hwnd, GWL_STYLE)&WS_VISIBLE)

#define HwndToBrowser(_hwnd) (CFileOpenBrowser *)GetWindowLong(_hwnd, DWL_USER)
#define StoreBrowser(_hwnd, _pbrs) \
    SetWindowLong(_hwnd, DWL_USER, (DWORD)_pbrs);

#ifdef DBCS
  #define ISBACKSLASH(szPath, nOffset) (IsBackSlash(szPath, szPath + nOffset))
#else
  #define ISBACKSLASH(szPath, nOffset) (szPath[nOffset] == CHAR_BSLASH)
#endif

#define IsInRange(id, idFirst, idLast) \
    ((UINT)((id) - idFirst) <= (UINT)(idLast - idFirst))




//
//  Typedef Declarations.
//

typedef LPVOID *LPLPVOID;

typedef struct _OFNINITINFO
{
    LPOPENFILEINFO  lpOFI;
    BOOL            bSave;
} OFNINITINFO, *LPOFNINITINFO;




//
//  Global Variables.
//

WNDPROC lpOKProc = NULL;

HWND gp_hwndActiveOpen = NULL;
HACCEL gp_haccOpen = NULL;
HACCEL gp_haccOpenView = NULL;
HHOOK gp_hHook = NULL;
int gp_nHookRef = -1;

static int g_cxSmIcon;
static int g_cySmIcon;

const LPCSTR c_szCommandsA[] =
{
    CMDSTR_NEWFOLDERA,
    CMDSTR_VIEWLISTA,
    CMDSTR_VIEWDETAILSA,
};

const LPCWSTR c_szCommandsW[] =
{
    CMDSTR_NEWFOLDERW,
    CMDSTR_VIEWLISTW,
    CMDSTR_VIEWDETAILSW,
};




//
//  Function Prototypes.
//

LRESULT CALLBACK
OKSubclass(
    HWND hOK,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

BOOL
InitImports(void);

void
FreeImports(void);

int
GetControlsBottom(
    HWND hDlg,
    HWND hwndExclude);

BOOL CALLBACK
OpenDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

HRESULT
ICoCreateInstance(
    REFCLSID rclsid,
    REFIID riid,
    LPLPVOID ppv);

void
CleanupDialog(
    HWND hDlg,
    BOOL fRet);




//
//  Context Help IDs.
//

DWORD aFileOpenHelpIDs[] =
{
    stc2,    IDH_OPEN_FILETYPE,   // The positions of these array elements
    cmb1,    IDH_OPEN_FILETYPE,   // shouldn't be changed without updating
    stc4,    IDH_OPEN_LOCATION,   // InitSaveAsControls().
    cmb2,    IDH_OPEN_LOCATION,
    stc1,    IDH_OPEN_FILES32,
    lst2,    IDH_OPEN_FILES32,    // defview
    stc3,    IDH_OPEN_FILENAME,
    edt1,    IDH_OPEN_FILENAME,
    chx1,    IDH_OPEN_READONLY,
    IDOK,    IDH_OPEN_BUTTON,

    0, 0
};

DWORD aFileSaveHelpIDs[] =
{
    stc2,    IDH_SAVE_FILETYPE,   // The positions of these array elements
    cmb1,    IDH_SAVE_FILETYPE,   // shouldn't be changed without updating
    stc4,    IDH_OPEN_LOCATION,   // InitSaveAsControls().
    cmb2,    IDH_OPEN_LOCATION,
    stc1,    IDH_OPEN_FILES32,
    lst2,    IDH_OPEN_FILES32,    // defview
    stc3,    IDH_OPEN_FILENAME,
    edt1,    IDH_OPEN_FILENAME,
    chx1,    IDH_OPEN_READONLY,
    IDOK,    IDH_SAVE_BUTTON,

    0, 0
};





////////////////////////////////////////////////////////////////////////////
//
//  CD_SendShareMsg
//
////////////////////////////////////////////////////////////////////////////

WORD CD_SendShareMsg(
    HWND hwnd,
    LPTSTR szFile,
    UINT ApiType)
{
#ifdef UNICODE
    if (ApiType == COMDLG_ANSI)
    {
        CHAR szFileA[MAX_PATH + 1];

        WideCharToMultiByte( CP_ACP,
                             0,
                             szFile,
                             -1,
                             szFileA,
                             MAX_PATH + 1,
                             NULL,
                             NULL );

        return ( (WORD)SendMessage( hwnd,
                                    msgSHAREVIOLATIONA,
                                    0,
                                    (LONG)(LPSTR)(szFileA) ) );
    }
    else
#endif
    {
        return ( (WORD)SendMessage( hwnd,
                                    msgSHAREVIOLATIONW,
                                    0,
                                    (LONG)(LPTSTR)(szFile) ) );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CD_SendHelpMsg
//
////////////////////////////////////////////////////////////////////////////

VOID CD_SendHelpMsg(
    LPOPENFILENAME pOFN,
    HWND hwndDlg,
    UINT ApiType)
{
#ifdef UNICODE
    if (ApiType == COMDLG_ANSI)
    {
        if (msgHELPA && pOFN->hwndOwner)
        {
            SendMessage( pOFN->hwndOwner,
                         msgHELPA,
                         (WPARAM)hwndDlg,
                         (LPARAM)pOFN );
        }
    }
    else
#endif
    {
        if (msgHELPW && pOFN->hwndOwner)
        {
            SendMessage( pOFN->hwndOwner,
                         msgHELPW,
                         (WPARAM)hwndDlg,
                         (LPARAM)pOFN );
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CD_SendOKMsg
//
////////////////////////////////////////////////////////////////////////////

LRESULT CD_SendOKMsg(
    HWND hwnd,
    LPOPENFILENAME pOFN,
    LPOPENFILEINFO pOFI)
{
    LRESULT Result;

#ifdef UNICODE
    if (pOFI->ApiType == COMDLG_ANSI)
    {
        ThunkOpenFileNameW2A(pOFI);
        Result = SendMessage(hwnd, msgFILEOKA, 0, (LPARAM)(pOFI->pOFNA));

        //
        //  For apps that side-effect pOFNA stuff and expect it to
        //  be preserved through dialog exit, update internal
        //  struct after the hook proc is called.
        //
        ThunkOpenFileNameA2W(pOFI);
    }
    else
#endif
    {
        Result = SendMessage(hwnd, msgFILEOKW, 0, (LPARAM)(pOFN));
    }

    return (Result);
}


////////////////////////////////////////////////////////////////////////////
//
//  CD_SendLBChangeMsg
//
////////////////////////////////////////////////////////////////////////////

LRESULT CD_SendLBChangeMsg(
    HWND hwnd,
    int Id,
    short Index,
    short Code,
    UINT ApiType)
{
#ifdef UNICODE
    if (ApiType == COMDLG_ANSI)
    {
        return (SendMessage(hwnd, msgLBCHANGEA, Id, MAKELONG(Index, Code)));
    }
    else
#endif
    {
        return (SendMessage(hwnd, msgLBCHANGEW, Id, MAKELONG(Index, Code)));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Macro calls to SendOFNotify
//
////////////////////////////////////////////////////////////////////////////

#define CD_SendShareNotify(_hwndTo, _hwndFrom, _szFile, _pofn, _pofi) \
    (WORD)SendOFNotify(_hwndTo, _hwndFrom, CDN_SHAREVIOLATION, _szFile, _pofn, _pofi)

#define CD_SendHelpNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_HELP, NULL, _pofn, _pofi)

#define CD_SendOKNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_FILEOK, NULL, _pofn, _pofi)

#define CD_SendTypeChangeNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_TYPECHANGE, NULL, _pofn, _pofi)

#define CD_SendInitDoneNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_INITDONE, NULL, _pofn, _pofi)

#define CD_SendSelChangeNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_SELCHANGE, NULL, _pofn, _pofi)

#define CD_SendFolderChangeNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_FOLDERCHANGE, NULL, _pofn, _pofi)


////////////////////////////////////////////////////////////////////////////
//
//  SendOFNotify
//
////////////////////////////////////////////////////////////////////////////

LRESULT SendOFNotify(
    HWND hwndTo,
    HWND hwndFrom,
    UINT code,
    LPTSTR szFile,
    LPOPENFILENAME pOFN,
    LPOPENFILEINFO pOFI)
{
    OFNOTIFY ofn;

#ifdef UNICODE
    if (pOFI->ApiType == COMDLG_ANSI)
    {
        OFNOTIFYA ofnA;
        LRESULT Result;

        //
        //  Convert the file name from Unicode to Ansi.
        //
        if (szFile)
        {
            CHAR szFileA[MAX_PATH + 1];

            WideCharToMultiByte( CP_ACP,
                                 0,
                                 szFile,
                                 -1,
                                 szFileA,
                                 MAX_PATH + 1,
                                 NULL,
                                 NULL );

            ofnA.pszFile = szFileA;
        }
        else
        {
            ofnA.pszFile = NULL;
        }

        //
        //  Convert the OFN from Unicode to Ansi.
        //
        ThunkOpenFileNameW2A(pOFI);

        ofnA.lpOFN = pOFI->pOFNA;

        Result = SendNotify(hwndTo, hwndFrom, code, &ofnA.hdr);

        //
        //  For apps that side-effect pOFNA stuff and expect it to
        //  be preserved through dialog exit, update internal
        //  struct after the hook proc is called.
        //
        ThunkOpenFileNameA2W(pOFI);

        return (Result);
    }
    else
#endif
    {
        ofn.pszFile = szFile;
        ofn.lpOFN   = pOFN;

        return (SendNotify(hwndTo, hwndFrom, code, &ofn.hdr));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  WAIT_CURSOR class
//
////////////////////////////////////////////////////////////////////////////

class WAIT_CURSOR
{
private:
    HCURSOR _hcurOld;

public:
    WAIT_CURSOR()
    {
        _hcurOld = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
    }

    ~WAIT_CURSOR()
    {
        ::SetCursor(_hcurOld);
    }
};


////////////////////////////////////////////////////////////////////////////
//
//  TEMPMEM class
//
////////////////////////////////////////////////////////////////////////////

class TEMPMEM
{
public:
    TEMPMEM(UINT cb)
    {
        m_uSize = cb;
        m_pMem = cb ? LocalAlloc(LPTR, cb) : NULL;
    }

    ~TEMPMEM()
    {
        if (m_pMem)
        {
            LocalFree(m_pMem);
        }
    }

    operator LPBYTE() const
    {
        return ((LPBYTE)m_pMem);
    }

    BOOL Resize(UINT cb);

private:
    LPVOID m_pMem;

protected:
    UINT m_uSize;
};


////////////////////////////////////////////////////////////////////////////
//
//  TEMPMEM::Resize
//
////////////////////////////////////////////////////////////////////////////

BOOL TEMPMEM::Resize(
    UINT cb)
{
    UINT uOldSize = m_uSize;

    m_uSize = cb;

    if (!cb)
    {
        if (m_pMem)
        {
            LocalFree(m_pMem);
            m_pMem = NULL;
        }

        return (TRUE);
    }

    if (!m_pMem)
    {
        m_pMem = LocalAlloc(LPTR, cb);
        return (m_pMem != NULL);
    }

    LPVOID pTemp = LocalReAlloc(m_pMem, cb, LHND);

    if (pTemp)
    {
        m_pMem = pTemp;
        return (TRUE);
    }

    m_uSize = uOldSize;
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  TEMPSTR class
//
////////////////////////////////////////////////////////////////////////////

class TEMPSTR : public TEMPMEM
{
public:
    TEMPSTR(UINT cc = 0) : TEMPMEM(cc * sizeof(TCHAR))
    {
    }

    operator LPTSTR() const
    {
        return ((LPTSTR)(LPBYTE) * (TEMPMEM *)this);
    }

    BOOL StrCpy(LPCTSTR pszText);
    BOOL StrCat(LPCTSTR pszText);
    BOOL StrSize(UINT cb)
    {
        return (TEMPMEM::Resize(cb * sizeof(TCHAR)));
    }
};


////////////////////////////////////////////////////////////////////////////
//
//  TEMPSTR::StrCpy
//
////////////////////////////////////////////////////////////////////////////

BOOL TEMPSTR::StrCpy(
    LPCTSTR pszText)
{
    if (!pszText)
    {
        StrSize(0);
        return (TRUE);
    }

    UINT uNewSize = lstrlen(pszText) + 1;

    if (!StrSize(uNewSize))
    {
        return (FALSE);
    }

    lstrcpy(*this, pszText);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  TEMPSTR::StrCat
//
////////////////////////////////////////////////////////////////////////////

BOOL TEMPSTR::StrCat(
    LPCTSTR pszText)
{
    if (!(LPTSTR)*this)
    {
        //
        //  This should 0 init.
        //
        if (!StrSize(MAX_PATH))
        {
            return (FALSE);
        }
    }

    UINT uNewSize = lstrlen(*this) + lstrlen(pszText) + 1;

    if (m_uSize < uNewSize * sizeof(TCHAR))
    {
        //
        //  Add on some more so we do not ReAlloc too often.
        //
        uNewSize += MAX_PATH;

        if (!StrSize(uNewSize))
        {
            return (FALSE);
        }
    }

    lstrcat(*this, pszText);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CDMessageBox
//
////////////////////////////////////////////////////////////////////////////

int _cdecl CDMessageBox(
    HWND hwndParent,
    UINT idText,
    UINT uFlags,
    ...)
{
    TCHAR szText[MAX_PATH + WARNINGMSGLENGTH];
    TCHAR szTitle[WARNINGMSGLENGTH];
    va_list ArgList;

    LoadString(g_hinst, idText, szTitle, ARRAYSIZE(szTitle));
    va_start(ArgList, uFlags);
    wvsprintf(szText, szTitle, ArgList);
    va_end(ArgList);

    GetWindowText(hwndParent, szTitle, ARRAYSIZE(szTitle));

    return (MessageBox(hwndParent, szText, szTitle, uFlags));
}


////////////////////////////////////////////////////////////////////////////
//
//  InvalidFileWarning
//
////////////////////////////////////////////////////////////////////////////

VOID InvalidFileWarning(
    HWND hWnd,
    LPTSTR szFile,
    int wErrCode)
{
    LPTSTR lpszContent = szFile;
    int isz;
    BOOL bDriveLetter = FALSE;

    if (lstrlen(szFile) > MAX_PATH)
    {
#ifdef DBCS
        EliminateString(szFile, MAX_PATH);
#else
        *(szFile + MAX_PATH) = CHAR_NULL;
#endif
    }

    switch (wErrCode)
    {
        case ( OF_ACCESSDENIED ) :
        case ( ERROR_NOT_READY ) :
        {
            isz = iszNoDiskInDrive;
            bDriveLetter = TRUE;
            break;
        }
        case ( OF_NODRIVE ) :
        {
            isz = iszDriveDoesNotExist;
            bDriveLetter = TRUE;
            break;
        }
        case ( OF_NOFILEHANDLES ) :
        {
            isz = iszNoFileHandles;
            break;
        }
        case ( OF_PATHNOTFOUND ) :
        {
            isz = iszPathNotFound;
            break;
        }
        case ( OF_FILENOTFOUND ) :
        {
            isz = iszFileNotFound;
            break;
        }
        case ( OF_DISKFULL ) :
        {
            isz = iszDiskFull;
            bDriveLetter = TRUE;
            break;
        }
        case ( OF_WRITEPROTECTION ) :
        {
            isz = iszWriteProtection;
            bDriveLetter = TRUE;
            break;
        }
        case ( OF_SHARINGVIOLATION ) :
        {
            isz = iszSharingViolation;
            break;
        }
        case ( OF_CREATENOMODIFY ) :
        {
            isz = iszCreateNoModify;
            break;
        }
        case ( OF_NETACCESSDENIED ) :
        {
            isz = iszNetworkAccessDenied;
            break;
        }
        case ( OF_PORTNAME ) :
        {
            isz = iszPortName;
            break;
        }
        case ( OF_LAZYREADONLY ) :
        {
            isz = iszReadOnly;
            break;
        }
        case ( OF_INT24FAILURE ) :
        {
            isz = iszInt24Error;
            break;
        }
        default :
        {
            isz = iszInvalidFileName;
            break;
        }
    }

//  StringLower(szFile);

    if (bDriveLetter)
    {
        CDMessageBox(hWnd, isz, MB_OK | MB_ICONEXCLAMATION, (TCHAR)*szFile);
    }
    else
    {
        CDMessageBox(hWnd, isz, MB_OK | MB_ICONEXCLAMATION, (LPTSTR)szFile);
    }

    if (isz == iszInvalidFileName)
    {
        PostMessage(hWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hWnd, edt1), 1);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetControlRect
//
////////////////////////////////////////////////////////////////////////////

void GetControlRect(
    HWND hwndDlg,
    UINT idOldCtrl,
    LPRECT lprc)
{
    HWND hwndOldCtrl = GetDlgItem(hwndDlg, idOldCtrl);

    GetWindowRect(hwndOldCtrl, lprc);
    MapWindowRect(HWND_DESKTOP, hwndDlg, lprc);
}


////////////////////////////////////////////////////////////////////////////
//
//  HideControl
//
//  Subroutine to hide a dialog control.
//
//  WARNING WARNING WARNING:  Some code in the new look depends on hidden
//  controls remaining where they originally were, even when disabled,
//  because they're templates for where to create new controls (the toolbar,
//  or the main list).  Therefore, HideControl() must not MOVE the control
//  being hidden - it may only hide and disable it.  If this needs to change,
//  there must be a separate hiding subroutine used for template controls.
//
////////////////////////////////////////////////////////////////////////////

void HideControl(
    HWND hwndDlg,
    UINT idControl)
{
    HWND hCtrl = ::GetDlgItem(hwndDlg, idControl);

    ::ShowWindow(hCtrl, SW_HIDE);
    ::EnableWindow(hCtrl, FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SelectEditText
//
////////////////////////////////////////////////////////////////////////////

void SelectEditText(
    HWND hwndDlg)
{
    Edit_SetSel(GetDlgItem(hwndDlg, edt1), 0, -1);
}


////////////////////////////////////////////////////////////////////////////
//
//  MYLISTBOXITEM class
//
//  One object of this class exists for each item in the location dropdown.
//
//  Data members:
//    psfSub   - instance of IShellFolder bound to this container
//    pidlThis - IDL of this container, relative to its parent
//    pidlFull - IDL of this container, relative to the desktop
//    cIndent  - indent level (0-based)
//    dwFlags  -
//        MLBI_PERMANENT - item is an "information source" and should
//                         always remain
//    dwAttrs  - attributes of this container as reported by GetAttributesOf()
//    iImage, iSelectedImage - indices into the system image list for this
//                             object
//
//  Member functions:
//    ShouldInclude() - returns whether item belongs in the location dropdown
//    IsShared() - returns whether an item is shared or not
//    SwitchCurrentDirectory() - changes the Win32 current directory to the
//                               directory indicated by this item
//
////////////////////////////////////////////////////////////////////////////

class MYLISTBOXITEM
{
public:
    IShellFolder *psfSub;
    IShellFolder *psfParent;
    LPITEMIDLIST pidlThis;
    LPITEMIDLIST pidlFull;
    DWORD cIndent;
    DWORD dwFlags;
    DWORD dwAttrs;
    int iImage;
    int iSelectedImage;

    MYLISTBOXITEM( MYLISTBOXITEM *pParentItem,
                   IShellFolder *psf,
                   LPCITEMIDLIST pidl,
                   DWORD c,
                   DWORD f );

    ~MYLISTBOXITEM();

    inline BOOL ShouldInclude()
    {
        return (dwAttrs & (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM));
    }

    inline BOOL IsShared()
    {
        return (dwAttrs & SFGAO_SHARE);
    }

    void SwitchCurrentDirectory();

    IShellFolder* GetShellFolder();
};


////////////////////////////////////////////////////////////////////////////
//
//  MYLISTBOXITEM::MYLISTBOXITEM
//
////////////////////////////////////////////////////////////////////////////

#define MLBI_PERMANENT        0x0001
#define MLBI_PSFFROMPARENT    0x0002

MYLISTBOXITEM::MYLISTBOXITEM(
    MYLISTBOXITEM *pParentItem,
    IShellFolder *psf,
    LPCITEMIDLIST pidl,
    DWORD c,
    DWORD f)
{
    cIndent = c;
    dwFlags = f;

    pidlThis = ILClone(pidl);
    if (pParentItem == NULL)
    {
        pidlFull = ILClone(pidl);
    }
    else
    {
        pidlFull = ILCombine(pParentItem->pidlFull, pidl);
    }

    if (pidlThis == NULL || pidlFull == NULL)
    {
        psfSub = NULL;
    }

    if (dwFlags & MLBI_PSFFROMPARENT)
    {
        psfParent = psf;
    }
    else
    {
        psfSub = psf;
    }
    psf->AddRef();


    dwAttrs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_SHARE;

    psf->GetAttributesOf(1, &pidl, &dwAttrs);

    iImage = SHMapPIDLToSystemImageListIndex(psf, pidl, &iSelectedImage);
}


////////////////////////////////////////////////////////////////////////////
//
//  MYLISTBOXITEM::~MYLISTBOXITEM
//
////////////////////////////////////////////////////////////////////////////

MYLISTBOXITEM::~MYLISTBOXITEM()
{
    if (psfSub != NULL)
    {
        psfSub->Release();
    }

    if (psfParent != NULL)
    {
        psfParent->Release();
    }

    if (pidlThis != NULL)
    {
        SHFree(pidlThis);
    }

    if (pidlFull != NULL)
    {
        SHFree(pidlFull);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ShouldIncludeObject
//
////////////////////////////////////////////////////////////////////////////

BOOL ShouldIncludeObject(
    LPSHELLFOLDER psfParent,
    LPCITEMIDLIST pidl)
{
    BOOL fInclude = FALSE;
    DWORD dwAttrs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;

    if (SUCCEEDED(psfParent->GetAttributesOf(1, &pidl, &dwAttrs)))
    {
        if (dwAttrs & (SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR))
        {
            fInclude = TRUE;
        }
    }
    return (fInclude);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsContainer
//
////////////////////////////////////////////////////////////////////////////

BOOL IsContainer(
    IShellFolder *psf,
    LPCITEMIDLIST pidl)
{
    DWORD dwAttrs = SFGAO_FOLDER;

    return (SUCCEEDED(psf->GetAttributesOf(1, &pidl, &dwAttrs)) &&
            (dwAttrs & SFGAO_FOLDER));
}


////////////////////////////////////////////////////////////////////////////
//
//  IsLink
//
////////////////////////////////////////////////////////////////////////////

BOOL IsLink(
    IShellFolder *psf,
    LPCITEMIDLIST pidl)
{
    DWORD dwAttrs = SFGAO_LINK;

    return (SUCCEEDED(psf->GetAttributesOf(1, &pidl, &dwAttrs)) &&
            (dwAttrs & SFGAO_LINK));
}


////////////////////////////////////////////////////////////////////////////
//
//  MYLISTBOXITEM::GetShellFolder
//
////////////////////////////////////////////////////////////////////////////

IShellFolder *MYLISTBOXITEM::GetShellFolder()
{
    if (!psfSub)
    {
        if (FAILED(psfParent->BindToObject( pidlThis,
                                            NULL,
                                            IID_IShellFolder,
                                            (LPLPVOID)&psfSub )))
        {
            psfSub = NULL;
        }
        else
        {
            psfParent->Release();
            psfParent = NULL;
        }
    }
    return (psfSub);
}


////////////////////////////////////////////////////////////////////////////
//
//  MYLISTBOXITEM::SwitchCurrentDirectory
//
////////////////////////////////////////////////////////////////////////////

void MYLISTBOXITEM::SwitchCurrentDirectory(void)
{
    TCHAR szDir[MAX_PATH + 1];

    if (!pidlFull)
    {
        SHGetSpecialFolderPath(NULL, szDir, CSIDL_DESKTOPDIRECTORY, FALSE);
    }
    else
    {
        SHGetPathFromIDList(pidlFull, szDir);
    }
    if (szDir[0])
    {
        SetCurrentDirectory(szDir);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser class
//
////////////////////////////////////////////////////////////////////////////

typedef BOOL (*EIOCALLBACK)(class CFileOpenBrowser*that, LPCITEMIDLIST pidl, LPARAM lParam);

typedef enum
{
    ECODE_S_OK     = 0,
    ECODE_BADDRIVE = 1,
    ECODE_BADPATH  = 2,
} ECODE;

typedef enum
{
    OKBUTTON_NONE     = 0x0000,
    OKBUTTON_NODEFEXT = 0x0001,
    OKBUTTON_QUOTED   = 0x0002,
} OKBUTTON_FLAGS;
typedef UINT OKBUTTONFLAGS;

class CFileOpenBrowser : public IShellBrowser, public ICommDlgBrowser
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd);
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode);

    // *** IShellBrowser methods *** (same as IOleInPlaceFrame)
    STDMETHOD(InsertMenusSB) (THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHOD(SetMenuSB) (THIS_ HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHOD(RemoveMenusSB) (THIS_ HMENU hmenuShared);
    STDMETHOD(SetStatusTextSB) (THIS_ LPCOLESTR lpszStatusText);
    STDMETHOD(EnableModelessSB) (THIS_ BOOL fEnable);
    STDMETHOD(TranslateAcceleratorSB) (THIS_ LPMSG lpmsg, WORD wID);

    // *** IShellBrowser methods ***
    STDMETHOD(BrowseObject)(THIS_ LPCITEMIDLIST pidl, UINT wFlags);
    STDMETHOD(GetViewStateStream)(THIS_ DWORD grfMode, LPSTREAM *pStrm);
    STDMETHOD(GetControlWindow)(THIS_ UINT id, HWND * lphwnd);
    STDMETHOD(SendControlMsg)(THIS_ UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT * pret);
    STDMETHOD(QueryActiveShellView)(THIS_ struct IShellView ** ppshv);
    STDMETHOD(OnViewWindowActive)(THIS_ struct IShellView * pshv);
    STDMETHOD(SetToolbarItems)(THIS_ LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags);

    // *** ICommDlgBrowser methods ***
    STDMETHOD(OnDefaultCommand) (THIS_ struct IShellView * ppshv);
    STDMETHOD(OnStateChange) (THIS_ struct IShellView * ppshv, ULONG uChange);
    STDMETHOD(IncludeObject) (THIS_ struct IShellView * ppshv, LPCITEMIDLIST lpItem);

    // *** Our own methods ***
    CFileOpenBrowser(HWND hDlg, BOOL fIsSaveAs);
    ~CFileOpenBrowser();
    HRESULT SwitchView(struct IShellFolder * psfNew, LPCITEMIDLIST pidlNew, FOLDERSETTINGS *pfs);
    void OnDblClick(BOOL bFromOKButton);
    LRESULT OnNotify(LPNMHDR lpnmhdr);
    void ViewCommand(UINT uIndex);
    void PaintDriveLine(DRAWITEMSTRUCT *lpdis);
    void GetFullPath(LPTSTR pszBuf);
    BOOL OnSelChange(int iItem = -1, BOOL bForceUpdate = FALSE);
    void OnDotDot();
    void RefreshFilter(HWND hwndFilter);
    BOOL JumpToPath(LPCTSTR pszDirectory, BOOL bTranslate = FALSE);
    BOOL JumpToIDList(LPCITEMIDLIST pidlNew, BOOL bTranslate = FALSE);
    BOOL SetDirRetry(LPTSTR pszDir, BOOL bNoValidate = FALSE);
    BOOL MultiSelectOKButton(LPCTSTR pszFiles, OKBUTTONFLAGS Flags);
    BOOL OKButtonPressed(LPCTSTR pszFile, OKBUTTONFLAGS Flags);
    UINT GetDirectoryFromLB(LPTSTR szBuffer, int *pichRoot);
    void SetCurrentFilter(LPCTSTR pszFilter, OKBUTTONFLAGS Flags = OKBUTTON_QUOTED);
    UINT GetFullEditName(LPTSTR pszBuf, UINT cLen, TEMPSTR *pTempStr = NULL, BOOL *pbNoDefExt = NULL);
    void ProcessEdit();
    LRESULT OnCommandMessage(WPARAM wParam, LPARAM lParam);
    BOOL OnCDMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RemoveOldPath(int *piNewSel);
    BOOL LinkMatchSpec(LPCITEMIDLIST pidl, LPCTSTR szFile, LPCTSTR szSpec);
    HRESULT InitShellLink();
    HRESULT ResolveLink(LPCTSTR pszLink, LPTSTR pszFile, UINT cchFile, WIN32_FIND_DATA *pfd);
    void SelFocusChange(BOOL bSelChange);
    void SelRename(void);
    void SetSaveButton(UINT idSaveButton);
    void RealSetSaveButton(UINT idSaveButton);
    void SetEditFile(LPTSTR pszFile, BOOL bShowExt, BOOL bSaveNullExt = TRUE);
    BOOL EnumItemObjects(UINT uItem, EIOCALLBACK pfnCallBack, LPARAM lParam);
    BOOL IsKnownExtension(LPCTSTR pszExtension);
    UINT FindNameInView(LPTSTR pszFile, OKBUTTONFLAGS Flags, LPTSTR pszPathName,
                        int nFileOffset, int nExtOffset, int *pnErrCode,
                        BOOL bTryAsDir = TRUE);
    void UpdateLevel(HWND hwndLB, int iInsert, MYLISTBOXITEM *pParentItem);
    void InitializeDropDown(HWND hwndCtl);
    BOOL FSChange(LONG lNotification, LPCITEMIDLIST *ppidl);
    int GetNodeFromIDList(LPCITEMIDLIST pidl);
    void Timer(WPARAM wID);
    BOOL CreateHookDialog(int nCtlsBottom);

    UINT cRef;                         // compobj refcount
    int iCurrentLocation;              // index of curr selection in location dropdown
    MYLISTBOXITEM *pCurrentLocation;   // ptr to object for same
    HWND hwndDlg;                      // handle of this dialog
    HWND hSubDlg;                      // handle of the hook dialog
    IShellView *psv;                   // current view object
    IShellFolder *psfCurrent;          // current shellfolder object
    IShellLink *psl;                   // Cached for use
    IPersistFile *ppf;                 // Cached for use
    HWND hwndView;                     // current view window
    HWND hwndToolbar;                  // toolbar window
    HWND hwndLastFocus;                // ctrl that had focus before OK button
    HIMAGELIST himl;                   // system imagelist (small images)
    TCHAR szLastFilter[MAX_PATH + 1];  // last filter chosen by the user
    TCHAR szStartDir[MAX_PATH + 1];    // saved starting directory
    TCHAR szCurDir[MAX_PATH + 1];      // Currently viewed dir (if FS)
    TCHAR szBuf[MAX_PATH + 4];         // scratch buffer
    TEMPSTR pszHideExt;                // saved file with extension
    TEMPSTR tszDefSave;                // saved file with extension
    TEMPSTR pszDefExt;                 // Writable version of the DefExt
    int iWaitCount;
    UINT uRegister;
    int iComboIndex;

    LPOPENFILENAME lpOFN;              // caller's OPENFILENAME struct

    BOOL bSave : 1;                    // whether this is a save-as dialog
    BOOL fShowExtensions : 1;          // whether to show extensions
    BOOL bUseHideExt : 1;              // whether pszHideExt is valid
    BOOL bDropped : 1;
    BOOL bNoInferDefExt : 1;           // don't get defext from combo
    BOOL fSelChangedPending : 1;       // we have a selchanging message pending

    LPOPENFILEINFO lpOFI;              // info for thunking (ansi callers only)
};

const TBBUTTON atbButtons[] =
{
    { 0, 0, 0, TBSTYLE_SEP, { 0, 0 }, 0, 0 },
    { VIEW_PARENTFOLDER, IDC_PARENT, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0, 0 }, 0, -1 },
    { 0, 0, 0, TBSTYLE_SEP, { 0, 0 }, 0, 0 },
    { VIEW_NEWFOLDER, IDC_NEWFOLDER, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0, 0 }, 0, -1 },
    { 0, 0, 0, TBSTYLE_SEP, { 0, 0 }, 0, 0 },
    { VIEW_LIST, IDC_VIEWLIST, TBSTATE_ENABLED | TBSTATE_CHECKED, TBSTYLE_CHECKGROUP, { 0, 0 }, 0, -1 },
    { VIEW_DETAILS, IDC_VIEWDETAILS, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, { 0, 0 }, 0, -1 }
};


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::CFileOpenBrowser
//
//  CFileOpenBrowser constructor.
//  Minimal construction of the object.  Much more construction in
//  InitLocation.
//
////////////////////////////////////////////////////////////////////////////

CFileOpenBrowser::CFileOpenBrowser(
    HWND hDlg,
    BOOL fIsSaveAs)
    : cRef(1),
      iCurrentLocation(-1),
      pCurrentLocation(NULL),
      psv(NULL),
      hwndDlg(hDlg),
      hwndView(NULL),
      psfCurrent(NULL),
      bSave(fIsSaveAs),
      iComboIndex(-1),
      psl(0)
{
    RECT rcToolbar;

    szLastFilter[0] = CHAR_NULL;

    GetControlRect(hwndDlg, stc1, &rcToolbar);

    hwndToolbar = CreateToolbarEx( hwndDlg,
                                   TBSTYLE_TOOLTIPS | WS_CHILD | CCS_NORESIZE |
                                   CCS_NODIVIDER, // | WS_CLIPSIBLINGS,
                                   IDC_TOOLBAR,
                                   12,
                                   HINST_COMMCTRL,
                                   IDB_VIEW_SMALL_COLOR,
                                   atbButtons,
                                   ARRAYSIZE(atbButtons),
                                   0,
                                   0,
                                   0,
                                   0,
                                   sizeof(TBBUTTON) );
    if (hwndToolbar)
    {
        ::SetWindowPos( hwndToolbar,
                        NULL,
                        rcToolbar.left,
                        rcToolbar.top,
                        rcToolbar.right - rcToolbar.left,
                        rcToolbar.bottom - rcToolbar.top,
                        SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW );
    }

    Shell_GetImageLists(NULL, &himl);

    //
    //  This setting could change on the fly, but I really don't care
    //  about that rare case.
    //
    SHELLSTATE ss;

    SHGetSetSettings(&ss, SSF_SHOWEXTENSIONS, FALSE);
    fShowExtensions = ss.fShowExtensions;
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::~CFileOpenBrowser
//
//  CFileOpenBrowser destructor.
//
////////////////////////////////////////////////////////////////////////////

CFileOpenBrowser::~CFileOpenBrowser()
{
    if (uRegister)
    {
        SHChangeNotifyDeregister(uRegister);

        uRegister = 0;
    }

    if (psl)
    {
        psl->Release();
        ppf->Release();

        psl = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::QueryInterface
//
//  Standard OLE2 style methods for this object.
//
////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFileOpenBrowser::QueryInterface(
    REFIID riid,
    LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IShellBrowser) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IShellBrowser *)this;
        ++cRef;
        return (S_OK);
    }
    else if (IsEqualIID(riid, IID_ICommDlgBrowser))
    {
        *ppvObj = (ICommDlgBrowser *)this;
        ++cRef;
        return (S_OK);
    }

    *ppvObj = NULL;
    return (E_NOINTERFACE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::AddRef
//
////////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE CFileOpenBrowser::AddRef()
{
    return (++cRef);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::Release
//
////////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE CFileOpenBrowser::Release()
{
    cRef--;
    if (cRef > 0)
    {
        return (cRef);
    }

    delete this;

    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetWindow
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::GetWindow(
    HWND *phwnd)
{
    *phwnd = hwndDlg;
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::ContextSensitiveHelp
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::ContextSensitiveHelp(
    BOOL fEnable)
{
    //
    //  Shouldn't need in a common dialog.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetStatusTextSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::SetStatusTextSB(
    LPCOLESTR pwch)
{
    //
    //  We don't have any status bar.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::EnableModelessSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::EnableModelessSB(
    BOOL fEnable)
{
    //
    //  We don't have any modeless window to be enabled/disabled.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::TranslateAcceleratorSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::TranslateAcceleratorSB(
    LPMSG pmsg,
    WORD wID)
{
    //
    //  We don't support EXE embedding.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::BrowseObject
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::BrowseObject(
    LPCITEMIDLIST pidl,
    UINT wFlags)
{
    //
    //  We don't support browsing, or more precisely, CDefView doesn't.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetViewStateStream
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::GetViewStateStream(
    DWORD grfMode,
    LPSTREAM *pStrm)
{
    //
    //  BUGBUG: We should implement this so there is some persistence
    //  for the file open dailog.
    //
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetControlWIndow
//
//  Get the handles of the various windows in the File Cabinet.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::GetControlWindow(
    UINT id,
    HWND *lphwnd)
{
    if (id == FCW_TOOLBAR)
    {
        *lphwnd = hwndToolbar;
        return (S_OK);
    }

    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SendControlMsg
//
////////////////////////////////////////////////////////////////////////////

#define SFVIDM_VIEW_FIRST         (SFVIDM_FIRST + 0x0028)
#define SFVIDM_VIEW_LIST          (SFVIDM_VIEW_FIRST + 0x0003)
#define SFVIDM_VIEW_DETAILS       (SFVIDM_VIEW_FIRST + 0x0004)

STDMETHODIMP CFileOpenBrowser::SendControlMsg(
    UINT id,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *pret)
{
    LRESULT lres = 0;
    if (id == FCW_TOOLBAR)
    {
        //
        //  We need to translate messages from defview intended for these
        //  buttons to our own.
        //
        switch (uMsg)
        {
            case ( TB_CHECKBUTTON ) :
            {
                switch (wParam)
                {
                    case ( SFVIDM_VIEW_DETAILS ) :
                    {
                        wParam = IDC_VIEWDETAILS;
                        break;
                    }
                    case ( SFVIDM_VIEW_LIST ) :
                    {
                        wParam = IDC_VIEWLIST;
                        break;
                    }
                    default :
                    {
                        goto Bail;
                    }
                }
                break;
            }
            default :
            {
                goto Bail;
                break;
            }
        }
        lres = SendMessage(hwndToolbar, uMsg, wParam, lParam);
    }

Bail:
    if (pret)
    {
        *pret = lres;
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::QueryActiveShellView
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::QueryActiveShellView(
    LPSHELLVIEW * ppsv)
{
    if (psv)
    {
        *ppsv = psv;
        psv->AddRef();
        return (S_OK);
    }
    *ppsv = NULL;
    return (E_NOINTERFACE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnViewWindowActive
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::OnViewWindowActive(
    LPSHELLVIEW psv)
{
    //
    //  No need to process this. We don't do menus.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::InsertMenusSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::InsertMenusSB(
    HMENU hmenuShared,
    LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetMenuSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::SetMenuSB(
    HMENU hmenuShared,
    HOLEMENU holemenu,
    HWND hwndActiveObject)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::RemoveMenusSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::RemoveMenusSB(
    HMENU hmenuShared)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetToolbarItems
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::SetToolbarItems(
    LPTBBUTTON lpButtons,
    UINT nButtons,
    UINT uFlags)
{
    //
    //  We don't let containers customize our toolbar.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnDefaultCommand
//
//  Process a double-click or Enter keystroke in the view control.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::OnDefaultCommand(
    struct IShellView *ppshv)
{
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    OnDblClick(FALSE);

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetCurrentFilter
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SetCurrentFilter(
    LPCTSTR pszFilter,
    OKBUTTONFLAGS Flags)
{
    LPTSTR lpNext;

    lstrcpyn(szLastFilter, pszFilter, ARRAYSIZE(szLastFilter));
    int nLeft = ARRAYSIZE(szLastFilter) - lstrlen(szLastFilter) - 1;

    //
    //  Do nothing if quoted.
    //
    if (Flags & OKBUTTON_QUOTED)
    {
        return;
    }

    //
    //  If pszFilter matches a filter spec, select that spec.
    //
    HWND hCmb = GetDlgItem(hwndDlg, cmb1);
    if (hCmb)
    {
        int nMax = ComboBox_GetCount(hCmb);
        int n;

        BOOL bCustomFilter = lpOFN->lpstrCustomFilter && *lpOFN->lpstrCustomFilter;

        for (n = 0; n < nMax; n++)
        {
            LPTSTR pFilter = (LPTSTR)ComboBox_GetItemData(hCmb, n);
            if (pFilter && pFilter != (LPTSTR)CB_ERR)
            {
                if (!lstrcmpi(pFilter, pszFilter))
                {
                    if (n != ComboBox_GetCurSel(hCmb))
                    {
                        ComboBox_SetCurSel(hCmb, n);
                    }
                    break;
                }
            }
        }
    }

    //
    //  For LFNs, tack on a '*' after non-wild extensions.
    //
    for (lpNext = szLastFilter; nLeft > 0; )
    {
        LPTSTR lpSemiColon = mystrchr(lpNext, CHAR_SEMICOLON);

        if (!lpSemiColon)
        {
            lpSemiColon = lpNext + lstrlen(lpNext);
        }

        TCHAR cTemp = *lpSemiColon;
        *lpSemiColon = CHAR_NULL;

        LPTSTR lpDot = mystrchr(lpNext, CHAR_DOT);

        //
        //  See if there is an extension that is not wild.
        //
        if (lpDot && *(lpDot + 1) && !IsWild(lpDot))
        {
            //
            //  Tack on a star.
            //  We know there is still enough room because nLeft > 0.
            //
            if (cTemp != CHAR_NULL)
            {
                hmemcpy( lpSemiColon + 2,
                         lpSemiColon + 1,
                         lstrlen(lpSemiColon + 1) * sizeof(TCHAR) );
            }
            *lpSemiColon = CHAR_STAR;

            ++lpSemiColon;
            --nLeft;
        }

        *lpSemiColon = cTemp;
        if (cTemp == CHAR_NULL)
        {
            break;
        }
        else
        {
            lpNext = lpSemiColon + 1;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFocusedChild
//
////////////////////////////////////////////////////////////////////////////

#define VC_NEWFOLDER    0
#define VC_VIEWLIST     1
#define VC_VIEWDETAILS  2

HWND GetFocusedChild(
    HWND hwndDlg,
    HWND hwndFocus)
{
    HWND hwndParent;

    if (!hwndDlg)
    {
        return (NULL);
    }

    if (!hwndFocus)
    {
        hwndFocus = ::GetFocus();
    }

    //
    //  Go up the parent chain until the parent is the main dialog.
    //
    while ((hwndParent=::GetParent(hwndFocus)) != hwndDlg)
    {
        if (!hwndParent)
        {
            return (NULL);
        }

        hwndFocus = hwndParent;
    }

    return (hwndFocus);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SwitchView
//
//  Switch the view control to a new container.
//
////////////////////////////////////////////////////////////////////////////

HRESULT CFileOpenBrowser::SwitchView(
    IShellFolder *psfNew,
    LPCITEMIDLIST pidlNew,
    FOLDERSETTINGS *pfs)
{
    IShellView *psvNew;

    if (!psfNew)
    {
        return (E_INVALIDARG);
    }

    HRESULT hres = psfNew->CreateViewObject( hwndDlg,
                                             IID_IShellView,
                                             (LPLPVOID)&psvNew );

    if (SUCCEEDED(hres))
    {
        IShellView *psvOld;
        HWND hwndNew;

        iWaitCount++;
        SetCursor(LoadCursor(NULL, IDC_WAIT));

        //
        //  The view window itself won't take the focus.  But we can set
        //  focus there and see if it bounces to the same place it is
        //  currently.  If that's the case, we want the new view window
        //  to get the focus;  otherwise, we put it back where it was.
        //
        BOOL bViewFocus = (GetFocusedChild(hwndDlg, NULL) == hwndView);

        psvOld = psv;

        //
        //  We attempt to blow off drawing on the main dialog.  Note that
        //  we should leave in SETREDRAW stuff to minimize flicker in case
        //  this fails.
        //
        BOOL bLocked = LockWindowUpdate(hwndDlg);

        //
        //  We need to kill the current psv before creating the new one in case
        //  the current one has a background thread going that is trying to
        //  call us back (IncludeObject).
        //
        if (psvOld)
        {
            SendMessage(hwndView, WM_SETREDRAW, FALSE, 0);
            psvOld->DestroyViewWindow();
            psv = NULL;

            //
            //  Don't release yet.  We will pass this to CreateViewWindow().
            //
        }

        //
        //  At this point, there should be no background processing happening.
        //
        psfCurrent = psfNew;
        SHGetPathFromIDList(pidlNew, szCurDir);

        //
        //  New windows (like the view window about to be created) show up at
        //  the bottom of the Z order, so I need to disable drawing of the
        //  subdialog while creating the view window; drawing will be enabled
        //  after the Z-order has been set properly.
        //
        if (hSubDlg)
        {
            SendMessage(hSubDlg, WM_SETREDRAW, FALSE, 0);
        }

        RECT rc;

        GetControlRect(hwndDlg, lst1, &rc);

        //
        //  psv must be set before creating the view window since we
        //  validate it on the IncludeObject callback.
        //
        psv = psvNew;

        hres = psvNew->CreateViewWindow(psvOld, pfs, this, &rc, &hwndNew);

        if (psvOld)
        {
            psvOld->Release();
        }

        if (hSubDlg)
        {
            //
            //  Turn REDRAW back on before changing the focus in case the
            //  SubDlg has the focus.
            //
            SendMessage(hSubDlg, WM_SETREDRAW, TRUE, 0);
        }

        if (SUCCEEDED(hres))
        {
            IContextMenu *pcm;
            TCHAR szTemp[10];
            BOOL bNewFolder, bViewList, bViewDetails;

            hwndView = hwndNew;

            if (SUCCEEDED(psvNew->GetItemObject( SVGIO_BACKGROUND,
                                                 IID_IContextMenu,
                                                 (LPVOID *)&pcm )))
            {
                bNewFolder = pcm->GetCommandString( (ULONG)CMDSTR_NEWFOLDER,
                                                    GCS_VALIDATE,
                                                    NULL,
                                                    (LPSTR)szTemp,
                                                    ARRAYSIZE(szTemp) );
                if (FAILED(bNewFolder))
                {
                    //
                    //  The szTemp parameter is not used in GCS_VALIDATE,
                    //  so there is no need to convert the string to either
                    //  Unicode or Ansi.
                    //
#ifdef UNICODE
                    bNewFolder = pcm->GetCommandString(
                                                   (ULONG)CMDSTR_NEWFOLDERA,
                                                   GCS_VALIDATEA,
                                                   NULL,
                                                   (LPSTR)szTemp,
                                                   ARRAYSIZE(szTemp) );
#else
                    bNewFolder = pcm->GetCommandString(
                                                   (ULONG)CMDSTR_NEWFOLDERW,
                                                   GCS_VALIDATEW,
                                                   NULL,
                                                   (LPSTR)szTemp,
                                                   ARRAYSIZE(szTemp) );
#endif
                }
                bNewFolder = (bNewFolder == S_OK);

                bViewList = pcm->GetCommandString( (ULONG)CMDSTR_VIEWLIST,
                                                   GCS_VALIDATE,
                                                   NULL,
                                                   (LPSTR)szTemp,
                                                   ARRAYSIZE(szTemp) );
                if (FAILED(bViewList))
                {
                    //
                    //  The szTemp parameter is not used in GCS_VALIDATE,
                    //  so there is no need to convert the string to either
                    //  Unicode or Ansi.
                    //
#ifdef UNICODE
                    bViewList = pcm->GetCommandString( (ULONG)CMDSTR_VIEWLISTA,
                                                       GCS_VALIDATEA,
                                                       NULL,
                                                       (LPSTR)szTemp,
                                                       ARRAYSIZE(szTemp) );
#else
                    bViewList = pcm->GetCommandString( (ULONG)CMDSTR_VIEWLISTW,
                                                       GCS_VALIDATEW,
                                                       NULL,
                                                       (LPSTR)szTemp,
                                                       ARRAYSIZE(szTemp) );
#endif
                }
                bViewList = (bViewList == S_OK);

                bViewDetails = pcm->GetCommandString( (ULONG)CMDSTR_VIEWDETAILS,
                                                      GCS_VALIDATE,
                                                      NULL,
                                                      (LPSTR)szTemp,
                                                      ARRAYSIZE(szTemp) );
                if (FAILED(bViewDetails))
                {
                    //
                    //  The szTemp parameter is not used in GCS_VALIDATE,
                    //  so there is no need to convert the string to either
                    //  Unicode or Ansi.
                    //
#ifdef UNICODE
                    bViewDetails = pcm->GetCommandString(
                                                   (ULONG)CMDSTR_VIEWDETAILSA,
                                                   GCS_VALIDATEA,
                                                   NULL,
                                                   (LPSTR)szTemp,
                                                   ARRAYSIZE(szTemp) );
#else
                    bViewDetails = pcm->GetCommandString(
                                                   (ULONG)CMDSTR_VIEWDETAILSW,
                                                   GCS_VALIDATEW,
                                                   NULL,
                                                   (LPSTR)szTemp,
                                                   ARRAYSIZE(szTemp) );
#endif
                }
                bViewDetails = (bViewDetails == S_OK);
                pcm->Release();
            }
            else
            {
                bNewFolder = bViewList = bViewDetails = FALSE;
            }

            ::SendMessage(hwndToolbar, TB_ENABLEBUTTON, IDC_NEWFOLDER,   bNewFolder);
            ::SendMessage(hwndToolbar, TB_ENABLEBUTTON, IDC_VIEWLIST,    bViewList);
            ::SendMessage(hwndToolbar, TB_ENABLEBUTTON, IDC_VIEWDETAILS, bViewDetails);

            //
            //  Move the view window to the right spot in the Z (tab) order.
            //
            SetWindowPos( hwndNew,
                          GetDlgItem(hwndDlg, lst1),
                          0,
                          0,
                          0,
                          0,
                          SWP_NOMOVE | SWP_NOSIZE );

            //
            //  Give it the right window ID for WinHelp.
            //
            SetWindowLong(hwndNew, GWL_ID, lst2);

            ::RedrawWindow( hwndView,
                            NULL,
                            NULL,
                            RDW_INVALIDATE | RDW_ERASE |
                                RDW_ALLCHILDREN | RDW_UPDATENOW );

            if (bViewFocus)
            {
                ::SetFocus(hwndView);
            }
        }
        else
        {
            psv = NULL;
            psvNew->Release();
        }

        //
        //  Let's draw again!
        //
        if (bLocked)
        {
            LockWindowUpdate(NULL);
        }

        iWaitCount--;
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    return (hres);
}


////////////////////////////////////////////////////////////////////////////
//
//  JustGetToolTipText
//
////////////////////////////////////////////////////////////////////////////

void JustGetToolTipText(
    UINT idCommand,
    LPTOOLTIPTEXT pTtt)
{
    if (!LoadString( ::g_hinst,
                     idCommand + MH_TOOLTIPBASE,
                     pTtt->szText,
                     ARRAYSIZE(pTtt->szText) ))
    {
        *pTtt->lpszText = 0;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnNotify
//
//  Process notify messages from the view -- for tooltips.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CFileOpenBrowser::OnNotify(
    LPNMHDR pnm)
{
    LRESULT lres = 0;

    switch (pnm->code)
    {
        case ( TTN_NEEDTEXT ) :
        {
            if (IsInRange(pnm->idFrom, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST))
            {
                if (hwndView)
                {
                    lres = ::SendMessage(hwndView, WM_NOTIFY, 0, (LPARAM)pnm);
                }
            }
            else
            {
                JustGetToolTipText(pnm->idFrom, (LPTOOLTIPTEXT)pnm);
            }
            lres = TRUE;
            break;
        }
        case ( NM_STARTWAIT ) :
        case ( NM_ENDWAIT ) :
        {
            iWaitCount += (pnm->code == NM_STARTWAIT ? 1 : -1);

            //
            //  What we really want is for the user to simulate a mouse
            //  move/setcursor.
            //
            SetCursor(LoadCursor(NULL, iWaitCount ? IDC_WAIT : IDC_ARROW));
            break;
        }
    }

    return (lres);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetViewItemText
//
//  Get the display name of a shell object.
//
////////////////////////////////////////////////////////////////////////////

void GetViewItemText(
    IShellFolder *psf,
    LPCITEMIDLIST pidl,
    LPTSTR pBuf,
    UINT cchBuf,
    BOOL fPath = TRUE)
{
    STRRET sr;

    if (SUCCEEDED(psf->GetDisplayNameOf( pidl,
                                         fPath
                                             ? SHGDN_INFOLDER | SHGDN_FORPARSING
                                             : SHGDN_INFOLDER,
                                         &sr )))
    {
        LPTSTR pszName = NULL;

#ifdef UNICODE
        switch (sr.uType)
        {
            case ( STRRET_OLESTR ) :
            {
                pszName = sr.pOleStr;
                break;
            }
            case ( STRRET_CSTRA ) :
            {
                UINT cchLen = lstrlenA(sr.cStr) + 1;

                pszName = (LPTSTR)SHAlloc(cchLen * sizeof(TCHAR));
                if (pszName)
                {
                    MultiByteToWideChar( CP_ACP,
                                         0,
                                         sr.cStr,
                                         cchLen,
                                         pszName,
                                         cchLen );
                }
                else
                {
                    break;
                }
                break;
            }
            case ( STRRET_OFFSETA ) :
            {
                LPSTR lpText = (LPSTR)(((LPBYTE)&pidl->mkid) + sr.uOffset);
                UINT cchLen = lstrlenA(lpText) + 1;

                pszName = (LPTSTR)SHAlloc(cchLen * sizeof(TCHAR));
                if (pszName)
                {
                    MultiByteToWideChar( CP_ACP,
                                         0,
                                         lpText,
                                         cchLen,
                                         pszName,
                                         cchLen );
                }
                else
                {
                    break;
                }
                break;
            }
            default :
            {
                //
                //  Unknown format.
                //
                break;
            }
        }
#else
        switch (sr.uType)
        {
            case ( STRRET_OLESTR ) :
            {
                LPOLESTR pwszDisplayName;

                //
                //  Convert from STRRET_OLESTR to STRRET_CSTR.
                //
                pwszDisplayName = sr.pOleStr;
                OleStrToStrN( sr.cStr,
                              ARRAYSIZE(sr.cStr),
                              pwszDisplayName,
                              (UINT)(-1) );
                SHFree(pwszDisplayName);
                sr.uType = STRRET_CSTR;

                // Fall Thru...
            }
            case ( STRRET_CSTRA ) :
            {
                pszName = sr.cStr;
                break;
            }
            case ( STRRET_OFFSETA ) :
            {
                pszName = (LPTSTR)(((LPBYTE)&pidl->mkid) + sr.uOffset);
                break;
            }
            default :
            {
                //
                //  Unknown format.
                //
                break;
            }
        }
#endif

        if (pszName != NULL)
        {
            lstrcpyn(pBuf, pszName, cchBuf);
#ifdef UNICODE
            SHFree(pszName);
#endif
        }
        else
        {
            *pBuf = CHAR_NULL;
        }
    }
    else
    {
        *pBuf = CHAR_NULL;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetListboxItem
//
//  Get a MYLISTBOXITEM object out of the location dropdown.
//
////////////////////////////////////////////////////////////////////////////

MYLISTBOXITEM *GetListboxItem(
    HWND hCtrl,
    int iItem)
{
    MYLISTBOXITEM *p = (MYLISTBOXITEM *)SendMessage( hCtrl,
                                                     CB_GETITEMDATA,
                                                     iItem,
                                                     NULL );
    if (p == (MYLISTBOXITEM *)CB_ERR)
    {
        return (NULL);
    }
    else
    {
        return (p);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  _ReleaseStgMedium
//
////////////////////////////////////////////////////////////////////////////

HRESULT _ReleaseStgMedium(
    LPSTGMEDIUM pmedium)
{
    if (pmedium->pUnkForRelease)
    {
        pmedium->pUnkForRelease->Release();
    }
    else
    {
        switch(pmedium->tymed)
        {
            case ( TYMED_HGLOBAL ) :
            {
                GlobalFree(pmedium->hGlobal);
                break;
            }
            default :
            {
                //
                //  Not fully implemented.
                //
                MessageBeep(0);
                break;
            }
        }
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetSaveButton
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SetSaveButton(
    UINT idSaveButton)
{
    PostMessage(hwndDlg, CDM_SETSAVEBUTTON, idSaveButton, 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::RealSetSaveButton
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::RealSetSaveButton(
    UINT idSaveButton)
{
    MSG msg;

    if (PeekMessage( &msg,
                     hwndDlg,
                     CDM_SETSAVEBUTTON,
                     CDM_SETSAVEBUTTON,
                     PM_NOREMOVE ))
    {
        //
        //  There is another SETSAVEBUTTON message in the queue, so blow off
        //  this one.
        //
        return;
    }

    if (bSave)
    {
        TCHAR szTemp[40];
        LPTSTR pszTemp = tszDefSave;

        //
        //  Load the string if not the "Save" string or there is no
        //  app-specified default.
        //
        if ((idSaveButton != iszFileSaveButton) || !pszTemp)
        {
            LoadString(g_hinst, idSaveButton, szTemp, ARRAYSIZE(szTemp));
            pszTemp = szTemp;
        }

        GetDlgItemText(hwndDlg, IDOK, szBuf, ARRAYSIZE(szBuf));
        if (lstrcmp(szBuf, pszTemp))
        {
            //
            //  Avoid some flicker.
            //
            SetDlgItemText(hwndDlg, IDOK, pszTemp);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetEditFile
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SetEditFile(
    LPTSTR pszFile,
    BOOL bShowExt,
    BOOL bSaveNullExt)
{
    BOOL bHasHiddenExt = FALSE;

    //
    //  Save the whole file name.
    //
    if (!pszHideExt.StrCpy(pszFile))
    {
        pszHideExt.StrCpy(NULL);
        bShowExt = TRUE;
    }

    //
    //  BUGBUG: This is bogus -- we only want to hide KNOWN extensions,
    //          not all extensions.
    //
    if (!bShowExt && !IsWild(pszFile))
    {
        LPTSTR pszExt = PathFindExtension(pszFile);
        if (*pszExt)
        {
            //
            //  If there was an extension, hide it.
            //
            *pszExt = 0;

            bHasHiddenExt = TRUE;
        }
    }

    SetDlgItemText(hwndDlg, edt1, pszFile);

    //
    //  If the initial file name has no extension, we want to do our normal
    //  extension finding stuff.  Any other time we get a file with no
    //  extension, we should not do this.
    //
    bUseHideExt = (LPTSTR)pszHideExt
                      ? (bSaveNullExt ? TRUE : bHasHiddenExt)
                      : FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  FindEOF
//
////////////////////////////////////////////////////////////////////////////

LPTSTR FindEOF(
    LPTSTR pszFiles)
{
    BOOL bQuoted;
    LPTSTR pszBegin = pszFiles;

    while (*pszBegin == CHAR_SPACE)
    {
        ++pszBegin;
    }

    //
    //  Note that we always assume a quoted string, even if no quotes exist,
    //  so the only file delimiters are '"' and '\0'.  This allows somebody to
    //  type <Start Menu> or <My Document> in the edit control and the right
    //  thing happens.
    //
    bQuoted = TRUE;

    if (*pszBegin == CHAR_QUOTE)
    {
        ++pszBegin;
    }

    lstrcpy(pszFiles, pszBegin);

    //
    //  Find the end of the filename (first quote or unquoted space).
    //
    for ( ; ; pszFiles = CharNext(pszFiles))
    {
        switch (*pszFiles)
        {
            case ( CHAR_NULL ) :
            {
                return (pszFiles);
            }
            case ( CHAR_SPACE ) :
            {
                if (!bQuoted)
                {
                    return (pszFiles);
                }
                break;
            }
            case ( CHAR_QUOTE ) :
            {
                //
                //  Note we only support '"' at the very beginning and very
                //  end of a file name.
                //
                return (pszFiles);
            }
            default :
            {
                break;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ConvertToNULLTerm
//
////////////////////////////////////////////////////////////////////////////

BOOL ConvertToNULLTerm(
    LPTSTR pchRead)
{
    BOOL bNoFilesYet = TRUE;
    LPTSTR pchWrite = pchRead;

    //
    //  Convert to a double-NULL terminated list.
    //
    for ( ; ; )
    {
        LPTSTR pchEnd = FindEOF(pchRead);

        //
        //  Mark the end of the filename with a NULL.
        //
        if (*pchEnd)
        {
            *pchEnd = NULL;
            bNoFilesYet = FALSE;

            lstrcpy(pchWrite, pchRead);
            pchWrite += pchEnd - pchRead + 1;
        }
        else
        {
            //
            //  Found EOL.  Make sure we did not end with spaces.
            //
            if (*pchRead)
            {
                lstrcpy(pchWrite, pchRead);
                pchWrite += pchEnd - pchRead + 1;
            }
            else if (bNoFilesYet)
            {
                //
                //  Nothing of significance in the edit control.
                //
                return (FALSE);
            }

            break;
        }

        pchRead = pchEnd + 1;
    }

    //
    //  Double-NULL terminate.
    //
    *pchWrite = CHAR_NULL;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SelFocusEnumCB
//
////////////////////////////////////////////////////////////////////////////

typedef struct _SELFOCUS
{
    BOOL    bSelChange;
    UINT    idSaveButton;
    int     nSel;
    TEMPSTR sHidden;
    TEMPSTR sDisplayed;
} SELFOCUS;

BOOL SelFocusEnumCB(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam)
{
    SELFOCUS *psf = (SELFOCUS *)lParam;
    DWORD dwAttrs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;
    TCHAR szBuf[MAX_PATH + 1];

    if (!pidl)
    {
        return (TRUE);
    }

    if (SUCCEEDED(that->psfCurrent->GetAttributesOf(1, &pidl, &dwAttrs)))
    {
        if (dwAttrs & SFGAO_FOLDER)
        {
            psf->idSaveButton = iszFileOpenButton;
        }
        else
        {
            if (psf->bSelChange && (dwAttrs & SFGAO_FILESYSTEM))
            {
                ++psf->nSel;

                if (that->lpOFN->Flags & OFN_ALLOWMULTISELECT)
                {
                    *szBuf = CHAR_QUOTE;
                    GetViewItemText( that->psfCurrent,
                                     pidl,
                                     szBuf + 1,
                                     ARRAYSIZE(szBuf) - 3 );
                    lstrcat(szBuf, TEXT("\" "));

                    if (!psf->sHidden.StrCat(szBuf))
                    {
                        psf->nSel = -1;
                        return (FALSE);
                    }

                    if (!that->fShowExtensions)
                    {
                        LPTSTR pszExt = PathFindExtension(szBuf + 1);
                        if (*pszExt)
                        {
                            *pszExt = 0;
                            lstrcat(szBuf, TEXT("\" "));
                        }
                    }

                    if (!psf->sDisplayed.StrCat(szBuf))
                    {
                        psf->nSel = -1;
                        return (FALSE);
                    }
                }
                else
                {
                    GetViewItemText(that->psfCurrent, pidl, szBuf, ARRAYSIZE(szBuf));
                    that->SetEditFile(szBuf, that->fShowExtensions);
                }
            }
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SelFocusChange
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SelFocusChange(
    BOOL bSelChange)
{
    SELFOCUS sf;

    sf.bSelChange = bSelChange;
    sf.idSaveButton = iszFileSaveButton;
    sf.nSel = 0;

    EnumItemObjects(SVGIO_SELECTION, SelFocusEnumCB, (LPARAM)&sf);

    if (lpOFN->Flags & OFN_ALLOWMULTISELECT)
    {
        switch (sf.nSel)
        {
            case ( -1 ) :
            {
                //
                //  Oops! We ran out of memory.
                //
                MessageBeep(0);
                return;
            }
            case ( 0 ) :
            {
                //
                //  No files selected; do not change edit control.
                //
                break;
            }
            case ( 1 ) :
            {
                //
                //  Strip off quotes so the single file case looks OK.
                //
                ConvertToNULLTerm(sf.sHidden);
                SetEditFile(sf.sHidden, fShowExtensions);

                sf.idSaveButton = iszFileSaveButton;
                break;
            }
            default :
            {
                SetEditFile(sf.sDisplayed, TRUE);
                pszHideExt.StrCpy(sf.sHidden);

                sf.idSaveButton = iszFileSaveButton;
                break;
            }
        }
    }

    SetSaveButton(sf.idSaveButton);
}


////////////////////////////////////////////////////////////////////////////
//
//  SelRenameCB
//
////////////////////////////////////////////////////////////////////////////

BOOL SelRenameCB(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam)
{
    DWORD dwAttrs = SFGAO_FOLDER;

    if (!pidl)
    {
        return (TRUE);
    }

    if (SUCCEEDED(that->psfCurrent->GetAttributesOf(1, &pidl, &dwAttrs)))
    {
        if (!(dwAttrs & SFGAO_FOLDER))
        {
            //
            //  If it is not a folder then set the selection to nothing
            //  so that whatever is in the edit box will be used.
            //
            that->psv->SelectItem(NULL, SVSI_DESELECTOTHERS);
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SelRename
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SelRename(void)
{
    EnumItemObjects(SVGIO_SELECTION, SelRenameCB, NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnStateChange
//
//  Process selection change in the view control.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::OnStateChange(
    struct IShellView *ppshv,
    ULONG uChange)
{
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    switch (uChange)
    {
        case ( CDBOSC_SETFOCUS ) :
        {
            if (bSave)
            {
                SelFocusChange(FALSE);
            }
            break;
        }
        case ( CDBOSC_KILLFOCUS ) :
        {
            SetSaveButton(iszFileSaveButton);
            break;
        }
        case ( CDBOSC_SELCHANGE ) :
        {
            //
            //  Post one of these messages, since we seem to get a whole bunch
            //  of them.
            //
            if (!fSelChangedPending)
            {
                fSelChangedPending = TRUE;
                PostMessage(hwndDlg, CDM_SELCHANGE, 0, 0);
            }
            break;
        }
        case ( CDBOSC_RENAME ) :
        {
            SelRename();
            break;
        }
        default :
        {
            return (E_NOTIMPL);
        }
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::IncludeObject
//
//  Tell the view control which objects to include in its enumerations.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::IncludeObject(
    struct IShellView *ppshv,
    LPCITEMIDLIST pidl)
{
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    DWORD dwAttrs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;
    if (SUCCEEDED(psfCurrent->GetAttributesOf(1, &pidl, &dwAttrs)))
    {
        if (!(dwAttrs & (SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR)))
        {
            return (S_FALSE);
        }
    }

    dwAttrs &= SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;
    if (*szLastFilter && (dwAttrs == SFGAO_FILESYSTEM))
    {
        GetViewItemText(psfCurrent, (LPITEMIDLIST)pidl, szBuf, ARRAYSIZE(szBuf));

        if (!LinkMatchSpec(pidl, szBuf, szLastFilter) &&
            !PathMatchSpec(szBuf, szLastFilter))
        {
            return (S_FALSE);
        }
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  ICoCreateInstance
//
//  Create an instance of the specified shell class, IClassFactory interface.
//  Lightweight version of the OLE2 binder.
//
////////////////////////////////////////////////////////////////////////////

HRESULT ICoCreateInstance(
    REFCLSID rclsid,
    REFIID riid,
    LPLPVOID ppv)
{
    LPCLASSFACTORY pcf;
    HRESULT hres = SHDllGetClassObject( rclsid,
                                        IID_IClassFactory,
                                        (LPLPVOID)&pcf );
    if (SUCCEEDED(hres))
    {
        hres = pcf->CreateInstance(NULL, riid, ppv);
        pcf->Release();
    }
    return (hres);
}


////////////////////////////////////////////////////////////////////////////
//
//  InsertItem
//
//  Insert a single item into the location dropdown.
//
////////////////////////////////////////////////////////////////////////////

BOOL InsertItem(
    HWND hCtrl,
    int iItem,
    MYLISTBOXITEM *pItem,
    TCHAR *pszName)
{
    LPTSTR pszChar;

    for (pszChar = pszName; *pszChar != CHAR_NULL; pszChar = CharNext(pszChar))
    {
        if (pszChar - pszName >= MAX_DRIVELIST_STRING_LEN - 1)
        {
            *pszChar = CHAR_NULL;
            break;
        }
    }

    if (SendMessage( hCtrl,
                     CB_INSERTSTRING,
                     iItem,
                     (LPARAM)(LPCTSTR)pszName ) == CB_ERR)
    {
        return (FALSE);
    }

    SendMessage(hCtrl, CB_SETITEMDATA, iItem, (LPARAM)pItem);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  LBItemCompareProc
//
////////////////////////////////////////////////////////////////////////////

int CALLBACK LBItemCompareProc(
    LPVOID p1,
    LPVOID p2,
    LPARAM lParam)
{
    IShellFolder *psfParent = (IShellFolder *)lParam;
    MYLISTBOXITEM *pItem1 = (MYLISTBOXITEM *)p1;
    MYLISTBOXITEM *pItem2 = (MYLISTBOXITEM *)p2;

    //
    //  Do default sorting (by name).
    //
    HRESULT hres = psfParent->CompareIDs(0, pItem1->pidlThis, pItem2->pidlThis);

    return ( (short)SCODE_CODE(GetScode(hres)) );
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::UpdateLevel
//
//  Insert the contents of a shell container into the location dropdown.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::UpdateLevel(
    HWND hwndLB,
    int iInsert,
    MYLISTBOXITEM *pParentItem)
{
    if (!pParentItem)
    {
        return;
    }

    LPENUMIDLIST penum;
    HDPA hdpa;
    DWORD cIndent = pParentItem->cIndent + 1;
    IShellFolder *psfParent = pParentItem->GetShellFolder();

    hdpa = DPA_Create(4);
    if (!hdpa)
    {
        //
        //  No memory: Cannot enum this level.
        //
        return;
    }

    if (SUCCEEDED(psfParent->EnumObjects(hwndLB, SHCONTF_FOLDERS, &penum)))
    {
        ULONG celt;
        LPITEMIDLIST pidl;

        while (penum->Next(1, &pidl, &celt) == S_OK && celt == 1)
        {
            //
            //  Note: We need to avoid creation of pItem if this is not
            //  a file system object (or ancestor) to avoid extra
            //  bindings.
            //
            if (ShouldIncludeObject(psfParent, pidl))
            {
                MYLISTBOXITEM *pItem =
                    new MYLISTBOXITEM( pParentItem,
                                       psfParent,
                                       pidl,
                                       cIndent,
                                       MLBI_PERMANENT | MLBI_PSFFROMPARENT );
                if (pItem != NULL)
                {
                    if (DPA_InsertPtr(hdpa, 0x7fff, pItem) < 0)
                    {
                        delete pItem;
                    }
                }
            }
            SHFree(pidl);

        }
        penum->Release();
    }

    DPA_Sort(hdpa, LBItemCompareProc, (LPARAM)psfParent);

    int nLBIndex, nDPAIndex, nDPAItems;
    BOOL bCurItemGone;

    nDPAItems = DPA_GetPtrCount(hdpa);
    nLBIndex = iInsert;

    bCurItemGone = FALSE;

    //
    //  Make sure the user is not playing with the selection right now.
    //
    ComboBox_ShowDropdown(hwndLB, FALSE);

    //
    //  We're all sorted, so now we can do a merge.
    //
    for (nDPAIndex = 0; ; ++nDPAIndex)
    {
        MYLISTBOXITEM *pNewItem;
        TCHAR szBuf[MAX_DRIVELIST_STRING_LEN];
        MYLISTBOXITEM *pOldItem;

        if (nDPAIndex < nDPAItems)
        {
            pNewItem = (MYLISTBOXITEM *)DPA_FastGetPtr(hdpa, nDPAIndex);
        }
        else
        {
            //
            //  Signal that we got to the end of the list.
            //
            pNewItem = NULL;
        }

        for (pOldItem = GetListboxItem(hwndLB, nLBIndex);
             pOldItem != NULL;
             pOldItem = GetListboxItem(hwndLB, ++nLBIndex))
        {
            int nCmp;

            if (pOldItem->cIndent < cIndent)
            {
                //
                //  We went up a level, so insert here.
                //
                break;
            }
            else if (pOldItem->cIndent > cIndent)
            {
                //
                //  We went down a level so ignore this.
                //
                continue;
            }

            //
            //  Set this to 1 at the end of the DPA to clear out deleted items
            //  at the end.
            //
            nCmp = !pNewItem
                       ? 1
                       : LBItemCompareProc( pNewItem,
                                            pOldItem,
                                            (LPARAM)psfParent );
            if (nCmp < 0)
            {
                //
                //  We found the first item greater than the new item, so
                //  add it in.
                //
                break;
            }
            else if (nCmp > 0)
            {
                //
                //  Oops! It looks like this item no longer exists, so
                //  delete it.
                //
                for ( ; ; )
                {
                    if (pOldItem == pCurrentLocation)
                    {
                        bCurItemGone = TRUE;
                        pCurrentLocation = NULL;
                    }

                    delete pOldItem;
                    SendMessage(hwndLB, CB_DELETESTRING, nLBIndex, NULL);

                    pOldItem = GetListboxItem(hwndLB, nLBIndex);

                    if (!pOldItem || pOldItem->cIndent <= cIndent)
                    {
                        break;
                    }
                }

                //
                //  We need to continue from the current position, not the
                //  next.
                //
                --nLBIndex;
            }
            else
            {
                //
                //  This item already exists, so no need to add it.
                //  Make sure we do not check this LB item again.
                //
                pOldItem->dwFlags |= MLBI_PERMANENT;
                ++nLBIndex;
                goto NotThisItem;
            }
        }

        if (!pNewItem)
        {
            //
            //  Got to the end of the list.
            //
            break;
        }

        GetViewItemText( psfParent,
                         pNewItem->pidlThis,
                         szBuf,
                         ARRAYSIZE(szBuf),
                         FALSE );
        if (szBuf[0] && InsertItem(hwndLB, nLBIndex, pNewItem, szBuf))
        {
            ++nLBIndex;
        }
        else
        {
NotThisItem:
            delete pNewItem;
        }
    }

    DPA_Destroy(hdpa);

    if (bCurItemGone)
    {
        //
        //  If we deleted the current selection, go back to the desktop.
        //
        ComboBox_SetCurSel(hwndLB, 0);
        OnSelChange(-1, TRUE);
    }

    iCurrentLocation = ComboBox_GetCurSel(hwndLB);
}


////////////////////////////////////////////////////////////////////////////
//
//  ClearListbox
//
//  Clear the location dropdown and delete all entries.
//
////////////////////////////////////////////////////////////////////////////

void ClearListbox(
    HWND hwndList)
{
    SendMessage(hwndList, WM_SETREDRAW, FALSE, NULL);
    int cItems = SendMessage(hwndList, CB_GETCOUNT, NULL, NULL);
    while (cItems--)
    {
        MYLISTBOXITEM *pItem = GetListboxItem(hwndList, 0);
        delete pItem;
        SendMessage(hwndList, CB_DELETESTRING, 0, NULL);
    }
    SendMessage(hwndList, WM_SETREDRAW, TRUE, NULL);
    InvalidateRect(hwndList, NULL, FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitFilterBox
//
//  Places the double null terminated list of filters in the combo box.
//
//  The list consists of pairs of null terminated strings, with an
//  additional null terminating the list.
//
////////////////////////////////////////////////////////////////////////////

DWORD InitFilterBox(
    HWND hDlg,
    LPCTSTR lpszFilter)
{
    DWORD nIndex = 0;
    UINT nLen;
    HWND hCmb = GetDlgItem(hDlg, cmb1);

    if (hCmb)
    {
        while (*lpszFilter)
        {
            //
            //  First string put in as string to show.
            //
            nIndex = ComboBox_AddString(hCmb, lpszFilter);

            nLen = lstrlen(lpszFilter) + 1;
            lpszFilter += nLen;

            //
            //  Second string put in as itemdata.
            //
            ComboBox_SetItemData(hCmb, nIndex, lpszFilter);

            //
            //  Advance to next element.
            //
            nLen = lstrlen(lpszFilter) + 1;
            lpszFilter += nLen;
        }
    }

    //
    //  BUGBUG: nIndex could be CB_ERR.
    //
    return (nIndex);
}


////////////////////////////////////////////////////////////////////////////
//
//  MoveControls
//
////////////////////////////////////////////////////////////////////////////

void MoveControls(
    HWND hDlg,
    BOOL bBelow,
    int nStart,
    int nXMove,
    int nYMove)
{
    HWND hwnd;
    RECT rcWnd;

    if (nXMove == 0 && nYMove == 0)
    {
        //
        //  Quick out if nothing to do.
        //
        return;
    }

    for (hwnd = GetWindow(hDlg, GW_CHILD);
         hwnd;
         hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        GetWindowRect(hwnd, &rcWnd);
        MapWindowRect(HWND_DESKTOP, hDlg, &rcWnd);

        if (bBelow)
        {
            if (rcWnd.top < nStart)
            {
                continue;
            }
        }
        else
        {
            if (rcWnd.left < nStart)
            {
                continue;
            }
        }

        SetWindowPos( hwnd,
                      NULL,
                      rcWnd.left + nXMove,
                      rcWnd.top + nYMove,
                      0,
                      0,
                      SWP_NOZORDER | SWP_NOSIZE );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  DummyDlgProc
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK DummyDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ResetDialogHeight
//
////////////////////////////////////////////////////////////////////////////

void ResetDialogHeight(
    HWND hDlg,
    HWND hwndExclude,
    int nCtlsBottom)
{
    int nDiffBottom = nCtlsBottom - GetControlsBottom(hDlg, hwndExclude);
    if (nDiffBottom > 0)
    {
        RECT rcFull;

        GetWindowRect(hDlg, &rcFull);
        SetWindowPos( hDlg,
                      NULL,
                      0,
                      0,
                      RectWid(rcFull),
                      RectHgt(rcFull) - nDiffBottom,
                      SWP_NOZORDER | SWP_NOMOVE );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::CreateHookDialog
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::CreateHookDialog(
    int nCtlsBottom)
{
    DWORD Flags = lpOFN->Flags;
    BOOL bRet = FALSE;
    HANDLE hTemplate;
    HINSTANCE hinst;
    LPCTSTR lpDlg;
    HWND hCtlCmn;
    RECT rcReal, rcSub;
    int nXMove, nXRoom, nYMove, nYRoom, nXStart, nYStart;
    DWORD dwStyle;
    DLGPROC lpfnHookProc;

    if (!(Flags & (OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
    {
        //
        //  No hook or template; nothing to do.
        //
        ResetDialogHeight(hwndDlg, NULL, nCtlsBottom);
        return (TRUE);
    }

    if (Flags & OFN_ENABLETEMPLATEHANDLE)
    {
        hTemplate = lpOFN->hInstance;
        hinst = ::g_hinst;
    }
    else
    {
        if (Flags & OFN_ENABLETEMPLATE)
        {
            if (!lpOFN->lpTemplateName)
            {
                StoreExtendedError(CDERR_NOTEMPLATE);
                return (FALSE);
            }
            if (!lpOFN->hInstance)
            {
                StoreExtendedError(CDERR_NOHINSTANCE);
                return (FALSE);
            }

            lpDlg = lpOFN->lpTemplateName;
            hinst = lpOFN->hInstance;
        }
        else
        {
            hinst = ::g_hinst;
            lpDlg = MAKEINTRESOURCE(DUMMYFILEOPENORD);
        }

        HRSRC hRes = FindResource(hinst, lpDlg, RT_DIALOG);
        if (hRes == NULL)
        {
            StoreExtendedError(CDERR_FINDRESFAILURE);
            return (FALSE);
        }
        if ((hTemplate = LoadResource(hinst, hRes)) == NULL)
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return (FALSE);
        }
    }

    if (!LockResource(hTemplate))
    {
        StoreExtendedError(CDERR_LOADRESFAILURE);
        return (FALSE);
    }

    dwStyle = ((LPDLGTEMPLATE)hTemplate)->style;
    if (!(dwStyle & WS_CHILD))
    {
        //
        //  I don't want to go poking in their template, and I don't want to
        //  make a copy, so I will just fail.  This also helps us weed out
        //  "old-style" templates that were accidentally used.
        //
        StoreExtendedError(CDERR_DIALOGFAILURE);
        return (FALSE);
    }

    if (Flags & OFN_ENABLEHOOK)
    {
        lpfnHookProc = (DLGPROC)lpOFN->lpfnHook;
    }
    else
    {
        lpfnHookProc = DummyDlgProc;
    }

#if 0
    //
    //  This is not needed.  WOW apps are not allowed to get the new
    //  explorer look.
    //
    UINT uiWOWFlag = 0;

    if (Flags & CD_WOWAPP)
    {
        uiWOWFlag = SCDLG_16BIT;
    }

    hSubDlg = CreateDialogIndirectParamAorW( hinst,
                                             (LPDLGTEMPLATE)hTemplate,
                                             hwndDlg,
                                             lpfnHookProc,
                                             (LPARAM)lpOFN,
                                             uiWOWFlag );
#endif

    hSubDlg = CreateDialogIndirectParam( hinst,
                                         (LPDLGTEMPLATE)hTemplate,
                                         hwndDlg,
                                         lpfnHookProc,
                                         (LPARAM)lpOFN );
    if (!hSubDlg)
    {
        StoreExtendedError(CDERR_DIALOGFAILURE);
        return (FALSE);
    }

    //
    //  We reset the height of the dialog after creating the hook dialog so
    //  the hook can hide controls in its WM_INITDIALOG message.
    //
    ResetDialogHeight(hwndDlg, hSubDlg, nCtlsBottom);

    //
    //  Now move all of the controls around.
    //
    GetClientRect(hwndDlg, &rcReal);
    GetClientRect(hSubDlg, &rcSub);

    hCtlCmn = GetDlgItem(hSubDlg, stc32);
    if (hCtlCmn)
    {
        RECT rcCmn;

        GetWindowRect(hCtlCmn, &rcCmn);
        MapWindowRect(HWND_DESKTOP, hSubDlg, &rcCmn);

        //
        //  Move the controls in our dialog to make room for the hook's
        //  controls above and to the left.
        //
        MoveControls(hwndDlg, FALSE, 0, rcCmn.left, rcCmn.top);

        //
        //  Calculate how far sub dialog controls need to move, and how much
        //  more room our dialog needs.
        //
        nXStart = rcCmn.right;
        nYStart = rcCmn.bottom;
        nXMove = (rcReal.right - rcReal.left) - (rcCmn.right - rcCmn.left);
        nYMove = (rcReal.bottom - rcReal.top) - (rcCmn.bottom - rcCmn.top);
        nXRoom = rcSub.right - (rcCmn.right - rcCmn.left);
        nYRoom = rcSub.bottom - (rcCmn.bottom - rcCmn.top);

        if (nXMove < 0)
        {
            //
            //  If the template size is too big, we need more room in the
            //  dialog.
            //
            nXRoom -= nXMove;
            nXMove = 0;
        }
        if (nYMove < 0)
        {
            //
            //  If the template size is too big, we need more room in the
            //  dialog.
            //
            nYRoom -= nYMove;
            nYMove = 0;
        }

        //
        //  Resize the "template" control so the hook knows the size of our
        //  stuff.
        //
        SetWindowPos( hCtlCmn,
                      NULL,
                      0,
                      0,
                      rcReal.right - rcReal.left,
                      rcReal.bottom - rcReal.top,
                      SWP_NOMOVE | SWP_NOZORDER );
    }
    else
    {
        //
        //  Extra controls go on the bottom by default.
        //
        nXStart = nYStart = nXMove = nXRoom = 0;

        nYMove = rcReal.bottom;
        nYRoom = rcSub.bottom;
    }

    MoveControls(hSubDlg, FALSE, nXStart, nXMove, 0);
    MoveControls(hSubDlg, TRUE, nYStart, 0, nYMove);

    //
    //  Resize our dialog and the sub dialog.
    //  BUGBUG: We need to check whether part of the dialog is off screen.
    //
    GetWindowRect(hwndDlg, &rcReal);
    SetWindowPos( hwndDlg,
                  NULL,
                  0,
                  0,
                  (rcReal.right - rcReal.left) + nXRoom,
                  (rcReal.bottom - rcReal.top) + nYRoom,
                  SWP_NOZORDER|SWP_NOMOVE );

    //
    //  Note that we are moving this to (0,0) and the bottom of the Z order.
    //
    GetWindowRect(hSubDlg, &rcReal);
    SetWindowPos( hSubDlg,
                  HWND_BOTTOM,
                  0,
                  0,
                  (rcReal.right - rcReal.left) + nXMove,
                  (rcReal.bottom - rcReal.top) + nYMove,
                  0 );

    ShowWindow(hSubDlg, SW_SHOW);

    CD_SendInitDoneNotify(hSubDlg, hwndDlg, lpOFN, lpOFI);

    bRet = TRUE;

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitSaveAsControls
//
//  Change the captions of a bunch of controls to say saveas-like things.
//
////////////////////////////////////////////////////////////////////////////

const struct
{
    UINT idControl;
    UINT idString;
} aSaveAsControls[] =
{
    { (UINT)-1, iszFileSaveTitle },         // -1 means the dialog itself
    { stc2,     iszSaveAsType },
    { IDOK,     iszFileSaveButton },
    { stc4,     iszFileSaveIn }
};

void InitSaveAsControls(
    HWND hDlg)
{
    for (UINT iControl = 0; iControl < ARRAYSIZE(aSaveAsControls); iControl++)
    {
        HWND hwnd = hDlg;
        TCHAR szText[80];

        if (aSaveAsControls[iControl].idControl != -1)
        {
            hwnd = GetDlgItem(hDlg, aSaveAsControls[iControl].idControl);
        }

        LoadString( g_hinst,
                    aSaveAsControls[iControl].idString,
                    szText,
                    ARRAYSIZE(szText) );
        SetWindowText(hwnd, szText);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetControlsBottom
//
//  Returns the bottom of the control farthest down (in screen coordinates).
//
////////////////////////////////////////////////////////////////////////////

int GetControlsBottom(
    HWND hDlg,
    HWND hwndExclude)
{
    RECT rc;
    HWND hwnd;
    int uBottom = 0x80000000;

    for (hwnd = GetWindow(hDlg, GW_CHILD);
         hwnd;
         hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        //
        //  Note we cannot use IsWindowVisible, since the parent is not visible.
        //  We do not want the magic static to be included.
        //
        if (!IsVisible(hwnd) || hwnd == hwndExclude)
        {
            continue;
        }

        GetWindowRect(hwnd, &rc);
        if (uBottom < rc.bottom)
        {
            uBottom = rc.bottom;
        }
    }

    return (uBottom);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitLocation
//
//  Main initialization (WM_INITDIALOG phase).
//
////////////////////////////////////////////////////////////////////////////

BOOL InitLocation(
    HWND hDlg,
    LPOFNINITINFO poii)
{
    HWND hCtrl = GetDlgItem(hDlg, cmb2);
    LPOPENFILENAME lpOFN = poii->lpOFI->pOFN;
    BOOL fIsSaveAs = poii->bSave;

    int nCtlsBottom = GetControlsBottom(hDlg, NULL);

    CFileOpenBrowser *pDlgStruct = new CFileOpenBrowser(hDlg, FALSE);
    if (pDlgStruct == NULL)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }
    StoreBrowser(hDlg, pDlgStruct);

    //
    //  Now that pDlgStruct is stored in the hDlg, it will get freed on the
    //  WM_DESTROY.
    //

    IShellFolder *psfRoot;
    if (FAILED(ICoCreateInstance( CLSID_ShellDesktop,
                                  IID_IShellFolder,
                                  (LPLPVOID)&psfRoot )))
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    LPITEMIDLIST pidlRoot = SHCloneSpecialIDList(hDlg, CSIDL_DESKTOP, FALSE);
    if (!pidlRoot)
    {
        psfRoot->Release();
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    //
    //  Add the desktop item itself.
    //
    MYLISTBOXITEM *pRootItem = new MYLISTBOXITEM( NULL,
                                                  psfRoot,
                                                  pidlRoot,
                                                  0,
                                                  MLBI_PERMANENT );
    SHFree(pidlRoot);

    if (pRootItem == NULL)
    {
        psfRoot->Release();
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    //
    //  Now that psfRoot is stored in the pRootItem, it will get freed on the
    //  delete.
    //

    TCHAR szScratch[MAX_PATH + 1];

    GetViewItemText(psfRoot, NULL, szScratch, ARRAYSIZE(szScratch));
    if (!InsertItem(hCtrl, 0, pRootItem, szScratch))
    {
        delete pRootItem;
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    //
    //  Now that pRootItem is stored in the combo, it will get freed on the
    //  WM_DESTROY.
    //

#if 0
    //
    //  Enumerate the desktop entries and add them.
    //
    pDlgStruct->UpdateLevel(hCtrl, 1, pRootItem);

    //
    //  Get the My Computer item and expand it - it's item 1.
    //
    MYLISTBOXITEM *pDrivesItem = GetListboxItem(hCtrl, 1);
    if (!pDrivesItem)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }
    pDlgStruct->UpdateLevel(hCtrl, 2, pDrivesItem);

    SHChangeNotifyEntry fsne[2];

    fsne[0].pidl = pRootItem->pidlFull;
    fsne[0].fRecursive = FALSE;

    fsne[1].pidl = pDrivesItem->pidlFull;
    fsne[1].fRecursive = FALSE;

    pDlgStruct->uRegister =
        SHChangeNotifyRegister(
            hDlg,
            SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
            SHCNE_ALLEVENTS & ~(SHCNE_CREATE | SHCNE_DELETE | SHCNE_RENAMEITEM),
            CDM_FSNOTIFY,
            2,
            fsne );
#endif

    pDlgStruct->pCurrentLocation = pRootItem;
    pDlgStruct->lpOFN = lpOFN;
    pDlgStruct->bSave = fIsSaveAs;

    pDlgStruct->lpOFI = poii->lpOFI;

    pDlgStruct->pszDefExt.StrCpy(lpOFN->lpstrDefExt);

    //
    //  Here follows all the caller-parameter-based initialization.
    //
    ::lpOKProc = (WNDPROC)::SetWindowLong( ::GetDlgItem(hDlg, IDOK),
                                           GWL_WNDPROC,
                                           (LONG)OKSubclass );

    if (lpOFN->Flags & OFN_CREATEPROMPT)
    {
        lpOFN->Flags |= (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
    }
    else if (lpOFN->Flags & OFN_FILEMUSTEXIST)
    {
        lpOFN->Flags |= OFN_PATHMUSTEXIST;
    }

    //
    //  Limit the text to the maximum path length instead of limiting it to
    //  the buffer length.  This allows users to type ..\..\.. and move
    //  around when the app gives an extremely small buffer.
    //
    SendDlgItemMessage(hDlg, edt1, EM_LIMITTEXT, MAX_PATH - 1, 0);

    SendDlgItemMessage(hDlg, cmb2, CB_SETEXTENDEDUI, 1, 0);
    SendDlgItemMessage(hDlg, cmb1, CB_SETEXTENDEDUI, 1, 0);

    //
    //  Check if original directory should be saved for later restoration.
    //
    if (lpOFN->Flags & OFN_NOCHANGEDIR)
    {
        GetCurrentDirectory( ARRAYSIZE(pDlgStruct->szStartDir),
                             pDlgStruct->szStartDir );
    }

    //
    //  Initialize all provided filters.
    //
    if (lpOFN->lpstrCustomFilter && *lpOFN->lpstrCustomFilter)
    {
        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_INSERTSTRING,
                            0,
                            (LONG)lpOFN->lpstrCustomFilter );
        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_SETITEMDATA,
                            0,
                            (LPARAM)(lpOFN->lpstrCustomFilter +
                                     lstrlen(lpOFN->lpstrCustomFilter) + 1) );
        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_LIMITTEXT,
                            (WPARAM)(lpOFN->nMaxCustFilter),
                            0L );
    }
    else
    {
        //
        //  Given no custom filter, the index will be off by one.
        //
        if (lpOFN->nFilterIndex != 0)
        {
            lpOFN->nFilterIndex--;
        }
    }

    //
    //  Listed filters next.
    //
    if (lpOFN->lpstrFilter)
    {
        if (lpOFN->nFilterIndex > InitFilterBox(hDlg, lpOFN->lpstrFilter))
        {
            lpOFN->nFilterIndex = 0;
        }
    }
    else
    {
        lpOFN->nFilterIndex = 0;
    }

    //
    //  If an entry exists, select the one indicated by nFilterIndex.
    //
    if ((lpOFN->lpstrFilter) ||
        (lpOFN->lpstrCustomFilter && *lpOFN->lpstrCustomFilter))
    {
        HWND hCmb1 = GetDlgItem(hDlg, cmb1);

        ComboBox_SetCurSel(hCmb1, lpOFN->nFilterIndex);

        pDlgStruct->RefreshFilter(hCmb1);
    }

    //
    //  Make sure to do this before checking if there is a title specified.
    //
    if (fIsSaveAs)
    {
        //
        //  Note we can do this even if there is a hook/template.
        //
        InitSaveAsControls(hDlg);
    }

    if (lpOFN->lpstrTitle && *lpOFN->lpstrTitle)
    {
        SetWindowText(hDlg, lpOFN->lpstrTitle);
    }

    if (lpOFN->Flags & OFN_HIDEREADONLY)
    {
        HideControl(hDlg, chx1);
    }
    else
    {
        CheckDlgButton(hDlg, chx1, (lpOFN->Flags & OFN_READONLY) ? 1 : 0);
    }

    if (!(lpOFN->Flags & OFN_SHOWHELP))
    {
        HideControl(hDlg, pshHelp);
    }

    if (!pDlgStruct->CreateHookDialog(nCtlsBottom))
    {
        return (FALSE);
    }

    //
    //  Explicitly set the focus since this is no longer the first item in the
    //  dialog template.
    //
    SetFocus(GetDlgItem(hDlg, edt1));

    //
    //  Check out if the filename contains a path.  If so, override whatever
    //  is contained in lpstrInitialDir.  Chop off the path and put up only
    //  the filename.
    //
    LPCTSTR lpstrInitialDir = lpOFN->lpstrInitialDir;
    LPTSTR lpInitialText = lpOFN->lpstrFile;

    if (lpInitialText && *lpInitialText)
    {
    //  StringLower(lpInitialText);
        if ( DBL_BSLASH(lpInitialText + 2) &&
             (*(lpInitialText + 1) == CHAR_COLON) )
        {
            lstrcpy(lpInitialText, lpInitialText + 2);
        }

        int nFileOffset;

        lstrcpyn(szScratch, lpInitialText, ARRAYSIZE(szScratch));

        nFileOffset = ParseFileNew(szScratch, NULL, FALSE);

        //
        //  Is the filename invalid?
        //
        if ( !(lpOFN->Flags & OFN_NOVALIDATE) &&
             (nFileOffset < 0) &&
             (nFileOffset != PARSE_EMPTYSTRING) )
        {
            StoreExtendedError(FNERR_INVALIDFILENAME);
            return (FALSE);
        }

        //
        //  It all looks valid.  I need to use to the original text because
        //  ParseFile does too much modifying.
        //
        PathRemoveBlanks(lpInitialText);
        LPTSTR pszFileName = PathFindFileName(lpInitialText);

        if (nFileOffset >= 0 && IsWild(pszFileName))
        {
            pDlgStruct->SetCurrentFilter(pszFileName);
        }

        nFileOffset = pszFileName - lpInitialText;
        if (nFileOffset > 0)
        {
            CopyMemory(szScratch, lpInitialText, nFileOffset * sizeof(TCHAR));
            szScratch[nFileOffset] = CHAR_NULL;
            PathRemoveBslash(szScratch);

            //
            //  If there is no specified initial dir or it is the same as the
            //  dir of the file, use the dir of the file and strip the path
            //  off the file.
            //
            if (!lpstrInitialDir || !lpstrInitialDir[0]
                || lstrcmpi(lpstrInitialDir, szScratch) == 0)
            {
                lpstrInitialDir = szScratch;
                lpInitialText = lpInitialText + nFileOffset;
            }
        }
    }

    if (lpstrInitialDir && *lpstrInitialDir)
    {
        //
        //  It's better to come up somewhere rather than just tell the app
        //  they have a bogus directory set.
        //
        pDlgStruct->JumpToPath(lpstrInitialDir, TRUE);
    }

    //
    //  This checks if the previous jump failed.
    //
    if (!pDlgStruct->psv)
    {
        //
        //  If we tried to set the dir above and it failed, we should
        //  probably always show the full path of the file.  If we didn't try
        //  above, then we are already showing the full path, so no problem.
        //
        lpInitialText = lpOFN->lpstrFile;
        pDlgStruct->JumpToPath(TEXT("."), TRUE);
    }

    if (!pDlgStruct->psv)
    {
        //
        //  Maybe the curdir has been deleted; try the desktop.
        //
        ITEMIDLIST idl = { 0 };

        //
        //  Do not try to translate this.
        //
        pDlgStruct->JumpToIDList(&idl, FALSE);
    }

    if (!pDlgStruct->psv)
    {
        //
        //  This would be very bad.
        //
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    //
    //  Show the window after creating the ShellView so we do not get a
    //  big ugly gray spot.
    //
    ::ShowWindow(hDlg, SW_SHOW);
    ::UpdateWindow(hDlg);

    if (lpInitialText)
    {
        //
        //  This is the one time I will show a file spec, since it would be
        //  too strange to have "All Files" showing in the Type box, while
        //  only text files are in the view.
        //
        pDlgStruct->SetEditFile(lpInitialText, pDlgStruct->fShowExtensions, FALSE);
        SelectEditText(hDlg);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CleanupDialog
//
//  Dialog cleanup, memory deallocation.
//
////////////////////////////////////////////////////////////////////////////

void CleanupDialog(
    HWND hDlg,
    BOOL fRet)
{
    CFileOpenBrowser *pDlgStruct = HwndToBrowser(hDlg);

    //
    //  Return the most recently used filter.
    //
    LPOPENFILENAME lpOFN = pDlgStruct->lpOFN;

    if (lpOFN->lpstrCustomFilter)
    {
        UINT len = lstrlen(lpOFN->lpstrCustomFilter) + 1;
        UINT sCount = lstrlen(pDlgStruct->szLastFilter);
        if (lpOFN->nMaxCustFilter > sCount + len)
        {
            lstrcpy(lpOFN->lpstrCustomFilter + len, pDlgStruct->szLastFilter);
        }
    }

    if ( (fRet == TRUE) &&
         pDlgStruct->hSubDlg &&
         ( CD_SendOKNotify(pDlgStruct->hSubDlg, hDlg, lpOFN, pDlgStruct->lpOFI) ||
           CD_SendOKMsg(pDlgStruct->hSubDlg, lpOFN, pDlgStruct->lpOFI) ) )
    {
        //
        //  Give the hook a chance to validate the file name.
        //
        return;
    }

    //
    //  We need to make sure the IShellBrowser is still around during
    //  destruction.
    //
    if (pDlgStruct->psv != NULL)
    {
        pDlgStruct->psv->DestroyViewWindow();
        pDlgStruct->psv->Release();

        pDlgStruct->psv = NULL;
    }

    if ((lpOFN->Flags & OFN_NOCHANGEDIR) && *pDlgStruct->szStartDir)
    {
        SetCurrentDirectory(pDlgStruct->szStartDir);
    }

    ::EndDialog(hDlg, fRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetParentItem
//
//  Given an item index in the location dropdown, get its parent item.
//
////////////////////////////////////////////////////////////////////////////

MYLISTBOXITEM *GetParentItem(HWND hwndCombo, int *piItem)
{
    int iItem = *piItem;
    MYLISTBOXITEM *pItem = GetListboxItem(hwndCombo, iItem);

    for (--iItem; iItem >= 0; iItem--)
    {
        MYLISTBOXITEM *pPrev = GetListboxItem(hwndCombo, iItem);
        if (pPrev->cIndent < pItem->cIndent)
        {
            *piItem = iItem;
            return (pPrev);
        }
    }

    return (NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFullPathEnumCB
//
////////////////////////////////////////////////////////////////////////////

BOOL GetFullPathEnumCB(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam)
{
    DWORD dwAttrs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;
    MYLISTBOXITEM *pLoc = that->pCurrentLocation;

    if (!pidl)
    {
        return (TRUE);
    }

    if ((SUCCEEDED(that->psfCurrent->GetAttributesOf(1, &pidl, &dwAttrs))) &&
        (dwAttrs & SFGAO_FILESYSTEM))
    {
        LPITEMIDLIST pidlFull;

        if (pLoc->pidlFull == NULL)
        {
            pidlFull = ILClone(pidl);
        }
        else
        {
            pidlFull = ILCombine(pLoc->pidlFull, pidl);
        }

        if (pidlFull != NULL)
        {
            SHGetPathFromIDList(pidlFull, (LPTSTR)lParam);
            SHFree(pidlFull);
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetFullPath
//
//  Calculate the full path to the selected object in the view.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::GetFullPath(
    LPTSTR pszBuf)
{
    *pszBuf = CHAR_NULL;

    EnumItemObjects(SVGIO_SELECTION, GetFullPathEnumCB, (LPARAM)pszBuf);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::RemoveOldPath
//
//  Removes old path elements from the location dropdown.  *piNewSel is the
//  listbox index of the leaf item which the caller wants to save.  All non-
//  permanent items that are not ancestors of that item are deleted.  The
//  index is updated appropriately if any items before it are deleted.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::RemoveOldPath(
    int *piNewSel)
{
    HWND hwndCombo = ::GetDlgItem(hwndDlg, cmb2);
    int iStart = *piNewSel;
    int iItem;
    UINT cIndent = 0;
    int iSubOnDel = 0;

    //
    //  Flush all non-permanent non-ancestor items before this one.
    //
    for (iItem = ComboBox_GetCount(hwndCombo) - 1; iItem >= 0; --iItem)
    {
        MYLISTBOXITEM *pItem = GetListboxItem(hwndCombo, iItem);

        if (iItem == iStart)
        {
            //
            //  Begin looking for ancestors and adjusting the sel position.
            //
            iSubOnDel = 1;
            cIndent = pItem->cIndent;
            continue;
        }

        if (pItem->cIndent < cIndent)
        {
            //
            //  We went back a level, so this must be an ancestor of the
            //  selected item.
            //
            cIndent = pItem->cIndent;
            continue;
        }

        //
        //  Make sure to check this after adjusting cIndent.
        //
        if (pItem->dwFlags & MLBI_PERMANENT)
        {
            continue;
        }

        SendMessage(hwndCombo, CB_DELETESTRING, iItem, NULL);
        delete pItem;
        *piNewSel -= iSubOnDel;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  FindLocation
//
//  Given a listbox item, find the index.
//  Just a linear search, but we shouldn't have more than ~10-20 items.
//
////////////////////////////////////////////////////////////////////////////

int FindLocation(
    HWND hwndCombo,
    MYLISTBOXITEM *pFindItem)
{
    int iItem;

    for (iItem = ComboBox_GetCount(hwndCombo) - 1; iItem >= 0; --iItem)
    {
        MYLISTBOXITEM *pItem = GetListboxItem(hwndCombo, iItem);

        if (pItem == pFindItem)
        {
            break;
        }
    }

    return (iItem);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnSelChange
//
//  Process the selection change in the location dropdown.
//
//  Chief useful feature is that it removes the items for the old path.
//  Returns TRUE only if it was possible to switch to the specified item.
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::OnSelChange(
    int iItem,
    BOOL bForceUpdate)
{
    HWND hwndCombo = GetDlgItem(hwndDlg, cmb2);
    BOOL bRet = TRUE;

    if (iItem == -1)
    {
        iItem = SendMessage(hwndCombo, CB_GETCURSEL, NULL, NULL);
    }

    MYLISTBOXITEM *pNewLocation = GetListboxItem(hwndCombo, iItem);
    MYLISTBOXITEM *pOldLocation = pCurrentLocation;
    BOOL bFirstTry = TRUE;
    BOOL bSwitchedBack = FALSE;

    if (bForceUpdate || pNewLocation != pOldLocation)
    {
        FOLDERSETTINGS fs;

        if (psv)
        {
            psv->GetCurrentInfo(&fs);
        }
        else
        {
            fs.ViewMode = FVM_LIST;
            fs.fFlags = lpOFN->Flags & OFN_ALLOWMULTISELECT ? 0 : FWF_SINGLESEL;
        }

        iCurrentLocation = iItem;
        pCurrentLocation = pNewLocation;

TryAgain:
        if (FAILED(SwitchView( pCurrentLocation->GetShellFolder(),
                               pCurrentLocation->pidlFull,
                               &fs )))
        {
            //
            //  We could not create the view for this location.
            //
            bRet = FALSE;

            //
            //  Try the previous folder.
            //
            if (bFirstTry)
            {
                bFirstTry = FALSE;
                pCurrentLocation = pOldLocation;
                int iOldItem = FindLocation(hwndCombo, pOldLocation);
                if (iOldItem >= 0)
                {
                    iCurrentLocation = iOldItem;
                    ComboBox_SetCurSel(hwndCombo, iCurrentLocation);

                    if (psv)
                    {
                        bSwitchedBack = TRUE;
                        goto SwitchedBack;
                    }
                    else
                    {
                        goto TryAgain;
                    }
                }
            }

            //
            //  Try the parent of the old item.
            //
            if (iCurrentLocation)
            {
                pCurrentLocation = GetParentItem(hwndCombo, &iCurrentLocation);
                ComboBox_SetCurSel(hwndCombo, iCurrentLocation);
                goto TryAgain;
            }

            //
            //  We cannot create the Desktop view.  I think we are in
            //  real trouble.  We had better bail out.
            //
            StoreExtendedError(CDERR_DIALOGFAILURE);
            CleanupDialog(hwndDlg, FALSE);
            return (FALSE);
        }

        ::SendMessage( hwndToolbar,
                       TB_SETSTATE,
                       IDC_PARENT,
                       iCurrentLocation ? TBSTATE_ENABLED : 0 );
        if (!iCurrentLocation || (pCurrentLocation->dwAttrs & SFGAO_FILESYSTEM))
        {
            pCurrentLocation->SwitchCurrentDirectory();
        }

        TCHAR szFile[MAX_PATH + 1];
        int nFileOffset;

        //
        //  We've changed folders; we'd better strip whatever is in the edit
        //  box down to the file name.
        //
        GetDlgItemText(hwndDlg, edt1, szFile, ARRAYSIZE(szFile));

        nFileOffset = ParseFileNew(szFile, NULL, FALSE);

        if (nFileOffset > 0 && !IsDirectory(szFile))
        {
            //
            //  The user may have typed an extension, so make sure to show it.
            //
            SetEditFile(szFile + nFileOffset, TRUE);
        }

        SetSaveButton(iszFileSaveButton);

SwitchedBack:
        RemoveOldPath(&iCurrentLocation);
    }

    if (!bSwitchedBack && hSubDlg)
    {
        CD_SendFolderChangeNotify(hSubDlg, hwndDlg, lpOFN, lpOFI);
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnDotDot
//
//  Process the open-parent-folder button on the toolbar.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::OnDotDot()
{
    HWND hwndCombo = GetDlgItem(hwndDlg, cmb2);

    int iItem = ComboBox_GetCurSel(hwndCombo);

    MYLISTBOXITEM *pItem = GetParentItem(hwndCombo, &iItem);

    SendMessage(hwndCombo, CB_SETCURSEL, iItem, NULL);

    //
    //  Delete old path from combo.
    //
    OnSelChange();
}


////////////////////////////////////////////////////////////////////////////
//
//  DblClkEnumCB
//
////////////////////////////////////////////////////////////////////////////

#define PIDL_NOTHINGSEL      (LPCITEMIDLIST)0
#define PIDL_MULTIPLESEL     (LPCITEMIDLIST)-1
#define PIDL_FOLDERSEL       (LPCITEMIDLIST)-2

BOOL DblClkEnumCB(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam)
{
    MYLISTBOXITEM *pLoc = that->pCurrentLocation;
    LPCITEMIDLIST *ppidl = (LPCITEMIDLIST *)lParam;

    if (!pidl)
    {
        pidl = *ppidl;

        if (pidl == PIDL_NOTHINGSEL)
        {
            //
            //  Nothing selected.
            //
            return (FALSE);
        }

        if (pidl == PIDL_MULTIPLESEL)
        {
            //
            //  More than one thing selected.
            //
            return (FALSE);
        }

        if (IsContainer(that->psfCurrent, pidl))
        {
            LPITEMIDLIST pidlFull = ILCombine(pLoc->pidlFull, pidl);

            if (pidlFull)
            {
                that->JumpToIDList(pidlFull);
                SHFree(pidlFull);
            }

            *ppidl = PIDL_FOLDERSEL;
        }

        return (FALSE);
    }

    if (*ppidl)
    {
        //
        //  More than one thing selected.
        //
        *ppidl = PIDL_MULTIPLESEL;
        return (FALSE);
    }

    *ppidl = pidl;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnDblClick
//
//  Process a double-click in the view control, either by choosing the
//  selected non-container object or by opening the selected container.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::OnDblClick(
    BOOL bFromOKButton)
{
    LPCITEMIDLIST pidlFirst = PIDL_NOTHINGSEL;

    if (psv)
    {
        EnumItemObjects(SVGIO_SELECTION, DblClkEnumCB, (LPARAM)&pidlFirst);
    }

    if (pidlFirst == PIDL_NOTHINGSEL)
    {
        //
        //  Nothing selected.
        //
        if (bFromOKButton)
        {
            //
            //  This means we got an IDOK when the focus was in the view,
            //  but nothing was selected.  Let's get the edit text and go
            //  from there.
            //
            ProcessEdit();
        }
    }
    else if (pidlFirst != PIDL_FOLDERSEL)
    {
        //
        //  This will change the edit box, but that's OK, since it probably
        //  already has.  This should take care of files with no extension.
        //
        SelFocusChange(TRUE);

        //
        //  This part will take care of resolving links.
        //
        ProcessEdit();
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::JumpToPath
//
//  Refocus the entire dialog on a different directory.
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::JumpToPath(
    LPCTSTR pszDirectory,
    BOOL bTranslate)
{
    BOOL bRet;
    TCHAR szTemp[MAX_PATH + 1];
    TCHAR szCurDir[MAX_PATH + 1];

    //
    //  This should do the whole job of canonicalizing the directory.
    //
    GetCurrentDirectory(ARRAYSIZE(szCurDir), szCurDir);
    PathCombine(szTemp, szCurDir, pszDirectory);

    LPITEMIDLIST pidlNew = ILCreateFromPath(szTemp);

    if (pidlNew == NULL)
    {
        return (FALSE);
    }

    bRet = JumpToIDList(pidlNew, bTranslate);

    SHFree(pidlNew);

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::JumpTOIDList
//
//  Refocus the entire dialog on a different IDList.
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::JumpToIDList(
    LPCITEMIDLIST pidlNew,
    BOOL bTranslate)
{
    LPITEMIDLIST pidlLog = NULL;

    if (bTranslate)
    {
        //
        //  Translate IDList's on the Desktop into the appropriate
        //  logical IDList.
        //
        pidlLog = SHLogILFromFSIL(pidlNew);
        if (pidlLog)
        {
            pidlNew = pidlLog;
        }
    }

    //
    //  Find the entry in the location dropdown that is the closest parent
    //  to the new location.
    //
    HWND hwndCombo = ::GetDlgItem(hwndDlg, cmb2);
    MYLISTBOXITEM *pBestParent = GetListboxItem(hwndCombo, 0);
    int iBestParent = 0;
    LPCITEMIDLIST pidlRelative = pidlNew;

    UINT cIndent = 0;
    BOOL fExact = FALSE;
    for (UINT iItem = 0; ; iItem++)
    {
        MYLISTBOXITEM *pNextItem = GetListboxItem(hwndCombo, iItem);
        if (pNextItem == NULL)
        {
            break;
        }
        if (pNextItem->cIndent != cIndent)
        {
            //
            //  Not the depth we want.
            //
            continue;
        }
        if (ILIsEqual(pNextItem->pidlFull, pidlNew))
        {
            fExact = TRUE;
            break;
        }
        LPCITEMIDLIST pidlChild = ILFindChild(pNextItem->pidlFull, pidlNew);
        if (pidlChild != NULL)
        {
            pBestParent = pNextItem;
            iBestParent = iItem;
            cIndent++;
            pidlRelative = pidlChild;
        }
    }

    //
    //  The path provided might have matched an existing item exactly.  In
    //  that case, just select the item.
    //
    if (fExact)
    {
        goto FoundIDList;
    }

    //
    //  Now, pBestParent is the closest parent to the item, iBestParent is
    //  its index, and cIndent is the next appropriate indent level.  Begin
    //  creating new items for the rest of the path.
    //
    iBestParent++;                // begin inserting after parent item
    for ( ; ; )
    {
        LPITEMIDLIST pidlFirst = ILCloneFirst(pidlRelative);
        if (pidlFirst == NULL)
        {
            break;
        }
        MYLISTBOXITEM *pNewItem =
            new MYLISTBOXITEM( pBestParent,
                               pBestParent->GetShellFolder(),
                               pidlFirst,
                               cIndent,
                               MLBI_PSFFROMPARENT );
        if (pNewItem == NULL)
        {
            break;
        }
        GetViewItemText( pBestParent->psfSub,
                         pidlFirst,
                         szBuf,
                         ARRAYSIZE(szBuf),
                         FALSE );
        InsertItem(hwndCombo, iBestParent, pNewItem, szBuf);
        SHFree(pidlFirst);
        pidlRelative = ILGetNext(pidlRelative);
        if (ILIsEmpty(pidlRelative))
        {
            break;
        }
        cIndent++;                // next one is indented one more level
        iBestParent++;            // and inserted after this one
        pBestParent = pNewItem;   // and is a child of the one we just inserted
    }

    iItem = iBestParent;

FoundIDList:

    if (pidlLog)
    {
        SHFree(pidlLog);
    }

    SendMessage(hwndCombo, CB_SETCURSEL, iItem, NULL);
    return (OnSelChange(iItem, TRUE));
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::ViewCommand
//
//  Process the new-folder button on the toolbar.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::ViewCommand(
    UINT uIndex)
{
    IContextMenu *pcm;

    if (SUCCEEDED(psv->GetItemObject( SVGIO_BACKGROUND,
                                      IID_IContextMenu,
                                      (LPVOID *)&pcm )))
    {
        CMINVOKECOMMANDINFOEX ici;

        ici.cbSize = sizeof(ici);
        ici.fMask = 0L;
        ici.hwnd = hwndDlg;
        ici.lpVerb = ::c_szCommandsA[uIndex];
        ici.lpParameters = NULL;
        ici.lpDirectory = NULL;
        ici.nShow = SW_NORMAL;
        ici.lpParametersW = NULL;
        ici.lpDirectoryW = NULL;

#ifdef UNICODE
        ici.lpVerbW = ::c_szCommandsW[uIndex];
        ici.fMask |= CMIC_MASK_UNICODE;
#endif

        HMENU hmContext = CreatePopupMenu();
        pcm->QueryContextMenu(hmContext, 0, 1, 256, 0);
        pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)(&ici));
        pcm->Release();
        DestroyMenu(hmContext);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::InitShellLink
//
////////////////////////////////////////////////////////////////////////////

HRESULT CFileOpenBrowser::InitShellLink(void)
{
    if (psl)
    {
        return (S_OK);
    }

    HRESULT hres = ICoCreateInstance( CLSID_ShellLink,
                                      IID_IShellLink,
                                      (LPLPVOID)&psl );

    if (FAILED(hres))
    {
        return (hres);
    }

    hres = psl->QueryInterface(IID_IPersistFile, (LPLPVOID)&ppf);
    if (FAILED(hres))
    {
        psl->Release();
        psl = NULL;
        return (hres);
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::ResolveLink
//
////////////////////////////////////////////////////////////////////////////

HRESULT CFileOpenBrowser::ResolveLink(
    LPCTSTR pszLink,
    LPTSTR pszFile,
    UINT cchFile,
    WIN32_FIND_DATA *pfd)
{
    HRESULT hres = InitShellLink();

    if (FAILED(hres))
    {
        return (hres);
    }

    WCHAR wszFile[MAX_PATH];

#ifdef UNICODE
    lstrcpyn(wszFile, pszLink, ARRAYSIZE(wszFile));
#else
    StrToOleStrN(wszFile, ARRAYSIZE(wszFile), pszLink, -1);
#endif

    hres = ppf->Load(wszFile, 0);
    if (FAILED(hres))
    {
        return (hres);
    }

    return (psl->GetPath(pszFile, cchFile, pfd, 0));
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::LinkMatchSpec
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::LinkMatchSpec(
    LPCITEMIDLIST pidl,
    LPCTSTR pszFile,
    LPCTSTR pszSpec)
{
    if (!IsLink(psfCurrent, pidl))
    {
        return (FALSE);
    }

    TCHAR szFile[MAX_PATH];
    TCHAR szLinkFile[MAX_PATH];
    WIN32_FIND_DATA fd = { 0, };

    PathCombine(szLinkFile, szCurDir, pszFile);
    if (ResolveLink(szLinkFile, szFile, MAX_PATH, &fd) != S_OK || !szFile[0])
    {
        return (FALSE);
    }

    if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
        PathMatchSpec(szFile, pszSpec))
    {
        return (TRUE);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MeasureDriveItems
//
//  Standard owner-draw code for the location dropdown.
//
////////////////////////////////////////////////////////////////////////////

#define MINIDRIVE_MARGIN     4
#define MINIDRIVE_WIDTH      (g_cxSmIcon)
#define MINIDRIVE_HEIGHT     (g_cySmIcon)
#define DRIVELIST_BORDER     3

void MeasureDriveItems(
    HWND hwndDlg,
    MEASUREITEMSTRUCT *lpmi)
{
    HDC hdc;
    HFONT hfontOld;
    int dyDriveItem;
    SIZE siz;

    hdc = GetDC(NULL);
    hfontOld = (HFONT)SelectObject( hdc,
                                    (HFONT)SendMessage( hwndDlg,
                                                        WM_GETFONT,
                                                        0,
                                                        0 ) );

    GetTextExtentPoint(hdc, TEXT("W"), 1, &siz);
    dyDriveItem = siz.cy;

    if (hfontOld)
    {
        SelectObject(hdc, hfontOld);
    }
    ReleaseDC(NULL, hdc);

    dyDriveItem += DRIVELIST_BORDER;
    if (dyDriveItem < MINIDRIVE_HEIGHT)
    {
        dyDriveItem = MINIDRIVE_HEIGHT;
    }

    lpmi->itemHeight = dyDriveItem;
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::PaintDriveLine
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::PaintDriveLine(
    DRAWITEMSTRUCT *lpdis)
{
    HDC hdc = lpdis->hDC;
    RECT rc = lpdis->rcItem;
    TCHAR szText[MAX_DRIVELIST_STRING_LEN];
    int offset = 0;
    int xString, yString, xMiniDrive, dyString;
    SIZE siz;

    if ((int)lpdis->itemID < 0)
    {
        return;
    }

    MYLISTBOXITEM *pItem = GetListboxItem(lpdis->hwndItem, lpdis->itemID);
    ::SendDlgItemMessage( hwndDlg,
                          cmb2,
                          CB_GETLBTEXT,
                          lpdis->itemID,
                          (LPARAM)szText );

    //
    //  Before doing anything, calculate the actual rectangle for the text.
    //
    if (!(lpdis->itemState & ODS_COMBOBOXEDIT))
    {
        offset = 10 * pItem->cIndent;
    }

    xMiniDrive = rc.left + DRIVELIST_BORDER + offset;
    rc.left = xString = xMiniDrive + MINIDRIVE_WIDTH + MINIDRIVE_MARGIN;
    GetTextExtentPoint(hdc, szText, lstrlen(szText), &siz);

    dyString = siz.cy;
    rc.right = rc.left + siz.cx;
    rc.left--;
    rc.right++;

    if (lpdis->itemAction != ODA_FOCUS)
    {
        FillRect(hdc, &lpdis->rcItem, GetSysColorBrush(COLOR_WINDOW));

        yString = rc.top + (rc.bottom - rc.top - dyString) / 2;

        SetBkColor( hdc,
                    GetSysColor( (lpdis->itemState & ODS_SELECTED)
                                     ? COLOR_HIGHLIGHT
                                     : COLOR_WINDOW ) );
        SetTextColor( hdc,
                      GetSysColor( (lpdis->itemState & ODS_SELECTED)
                                       ? COLOR_HIGHLIGHTTEXT
                                       : COLOR_WINDOWTEXT ) );

        if ((lpdis->itemState & ODS_COMBOBOXEDIT) &&
            (rc.right > lpdis->rcItem.right))
        {
            //
            //  Need to clip as user does not!
            //
            rc.right = lpdis->rcItem.right;
            ExtTextOut( hdc,
                        xString,
                        yString,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &rc,
                        szText,
                        lstrlen(szText),
                        NULL );
        }
        else
        {
            ExtTextOut( hdc,
                        xString,
                        yString,
                        ETO_OPAQUE,
                        &rc,
                        szText,
                        lstrlen(szText),
                        NULL );
        }

        ImageList_Draw( himl,
                        (lpdis->itemID == (UINT)iCurrentLocation)
                            ? pItem->iSelectedImage
                            : pItem->iImage,
                        hdc,
                        xMiniDrive,
                        rc.top + (rc.bottom - rc.top - MINIDRIVE_HEIGHT) / 2,
                        (pItem->IsShared()
                            ? INDEXTOOVERLAYMASK(IDOI_SHARE)
                            : 0) |
                        ((lpdis->itemState & ODS_SELECTED)
                            ? (ILD_SELECTED | ILD_FOCUS)
                            : ILD_NORMAL) );
    }

    if (lpdis->itemAction == ODA_FOCUS ||
        (lpdis->itemState & ODS_FOCUS))
    {
        DrawFocusRect(hdc, &rc);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::RefreshFilter
//
//  Refresh the view given any change in the user's choice of wildcard
//  filter.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::RefreshFilter(
    HWND hwndFilter)
{
    WAIT_CURSOR w;

    lpOFN->Flags &= ~OFN_FILTERDOWN;

    short nIndex = (short) SendMessage(hwndFilter, CB_GETCURSEL, 0, 0L);
    if (nIndex < 0)
    {
        //
        //  No current selection.
        //
        return;
    }

    BOOL bCustomFilter = lpOFN->lpstrCustomFilter && *lpOFN->lpstrCustomFilter;

    lpOFN->nFilterIndex = nIndex;
    if (!bCustomFilter)
    {
        lpOFN->nFilterIndex++;
    }

    LPTSTR lpFilter;

    //
    //  Must also check if filter contains anything.
    //
    lpFilter = (LPTSTR)ComboBox_GetItemData(hwndFilter, nIndex);

    if (*lpFilter)
    {
        SetCurrentFilter(lpFilter);

        //
        //  Provide dynamic pszDefExt updating when lpstrDefExt is app
        //  initialized.
        //
        if (!bNoInferDefExt && lpOFN->lpstrDefExt)
        {
            //
            //  We are looking for "foo*.ext[;...]".  We will grab ext as the
            //  default extension.  If not of this form, use the default
            //  extension passed in.
            //
            LPTSTR lpDot = mystrchr(lpFilter, CHAR_DOT);

            //
            //  Skip past the CHAR_DOT.
            //
            if (lpDot && pszDefExt.StrCpy(lpDot + 1))
            {
                LPTSTR lpSemiColon = mystrchr(pszDefExt, CHAR_SEMICOLON);
                if (lpSemiColon)
                {
                    *lpSemiColon = CHAR_NULL;
                }

                if (IsWild(pszDefExt))
                {
                    pszDefExt.StrCpy(lpOFN->lpstrDefExt);
                }
            }
            else
            {
                pszDefExt.StrCpy(lpOFN->lpstrDefExt);
            }
        }

        GetDlgItemText(hwndDlg, edt1, szBuf, ARRAYSIZE(szBuf));
        if (IsWild(szBuf))
        {
            //
            //  We should not show a filter that we are not using.
            //
            *szBuf = CHAR_NULL;
            SetEditFile(szBuf, TRUE);
        }

        if (psv)
        {
            psv->Refresh();
        }
    }

    if (hSubDlg)
    {
        if (!CD_SendTypeChangeNotify(hSubDlg, hwndDlg, lpOFN, lpOFI))
        {
            CD_SendLBChangeMsg(hSubDlg, cmb1, nIndex, CD_LBSELCHANGE, lpOFI->ApiType);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetDirectoryFromLB
//
//  Return the dropdown's directory and its length.
//  Set *pichRoot to the start of the path (C:\ or \\server\share\).
//
////////////////////////////////////////////////////////////////////////////

UINT CFileOpenBrowser::GetDirectoryFromLB(
    LPTSTR pszBuf,
    int *pichRoot)
{
    if (pCurrentLocation->pidlFull != NULL &&
        SHGetPathFromIDList(pCurrentLocation->pidlFull, pszBuf))
    {
        PathAddBackslash(pszBuf);
        LPTSTR pszBackslash = mystrchr(pszBuf + 2, CHAR_BSLASH);
        if (pszBackslash != NULL)
        {
            //
            //  For UNC paths, the "root" is on the next backslash.
            //
            if (DBL_BSLASH(pszBuf))
            {
                pszBackslash = mystrchr(pszBackslash + 1, CHAR_BSLASH);
            }
            UINT cchRet = lstrlen(pszBuf);
            *pichRoot = (pszBackslash != NULL) ? pszBackslash - pszBuf : cchRet;
            return (cchRet);
        }
    }
    *pichRoot = 0;
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::EnumItemObjects
//
////////////////////////////////////////////////////////////////////////////

typedef BOOL (*EIOCALLBACK)(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam);

BOOL CFileOpenBrowser::EnumItemObjects(
    UINT uItem,
    EIOCALLBACK pfnCallBack,
    LPARAM lParam)
{
    FORMATETC fmte = { g_cfCIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    BOOL bRet = FALSE;
    LPCITEMIDLIST pidl;
    LPIDA pida;
    int cItems, i;
    IDataObject *pdtobj;
    STGMEDIUM medium;

    if (!psv || FAILED(psv->GetItemObject( uItem,
                                           IID_IDataObject,
                                           (LPVOID *)&pdtobj )))
    {
        goto Error0;
    }

    if (FAILED(pdtobj->GetData(&fmte, &medium)))
    {
        goto Error1;
    }

    pida = (LPIDA)GlobalLock(medium.hGlobal);
    cItems = pida->cidl;

    for (i = 1; ; ++i)
    {
        if (i > cItems)
        {
            //
            //  We got to the end of the list without a failure.
            //  Call back one last time with NULL.
            //
            bRet = pfnCallBack(this, NULL, lParam);
            break;
        }

        pidl = LPIDL_GetIDList(pida, i);

        if (!pfnCallBack(this, pidl, lParam))
        {
            break;
        }
    }

    GlobalUnlock(medium.hGlobal);

    _ReleaseStgMedium(&medium);

Error1:
    pdtobj->Release();
Error0:
    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  FindNameEnumCB
//
////////////////////////////////////////////////////////////////////////////

#define FE_INVALID_VALUE     0x0000
#define FE_OUTOFMEM          0x0001
#define FE_TOOMANY           0x0002
#define FE_CHANGEDDIR        0x0003
#define FE_FILEERR           0x0004
#define FE_FOUNDNAME         0x0005

typedef struct _FINDNAMESTRUCT
{
    LPTSTR        pszFile;
    UINT          uRet;
    LPCITEMIDLIST pidlFound;
} FINDNAMESTRUCT;


BOOL FindNameEnumCB(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam)
{
    SHFILEINFO sfi;
    FINDNAMESTRUCT *pfns = (FINDNAMESTRUCT *)lParam;

    if (!pidl)
    {
        if (!pfns->pidlFound)
        {
            return (FALSE);
        }

        GetViewItemText( that->psfCurrent,
                         pfns->pidlFound,
                         pfns->pszFile,
                         MAX_PATH );

        if (IsContainer(that->psfCurrent, pfns->pidlFound))
        {
            LPITEMIDLIST pidlFull = ILCombine( that->pCurrentLocation->pidlFull,
                                               pfns->pidlFound );

            if (pidlFull)
            {
                if (that->JumpToIDList(pidlFull))
                {
                    pfns->uRet = FE_CHANGEDDIR;
                }
                else if (!that->psv)
                {
                    pfns->uRet = FE_OUTOFMEM;
                }
                SHFree(pidlFull);

                if (pfns->uRet != FE_INVALID_VALUE)
                {
                    return (TRUE);
                }
            }
        }

        pfns->uRet = FE_FOUNDNAME;
        return (TRUE);
    }

    if (!SHGetFileInfo( (LPCTSTR)pidl,
                        0,
                        &sfi,
                        0,
                        SHGFI_DISPLAYNAME | SHGFI_PIDL ))
    {
        //
        //  This will never happen, right?
        //
        return (TRUE);
    }

    if (lstrcmpi(sfi.szDisplayName, pfns->pszFile) != 0)
    {
        //
        //  Continue the enumeration.
        //
        return (TRUE);
    }

    if (!pfns->pidlFound)
    {
        pfns->pidlFound = pidl;

        //
        //  Continue looking for more matches.
        //
        return (TRUE);
    }

    //
    //  We already found a match, so select the first one and stop the search.
    //
    //  BUGBUG: The focus must be set to hwndView before changing selection or
    //  the GetItemObject may not work.
    //
    FORWARD_WM_NEXTDLGCTL(that->hwndDlg, that->hwndView, 1, SendMessage);
    that->psv->SelectItem( pfns->pidlFound,
                           SVSI_SELECT | SVSI_DESELECTOTHERS |
                               SVSI_ENSUREVISIBLE | SVSI_FOCUSED );

    pfns->pidlFound = NULL;
    pfns->uRet = FE_TOOMANY;

    //
    //  Stop enumerating.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CDPathQualify
//
////////////////////////////////////////////////////////////////////////////

void CDPathQualify(
    LPCTSTR lpFile,
    LPTSTR pszPathName)
{
    TCHAR szCurDir[MAX_PATH + 1];

    lstrcpy(pszPathName, lpFile);

    //
    //  This should do the whole job of canonicalizing the directory.
    //
    GetCurrentDirectory(ARRAYSIZE(szCurDir), szCurDir);
    PathCombine(pszPathName, szCurDir, pszPathName);
}


////////////////////////////////////////////////////////////////////////////
//
//  VerifyOpen
//
//  Returns:   0    success
//             !0   dos error code
//
////////////////////////////////////////////////////////////////////////////

int VerifyOpen(
    LPCTSTR lpFile,
    LPTSTR pszPathName)
{
    HANDLE hf;

    CDPathQualify(lpFile, pszPathName);

    hf = CreateFile( pszPathName,
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL );
    if (hf == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }
    else
    {
        CloseHandle(hf);
        return (0);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::IsKnownExtension
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::IsKnownExtension(
    LPCTSTR pszExtension)
{
    if ((LPTSTR)pszDefExt && lstrcmpi(pszExtension + 1, pszDefExt) == 0)
    {
        //
        //  It's the default extension, so no need to add it again.
        //
        return (TRUE);
    }

    if (RegQueryValue(HKEY_CLASSES_ROOT, pszExtension, NULL, 0) == ERROR_SUCCESS)
    {
        //
        //  It's a registered extension, so the user is trying to force
        //  the type.
        //
        return (TRUE);
    }

    if (lpOFN->lpstrFilter)
    {
        LPCTSTR pFilter = lpOFN->lpstrFilter;

        while (*pFilter)
        {
            //
            //  Skip visual.
            //
            pFilter = pFilter + lstrlen(pFilter) + 1;

            //
            //  Search extension list.
            //
            while (*pFilter)
            {
                //
                //  Check extensions of the form '*.ext' only.
                //
                if (*pFilter == CHAR_STAR && *(++pFilter) == CHAR_DOT)
                {
                    LPCTSTR pExt = pszExtension + 1;

                    pFilter++;

                    while (*pExt && *pExt == *pFilter)
                    {
#ifdef DBCS
                        if (IsDBCSLeadByte(*pExt))
                        {
                            if (*(pExt + 1) != *(pFilter + 1))
                            {
                                break;
                            }
                            pExt++;
                            pFilter++;
                        }
#endif
                        pExt++;
                        pFilter++;
                    }

                    if (!*pExt && (*pFilter == CHAR_SEMICOLON || !*pFilter))
                    {
                        //
                        //  We have a match.
                        //
                        return (TRUE);
                    }
                }

                //
                //  Skip to next extension.
                //
                while (*pFilter)
                {
                    TCHAR ch = *pFilter;
                    pFilter = CharNext(pFilter);
                    if (ch == CHAR_SEMICOLON)
                    {
                        break;
                    }
                }
            }

            //
            //  Skip extension string's terminator.
            //
            pFilter++;
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::FindNameInView
//
//  We will only resolve a link once.  If you have a link to a link, then
//  we will return the second link.
//
////////////////////////////////////////////////////////////////////////////

#define NUM_LINKLOOPS 1

UINT CFileOpenBrowser::FindNameInView(
    LPTSTR pszFile,
    OKBUTTONFLAGS Flags,
    LPTSTR pszPathName,
    int nFileOffset,
    int nExtOffset,
    int *pnErrCode,
    BOOL bTryAsDir)
{
    UINT uRet;
    FINDNAMESTRUCT fns =
    {
        pszFile,
        FE_INVALID_VALUE,
        NULL,
    };
    BOOL bGetOut = TRUE;
    BOOL bAddExt = FALSE;
    BOOL bHasExt = nExtOffset;
    TCHAR szTemp[MAX_PATH + 1];

    int nNewExt = lstrlen(pszFile);

    //
    //  If no extension, point at the end of the file name.
    //
    if (!nExtOffset)
    {
        nExtOffset = nNewExt;
    }

    //
    //  HACK: We could have a link that points to another link that points to
    //  another link, ..., that points back to the original file.  We will not
    //  loop more than NUM_LINKLOOPS times before giving up.

    int nLoop = NUM_LINKLOOPS;

    if (Flags & (OKBUTTON_NODEFEXT | OKBUTTON_QUOTED))
    {
        goto VerifyTheName;
    }

    if (bHasExt)
    {
        if (IsKnownExtension(pszFile + nExtOffset))
        {
            goto VerifyTheName;
        }

        //
        //  Don't attempt 2 extensions on SFN volume.
        //
        CDPathQualify(pszFile, pszPathName);
        if (!IsLFNDrive(pszPathName))
        {
            goto VerifyTheName;
        }
    }

    bGetOut = FALSE;

    if ((LPTSTR)pszDefExt &&
        ((DWORD)nNewExt + lstrlen(pszDefExt) < lpOFN->nMaxFile))
    {
        bAddExt = TRUE;

        //
        //  Note that we check lpstrDefExt to see if they want an automatic
        //  extension, but actually copy pszDefExt.
        //
        AppendExt(pszFile, pszDefExt, FALSE);

        //
        //  So we've added the default extension.  If there's a directory
        //  that matches this name, all attempts to open/create the file
        //  will fail, so simply change to the directory as if they had
        //  typed it in.  Note that by putting this test here, if there
        //  was a directory without the extension, we would have already
        //  switched to it.
        //

VerifyTheName:
        //
        //  Note that this also works for a UNC name, even on a net that
        //  does not support using UNC's directly.  It will also do the
        //  right thing for links to things.  We do not validate if we
        //  have not dereferenced any links, since that should have
        //  already been done.
        //
        if (bTryAsDir && SetDirRetry(pszFile, nLoop == NUM_LINKLOOPS))
        {
            return (FE_CHANGEDDIR);
        }

        *pnErrCode = VerifyOpen(pszFile, pszPathName);

        if (*pnErrCode == 0 || *pnErrCode == OF_SHARINGVIOLATION)
        {
            //
            //  This may be a link to something, so we should try to
            //  resolve it.
            //
            if (!(lpOFN->Flags & OFN_NODEREFERENCELINKS) && nLoop > 0)
            {
                --nLoop;

                LPITEMIDLIST pidl;
                DWORD dwAttr = SFGAO_LINK;
                HRESULT hRes;

                //
                //  ILCreateFromPath is slow (especially on a Net path),
                //  so just try to parse the name in the current folder if
                //  possible.
                //
                if (nFileOffset || nLoop < NUM_LINKLOOPS - 1)
                {
                    hRes = SHILCreateFromPath(pszPathName, &pidl, &dwAttr);
                }
                else
                {
                    WCHAR wszDisplayName[MAX_PATH + 1];
                    ULONG chEaten;

#ifdef UNICODE
                    lstrcpyn(wszDisplayName, pszFile, ARRAYSIZE(wszDisplayName));
#else
                    StrToOleStrN( wszDisplayName,
                                  ARRAYSIZE(wszDisplayName),
                                  pszFile,
                                  -1 );
#endif
                    hRes = psfCurrent->ParseDisplayName( NULL,
                                                         NULL,
                                                         wszDisplayName,
                                                         &chEaten,
                                                         &pidl,
                                                         &dwAttr );
                }

                if (SUCCEEDED(hRes))
                {
                    if (pidl)
                    {
                        SHFree(pidl);
                    }

                    if ((dwAttr & SFGAO_LINK) &&
                        ResolveLink( pszPathName,
                                     szTemp,
                                     ARRAYSIZE(szTemp),
                                     NULL ) == S_OK)
                    {
                        //
                        //  It was a link, and it "dereferenced" to something,
                        //  so we should try again with that new file.
                        //
                        lstrcpy(pszFile, szTemp);
                        goto VerifyTheName;
                    }
                }
            }

            return (FE_FOUNDNAME);
        }

        if (bGetOut ||
            (*pnErrCode != OF_FILENOTFOUND && *pnErrCode != OF_PATHNOTFOUND))
        {
            return (FE_FILEERR);
        }

        if (bSave)
        {
            //
            //  Do no more work if creating a new file.
            //
            return (FE_FOUNDNAME);
        }
    }

    //
    //  Make sure we do not loop forever.
    //
    bGetOut = TRUE;

    if (bSave)
    {
        //
        //  Do no more work if creating a new file.
        //
        goto VerifyTheName;
    }

    pszFile[nNewExt] = CHAR_NULL;

    if (bTryAsDir && nFileOffset)
    {
        TCHAR cSave = *(pszFile + nFileOffset);
        *(pszFile + nFileOffset) = CHAR_NULL;

        //
        //  We need to have the view on the dir with the file to do the
        //  next steps.
        //
        BOOL bOK = JumpToPath(pszFile);
        *(pszFile + nFileOffset) = cSave;

        if (!psv)
        {
            //
            //  We're dead.
            //
            return (FE_OUTOFMEM);
        }

        if (bOK)
        {
            lstrcpy(pszFile, pszFile + nFileOffset);
            nNewExt -= nFileOffset;
            SetEditFile(pszFile, TRUE);
        }
        else
        {
            *pnErrCode = OF_PATHNOTFOUND;
            return (FE_FILEERR);
        }
    }

    EnumItemObjects(SVGIO_ALLVIEW, FindNameEnumCB, (LPARAM)&fns);
    switch (fns.uRet)
    {
        case ( FE_INVALID_VALUE ) :
        {
            break;
        }
        case ( FE_FOUNDNAME ) :
        {
            goto VerifyTheName;
        }
        default :
        {
            uRet = fns.uRet;
            goto VerifyAndRet;
        }
    }

#ifdef FIND_FILES_NOT_IN_VIEW
    //
    //  We still have not found a match, so try enumerating files we cannot
    //  see.
    //
    int nFilterLen;

    nFilterLen = lstrlen(szLastFilter);
    if (nFilterLen + nNewExt + ARRAYSIZE(szDotStar) < ARRAYSIZE(szLastFilter))
    {
        TCHAR szSaveFilter[ARRAYSIZE(szLastFilter)];

        lstrcpy(szSaveFilter, szLastFilter);

        //
        //  Add "Joe.*" to the current filter and refresh.
        //
        szLastFilter[nFilterLen] = CHAR_SEMICOLON;
        lstrcpy(szLastFilter+nFilterLen + 1, pszFile);
        lstrcat(szLastFilter, szDotStar);

        HANDLE hf;
        WIN32_FIND_DATA fd;

        //
        //  Make sure we are in a FileSystem folder, and then see if at least
        //  one file named "Joe.*" exists.
        //
        //  Note we always set the current directory.
        //
        if ((pCurrentLocation->dwAttrs & SFGAO_FILESYSTEM) &&
            (hf = FindFirstFile( szLastFilter + nFilterLen + 1,
                                 &fd)) !=INVALID_HANDLE_VALUE)
        {
            psv->Refresh();

            fns.pidlFound = NULL;
            EnumItemObjects(SVGIO_ALLVIEW, FindNameEnumCB, (LPARAM)&fns);

            FindClose(hf);
        }

        //
        //  We must not restore the filter until AFTER getting the
        //  EnumItemObjects in case there is a background thread doing the
        //  enumeration.  ALLVIEW should cause the threads to sync up before
        //  returning.
        //
        lstrcpy(szLastFilter, szSaveFilter);

        switch (fns.uRet)
        {
            case ( FE_INVALID_VALUE ) :
            {
                break;
            }
            case ( FE_FOUNDNAME ) :
            {
                goto VerifyTheName;
            }
            default :
            {
                uRet = fns.uRet;
                goto VerifyAndRet;
            }
        }
    }
#endif

    if (bAddExt)
    {
        //
        //  Before we fail, check to see if the file typed sans default
        //  extension exists.
        //
        *pnErrCode = VerifyOpen(pszFile, pszPathName);
        if (*pnErrCode == 0 || *pnErrCode == OF_SHARINGVIOLATION)
        {
            //
            //  We will never hit this case for links (because they
            //  have registered extensions), so we don't need
            //  to goto VerifyTheName (which also calls VerifyOpen again).
            //
            return (FE_FOUNDNAME);
        }

        //
        //  I still can't find it?  Try adding the default extension and
        //  return failure.
        //
        AppendExt(pszFile, pszDefExt, FALSE);
    }

    uRet = FE_FILEERR;

VerifyAndRet:
    *pnErrCode = VerifyOpen(pszFile, pszPathName);
    return (uRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetDirRetry
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::SetDirRetry(
    LPTSTR pszDir,
    BOOL bNoValidate)
{
    if (SetCurrentDirectory(pszDir))
    {
JumpThere:
        JumpToPath(TEXT("."));
        return (TRUE);
    }

    if (bNoValidate || !IsUNC(pszDir))
    {
        return (FALSE);
    }


    //
    //  It may have been a password problem, so try to add the connection.
    //  Note that if we are on a net that does not support CD'ing to UNC's
    //  directly, this call will connect it to a drive letter.
    //
    if (!SHValidateUNC(hwndDlg, pszDir, 0))
    {
        switch (GetLastError())
        {
            case ERROR_CANCELLED:
            {
                //
                //  We don't want to put up an error message if they
                //  canceled the password dialog.
                //
                return (TRUE);
            }
            default:
            {
                //
                //  Some other error we don't know about.
                //
                return (FALSE);
            }
        }
    }

    //
    //  We connected to it, so try to switch to it again.
    //
    if (SetCurrentDirectory(pszDir))
    {
        goto JumpThere;
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::MultiSelectOKButton
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::MultiSelectOKButton(
    LPCTSTR pszFiles,
    OKBUTTONFLAGS Flags)
{
    TCHAR szPathName[MAX_PATH];
    int nErrCode;
    LPTSTR pchRead, pchWrite;
    UINT cch, cchCurDir, cchFiles;

    //
    //  This doesn't really mean anything for multiselection.
    //
    lpOFN->nFileExtension = 0;

    if (!lpOFN->lpstrFile)
    {
        return (TRUE);
    }

    //
    //  Check for space for first full path element.
    //
    cchCurDir = lstrlen(szCurDir) + 1;
    cchFiles  = lstrlen(pszFiles) + 1;
    cch = cchCurDir + cchFiles;

    if (cch > (UINT)lpOFN->nMaxFile)
    {
        //
        //  Buffer is too small, so return the size of the buffer
        //  required to hold the string.
        //
        //  cch is not really the number of characters needed, but it
        //  should be close.
        //
        if (lpOFN->nMaxFile >= 3)
        {
#ifdef UNICODE
            lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
            lpOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
#else
            lpOFN->lpstrFile[0] = (TCHAR)LOBYTE(cch);
            lpOFN->lpstrFile[1] = (TCHAR)HIBYTE(cch);
#endif
            lpOFN->lpstrFile[2] = CHAR_NULL;
        }
        else
        {
#ifdef UNICODE
            lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
            if (lpOFN->nMaxFile == 2)
            {
                lpOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
            }
#else
            lpOFN->lpstrFile[0] = LOBYTE(cch);
            if (lpOFN->nMaxFile == 2)
            {
                lpOFN->lpstrFile[1] = HIBYTE(cch);
            }
#endif
        }

        return (TRUE);
    }

    TEMPSTR psFiles(cchFiles);
    pchRead = psFiles;
    if (!pchRead)
    {
        //
        //  Out of memory.
        //  BUGBUG: There should be some sort of error message here.
        //
        return (FALSE);
    }

    //
    //  Copy in the full path as the first element.
    //
    lstrcpy(lpOFN->lpstrFile, szCurDir);

    //
    //  Set nFileOffset to 1st file.
    //
    lpOFN->nFileOffset = cchCurDir;
    pchWrite = lpOFN->lpstrFile + cchCurDir;

    //
    //  We know there is enough room for the whole string.
    //
    lstrcpy(pchRead, pszFiles);

    //
    //  This should only compact the string.
    //
    if (!ConvertToNULLTerm(pchRead))
    {
        return (FALSE);
    }

    for ( ; *pchRead; pchRead += lstrlen(pchRead) + 1)
    {
        int nFileOffset, nExtOffset;
        TCHAR szBasicPath[MAX_PATH];

        lstrcpy(szBasicPath, pchRead);

        nFileOffset = ParseFileNew(szBasicPath, &nExtOffset, FALSE);

        if (nFileOffset < 0)
        {
            InvalidFileWarning(hwndDlg, pchRead, nFileOffset);
            return (FALSE);
        }

        //
        //  Pass in 0 for the file offset to make sure we do not switch
        //  to another folder.
        //
        switch (FindNameInView( szBasicPath,
                                Flags,
                                szPathName,
                                nFileOffset,
                                nExtOffset,
                                &nErrCode,
                                FALSE ))
        {
            case ( FE_OUTOFMEM ) :
            case ( FE_CHANGEDDIR ) :
            {
                return (FALSE);
            }
            case ( FE_TOOMANY ) :
            {
                CDMessageBox( hwndDlg,
                              iszTooManyFiles,
                              MB_OK | MB_ICONEXCLAMATION,
                              pchRead );
                return (FALSE);
            }
            default :
            {
                break;
            }
        }

        if ( nErrCode &&
             ( (lpOFN->Flags & OFN_FILEMUSTEXIST) ||
               (nErrCode != OF_FILENOTFOUND) ) &&
             ( (lpOFN->Flags & OFN_PATHMUSTEXIST) ||
               (nErrCode != OF_PATHNOTFOUND) ) &&
             ( !(lpOFN->Flags & OFN_SHAREAWARE) ||
               (nErrCode != OF_SHARINGVIOLATION) ) )
        {
            if ((nErrCode == OF_SHARINGVIOLATION) && hSubDlg)
            {
                int nShareCode = CD_SendShareNotify( hSubDlg,
                                                     hwndDlg,
                                                     szPathName,
                                                     lpOFN,
                                                     lpOFI );

                if (nShareCode == OFN_SHARENOWARN)
                {
                    return (FALSE);
                }
                else if (nShareCode == OFN_SHAREFALLTHROUGH)
                {
                    goto EscapedThroughShare;
                }
                else
                {
                    //
                    //  They might not have handled the notification, so try
                    //  the registered message.
                    //
                    nShareCode = CD_SendShareMsg(hSubDlg, szPathName, lpOFI->ApiType);

                    if (nShareCode == OFN_SHARENOWARN)
                    {
                        return (FALSE);
                    }
                    else if (nShareCode == OFN_SHAREFALLTHROUGH)
                    {
                        goto EscapedThroughShare;
                    }
                }
            }
            else if (nErrCode == OF_ACCESSDENIED)
            {
                szPathName[0] |= 0x60;
                if (GetDriveType(szPathName) != DRIVE_REMOVABLE)
                {
                    nErrCode = OF_NETACCESSDENIED;
                }
            }

            //
            //  BUGBUG: These will never be set.
            //
            if ((nErrCode == OF_WRITEPROTECTION) ||
                (nErrCode == OF_DISKFULL)        ||
                (nErrCode == OF_ACCESSDENIED))
            {
                *pchRead = szPathName[0];
            }

MultiWarning:
            InvalidFileWarning(hwndDlg, pchRead, nErrCode);
            return (FALSE);
        }

EscapedThroughShare:
        if (nErrCode == 0)
        {
            //
            //  Successfully opened.
            //
            if ((lpOFN->Flags & OFN_NOREADONLYRETURN) &&
                (GetFileAttributes(szPathName) & FILE_ATTRIBUTE_READONLY))
            {
                nErrCode = OF_LAZYREADONLY;
                goto MultiWarning;
            }

            if ((bSave || (lpOFN->Flags & OFN_NOREADONLYRETURN)) &&
                (nErrCode = WriteProtectedDirCheck(szPathName)))
            {
                goto MultiWarning;
            }

            if (lpOFN->Flags & OFN_OVERWRITEPROMPT)
            {
                if (bSave && !FOkToWriteOver(hwndDlg, szPathName))
                {
                    PostMessage( hwndDlg,
                                 WM_NEXTDLGCTL,
                                 (WPARAM)GetDlgItem(hwndDlg, edt1),
                                 1 );
                    return (FALSE);
                }
            }
        }

        //
        //  Add some more in case the file name got larger.
        //
        cch += lstrlen(szBasicPath) - lstrlen(pchRead);
        if (cch > (UINT)lpOFN->nMaxFile)
        {
            //
            //  Buffer is too small, so return the size of the buffer
            //  required to hold the string.
            //
            if (lpOFN->nMaxFile >= 3)
            {
#ifdef UNICODE
                lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                lpOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
#else
                lpOFN->lpstrFile[0] = (TCHAR)LOBYTE(cch);
                lpOFN->lpstrFile[1] = (TCHAR)HIBYTE(cch);
#endif
                lpOFN->lpstrFile[2] = CHAR_NULL;
            }
            else
            {
#ifdef UNICODE
                lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                if (lpOFN->nMaxFile == 2)
                {
                    lpOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
                }
#else
                lpOFN->lpstrFile[0] = LOBYTE(cch);
                if (lpOFN->nMaxFile == 2)
                {
                    lpOFN->lpstrFile[1] = HIBYTE(cch);
                }
#endif
            }

            return (TRUE);
        }

        //
        //  We already know we have anough room.
        //
        lstrcpy(pchWrite, szBasicPath);
        pchWrite += lstrlen(pchWrite) + 1;
    }

    //
    //  double-NULL terminate.
    //
    *pchWrite = CHAR_NULL;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OKButtonPressed
//
//  Process the OK button being pressed.  This may involve jumping to a path,
//  changing the filter, actually choosing a file to open or save as, or who
//  knows what else.
//
//  Note:  There are 4 cases for validation of a file name:
//    1) OFN_NOVALIDATE        Allows invalid characters
//    2) No validation flags   No invalid characters, but path need not exist
//    3) OFN_PATHMUSTEXIST     No invalid characters, path must exist
//    4) OFN_FILEMUSTEXIST     No invalid characters, path & file must exist
//
////////////////////////////////////////////////////////////////////////////

#define PathFileExists(lpszPath) (GetFileAttributes(lpszPath) != (UINT)-1)

BOOL CFileOpenBrowser::OKButtonPressed(
    LPCTSTR pszFile,
    OKBUTTONFLAGS Flags)
{
    TCHAR szPathName[MAX_PATH];
    TCHAR szBasicPath[MAX_PATH];
    int nErrCode;
    ECODE eCode = ECODE_S_OK;
    int cch;
    int nFileOffset, nExtOffset, nOldExt;
    TCHAR ch;
    BOOL bAddExt = FALSE;
    BOOL bUNCName = FALSE;
    int nTempOffset;
    BOOL bIsDir;

    if (mystrchr(pszFile, CHAR_QUOTE) && (lpOFN->Flags & OFN_ALLOWMULTISELECT))
    {
        return (MultiSelectOKButton(pszFile, Flags));
    }

#ifdef UNICODE
    if (pszFile[1] == CHAR_COLON || DBL_BSLASH(pszFile))
#else
    if ((!IsDBCSLeadByte(pszFile[0]) && pszFile[1] == CHAR_COLON) ||
        DBL_BSLASH(pszFile))
#endif
    {
        //
        //  If a drive or UNC was specified, use it.
        //
        lstrcpyn(szBasicPath, pszFile, ARRAYSIZE(szBasicPath) - 1);
        nTempOffset = 0;
    }
    else
    {
        //
        //  Grab the directory from the listbox.
        //
        cch = GetDirectoryFromLB(szBasicPath, &nTempOffset);

        if (pszFile[0] == CHAR_BSLASH)
        {
            //
            //  If a directory from the root was given, put it
            //  immediately off the root (\\server\share or a:).
            //
            lstrcpyn( szBasicPath + nTempOffset,
                      pszFile,
                      ARRAYSIZE(szBasicPath) - nTempOffset - 1 );
        }
        else
        {
            //
            //  Tack the file to the end of the path.
            //
            lstrcpyn(szBasicPath + cch, pszFile, ARRAYSIZE(szBasicPath) - cch - 1);
        }
    }

    nFileOffset = ParseFileOld(szBasicPath, &nExtOffset, &nOldExt, FALSE);

    if (nFileOffset == PARSE_EMPTYSTRING)
    {
        if (psv)
        {
            psv->Refresh();
        }
        return (FALSE);
    }
    else if ((nFileOffset != PARSE_DIRECTORYNAME) &&
             (lpOFN->Flags & OFN_NOVALIDATE))
    {
        lpOFN->nFileOffset = nFileOffset;
        lpOFN->nFileExtension = nOldExt;
        if (lpOFN->lpstrFile)
        {
            cch = lstrlen(szBasicPath);
            if (cch <= LOWORD(lpOFN->nMaxFile))
            {
                lstrcpy(lpOFN->lpstrFile, szBasicPath);
            }
            else
            {
                //
                //  Buffer is too small, so return the size of the buffer
                //  required to hold the string.
                //
                if (lpOFN->nMaxFile >= 3)
                {
                    //
                    //  For single file requests, we will never go over 64K
                    //  because the filesystem is limited to 256.
                    //
#ifdef UNICODE
                    lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                    lpOFN->lpstrFile[1] = CHAR_NULL;
#else
                    lpOFN->lpstrFile[0] = LOBYTE(cch);
                    lpOFN->lpstrFile[1] = HIBYTE(cch);
                    lpOFN->lpstrFile[2] = 0;
#endif
                }
                else
                {
#ifdef UNICODE
                    lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                    if (lpOFN->nMaxFile == 2)
                    {
                        lpOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
                    }
#else
                    lpOFN->lpstrFile[0] = LOBYTE(cch);
                    if (lpOFN->nMaxFile == 2)
                    {
                        lpOFN->lpstrFile[1] = HIBYTE(cch);
                    }
#endif
                }
            }
        }
        return (TRUE);
    }
    else if (nFileOffset == PARSE_DIRECTORYNAME)
    {
        //
        //  See if it ends in slash.
        //
        if (ISBACKSLASH(szBasicPath, nExtOffset - 1))
        {
            //
            //  "\\server\share\" and "c:\" keep the trailing backslash,
            //  all other paths remove the trailing backslash. Note that
            //  we don't remove the slash if the user typed the path directly
            //  (nTempOffset is 0 in that case).
            //
            if ((nExtOffset != 1) &&
                (szBasicPath[nExtOffset - 2] != CHAR_COLON) &&
                (nExtOffset != nTempOffset + 1))
            {
                szBasicPath[nExtOffset - 1] = CHAR_NULL;
            }
        }
        else if ( (szBasicPath[nExtOffset - 1] == CHAR_DOT) &&
                  ( (szBasicPath[nExtOffset - 2] == CHAR_DOT) ||
                    ISBACKSLASH(szBasicPath, nExtOffset - 2) ) &&
                  IsUNC(szBasicPath) )
        {
            //
            //  Add a trailing slash to UNC paths ending with ".." or "\."
            //
            szBasicPath[nExtOffset] = CHAR_BSLASH;
            szBasicPath[nExtOffset + 1] = CHAR_NULL;
        }

        //
        //  Fall through to Directory Checking.
        //
    }
    else if (nFileOffset < 0)
    {
        nErrCode = nFileOffset;

        //
        //  I don't recognize this, so try to jump there.
        //  This is where servers get processed.
        //
        if (JumpToPath(szBasicPath))
        {
            return (FALSE);
        }

        //
        //  Fall through to the rest of the processing to warn the user.
        //

Warning:
        if (bUNCName)
        {
            cch = lstrlen(szBasicPath) - 1;
            if ((szBasicPath[cch] == CHAR_BSLASH) &&
                (szBasicPath[cch - 1] == CHAR_DOT) &&
                (ISBACKSLASH(szBasicPath, cch - 2)))
            {
                szBasicPath[cch - 2] = CHAR_NULL;
            }
        }
        else if ((nFileOffset == 2) && (szBasicPath[2] == CHAR_DOT))
        {
            lstrcpy(szBasicPath + 2, szBasicPath + 4);
        }

        //  If the disk is not a floppy and they tell me there's no
        //  disk in the drive, don't believe them.  Instead, put up the
        //  error message that they should have given us.  (Note that the
        //  error message is checked first since checking the drive type
        //  is slower.)
        //

        //
        //  I will assume that if we get error 0 or 1 or removable
        //  that we will assume removable.
        //
        if (nErrCode == OF_ACCESSDENIED)
        {
            szPathName[0] |= 0x60;
            if (bUNCName || GetDriveType(szPathName) <= DRIVE_REMOVABLE)
            {
                nErrCode = OF_NETACCESSDENIED;
            }
        }

        if ((nErrCode == OF_WRITEPROTECTION) ||
            (nErrCode == OF_DISKFULL)        ||
            (nErrCode == OF_ACCESSDENIED))
        {
            szBasicPath[0] = szPathName[0];
        }
        InvalidFileWarning(hwndDlg, szBasicPath, nErrCode);
        return (FALSE);
    }

    //
    //  We either have a file pattern or a real file.
    //    If it's a UNC name
    //        (1) Fall through to file name testing
    //    Else if it's a directory
    //        (1) Add on default pattern
    //        (2) Act like it's a pattern (goto pattern (1))
    //    Else if it's a pattern
    //        (1) Update everything
    //        (2) display files in whatever dir we're now in
    //    Else if it's a file name!
    //        (1) Check out the syntax
    //        (2) End the dialog given OK
    //        (3) Beep/message otherwise
    //

    //
    //  Directory ?? this must succeed for relative paths.
    //  NOTE: It won't succeed for relative paths that walk off the root.
    //
    bIsDir = SetDirRetry(szBasicPath);

    //
    //  We need to parse again in case SetDirRetry changed a UNC path to use
    //  a drive letter.
    //
    nFileOffset = ParseFileOld(szBasicPath, &nExtOffset, &nOldExt, FALSE);

    nTempOffset = nFileOffset;

    if (bIsDir)
    {
        return (FALSE);
    }
    else if (IsUNC(szBasicPath))
    {
        //
        //  UNC Name.
        //
        bUNCName = TRUE;
    }
    else if (nFileOffset > 0)
    {
        //
        //  There is a path in the string.
        //
        if ((nFileOffset > 1) &&
            (szBasicPath[nFileOffset - 1] != CHAR_COLON) &&
            (szBasicPath[nFileOffset - 2] != CHAR_COLON))
        {
            nTempOffset--;
        }
        GetCurrentDirectory(ARRAYSIZE(szBuf), szBuf);
        ch = szBasicPath[nTempOffset];
        szBasicPath[nTempOffset] = 0;
        if (SetCurrentDirectory(szBasicPath))
        {
            SetCurrentDirectory(szBuf);
        }
        else
        {
            switch (GetLastError())
            {
                case ( ERROR_NOT_READY ) :
                {
                    eCode = ECODE_BADDRIVE;
                    break;
                }
                default :
                {
                    eCode = ECODE_BADPATH;
                    break;
                }
            }
        }
        szBasicPath[nTempOffset] = ch;
    }
    else if (nFileOffset == PARSE_DIRECTORYNAME)
    {
        TCHAR szD[4];

        szD[0] = *szBasicPath;
        szD[1] = CHAR_COLON;
        szD[2] = CHAR_BSLASH;
        szD[3] = 0;
        if (PathFileExists(szD))
        {
            eCode = ECODE_BADPATH;
        }
        else
        {
            eCode = ECODE_BADDRIVE;
        }
    }

    //
    //  Was there a path and did it fail?
    //
    if ( !bUNCName &&
         nFileOffset &&
         eCode != ECODE_S_OK &&
         (lpOFN->Flags & OFN_PATHMUSTEXIST) )
    {
        if (eCode == ECODE_BADPATH)
        {
            nErrCode = OF_PATHNOTFOUND;
        }
        else if (eCode == ECODE_BADDRIVE)
        {
            TCHAR szD[4];

            //
            //  We can get here without performing an OpenFile call.  As
            //  such the szPathName can be filled with random garbage.
            //  Since we only need one character for the error message,
            //  set szPathName[0] to the drive letter.
            //
            szPathName[0] = szD[0] = *szBasicPath;
            szD[1] = CHAR_COLON;
            szD[2] = CHAR_BSLASH;
            szD[3] = 0;
            switch (GetDriveType(szD))
            {
                case ( DRIVE_REMOVABLE ) :
                {
                    nErrCode = ERROR_NOT_READY;
                    break;
                }
                case ( 1 ) :
                {
                    //
                    //  Drive does not exist.
                    //
                    nErrCode = OF_NODRIVE;
                    break;
                }
                default :
                {
                    nErrCode = OF_PATHNOTFOUND;
                }
            }
        }
        else
        {
            nErrCode = OF_FILENOTFOUND;
        }
        goto Warning;
    }

    //
    //  Full pattern?
    //
    if (IsWild(szBasicPath + nFileOffset))
    {
        if (!bUNCName)
        {
            SetCurrentFilter(szBasicPath + nFileOffset, Flags);
            if (nTempOffset)
            {
                szBasicPath[nTempOffset] = 0;
                JumpToPath(szBasicPath);
            }
            else if (psv)
            {
                psv->Refresh();
            }
            return (FALSE);
        }
        else
        {
            SetCurrentFilter(szBasicPath + nFileOffset, Flags);

            szBasicPath[nFileOffset] = CHAR_NULL;
            JumpToPath(szBasicPath);

        //  StringLower(szLastFilter);
            return (FALSE);
        }
    }

    if (PortName(szBasicPath + nFileOffset))
    {
        nErrCode = OF_PORTNAME;
        goto Warning;
    }

    //
    //  Check if we've received a string in the form "C:filename.ext".
    //  If we have, convert it to the form "C:.\filename.ext".  This is done
    //  because the kernel will search the entire path, ignoring the drive
    //  specification after the initial search.  Making it include a slash
    //  causes kernel to only search at that location.
    //
    //  Note:  Only increment nExtOffset, not nFileOffset.  This is done
    //  because only nExtOffset is used later, and nFileOffset can then be
    //  used at the Warning: label to determine if this hack has occurred,
    //  and thus it can strip out the ".\" when putting put the error.
    //
    if ((nFileOffset == 2) && (szBasicPath[1] == CHAR_COLON))
    {
        lstrcpy(szBuf, szBasicPath + 2);
        lstrcpy(szBasicPath + 4, szBuf);
        szBasicPath[2] = CHAR_DOT;
        szBasicPath[3] = CHAR_BSLASH;
        nExtOffset += 2;
    }

    //
    //  Add the default extension unless filename ends with period or no
    //  default extension exists.  If the file exists, consider asking
    //  permission to overwrite the file.
    //
    //  NOTE: When no extension given, default extension is tried 1st.
    //  FindNameInView calls VerifyOpen before returning.
    //
    szPathName[0] = 0;
    switch (FindNameInView( szBasicPath,
                            Flags,
                            szPathName,
                            nFileOffset,
                            nExtOffset,
                            &nErrCode ))
    {
        case ( FE_OUTOFMEM ) :
        case ( FE_CHANGEDDIR ) :
        {
            return (FALSE);
        }
        case ( FE_TOOMANY ) :
        {
            CDMessageBox( hwndDlg,
                          iszTooManyFiles,
                          MB_OK | MB_ICONEXCLAMATION,
                          szBasicPath );
            return (FALSE);
        }
        default :
        {
            break;
        }
    }

    switch (nErrCode)
    {
        case ( 0 ) :
        {
            //
            //  Is the file read-only?
            //
            if ((lpOFN->Flags & OFN_NOREADONLYRETURN) &&
                (GetFileAttributes(szPathName) & FILE_ATTRIBUTE_READONLY))
            {
                nErrCode = OF_LAZYREADONLY;
                goto Warning;
            }

            if ((bSave || (lpOFN->Flags & OFN_NOREADONLYRETURN)) &&
                (nTempOffset = WriteProtectedDirCheck(szPathName)))
            {
                nErrCode = nTempOffset;
                goto Warning;
            }

            if (lpOFN->Flags & OFN_OVERWRITEPROMPT)
            {
                if (bSave && !FOkToWriteOver(hwndDlg, szPathName))
                {
                    PostMessage( hwndDlg,
                                 WM_NEXTDLGCTL,
                                 (WPARAM)GetDlgItem(hwndDlg, edt1),
                                 1 );
                    return (FALSE);
                }
            }
            if (nErrCode == OF_SHARINGVIOLATION)
            {
                goto SharingViolationInquiry;
            }
            break;
        }
        case ( OF_SHARINGVIOLATION ) :
        {
SharingViolationInquiry:
            //
            //  If the app is "share aware", fall through.
            //  Otherwise, ask the hook function.
            //
            if (!(lpOFN->Flags & OFN_SHAREAWARE))
            {
                if (hSubDlg)
                {
                    int nShareCode = CD_SendShareNotify( hSubDlg,
                                                         hwndDlg,
                                                         szPathName,
                                                         lpOFN,
                                                         lpOFI );
                    if (nShareCode == OFN_SHARENOWARN)
                    {
                        return (FALSE);
                    }
                    else if (nShareCode != OFN_SHAREFALLTHROUGH)
                    {
                        //
                        //  They might not have handled the notification,
                        //  so try the registered message.
                        //
                        nShareCode = CD_SendShareMsg(hSubDlg, szPathName, lpOFI->ApiType);
                        if (nShareCode == OFN_SHARENOWARN)
                        {
                            return (FALSE);
                        }
                        else if (nShareCode != OFN_SHAREFALLTHROUGH)
                        {
                            goto Warning;
                        }
                    }
                }
                else
                {
                    goto Warning;
                }
            }
            break;
        }
        case ( OF_FILENOTFOUND ) :
        case ( OF_PATHNOTFOUND ) :
        {
            if (!bSave)
            {
                //
                //  The file or path wasn't found.
                //  If this is a save dialog, we're ok, but if it's not,
                //  we're toast.
                //
                if (lpOFN->Flags & OFN_FILEMUSTEXIST)
                {
                    if (lpOFN->Flags & OFN_CREATEPROMPT)
                    {
                        int nCreateCode = CreateFileDlg(hwndDlg, szBasicPath);
                        if (nCreateCode != IDYES)
                        {
                            return (FALSE);
                        }
                    }
                    else
                    {
                        goto Warning;
                    }
                }
            }
            goto VerifyPath;
        }
        default :
        {
            if (!bSave)
            {
                goto Warning;
            }


            //
            //  The file doesn't exist.  Can it be created?  This is needed
            //  because there are many extended characters which are invalid
            //  which won't be caught by ParseFile.
            //
            //  Two more good reasons:  Write-protected disks & full disks.
            //
            //  BUT, if they don't want the test creation, they can request
            //  that we not do it using the OFN_NOTESTFILECREATE flag.  If
            //  they want to create files on a share that has
            //  create-but-no-modify privileges, they should set this flag
            //  but be ready for failures that couldn't be caught, such as
            //  no create privileges, invalid extended characters, a full
            //  disk, etc.
            //

VerifyPath:
            //
            //  Verify the path.
            //
            if (lpOFN->Flags & OFN_PATHMUSTEXIST)
            {
                if (!(lpOFN->Flags & OFN_NOTESTFILECREATE))
                {
                    HANDLE hf = CreateFile( szBasicPath,
                                            GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,
                                            CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL );
                    if (hf != INVALID_HANDLE_VALUE)
                    {
                        CloseHandle(hf);

                        //
                        //  This test is here to see if we were able to
                        //  create it, but couldn't delete it.  If so,
                        //  warn the user that the network admin has given
                        //  him create-but-no-modify privileges.  As such,
                        //  the file has just been created, but we can't
                        //  do anything with it, it's of 0 size.
                        //
                        if (!DeleteFile(szBasicPath))
                        {
                            nErrCode = OF_CREATENOMODIFY;
                            goto Warning;
                        }
                    }
                    else
                    {
                        //
                        //  Unable to create it.
                        //
                        //  If it's not write-protection, a full disk,
                        //  network protection, or the user popping the
                        //  drive door open, assume that the filename is
                        //  invalid.
                        //
                        nErrCode = GetLastError();
                        switch (nErrCode)
                        {
                            case ( OF_WRITEPROTECTION ) :
                            case ( OF_DISKFULL ) :
                            case ( OF_NETACCESSDENIED ) :
                            case ( OF_ACCESSDENIED ) :
                            {
                                break;
                            }
                            default :
                            {
                                nErrCode = 0;
                                break;
                            }
                        }

                        goto Warning;
                    }
                }
            }
        }
    }

    WAIT_CURSOR w;

    nFileOffset = ParseFileOld(szPathName, &cch, &nOldExt, FALSE);

    lpOFN->nFileOffset = nFileOffset;
    lpOFN->nFileExtension = nOldExt;

    lpOFN->Flags &= ~OFN_EXTENSIONDIFFERENT;
    if (lpOFN->lpstrDefExt && lpOFN->nFileExtension)
    {
        TCHAR szPrivateExt[MIN_DEFEXT_LEN];

        //
        //  Check against lpOFN->lpstrDefExt, not pszDefExt.
        //
        lstrcpyn(szPrivateExt, lpOFN->lpstrDefExt, MIN_DEFEXT_LEN);
        if (lstrcmpi(szPrivateExt, szPathName + nOldExt))
        {
            lpOFN->Flags |= OFN_EXTENSIONDIFFERENT;
        }
    }

    if (lpOFN->lpstrFile)
    {
        cch = lstrlen(szPathName) + 1;
        if (lpOFN->Flags & OFN_ALLOWMULTISELECT)
        {
            //
            //  Extra room for double-NULL.
            //
            ++cch;
        }

        if (cch <= LOWORD(lpOFN->nMaxFile))
        {
            lstrcpy(lpOFN->lpstrFile, szPathName);
            if (lpOFN->Flags & OFN_ALLOWMULTISELECT)
            {
                //
                //  Double-NULL terminate.
                //
                *(lpOFN->lpstrFile + cch - 1) = CHAR_NULL;
            }

            if (!(lpOFN->Flags & OFN_NOCHANGEDIR) && !bUNCName && nFileOffset)
            {
                TCHAR ch = lpOFN->lpstrFile[nFileOffset];
                lpOFN->lpstrFile[nFileOffset] = CHAR_NULL;
                SetCurrentDirectory(lpOFN->lpstrFile);
                lpOFN->lpstrFile[nFileOffset] = ch;
            }
        }
        else
        {
            //
            //  Buffer is too small, so return the size of the buffer
            //  required to hold the string.
            //
            if (lpOFN->nMaxFile >= 3)
            {
#ifdef UNICODE
                lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                lpOFN->lpstrFile[1] = CHAR_NULL;
#else
                lpOFN->lpstrFile[0] = LOBYTE(cch);
                lpOFN->lpstrFile[1] = HIBYTE(cch);
                lpOFN->lpstrFile[2] = 0;
#endif
            }
            else
            {
#ifdef UNICODE
                lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                if (lpOFN->nMaxFile == 2)
                {
                    lpOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
                }
#else
                lpOFN->lpstrFile[0] = LOBYTE(cch);
                if (lpOFN->nMaxFile == 2)
                {
                    lpOFN->lpstrFile[1] = HIBYTE(cch);
                }
#endif
            }
        }
    }

    if (!PathIsExe(szPathName))
    {
        SHAddToRecentDocs(SHARD_PATH, szPathName);
    }

    //
    //  File Title.
    //  Note that it's cut off at whatever the buffer length
    //    is, so if the buffer's too small, no notice is given.
    //
    if (lpOFN->lpstrFileTitle)
    {
        cch = lstrlen(szPathName + nFileOffset);
        if ((DWORD)cch >= lpOFN->nMaxFileTitle)
        {
#ifdef DBCS
            EliminateString( szPathName + nFileOffset,
                             lpOFN->nMaxFileTitle - 1 );
#else
            szPathName[nFileOffset + lpOFN->nMaxFileTitle - 1] = CHAR_NULL;
#endif
        }
        lstrcpy(lpOFN->lpstrFileTitle, szPathName + nFileOffset);
    }

    if (!(lpOFN->Flags & OFN_HIDEREADONLY))
    {
        //
        //  Read-only checkbox visible?
        //
        if (IsDlgButtonChecked(hwndDlg, chx1))
        {
            lpOFN->Flags |=  OFN_READONLY;
        }
        else
        {
            lpOFN->Flags &= ~OFN_READONLY;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DriveList_OpenClose
//
//  Change the state of a drive list.
//
////////////////////////////////////////////////////////////////////////////

#define OCDL_TOGGLE     0x0000
#define OCDL_OPEN       0x0001
#define OCDL_CLOSE      0x0002

void DriveList_OpenClose(
    UINT uAction,
    HWND hwndDriveList)
{
    if (!hwndDriveList || !IsWindowVisible(hwndDriveList))
    {
        return;
    }

TryAgain:
    switch (uAction)
    {
        case ( OCDL_TOGGLE ) :
        {
            uAction = SendMessage(hwndDriveList, CB_GETDROPPEDSTATE, 0, 0L)
                          ? OCDL_CLOSE
                          : OCDL_OPEN;
            goto TryAgain;
            break;
        }
        case ( OCDL_OPEN ) :
        {
            SetFocus(hwndDriveList);
            SendMessage(hwndDriveList, CB_SHOWDROPDOWN, TRUE, 0);
            break;
        }
        case ( OCDL_CLOSE ) :
        {
            if (GetFocus() == hwndDriveList)
            {
                SendMessage(hwndDriveList, CB_SHOWDROPDOWN, FALSE, 0);
            }
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetFullEditName
//
//  Returns the number of characters needed to get the full path, including
//  the NULL.
//
////////////////////////////////////////////////////////////////////////////

UINT CFileOpenBrowser::GetFullEditName(
    LPTSTR pszBuf,
    UINT cLen,
    TEMPSTR *pTempStr,
    BOOL *pbNoDefExt)
{
    UINT cTotalLen;
    HWND hwndEdit;

    if (bUseHideExt)
    {
        cTotalLen = lstrlen(pszHideExt) + 1;
    }
    else
    {
        hwndEdit = GetDlgItem(hwndDlg, edt1);

        cTotalLen = GetWindowTextLength(hwndEdit) + 1;
    }

    if (pTempStr)
    {
        if (!pTempStr->StrSize(cTotalLen))
        {
            return ((UINT)-1);
        }

        pszBuf = *pTempStr;
        cLen = cTotalLen;
    }

    if (bUseHideExt)
    {
        lstrcpyn(pszBuf, pszHideExt, cLen);
    }
    else
    {
        GetWindowText(hwndEdit, pszBuf, cLen);
    }

    if (pbNoDefExt)
    {
        *pbNoDefExt = bUseHideExt;
    }

    return (cTotalLen);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::ProcessEdit
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::ProcessEdit()
{
    TEMPSTR pMultiSel;
    LPTSTR pszFile;
    BOOL bNoDefExt;
    OKBUTTONFLAGS Flags = OKBUTTON_NONE;

    if (lpOFN->Flags & OFN_ALLOWMULTISELECT)
    {
        if (GetFullEditName( szBuf,
                             ARRAYSIZE(szBuf),
                             &pMultiSel,
                             &bNoDefExt ) == (UINT)-1)
        {
            //
            //  BUGBUG: There should be some error message here.
            //
            return;
        }
        pszFile = pMultiSel;
    }
    else
    {
        GetFullEditName(szBuf, ARRAYSIZE(szBuf), NULL, &bNoDefExt);
        pszFile = szBuf;

        PathRemoveBlanks(pszFile);

        int nLen = lstrlen(pszFile);

        if (*pszFile == CHAR_QUOTE)
        {
            LPTSTR pPrev = CharPrev(pszFile, pszFile + nLen);
            if (*pPrev == CHAR_QUOTE && pszFile != pPrev)
            {
                Flags |= OKBUTTON_QUOTED;

                //
                //  Strip the quotes.
                //
                *pPrev = CHAR_NULL;
                lstrcpy(pszFile, pszFile + 1);
            }
        }
    }

    if (bNoDefExt)
    {
        Flags |= OKBUTTON_NODEFEXT;
    }

#ifdef UNICODE
    //
    //  Visual Basic passes in an uninitialized lpDefExts string.
    //  Since we only have to use it in OKButtonPressed, update
    //  lpstrDefExts here along with whatever else is only needed
    //  in OKButtonPressed.
    //
    if (lpOFI->ApiType == COMDLG_ANSI)
    {
        ThunkOpenFileNameA2WDelayed(lpOFI);
    }
#endif

    if (OKButtonPressed(pszFile, Flags))
    {
        BOOL bReturn = TRUE;

        if (lpOFN->lpstrFile)
        {
            if (!(lpOFN->Flags & OFN_NOVALIDATE))
            {
                if (lpOFN->nMaxFile >= 3)
                {
                    if ((lpOFN->lpstrFile[0] == 0) ||
                        (lpOFN->lpstrFile[1] == 0) ||
                        (lpOFN->lpstrFile[2] == 0))
                    {
                        bReturn = FALSE;
                        StoreExtendedError(FNERR_BUFFERTOOSMALL);
                    }
                }
                else
                {
                    bReturn = FALSE;
                    StoreExtendedError(FNERR_BUFFERTOOSMALL);
                }
            }
        }

        CleanupDialog(hwndDlg, bReturn);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::InitializeDropDown
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::InitializeDropDown(
    HWND hwndCtl)
{
    if (!bDropped)
    {
        MYLISTBOXITEM *pParentItem;
        SHChangeNotifyEntry fsne[2];

        pParentItem = GetListboxItem(hwndCtl, NODE_DESKTOP);
        UpdateLevel(hwndCtl, NODE_DESKTOP + 1, pParentItem);

        fsne[0].pidl = pParentItem->pidlFull;
        fsne[0].fRecursive = FALSE;

        pParentItem = GetListboxItem(hwndCtl, NODE_DRIVES);
        UpdateLevel(hwndCtl, NODE_DRIVES + 1, pParentItem);

        bDropped = TRUE;

        fsne[1].pidl = pParentItem->pidlFull;
        fsne[1].fRecursive = FALSE;

        uRegister = SHChangeNotifyRegister(
                        hwndDlg,
                        SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
                        SHCNE_ALLEVENTS &
                            ~(SHCNE_CREATE | SHCNE_DELETE | SHCNE_RENAMEITEM),
                        CDM_FSNOTIFY,
                        2,
                        fsne );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnCommandMessage
//
//  Process a WM_COMMAND message for the dialog.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CFileOpenBrowser::OnCommandMessage(
    WPARAM wParam,
    LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);

    switch (idCmd)
    {
        case ( edt1 ) :
        {
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
            {
                //
                //  The user started typing, so delete the hidden extension.
                //
                bUseHideExt = FALSE;
            }
            break;
        }
        case ( cmb2 ) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case ( CBN_CLOSEUP ) :
                {
                    OnSelChange();
                    SelectEditText(hwndDlg);
                    return (TRUE);
                }
                case ( CBN_DROPDOWN ) :
                {
                    InitializeDropDown(GET_WM_COMMAND_HWND(wParam, lParam));
                    break;
                }
            }
            break;
        }
        case ( cmb1 ) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case ( CBN_DROPDOWN ) :
                {
                    iComboIndex = SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                               CB_GETCURSEL,
                                               NULL,
                                               NULL );
                    break;
                }
                //
                //  We're trying to see if anything changed after
                //  (and only after) the user is done scrolling through the
                //  drop down. When the user tabs away from the combobox, we
                //  do not get a CBN_SELENDOK.
                //  Why not just use CBN_SELCHANGE? Because then we'd refresh
                //  the view (very slow) as the user scrolls through the
                //  combobox.
                //
                case ( CBN_CLOSEUP ) :
                case ( CBN_SELENDOK ) :
                {
                    //
                    //  Did anything change?
                    //
                    if (iComboIndex >= 0 &&
                        iComboIndex == SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                                    CB_GETCURSEL,
                                                    NULL,
                                                    NULL ))
                    {
                        break;
                    }
                }
                case ( MYCBN_DRAW ) :
                {
                    RefreshFilter(GET_WM_COMMAND_HWND(wParam, lParam));
                    iComboIndex = -1;
                    return (TRUE);
                }
                default :
                {
                    break;
                }
            }
            break;
        }
        case ( IDC_PARENT ) :
        {
            OnDotDot();
            SelectEditText(hwndDlg);
            break;
        }
        case ( IDC_NEWFOLDER ) :
        {
            ViewCommand(VC_NEWFOLDER);
            break;
        }
        case ( IDC_VIEWLIST ) :
        {
            ViewCommand(VC_VIEWLIST);
            break;
        }
        case ( IDC_VIEWDETAILS ) :
        {
            ViewCommand(VC_VIEWDETAILS);
            break;
        }
        case ( IDOK ) :
        {
            HWND hwndFocus = ::GetFocus();

            if (hwndFocus == ::GetDlgItem(hwndDlg, IDOK))
            {
                hwndFocus = hwndLastFocus;
            }

            hwndFocus = GetFocusedChild(hwndDlg, hwndFocus);

            if (hwndFocus == hwndView)
            {
                OnDblClick(TRUE);
            }
            else
            {
                ProcessEdit();
            }

            SelectEditText(hwndDlg);

            break;
        }
        case ( IDCANCEL ) :
        {
            bUserPressedCancel = TRUE;
            CleanupDialog(hwndDlg, FALSE);
            return (TRUE);
        }
        case ( pshHelp ) :
        {
            if (hSubDlg)
            {
                CD_SendHelpNotify(hSubDlg, hwndDlg, lpOFN, lpOFI);
            }

            if (lpOFN->hwndOwner)
            {
                CD_SendHelpMsg(lpOFN, hwndDlg, lpOFI->ApiType);
            }
            break;
        }
        case ( IDC_DROPDRIVLIST ) :
        {
            DriveList_OpenClose(OCDL_TOGGLE, GetDlgItem(hwndDlg, cmb2));
            break;
        }
        case ( IDC_REFRESH ) :
        {
            if (psv)
            {
                psv->Refresh();
            }
            break;
        }
        case ( IDC_PREVIOUSFOLDER ) :
        {
            OnDotDot();
            break;
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnCDMessage
//
//  Process a special CommDlg message for the dialog.
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::OnCDMessage(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LONG lResult;
    LPCITEMIDLIST pidl;

    switch (uMsg)
    {
        case ( CDM_GETSPEC ) :
        {
            lResult = GetFullEditName((LPTSTR)lParam, wParam);
            break;
        }
        case ( CDM_GETFILEPATH ) :
        case ( CDM_GETFOLDERPATH ) :
        case ( CDM_GETFOLDERIDLIST ) :
        {
            pidl = pCurrentLocation->pidlFull;

            lResult = ILGetSize(pidl);

            if (uMsg == CDM_GETFOLDERIDLIST)
            {
                if ((LONG)wParam < lResult)
                {
                    break;
                }

                CopyMemory((LPBYTE)lParam, (LPBYTE)pidl, lResult);
                break;
            }

            TCHAR szDir[MAX_PATH];

            if (!SHGetPathFromIDList(pidl, szDir))
            {
                lResult = -1;
                break;
            }

            if (uMsg == CDM_GETFOLDERPATH)
            {
CopyAndReturn:
                lResult = lstrlen(szDir) + 1;
                if ((int)wParam > lResult)
                {
                    wParam = lResult;
                }
                lstrcpyn((LPTSTR)lParam, szDir, wParam);
                break;
            }

            //
            //  We'll just fall through to the error case for now, since
            //  doing the full combine is not an easy thing.
            //
            TCHAR szFile[MAX_PATH];

            if ( GetFullEditName(szFile, ARRAYSIZE(szFile)) >
                 ARRAYSIZE(szFile) - 5 )
            {
                //
                //  Oops!  It looks like we filled our buffer!
                //
                lResult = -1;
                break;
            }

            PathCombine(szDir, szDir, szFile);
            goto CopyAndReturn;
        }
        case ( CDM_SETCONTROLTEXT ) :
        {
            if (bSave && wParam == IDOK)
            {
                tszDefSave.StrCpy((LPTSTR)lParam);

                //
                //  Do this to set the OK button correctly.
                //
                SelFocusChange(TRUE);
            }
            else
            {
                SetDlgItemText(hwndDlg, wParam, (LPTSTR)lParam);
            }
            break;
        }
        case ( CDM_HIDECONTROL ) :
        {
            ShowWindow(GetDlgItem(hwndDlg, wParam), SW_HIDE);
            break;
        }
        case ( CDM_SETDEFEXT ) :
        {
            pszDefExt.StrCpy((LPTSTR)lParam);
            bNoInferDefExt = TRUE;
            break;
        }
        default:
        {
            lResult = -1;
            break;
        }
    }

    SetWindowLong(hwndDlg, DWL_MSGRESULT, lResult);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  OKSubclass
//
//  Subclass window proc for the OK button.
//
//  The OK button is subclassed so we know which control had focus before
//  the user clicked OK.  This in turn lets us know whether to process OK
//  based on the current selection in the listview, or the current text
//  in the edit control.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK OKSubclass(
    HWND hOK,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
        case ( WM_SETFOCUS ) :
        {
            HWND hwndDlg = ::GetParent(hOK);
            CFileOpenBrowser *pDlgStruct = HwndToBrowser(hwndDlg);
            if (pDlgStruct != NULL)
            {
                pDlgStruct->hwndLastFocus = (HWND)wParam;
            }
        }
        break;
    }
    return (::CallWindowProc(::lpOKProc, hOK, msg, wParam, lParam));
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetNodeFromIDList
//
////////////////////////////////////////////////////////////////////////////

int CFileOpenBrowser::GetNodeFromIDList(
    LPCITEMIDLIST pidl)
{
    int i;
    HWND hwndCB = GetDlgItem(hwndDlg, cmb2);

    Assert(this->bDropped);

    //
    //  Just check DRIVES and DESKTOP.
    //
    for (i = NODE_DRIVES; i >= NODE_DESKTOP; --i)
    {
        MYLISTBOXITEM *pItem = GetListboxItem(hwndCB, i);

        if (pItem && ILIsEqual(pidl, pItem->pidlFull))
        {
            break;
        }
    }

    return (i);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::FSChange
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::FSChange(
    LONG lNotification,
    LPCITEMIDLIST *ppidl)
{
    int iNode = -1;
    LPCITEMIDLIST pidl = ppidl[0];
    LPCITEMIDLIST pidlExtra = ppidl[1];
    LPITEMIDLIST pidlClone;

    switch (lNotification)
    {
        case ( SHCNE_RENAMEFOLDER ) :
        {
            //
            //  Rename is special.  We need to invalidate both
            //  the pidl and the pidlExtra. so we call ourselves.
            //
            FSChange(0, &pidlExtra);
        }
        case ( 0 ) :
        case ( SHCNE_MKDIR ) :
        case ( SHCNE_RMDIR ) :
        {
            pidlClone = ILClone(pidl);
            if (!pidlClone)
            {
                break;
            }
            ILRemoveLastID(pidlClone);

            iNode = GetNodeFromIDList(pidlClone);
            ILFree(pidlClone);
            break;
        }
        case ( SHCNE_UPDATEITEM ) :
        case ( SHCNE_NETSHARE ) :
        case ( SHCNE_NETUNSHARE ) :
        case ( SHCNE_UPDATEDIR ) :
        {
            iNode = GetNodeFromIDList(pidl);
            break;
        }
        case ( SHCNE_DRIVEREMOVED ) :
        case ( SHCNE_DRIVEADD ) :
        case ( SHCNE_MEDIAINSERTED ) :
        case ( SHCNE_MEDIAREMOVED ) :
        {
            iNode = NODE_DRIVES;
            break;
        }
#if 0
        case ( SHCNE_SERVERDISCONNECT ) :
        {
            //
            //  Nuke all our kids and mark ourselves invalid.
            //
            lpNode = GetNodeFromIDList(pidl, 0);
            if (lpNode && NodeHasKids(lpNode))
            {
                int i;

                for (i = GetKidCount(lpNode) - 1; i >= 0; i--)
                {
                    OTRelease(GetNthKid(lpNode, i));
                }
                DPA_Destroy(lpNode->hdpaKids);
                lpNode->hdpaKids = KIDSUNKNOWN;
                OTInvalidateNode(lpNode);
                SFCFreeNode(lpNode);
            }
            else
            {
                lpNode = NULL;
            }
        }
#endif
    }

    if (iNode >= 0)
    {
        //
        //  We want to delay the processing a little because we always do
        //  a full update, so we should accumulate.
        //
        SetTimer(hwndDlg, TIMER_FSCHANGE + iNode, 100, NULL);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::Timer
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::Timer(
    WPARAM wID)
{
    KillTimer(hwndDlg, wID);

    wID -= TIMER_FSCHANGE;

    Assert(this->bDropped);
    switch (wID)
    {
        case ( NODE_DESKTOP ) :
        case ( NODE_DRIVES ) :
        {
            HWND hwndCB;
            MYLISTBOXITEM *pParentItem;

            hwndCB = GetDlgItem(hwndDlg, cmb2);

            pParentItem = GetListboxItem(hwndCB, wID);

            UpdateLevel(hwndCB, wID + 1, pParentItem);
            break;
        }
        default :
        {
            return;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  OpenDlgProc
//
//  Main dialog procedure for file open dialogs.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK OpenDlgProc(
    HWND hDlg,               // window handle of the dialog box
    UINT message,            // type of message
    WPARAM wParam,           // message-specific information
    LPARAM lParam)
{
    CFileOpenBrowser *pDlgStruct = HwndToBrowser(hDlg);

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            //
            //  Initialize dialog box.
            //
            LPOFNINITINFO poii = (LPOFNINITINFO)lParam;

            if (!InitLocation(hDlg, poii))
            {
                ::EndDialog(hDlg, FALSE);
            }

            //
            //  Always return FALSE to indicate we have already set the focus.
            //
            return (FALSE);
        }
        case ( WM_DESTROY ) :
        {
            //
            //  Make sure we do not respond to any more messages.
            //
            StoreBrowser(hDlg, NULL);
            ClearListbox(GetDlgItem(hDlg, cmb2));
            if (pDlgStruct)
            {
                pDlgStruct->Release();
            }
            break;
        }
        case ( WM_ACTIVATE ) :
        {
            if (wParam == WA_INACTIVE)
            {
                //
                //  Make sure some other Open dialog has not already grabbed
                //  the focus.  This is a process global, so it should not
                //  need to be protected.
                //
                if (gp_hwndActiveOpen == hDlg)
                {
                    gp_hwndActiveOpen = NULL;
                }
            }
            else
            {
                gp_hwndActiveOpen = hDlg;
            }
            break;
        }
        case ( WM_COMMAND ) :
        {
            //
            //  Received a command.
            //
            if (pDlgStruct)
            {
                return (pDlgStruct->OnCommandMessage(wParam, lParam));
            }
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            if (pDlgStruct)
            {
                pDlgStruct->PaintDriveLine((DRAWITEMSTRUCT *)lParam);
            }
            return (TRUE);
        }
        case ( WM_MEASUREITEM ) :
        {
            MeasureDriveItems(hDlg, (MEASUREITEMSTRUCT *)lParam);
            return (TRUE);
        }
        case ( WM_NOTIFY ) :
        {
            if (pDlgStruct)
            {
                return (pDlgStruct->OnNotify((LPNMHDR)lParam));
            }
            break;
        }
        case ( WM_SETCURSOR ) :
        {
            if (pDlgStruct && pDlgStruct->iWaitCount)
            {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
                SetDlgMsgResult(hDlg, message, (LRESULT)TRUE);
                return (TRUE);
            }
            break;
        }
        case ( WM_HELP ) :
        {
            if ( ((HWND)((LPHELPINFO)lParam)->hItemHandle) !=
                 pDlgStruct->hwndToolbar )
            {
                HWND hwndItem = (HWND)((LPHELPINFO)lParam)->hItemHandle;

                //
                //  We assume that the defview has one child window that
                //  covers the entire defview window.
                //
                HWND hwndDefView = GetDlgItem(hDlg, lst2);
                if (GetParent(hwndItem) == hwndDefView)
                {
                    hwndItem = hwndDefView;
                }

                WinHelp( hwndItem,
                         NULL,
                         HELP_WM_HELP,
                         (DWORD)(LPTSTR)(pDlgStruct && pDlgStruct->bSave
                                             ? aFileSaveHelpIDs
                                             : aFileOpenHelpIDs) );
            }
            return (TRUE);
        }
        case ( WM_CONTEXTMENU ) :
        {
            if (((HWND)wParam) != pDlgStruct->hwndToolbar)
            {
                WinHelp( (HWND)wParam,
                         NULL,
                         HELP_CONTEXTMENU,
                         (DWORD)(LPVOID)(pDlgStruct && pDlgStruct->bSave
                                             ? aFileSaveHelpIDs
                                             : aFileOpenHelpIDs) );
            }
            return (TRUE);
        }
        case ( CWM_GETISHELLBROWSER ) :
        {
            ::SetWindowLong(hDlg, DWL_MSGRESULT, (LRESULT)pDlgStruct);
            return (TRUE);
        }
        case ( CDM_SETSAVEBUTTON ) :
        {
            if (pDlgStruct)
            {
                pDlgStruct->RealSetSaveButton((UINT)wParam);
            }
            break;
        }
        case ( CDM_FSNOTIFY ) :
        {
            if (!pDlgStruct)
            {
                return (0L);
            }

            return (pDlgStruct->FSChange(lParam, (LPCITEMIDLIST*)wParam));
        }
        case ( CDM_SELCHANGE ) :
        {
            if (!pDlgStruct)
            {
                break;
            }
            pDlgStruct->fSelChangedPending = FALSE;
            pDlgStruct->SelFocusChange(TRUE);
            if (pDlgStruct->hSubDlg)
            {
                CD_SendSelChangeNotify( pDlgStruct->hSubDlg,
                                        hDlg,
                                        pDlgStruct->lpOFN,
                                        pDlgStruct->lpOFI );
            }
            break;
        }
        case ( WM_TIMER ) :
        {
            pDlgStruct->Timer(wParam);
            break;
        }
        default :
        {
            if (IsInRange(message, CDM_FIRST, CDM_LAST) && pDlgStruct)
            {
                return (pDlgStruct->OnCDMessage(message, wParam, lParam));
            }

            break;
        }
    }

    //
    //  Did not process the message.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  OpenFileHookProc
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK OpenFileHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    CFileOpenBrowser *pDlgStruct;
    MSG *lpMsg;

    if (nCode < 0)
    {
        return (DefHookProc(nCode, wParam, lParam, &gp_hHook));
    }

    if (nCode != MSGF_DIALOGBOX)
    {
        return (0);
    }

    lpMsg = (MSG *)lParam;

    //
    //  Check if this message is for the last active OpenDialog in this
    //  process.
    //
    //  Note: This is only done for WM_KEY* messages so that we do not slow
    //        down this window too much.
    //
    if (IsInRange(lpMsg->message, WM_KEYFIRST, WM_KEYLAST))
    {
        HWND hwndActiveOpen = gp_hwndActiveOpen;
        HWND hwndFocus = GetFocusedChild(hwndActiveOpen, lpMsg->hwnd);

        if (hwndFocus &&
            (pDlgStruct = HwndToBrowser(hwndActiveOpen)) != NULL)
        {
            if (pDlgStruct->psv && hwndFocus == pDlgStruct->hwndView)
            {
                if (pDlgStruct->psv->TranslateAccelerator(lpMsg) == S_OK)
                {
                    return (1);
                }

                if (gp_haccOpenView &&
                    TranslateAccelerator( hwndActiveOpen,
                                          gp_haccOpenView,
                                          lpMsg ))
                {
                    return (1);
                }
            }
            else
            {
                if (gp_haccOpen &&
                    TranslateAccelerator( hwndActiveOpen,
                                          gp_haccOpen,
                                          lpMsg ))
                {
                    return (1);
                }

                //
                //  Note that the view won't be allowed to translate when the
                //  focus is not there.
                //
            }
        }
    }

    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  NewGetFileName
//
////////////////////////////////////////////////////////////////////////////

BOOL NewGetFileName(
    LPOPENFILEINFO lpOFI,
    BOOL bSave)
{
    OFNINITINFO oii = { lpOFI, bSave };
    LPOPENFILENAME lpOFN = lpOFI->pOFN;
    BOOL bHooked = FALSE;
    WORD wErrorMode;

    if (lpOFN->lStructSize != sizeof(OPENFILENAME))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    if (!InitImports())
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    wErrorMode = (WORD)SetErrorMode(SEM_NOERROR);
    SetErrorMode(SEM_NOERROR | wErrorMode);

    //
    //  These hooks are REALLY stupid.  I am compelled to keep the hHook in a
    //  global because my callback needs it, but I have no lData where I could
    //  possibly store it.
    //  Note that we initialize nHookRef to -1 so we know when the first
    //  increment is.
    //
    if (InterlockedIncrement((LPLONG)&gp_nHookRef) == 0)
    {
        gp_hHook = SetWindowsHookEx( WH_MSGFILTER,
                                     OpenFileHookProc,
                                     0,
                                     GetCurrentThreadId() );
        if (gp_hHook)
        {
            bHooked = TRUE;
        }
        else
        {
            --gp_nHookRef;
        }
    }
    else
    {
        bHooked = TRUE;
    }

    if (!gp_haccOpen)
    {
        gp_haccOpen = LoadAccelerators( g_hinst,
                                        MAKEINTRESOURCE(IDA_OPENFILE) );
    }
    if (!gp_haccOpenView)
    {
        gp_haccOpenView = LoadAccelerators( g_hinst,
                                            MAKEINTRESOURCE(IDA_OPENFILEVIEW) );
    }

    HIMAGELIST himl;
    Shell_GetImageLists(NULL, &himl);
    ImageList_GetIconSize(himl, &g_cxSmIcon, &g_cySmIcon);

    int nRet = DialogBoxParam( ::g_hinst,
                               MAKEINTRESOURCE(NEWFILEOPENORD),
                               lpOFN->hwndOwner,
                               OpenDlgProc,
                               (LPARAM)(LPOFNINITINFO)&oii );

    if (bHooked)
    {
        //
        //  Put this in a local so we don't need a critical section.
        //
        HHOOK hHook = gp_hHook;

        if (InterlockedDecrement((LPLONG)&gp_nHookRef) < 0)
        {
            UnhookWindowsHookEx(hHook);
        }
    }

    switch (nRet)
    {
        case ( TRUE ) :
        {
            break;
        }
        case ( FALSE ) :
        {
            if ((!bUserPressedCancel) && (!GetStoredExtendedError()))
            {
                StoreExtendedError(CDERR_DIALOGFAILURE);
            }
            break;
        }
        default :
        {
            StoreExtendedError(CDERR_DIALOGFAILURE);
            nRet = FALSE;
            break;
        }
    }

    //
    //  BUGBUG.
    //  There is a race condition here where we free dlls but a thread
    //  using this stuff still hasn't terminated so we page fault.
    //
//  FreeImports();

    SetErrorMode(wErrorMode);

    return (nRet);
}


extern "C" {

////////////////////////////////////////////////////////////////////////////
//
//  NewGetOpenFileName
//
////////////////////////////////////////////////////////////////////////////

BOOL NewGetOpenFileName(
    LPOPENFILEINFO lpOFI)
{
    return (NewGetFileName(lpOFI, FALSE));
}


////////////////////////////////////////////////////////////////////////////
//
//  NewGetSaveFileName
//
////////////////////////////////////////////////////////////////////////////

BOOL NewGetSaveFileName(
    LPOPENFILEINFO lpOFI)
{
    return (NewGetFileName(lpOFI, TRUE));
}

}   // extern "C"


////////////////////////////////////////////////////////////////////////////
//
//  Overloaded allocation operators.
//
////////////////////////////////////////////////////////////////////////////

static inline void * __cdecl operator new(
    unsigned int size)
{
    return ((void *)LocalAlloc(LPTR, size));
}

static inline void __cdecl operator delete(
    void *ptr)
{
    LocalFree(ptr);
}

extern "C" inline __cdecl _purecall(void)
{
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitImports
//
//  Import all the APIs we need from shell32.dll and comctl32.dll, so we
//  don't have hard links to them.
//
////////////////////////////////////////////////////////////////////////////

BOOL InitImports(void)
{
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeImports
//
//  Unload the DLLs we loaded in InitImports().
//  This should only be called at ProcessDetach time, since we do not NULL
//  out all the things that are now invalid.
//
////////////////////////////////////////////////////////////////////////////

const TCHAR c_szShellDll[] = TEXT("shell32.dll");
const TCHAR c_szComCtl32[] = TEXT("comctl32.dll");

typedef struct _DLLINFO
{
    HINSTANCE hInst;
    LPCTSTR pszInst;
} DLLINFO;

DLLINFO diShellDll =
{
    NULL, c_szShellDll
};
DLLINFO diComCtl32 =
{
    NULL, c_szComCtl32
};

void FreeImports(void)
{
    //
    //  No critical section needed since only touching process globals
    //  during process detach.
    //
    if (diShellDll.hInst)
    {
        FreeLibrary(diShellDll.hInst);
    }
    if (diComCtl32.hInst)
    {
        FreeLibrary(diComCtl32.hInst);
    }
}


