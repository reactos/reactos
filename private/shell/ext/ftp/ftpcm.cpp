/*****************************************************************************\
    FILE:  ftpcm.cpp - IContextMenu interface
\*****************************************************************************/

#include "priv.h"
#include "ftpcm.h"
#include "util.h"
#include "ftpprop.h"
#include "ftpurl.h"
#include "dialogs.h"
#include "statusbr.h"
#include "newmenu.h"
#include "view.h"
#include "resource.h"



/*****************************************************************************\
 *
 *    VERBINFO, c_rgvi
 *
 *    Information about which capabilities correspond to which verbs.
 *
 *    If the item ID is in the range 0 ... IDC_ITEM_MAX, then it is
 *    relative to the base address.
 *
\*****************************************************************************/

#pragma BEGIN_CONST_DATA

#define     CMDSTR_LOGINASA          "Login As"

// BUGBUG -- split into two arrays, cobbled together into a structure

struct VERBINFO
{
    UINT  idc;
    DWORD sfgao;
    LPCTSTR ptszCmd;
} c_rgvi[] = {
/* If you edit anything below this comment, make sure to update below */
    {    IDM_SHARED_EDIT_COPY,    SFGAO_CANCOPY,        TEXT("copy"),     },
#ifdef FEATURE_CUT_MOVE
    {    IDM_SHARED_EDIT_CUT,    SFGAO_CANMOVE,        TEXT("cut"),    },
#endif // FEATURE_CUT_MOVE
    {    IDM_SHARED_FILE_LINK,    SFGAO_CANLINK,        TEXT("link"),     },
    {    IDM_SHARED_FILE_RENAME,    SFGAO_CANRENAME,    TEXT("rename"),    },
    {    IDM_SHARED_FILE_DELETE,    SFGAO_CANDELETE,    TEXT("delete"),    },
    {    IDM_SHARED_FILE_PROP,    SFGAO_HASPROPSHEET,    TEXT("properties"), },
    {    IDM_SHARED_EDIT_PASTE,    SFGAO_DROPTARGET,    TEXT("paste"),    },
/* CVI_NONREQ is the number of items in c_rgvi up to this point */
/* The following entries must be in IDC_ITEM_* order */
    {    IDC_ITEM_OPEN,        SFGAO_FOLDER,        TEXT("open"),    },
    {    IDC_ITEM_EXPLORE,    SFGAO_FOLDER,        TEXT("explore"),},
    {    IDC_ITEM_DOWNLOAD,    SFGAO_CANCOPY,        TEXT("download"),},
    {    IDC_ITEM_BKGNDPROP,    0,                    TEXT("backgroundproperties"),},
    {    IDC_LOGIN_AS,        0,                    TEXT(CMDSTR_LOGINASA),},
    {    IDC_ITEM_NEWFOLDER,    0,                    CMDSTR_NEWFOLDER,},
/* The preceding entries must be in IDC_ITEM_* order */
/* If you edit anything above this comment, make sure to update below */
};

#ifdef FEATURE_CUT_MOVE
#define CVI_NONREQ   7        /* See remarks above */
#else // FEATURE_CUT_MOVE
#define CVI_NONREQ   6        /* See remarks above */
#endif // FEATURE_CUT_MOVE
#define IVI_REQ        CVI_NONREQ    /* First required verb */
#define IVI_MAX        ARRAYSIZE(c_rgvi)    /* One past last value index */

#pragma END_CONST_DATA



/*****************************************************************************\
    FUNCTION:   _RemoveContextMenuItems
 
    Remove context menu items based on attribute flags.
  
      If we have a drop target, ping it to see if the object on the
      clipboard is pasteable.  If not, then disable Paste.  (Shell UI
      says that you don't remove Paste, merely disable it.)
    
    Return the number of items removed.
\*****************************************************************************/
int CFtpMenu::_RemoveContextMenuItems(HMENU hmenu, UINT idCmdFirst, DWORD sfgao)
{
    int ivi;
    int nItemRemoved = 0;

    for (ivi = 0; ivi < CVI_NONREQ; ivi++)
    {
        if (!(sfgao & c_rgvi[ivi].sfgao))
        {
            EnableMenuItem(hmenu, (c_rgvi[ivi].idc + idCmdFirst), MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            nItemRemoved++;
        }
    }

    // See if the clipboard format is supported
    if (sfgao & SFGAO_DROPTARGET)
    {
        IDataObject *pdto;
        DWORD grflEffects = 0;        // Clipboard not available

        if (EVAL(SUCCEEDED(OleGetClipboard(&pdto))))
        {
            CFtpDrop * pfdrop;
            
            if (SUCCEEDED(CFtpDrop_Create(m_pff, m_hwnd, &pfdrop)))
            {
                grflEffects = pfdrop->GetEffectsAvail(pdto);
                pfdrop->Release();
            }
            pdto->Release();
        }

        if (!(grflEffects & (DROPEFFECT_COPY | DROPEFFECT_MOVE)))
        {
            EnableMenuItem(hmenu, (IDM_SHARED_EDIT_PASTE + idCmdFirst),
                       MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            nItemRemoved++;
        }
#ifdef _SOMEDAY_PASTESHORTCUT
        if (!(grflEffects & DROPEFFECT_LINK))
        {
            EnableMenuItem(hmenu, (IDM_SHARED_EDIT_PASTE_SHORTCUT + idCmdFirst),
                       MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            nItemRemoved++;
        }
#endif
    }

    return nItemRemoved;
}


/*****************************************************************************\
    FUNCTION: _AddToRecentDocs

    DESCRIPTION:
        This method will add the item to the Recent Docs MRU.  The pidl parameter
    is a fully qualified pidl all the way to the root of the public shell name space
    (desktop).
\*****************************************************************************/
HRESULT CFtpMenu::_AddToRecentDocs(LPCITEMIDLIST pidl)
{
    // We may want to filter on verb.
    SHAddToRecentDocs(SHARD_PIDL, (LPCVOID) pidl);

    return S_OK;
}


typedef struct
{
    LPCWIRESTR pwSoftLink;
    LPWIRESTR pwFtpPath;
    DWORD cchSize;
} SOFTLINKDESTCBSTRUCT;

HRESULT CFtpMenu::_SoftLinkDestCB(HINTERNET hint, HINTPROCINFO * phpi, LPVOID pvsldcbs, BOOL * pfReleaseHint)
{
    HRESULT hr = S_OK;
    WIRECHAR wFrom[MAX_PATH];
    SOFTLINKDESTCBSTRUCT * psldcbs = (SOFTLINKDESTCBSTRUCT *) pvsldcbs;
    DWORD cchSize = ARRAYSIZE(wFrom);

    // Normally, I hate hard coding the buffer size, but passing structs to callbacks is such a pain
    // and this won't change.
    hr = FtpGetCurrentDirectoryWrap(hint, TRUE, wFrom, cchSize);
    if (SUCCEEDED(hr))
    {
        hr = FtpSetCurrentDirectoryWrap(hint, TRUE, psldcbs->pwSoftLink);
        if (SUCCEEDED(hr))
        {
            hr = FtpGetCurrentDirectoryWrap(hint, TRUE, psldcbs->pwFtpPath, psldcbs->cchSize);
            if (SUCCEEDED(hr))
            {
                // BUGBUG: Do we need to return?
                hr = FtpSetCurrentDirectoryWrap(hint, TRUE, wFrom);
            }
        }
    }

    return hr;
}


LPITEMIDLIST CFtpMenu::GetSoftLinkDestination(LPCITEMIDLIST pidlToSoftLink)
{
    LPITEMIDLIST pidlToDest = NULL;
    WIRECHAR wSoftLinkName[MAX_PATH];
    WIRECHAR wFtpPath[MAX_PATH];
    SOFTLINKDESTCBSTRUCT sldcbs = {wSoftLinkName, wFtpPath, ARRAYSIZE(wFtpPath)};

    StrCpyNA(wSoftLinkName, FtpPidl_GetLastItemWireName(pidlToSoftLink), ARRAYSIZE(wSoftLinkName));
    StrCpyNA(wFtpPath, FtpPidl_GetLastItemWireName(pidlToSoftLink), ARRAYSIZE(wFtpPath));

    // NULL hwnd because I don't want UI.
    if (EVAL(SUCCEEDED(m_pfd->WithHint(NULL, NULL, _SoftLinkDestCB, (LPVOID) &sldcbs, _punkSite, m_pff))))
    {
        EVAL(SUCCEEDED(CreateFtpPidlFromUrlPathAndPidl(pidlToSoftLink, m_pff->GetCWireEncoding(), wFtpPath, &pidlToDest)));
    }

    return pidlToDest;
}


// Someday maybe add: (SEE_MASK_UNICODE | SEE_MASK_FLAG_TITLE)
#define SEE_MASK_SHARED (SEE_MASK_FLAG_NO_UI | SEE_MASK_HOTKEY | SEE_MASK_NO_CONSOLE)

#define FILEATTRIB_DIRSOFTLINK (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)

/*****************************************************************************\
    FUNCTION: _ApplyOne

    DESCRIPTION:
        This function will ShellExec() the pidl.  
        
    SECURITY ISSUES:
        We don't need to worry about the 'Open' verb on folders because that
    is always safe.  The 'Open' verb on files is safe because we later
    redirect the functionality to the original URLMON ftp support which
    goes through code download.  This displays dialogs, checks certs, and
    does all the zones checking and admin policies.
\*****************************************************************************/
HRESULT CFtpMenu::_ApplyOne(CFtpMenu * pfcm, LPCMINVOKECOMMANDINFO pici, LPCTSTR pszCmd, LPCITEMIDLIST pidl)
{
    HRESULT hr;
    SHELLEXECUTEINFO sei;
    LPITEMIDLIST pidlFullPriv = pfcm->m_pff->CreateFullPrivatePidl(pidl);

    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);

    LPITEMIDLIST pidlFullPub = NULL;

    // TODO/BUGBUG: It would be nice to see if the pidl is a SoftLink (FtpPidl_IsSoftLink)
    //     and if so, step into the directory, get the directory path and then create
    //     a pidl from that path so we end up showing the user the real destination
    //     of the softlink.
    if (FILEATTRIB_DIRSOFTLINK == (FILEATTRIB_DIRSOFTLINK & FtpPidl_GetAttributes(pidlFullPriv)))
    {
        LPITEMIDLIST pidlNew = pfcm->GetSoftLinkDestination(pidlFullPriv);

        // Switch pidls if it worked, otherwise, using the original pidl isn't that bad, so it
        // will be the fall back case if things don't work out.
        if (pidlNew)
        {
            ILFree(pidlFullPriv);
            pidlFullPriv = pidlNew;
        }

        pidlFullPub = pfcm->m_pff->CreateFullPublicPidl(pidlFullPriv);
    }
    else
    {
        // Yes, so we need to use it in the pidl we pass to ShellExecute.
        pidlFullPub = ILCombine(pfcm->m_pff->GetPublicRootPidlReference(), pidl);
    }

    // Titles are excluded because there is no lpTitle in the sei.
    // Unicode is excluded because we don't do UNICODE; in fact,
    // we filter it out up front!
    ASSERT(SEE_MASK_FLAG_NO_UI == CMIC_MASK_FLAG_NO_UI);
    ASSERT(SEE_MASK_HOTKEY == CMIC_MASK_HOTKEY);
    ASSERT(SEE_MASK_NO_CONSOLE == CMIC_MASK_NO_CONSOLE);

    sei.fMask |= SEE_MASK_IDLIST | (pici->fMask & SEE_MASK_SHARED);
    sei.hwnd = pici->hwnd;
    sei.nShow = pici->nShow;
    sei.dwHotKey = pici->dwHotKey;
    sei.hIcon = pici->hIcon;
    sei.lpIDList = (void *) pidlFullPub;

    if (EVAL(sei.lpIDList))
    {
        TCHAR szParameters[MAX_URL_STRING];
        TCHAR szDirectory[MAX_PATH];

        if (pici->lpParameters)
            SHAnsiToTChar(pici->lpParameters, szParameters, ARRAYSIZE(szParameters));

        if (pici->lpDirectory)
            SHAnsiToTChar(pici->lpDirectory, szDirectory, ARRAYSIZE(szDirectory));

        sei.lpVerb = pszCmd;
        sei.lpParameters = (pici->lpParameters ? szParameters : NULL);
        sei.lpDirectory = (pici->lpDirectory ? szDirectory : NULL);
        if (EVAL(ShellExecuteEx(&sei)))
        {
            // Yes, so we need to use it in the pidl we pass to ShellExecute.
            LPITEMIDLIST pidlFullPubTarget = ILCombine(pfcm->m_pff->GetPublicTargetPidlReference(), pidl);

            if (pidlFullPubTarget)
            {
                EVAL(SUCCEEDED(pfcm->_AddToRecentDocs(pidlFullPubTarget)));        // We don't care if AddToRecent works or not.
                ILFree(pidlFullPubTarget);
                hr = S_OK;
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
        hr = E_OUTOFMEMORY;

    if (pidlFullPub)
        ILFree(pidlFullPub);

    if (pidlFullPriv)
        ILFree(pidlFullPriv);

    return hr;
}





/*****************************************************************************\
 *
 *    _InvokeOneCB
 *
 *    Invoke the command on the single pidl.
 *
\*****************************************************************************/

int CFtpMenu::_InvokeOneCB(LPVOID pvPidl, LPVOID pv)
{
    LPCITEMIDLIST pidl = (LPCITEMIDLIST) pvPidl;
    PEII peii = (PEII) pv;

    ASSERT(peii && peii->pfcm);
    return peii->pfcm->_InvokeOne(pidl, peii);
}

int CFtpMenu::_InvokeOne(LPCITEMIDLIST pidl, PEII peii)
{
    ASSERT(ILIsSimple(pidl));

    if (GetAsyncKeyState(VK_ESCAPE) >= 0)
    {
        if (EVAL(SUCCEEDED(peii->hres)))
            peii->hres = peii->pfn(peii->pfcm, peii->pici, peii->ptszCmd, pidl);
    }
    else
        peii->hres = HRESULT_FROM_WIN32(ERROR_CANCELLED);

    return SUCCEEDED(peii->hres);
}





/*****************************************************************************\
 *
 *    _EnumInvoke
 *
 * Invoke the command on each object in the list, assuming that
 * permissions are properly set.  (We need to check the permissions
 * in case somebody randomly threw the verb at us.)
 *
\*****************************************************************************/

STDMETHODIMP CFtpMenu::_EnumInvoke(LPCMINVOKECOMMANDINFO pici, INVOKEPROC pfn, LPCTSTR pszCmd)
{
    EII eii;
    eii.pfcm = this;
    eii.pici = pici;
    eii.pfn = pfn;
    eii.ptszCmd = pszCmd;
    eii.hres = S_OK;

    if (m_pflHfpl->GetCount())
        m_pflHfpl->Enum(_InvokeOneCB, (LPVOID) &eii);
    else
        _InvokeOne(c_pidlNil, &eii);

    return eii.hres;
}





/*****************************************************************************\
 *
 * _InvokeRename
 *
 * Rename the object to the indicated name.
 *
 *  The rename verb should have been enabled only if the pidl list
 *  is singleton.  Of course, that doesn't prevent some random bozo
 *  from throwing the word "rename" at us from out of the blue, so
 *  we need to remain on guard.
 *
 * _UNOBVIOUS_:  If the user does an in-place rename, we don't get
 *  a "rename" command invoked against our context menu.  Instead,
 *  the shell goes straight for the SetNameOf method in the ShellFolder.
 *  Which means that we cannot put UI in the context menu (which is the
 *  obvious place for it, because it has a CMIC_MASK_FLAG_NO_UI bit);
 *  we must put it into SetNameOf, which is annoying because it means
 *  there is no way to programmatically perform a SetNameOf without UI.
 *
 *  _SOMEDAY_
 *  We fix this unobvious-ness by passing the CMIC_MASK_FLAG_NO_UI bit
 *  through to our SetNameOf backdoor, so you can programmatically
 *  rename a file without UI by going through the IContextMenu.
 *
\*****************************************************************************/

HRESULT CFtpMenu::_InvokeRename(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr;

    if (EVAL((m_sfgao & SFGAO_CANRENAME) && m_pfd))
    {
        ASSERT(m_pflHfpl->GetCount() == 1);
        if (EVAL(pici->lpParameters))
        {
            TCHAR szParams[MAX_URL_STRING];

            ASSERT(pici->hwnd);
            SHAnsiToTChar(pici->lpParameters, szParams, ARRAYSIZE(szParams));
            hr = m_pfd->SetNameOf(m_pff, pici->hwnd, m_pflHfpl->GetPidl(0), szParams, SHGDN_INFOLDER, 0);
        }
        else
            hr = E_INVALIDARG;    // Arguments required
    }
    else
        hr = E_ACCESSDENIED;        // Can't rename this

    return hr;
}


/*****************************************************************************\
 *    _InvokeCutCopy
 *
 *    Cut or copy the selection to the OLE clipboard.  No big deal.
 *
 *    Note that GetUIObjectOfHfpl(IID_IDataObject) will fail if we
 *    are talking about ourself.  Maybe it shouldn't but it does today.
\*****************************************************************************/
HRESULT CFtpMenu::_InvokeCutCopy(UINT_PTR id, LPCMINVOKECOMMANDINFO pici)
{
    IDataObject * pdo;
    HRESULT hr;

    hr = m_pff->GetUIObjectOfHfpl(pici->hwnd, m_pflHfpl, IID_IDataObject, (LPVOID *)&pdo, m_fBackground);
    if (EVAL(SUCCEEDED(hr)))
    {
        DWORD dwEffect = ((DFM_CMD_COPY == id) ? DROPEFFECT_COPY : DROPEFFECT_MOVE);

        EVAL(SUCCEEDED(DataObj_SetPreferredEffect(pdo, dwEffect)));

        ShellFolderView_SetPoints(m_hwnd, pdo);
        hr = OleSetClipboard(pdo);    // Will do its own AddRef
        ShellFolderView_SetClipboard(m_hwnd, id);
        
        if (pdo)
            pdo->Release();
    }
    else
        ASSERT(0);         // BUGBUG -- error UI 

    return hr;
}

/*****************************************************************************\
    FUNCTION: _DoDrop

    DESCRIPTION:
        The user just did a Paste on FTP so we want to do the operation.
    We will use our Drag & Drop code to carry out the operation.  We don't
    currently support optimized FTP operations but a lot could be done if
    we did.

    First we need to find out if the caller did "Cut" or "Copy" to create
    the IDataObject.  We can find out by asking the IDataObject for the
    CFSTR_PREFERREDDROPEFFECT.
\*****************************************************************************/
HRESULT CFtpMenu::_DoDrop(IDropTarget * pdt, IDataObject * pdo)
{
    POINTL pt = {0, 0};
    DWORD dwEffect = DROPEFFECT_COPY;   // Default
    HRESULT hr = DataObj_GetDWORD(pdo, g_dropTypes[DROP_PrefDe].cfFormat, &dwEffect);

#ifndef FEATURE_CUT_MOVE    
    dwEffect = DROPEFFECT_COPY;     // Forcibly remove the MOVE effect
#endif // FEATURE_CUT_MOVE

    hr = pdt->DragEnter(pdo, MK_LBUTTON, pt, &dwEffect);
    if (EVAL(SUCCEEDED(hr)) && dwEffect)
    {
#ifndef FEATURE_CUT_MOVE    
        dwEffect = DROPEFFECT_COPY;     // Forcibly remove the MOVE effect
#endif // FEATURE_CUT_MOVE
        hr = pdt->Drop(pdo, MK_LBUTTON, pt, &dwEffect);
    }
    else
        pdt->DragLeave();

    return hr;
}

/*****************************************************************************\
 *
 *    _InvokePaste
 *
 *    Copy from the OLE clipboard into the selcted folder (which might
 *    be ourselves).
 *
\*****************************************************************************/

HRESULT CFtpMenu::_InvokePaste(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hres = E_FAIL;

    if (EVAL(m_sfgao & SFGAO_DROPTARGET))
    {
        IDataObject *pdto;
        hres = OleGetClipboard(&pdto);
        if (EVAL(SUCCEEDED(hres)))
        {
            IDropTarget *pdt;
            hres = m_pff->GetUIObjectOfHfpl(pici->hwnd, m_pflHfpl, IID_IDropTarget, (LPVOID *)&pdt, m_fBackground);
            if (EVAL(SUCCEEDED(hres)))
            {
                hres = _DoDrop(pdt, pdto);
                if (pdt)
                    pdt->Release();
            }
            else
            {
                // BUGBUG -- error UI
            }
            if (pdto)
                pdto->Release();
        }
        else
        {
            // BUGBUG -- error UI
        }
    }
    else
    {
        // BUGBUG -- error UI
    }

    return hres;
}


//===========================
// *** IContextMenu Interface ***
//===========================


/*****************************************************************************\
    FUNCTION: _ContainsForgroundItems
  
    DESCRIPTION:
        We want to know if the user selected items in the view and then invoked
    some menu (Context Menu, File Menu, CaptionBar icon menu, etc.).  Normally
    this is as simple as seeing if (0 == m_pflHfpl->GetCount()).  However,
    there is one other nasty case where (1 == m_pflHfpl->GetCount()) and
    the user still didn't select anything.  This case happens when the user
    is at the root of a FTP share and the CaptionBar menu is dropped down.
    In that case, the single pidl is the pidl to the ftp root.
\*****************************************************************************/
BOOL CFtpMenu::_ContainsForgroundItems(void)
{
    BOOL fIsForground = (0 != m_pflHfpl->GetCount());

    if (fIsForground && (1 == m_pflHfpl->GetCount()))
    {
        LPITEMIDLIST pidl = m_pflHfpl->GetPidl(0);

        if (FtpID_IsServerItemID(pidl) && ILIsEmpty(_ILNext(pidl)))
        {
            if (!m_pfd)
            {
                CFtpSite * pfs;

                // In this strange case, our m_pfd is NULL, so we need
                // to create it from pidl.
                if (EVAL(SUCCEEDED(SiteCache_PidlLookup(pidl, FALSE, m_pff->GetItemAllocatorDirect(), &pfs))))
                {
                    EVAL(SUCCEEDED(pfs->GetFtpDir(pidl, &m_pfd)));
                    pfs->Release();
                }
            }
            fIsForground = FALSE;
        }
    }

    return fIsForground;
}


BOOL CFtpMenu::_IsCallerCaptionBar(UINT indexMenu, UINT uFlags)
{
    BOOL fFromCaptionBar;

    if ((0 == uFlags) && (1 == indexMenu))
        fFromCaptionBar = TRUE;
    else
        fFromCaptionBar = FALSE;

    return fFromCaptionBar;
}


/*****************************************************************************\
    FUNCTION: IContextMenu::QueryContextMenu
  
    DESCRIPTION:
        Given an existing context menu hmenu, insert new context menu
    items at location indexMenu (indexMenu = index to menu indexMenu), returning the
    number of menu items added.

    The incoming flags control how much goop we add to the menu.
    It is important not to add "Delete", "Rename", etc., to context
    menus that come from shortcuts, else the user gets hit with
    two "Delete" verbs, one to delete the object from the FTP site,
    and the other to delete the shortcut.  How confusing...

    hmenu     - destination menu
    indexMenu - location at which menu items should be inserted
    idCmdFirst - first available menu identifier
    idCmdLast - first unavailable menu identifier

    _UNDOCUMENTED_:  The "shared" menu items are not documented.
    Particularly gruesome, because the "shared" menu items are the
    only way to get Rename, Delete, etc. to work.  You can't roll
    your own, because those magics are handled partly in the
    enclosing shell view.

    _UNOBVIOUS_:  The context menu for the folder itself is
    extremely squirly.  It's not like a normal context menu.
    Rather, you add the "New" verb, and any custom verbs, but
    none of the standard folder verbs.

    PARAMS:
        Often, we need to key off strange parameter heiristicts to
    determine who our caller is so we don't enable certain items.
    "Rename" from the from CaptionBar is one example.  Here are what
    we are passed in the different situations:

    CaptionBar:
        QCM(hmenu, 1, idCmdFirst, idCmdLast, 0) m_pflHfpl contains 1
    FileMenu w/1 Selected:
        QCM(hmenu, 0, idCmdFirst, idCmdLast, CMF_DVFILE | CMF_NODEFAULT) m_pflHfpl contains 1
    0 Items Selected:
        QCM(hmenu, -1, idCmdFirst, idCmdLast, 0) m_pflHfpl contains 0
    1 Items Selected:
        QCM(hmenu, 0, idCmdFirst, idCmdLast, CMF_CANRENAME) m_pflHfpl contains 1
    2 Items Selected:
        QCM(hmenu, 0, idCmdFirst, idCmdLast, CMF_CANRENAME) m_pflHfpl contains 2
\*****************************************************************************/
HRESULT CFtpMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hr = S_OK;

    //  HACK: I assume that they are querying during a WM_INITMENUPOPUP or equivelant
    GetCursorPos(&m_ptNewItem);
    m_uFlags = uFlags;

    if (!m_fBackground)
    {
        BOOL fAllFolders = m_pflHfpl->AreAllFolders();

        //  _UNDOCUMENTED_: CMF_DVFILE is not a documented flag.
        if (!(uFlags & (CMF_DVFILE | CMF_VERBSONLY)))
        {
            DWORD sfgao = m_sfgao;

            // We don't support Delete or Rename from the Caption Bar
            if (_IsCallerCaptionBar(indexMenu, uFlags))
                sfgao &= ~(SFGAO_CANDELETE | SFGAO_CANRENAME); // Clear these two.

            //  Not on the "File" menu, and not from a shortcut.
            //  Add the "Delete", "Rename", etc. stuff, then go
            //  enable/disable them as needed.
            AddToPopupMenu(hmenu, IDM_ITEMCONTEXT, IDM_M_SHAREDVERBS, indexMenu, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
            _RemoveContextMenuItems(hmenu, idCmdFirst, sfgao);
        }

        // Add Download if there is anything inside.
        // The assertion makes sure that idCmdLast is set properly.
        ASSERT(IDC_ITEM_DOWNLOAD > IDC_ITEM_OPEN);
        if (!_IsCallerCaptionBar(indexMenu, uFlags))
        {
            // Don't add "Copy To Folder" in the caption bar because it doesn't work for the root of
            // an ftp server.  We aren't going to support it in subdirectories.
            AddToPopupMenu(hmenu, IDM_ITEMCONTEXT, IDM_M_VERBS, indexMenu, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
        }
        
        if (!(uFlags & CMF_NODEFAULT))
            SetMenuDefaultItem(hmenu, IDC_ITEM_DOWNLOAD + idCmdFirst, MM_ADDSEPARATOR);

        AddToPopupMenu(hmenu, IDM_ITEMCONTEXT, (fAllFolders ? IDM_M_FOLDERVERBS : IDM_M_FILEVERBS), indexMenu, idCmdFirst, 
                        idCmdLast, (_IsCallerCaptionBar(indexMenu, uFlags) ? 0 : MM_ADDSEPARATOR));
        if (fAllFolders && (SHELL_VERSION_W95NT4 == GetShellVersion()))
        {
            // On shell32 v3 (Win95 & NT4) I remove the 'Explore' verb because the shell has bugs
            // that aren't fixable are easy to fix.
            EVAL(DeleteMenu(hmenu, (IDC_ITEM_EXPLORE + idCmdFirst), MF_BYCOMMAND));
            TraceMsg(TF_FTPOPERATION, "QueryContextMenu() Removing 'Explorer' because it's shell v3");
            SetMenuDefaultItem(hmenu, idCmdFirst + IDC_ITEM_OPEN, 0);
        }
        else if (!(uFlags & CMF_NODEFAULT))
            SetMenuDefaultItem(hmenu, idCmdFirst + (((uFlags & CMF_EXPLORE) && fAllFolders)? IDC_ITEM_EXPLORE : IDC_ITEM_OPEN), 0);
    }
    else
    {                // Folder background menu
        AddToPopupMenu(hmenu, IDM_ITEMCONTEXT, IDM_M_BACKGROUNDVERBS, indexMenu, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
        // Did the menu come from the file menu?
        if (CMF_DVFILE == (CMF_DVFILE & uFlags))
        {
            // Yes, then we want to delete the "Properties" background menu item because one
            // was already merged in for the selected files.  The other Properties will
            // be there but grayed out if nothing was selected.
            EVAL(DeleteMenu(hmenu, (IDC_ITEM_BKGNDPROP + idCmdFirst), MF_BYCOMMAND));
        }

        MergeInToPopupMenu(hmenu, IDM_M_BACKGROUND_POPUPMERGE, indexMenu, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
    }

    if (EVAL(SUCCEEDED(hr)))
        hr = ResultFromShort(IDC_ITEM_MAX);

    _SHPrettyMenu(hmenu);
    return hr;
}

/*****************************************************************************\
 *
 *    IContextMenu::GetCommandString
 *
 *    Somebody wants to convert a command id into a string of some sort.
 *
\*****************************************************************************/

HRESULT CFtpMenu::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwRsv, LPSTR pszName, UINT cchMax)
{
    HRESULT hr = E_FAIL;
    BOOL fUnicode = FALSE;

    if (idCmd < IDC_ITEM_MAX)
    {
        switch (uFlags)
        {
        case GCS_HELPTEXTW:
            fUnicode = TRUE;
            // Fall thru...
        case GCS_HELPTEXTA:
            GetHelpText:
            if (EVAL(cchMax))
            {
                BOOL fResult;
                pszName[0] = '\0';
                 
                if (fUnicode)
                    fResult = LoadStringW(HINST_THISDLL, IDS_ITEM_HELP((UINT)idCmd), (LPWSTR)pszName, cchMax);
                else
                    fResult = LoadStringA(HINST_THISDLL, IDS_ITEM_HELP((UINT)idCmd), pszName, cchMax);
                if (EVAL(fResult))
                    hr = S_OK;
                else
                    hr = E_INVALIDARG;
            }
            else
                hr = E_INVALIDARG;
        break;

        case GCS_VALIDATEW:
        case GCS_VALIDATEA:
            hr = S_OK;
            break;

        case GCS_VERBW:
            fUnicode = TRUE;
            // Fall thru...
        case GCS_VERBA:
        {
            int ivi;
            for (ivi = 0; ivi < IVI_MAX; ivi++)
            {
                if (c_rgvi[ivi].idc == idCmd)
                {
                    if (fUnicode)
                        SHTCharToUnicode(c_rgvi[ivi].ptszCmd, (LPWSTR)pszName, cchMax);
                    else
                        SHTCharToAnsi(c_rgvi[ivi].ptszCmd, pszName, cchMax);

                    hr = S_OK;
                    break;
                }
            }

            if (!EVAL(ivi < IVI_MAX))
                hr = E_INVALIDARG;
            break;
        }

        default:
            hr = E_NOTIMPL;
            break;
        }
    }
    else
    {
        //  _UNOBVIOUS_:  Another place where PASTE rears its ugly head.
        //  We must generate the help text for it ourselves, even though
        //  the menu item "sort of" belongs to the shell.
        if ((idCmd == SHARED_EDIT_PASTE) &&
            ((uFlags == GCS_HELPTEXTW) || (uFlags == GCS_HELPTEXTA)))
        {
            goto GetHelpText;
        }

        hr = E_INVALIDARG;
    }

    return hr;
}


HRESULT UpdateDeleteProgressStr(IProgressDialog * ppd, LPCTSTR pszFileName)
{
    HRESULT hr = E_FAIL;
    TCHAR szTemplate[MAX_PATH];

    if (EVAL(LoadString(HINST_THISDLL, IDS_DELETING, szTemplate, ARRAYSIZE(szTemplate))))
    {
        TCHAR szStatusStr[MAX_PATH];
        WCHAR wzStatusStr[MAX_PATH];

        wnsprintf(szStatusStr, ARRAYSIZE(szStatusStr), szTemplate, pszFileName);
        SHTCharToUnicode(szStatusStr, wzStatusStr, ARRAYSIZE(wzStatusStr));
        EVAL(SUCCEEDED(hr = ppd->SetLine(2, wzStatusStr, FALSE, NULL)));
    }

    return hr;
}


HRESULT FtpChangeNotifyDirPatch(HWND hwnd, LONG wEventId, CFtpFolder * pff, LPCITEMIDLIST pidlFull, LPCITEMIDLIST pidl2, BOOL fTopLevel)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlParent = ILClone(pidlFull);

    if (pidlParent)
    {
        ILRemoveLastID(pidlParent);
        CFtpDir * pfd = pff->GetFtpDirFromPidl(pidlParent);
    
        if (pfd)
        {
            FtpChangeNotify(hwnd, wEventId, pff, pfd, ILFindLastID(pidlFull), pidl2, fTopLevel);
            pfd->Release();
        }

        ILFree(pidlParent);
    }

    return hr;
}


// The following struct is used when recursively downloading
// files/dirs from the FTP server after a "Download" verb.
typedef struct tagDELETESTRUCT
{
    LPCITEMIDLIST           pidlRoot;          // Base URL of the Download Source
    CFtpFolder *            pff;               // Allocator to create temp pidls.
    IMalloc *               pm;                // Allocator to create temp pidls.
    LPCMINVOKECOMMANDINFO   pdoi;              // Our call.
    HWND                    hwnd;              // HWND for UI
    CStatusBar *            psb;               // Used to display info during the delete
    IProgressDialog *       ppd;               // Used to display progress during the delete.
    DWORD                   dwTotalFiles;      // How many files are there to delete total.
    DWORD                   dwDeletedFiles;    // How many files have already been deleted.
    BOOL                    fInDeletePass;     // Are we in the 'Count Files to Delete' or 'Delete Files' pass?
} DELETESTRUCT;

/*****************************************************************************\
     FUNCTION: DeleteItemCB
 
    DESCRIPTION:
        This function will download the specified item and it's contents if it
    is a directory.
\*****************************************************************************/
HRESULT _DeleteItemPrep(HINTERNET hint, LPCITEMIDLIST pidlFull, BOOL fIsTopLevel, DELETESTRUCT * pDelete)
{
    HRESULT hr = S_OK;

    if (pDelete->ppd && pDelete->ppd->HasUserCancelled())
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

    if (SUCCEEDED(hr))  // May have been cancelled
    {
        DWORD dwError = 0;

        if (pDelete->fInDeletePass && pDelete->psb)
            pDelete->psb->SetStatusMessage(IDS_DELETING, FtpPidl_GetLastFileDisplayName(pidlFull));

        if (pDelete->fInDeletePass && pDelete->ppd)
            EVAL(SUCCEEDED(UpdateDeleteProgressStr(pDelete->ppd, FtpPidl_GetLastFileDisplayName(pidlFull))));

        // Is this a dir/folder that we need to recurse into? OR
        // Is this a SoftLink?
        if ((FILE_ATTRIBUTE_DIRECTORY & FtpPidl_GetAttributes(pidlFull)) ||
            (0 == FtpPidl_GetAttributes(pidlFull)))
        {
            // This is the head of the recursion.  We will do nothing now and we will
            // wait to delete the dir in the recursion tail because we need to wait
            // until all the files are gone.

            // Don't delete softlinks because of the recursion problem.
        }
        else
        {
            if (pDelete->fInDeletePass)
            {
                if (pDelete->ppd)
                    EVAL(SUCCEEDED(pDelete->ppd->SetProgress(pDelete->dwDeletedFiles, pDelete->dwTotalFiles)));

                // Contemplate adding a callback function in order to feed the status bar.
                hr = FtpDeleteFileWrap(hint, TRUE, FtpPidl_GetLastItemWireName(pidlFull));
                if (FAILED(hr))
                {
                    // We need to display the error now while the extended error info is still valid.
                    // This is because as we walk out of the resursive call, we will be calling
                    // FtpSetCurrentDirectory() which will wipe clean the extended error msg.
                    if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr))
                    {
                        DisplayWininetError(pDelete->hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DELETE, IDS_FTPERR_WININET, MB_OK, pDelete->ppd);
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);  // Wrong permissions
                    }
                }
                else
                    FtpChangeNotifyDirPatch(pDelete->hwnd, SHCNE_DELETE, pDelete->pff, pidlFull, NULL, fIsTopLevel);

                pDelete->dwDeletedFiles++;
                TraceMsg(TF_FTPOPERATION, "DeleteItemCB() FtpDeleteFileA() returned dwError=%#08lx.  File=%s", dwError, FtpPidl_GetLastFileDisplayName(pidlFull));
            }
            else
                pDelete->dwTotalFiles++;
        }
    }

    return hr;
}

HRESULT _DeleteItemCleanUp(HRESULT hr, DELETESTRUCT * pDelete)
{
    if (pDelete->ppd && pDelete->ppd->HasUserCancelled())
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

    if (pDelete->fInDeletePass) // Only display errors and fire ChangeNotify if in Delete pass.
    {
        if ((FAILED(hr)) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr))
        {
            int nResult = DisplayWininetError(pDelete->hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DELETE, IDS_FTPERR_WININET, MB_OK, pDelete->ppd);
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // Don't display any more error dialogs.
        }
    }

    return hr;
}


HRESULT FtpRemoveDirectoryWithCN(HWND hwnd, HINTERNET hint, CFtpFolder * pff, LPCITEMIDLIST pidlFull, BOOL fIsTopLevel)
{
    HRESULT hr = S_OK;

    hr = FtpRemoveDirectoryWrap(hint, TRUE, FtpPidl_GetLastItemWireName(pidlFull));
    if (SUCCEEDED(hr))
    {
        hr = FtpChangeNotifyDirPatch(hwnd, SHCNE_RMDIR, pff, pidlFull, NULL, fIsTopLevel);
        TraceMsg(TF_WININET_DEBUG, "FtpRemoveDirectoryWithCN() FtpRemoveDirectory(%hs) returned %#08lx", FtpPidl_GetLastItemWireName(pidlFull), hr);
    }

    return hr;
}


INT ILCountItemIDs(LPCITEMIDLIST pidl)
{
    INT nCount = 0;

    if (pidl)
    {
        while (!ILIsEmpty(pidl))
        {
            pidl = _ILNext(pidl);
            nCount++;
        }
    }

    return nCount;
}


/*****************************************************************************\
     FUNCTION: _IsTopLevel
 
    DESCRIPTION:
\*****************************************************************************/
BOOL _IsTopLevel(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlCurrent)
{
    INT nRoot = ILCountItemIDs(pidlRoot);
    INT nCurrent = ILCountItemIDs(pidlCurrent);

    // It is the root if nCurrent has no more than 1 more than nRoot
    return (((nRoot + 1) >= nCurrent) ? TRUE : FALSE);
}


/*****************************************************************************\
     FUNCTION: DeleteItemCB
 
    DESCRIPTION:
        This function will download the specified item and it's contents if it
    is a directory.  Since this is in the line of recursion, we need to have the
    stack be as small as possible.  Therefore, we call _DeleteItemPrep() to use
    as much stack as needed to do the majority of the work and the clean up the
    stack before we do the recursion.  The only information we need from it is
    pszUrlPath which we put on the stack and heap and clean up our selves.
\*****************************************************************************/
HRESULT DeleteItemCB(LPVOID pvFuncCB, HINTERNET hint, LPCITEMIDLIST pidlFull, BOOL * pfValidhinst, LPVOID pvData)
{
    DELETESTRUCT * pDelete = (DELETESTRUCT *) pvData;
    BOOL fIsTopLevel = _IsTopLevel(pDelete->pidlRoot, pidlFull);
    HRESULT hr = _DeleteItemPrep(hint, pidlFull, fIsTopLevel, pDelete);

    if (SUCCEEDED(hr) && (FILE_ATTRIBUTE_DIRECTORY & FtpPidl_GetAttributes(pidlFull)))
    {
        hr = EnumFolder((LPFNPROCESSITEMCB) pvFuncCB, hint, pidlFull, pDelete->pff->GetCWireEncoding(), pfValidhinst, pvData);
        if (SUCCEEDED(hr))
        {
            if (pDelete->fInDeletePass)
            {
                hr = FtpRemoveDirectoryWithCN(pDelete->hwnd, hint, pDelete->pff, pidlFull, fIsTopLevel);
//                TraceMsg(TF_FTPOPERATION, "DeleteItemCB() FtpRemoveDirectoryA() returned hr=%#08lx.", hr);
                pDelete->dwDeletedFiles++;
            }
            else
                pDelete->dwTotalFiles++;
        }
    }

    hr = _DeleteItemCleanUp(hr, pDelete);
    return hr;
}



/*****************************************************************************\
    FUNCTION: _InvokeLoginAsVerb

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpMenu::_InvokeLoginAsVerb(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = E_FAIL;

    if (EVAL(m_pfd))
        hr = LoginAs(pici->hwnd, m_pff, m_pfd, _punkSite);

    return hr;
}



/*****************************************************************************\
    FUNCTION: _InvokeNewFolderVerb

    DESCRIPTION:
        The user just selected "New Folder", so we need to create a new folder.
\*****************************************************************************/
HRESULT CFtpMenu::_InvokeNewFolderVerb(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = E_FAIL;

    if (m_pfd)
        hr = CreateNewFolder(m_hwnd, m_pff, m_pfd, _punkSite, (m_uFlags & CMF_DVFILE), m_ptNewItem);

    return hr;
}


/*****************************************************************************\
    FUNCTION: _InvokeDeleteVerb

    DESCRIPTION:
        The user just selected file(s) and/or folder(s) and selected the
    "download" verb.  We need to:
    1. Display UI to ask the user for the destination directory.
    2. Download each item (pidl) into that directory.
\*****************************************************************************/
HRESULT CFtpMenu::_InvokeDeleteVerb(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = S_OK;

    if (EVAL(m_pfd))
    {
        if (m_sfgao & SFGAO_CANDELETE)
        {
            if (!(pici->fMask & CMIC_MASK_FLAG_NO_UI))
            {
                ASSERT(pici->hwnd);
                switch (FtpConfirmDeleteDialog(ChooseWindow(pici->hwnd, m_hwnd), m_pflHfpl, m_pff))
                {
                case IDC_REPLACE_YES:
                    hr = S_OK;
                    break;

                default:                
                    // FALLTHROUGH
                case IDC_REPLACE_CANCEL:
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);       // Cancel all copies.
                    break;

                case IDC_REPLACE_NO:
                    hr = S_FALSE;
                    break;
                }
            }
            else
                hr = S_OK;

            if (hr == S_OK)
            {
                CStatusBar * psb = _GetStatusBar();
                IProgressDialog * ppd = CProgressDialog_CreateInstance(IDS_DELETE_TITLE, IDA_FTPDELETE);
                LPITEMIDLIST pidlRoot = ILClone(m_pfd->GetPidlReference());
                DELETESTRUCT delStruct = {pidlRoot, m_pff, m_pff->m_pm, pici, ChooseWindow(pici->hwnd, m_hwnd), psb, ppd, 0, 0, FALSE};

                if (EVAL(SUCCEEDED(hr)))
                {
                    HINTERNET hint;

                    m_pfd->GetHint(NULL, NULL, &hint, _punkSite, m_pff);
                    if (EVAL(hint))
                    {
                        HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

                        if (EVAL(ppd))
                        {
                            WCHAR wzProgressDialogStr[MAX_PATH];
                            HWND hwndParent = NULL;

                            // BUGBUG: DefView (defview.cpp CDefView::QueryInterface()) doesn't support IOleWindow so our
                            //         progress dialog isn't correctly parented.

                            // If the caller was nice enough to SetSite() with their punk, I will be nice enough to make
                            // their window may progress dialog's parent window.
                            IUnknown_GetWindow(_punkSite, &hwndParent);
                            if (!hwndParent)
                                hwndParent = m_hwnd;

                            // Normally we always want UI, but in one case we don't.  If the
                            // user does a DROPEFFECT_MOVE, it really is a DROPEFFECT_COPY
                            // and then a IContextMenu::InvokeCommand(SZ_VERB_DELETEA).
                            // The progress was done in the copy thread and isn't needed
                            // in the delete thread.
//                            ASSERT(hwndParent);

                            // We give a NULL punkEnableModless because we don't want to go modal.
                            EVAL(SUCCEEDED(ppd->StartProgressDialog(hwndParent, NULL, PROGDLG_AUTOTIME, NULL)));
                            // Tell the user we are calculating how long it will take.
                            if (EVAL(LoadStringW(HINST_THISDLL, IDS_PROGRESS_DELETETIMECALC, wzProgressDialogStr, ARRAYSIZE(wzProgressDialogStr))))
                                EVAL(SUCCEEDED(ppd->SetLine(2, wzProgressDialogStr, FALSE, NULL)));
                        }

                        // Tell the user we are calculating how long it will take.
                        hr = m_pflHfpl->RecursiveEnum(pidlRoot, DeleteItemCB, hint, (LPVOID) &delStruct);
                        if (ppd)
                        {
                            // Reset because RecursiveEnum(DeleteItemCB) can take a long time and the estimated time
                            // is based on the time between ::StartProgressDialog() and the first
                            // ::SetProgress() call.
                            EVAL(SUCCEEDED(ppd->Timer(PDTIMER_RESET, NULL)));
                        }

                        delStruct.fInDeletePass = TRUE;
                        if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)  // This is the only error we care about.
                        {
                            m_pflHfpl->UseCachedDirListings(TRUE);    // Get the perf advantage now because we just updated the cache a few lines up.
                            hr = m_pflHfpl->RecursiveEnum(pidlRoot, DeleteItemCB, hint, (LPVOID) &delStruct);
                        }

                        if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr))
                        {
                            DisplayWininetError(pici->hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DELETE, IDS_FTPERR_WININET, MB_OK, ppd);
                            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);  // Wrong permissions
                        }

                        if (psb)
                            psb->SetStatusMessage(IDS_EMPTY, NULL);
                        if (ppd)
                        {
                            ppd->StopProgressDialog();
                            ppd->Release();
                        }

                        ILFree(pidlRoot);
                        SetCursor(hCursorOld);  // Restore old cursor.
                    }

                    m_pfd->ReleaseHint(hint);
                }
            }
        }
        else
        {
            DisplayWininetError(pici->hwnd, TRUE, ResultFromScode(E_ACCESSDENIED), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DELETE, IDS_FTPERR_WININET, MB_OK, NULL);
            hr = E_ACCESSDENIED;  // Wrong permissions
        }
    }

    return hr;
}




/*****************************************************************************\
    FUNCTION: _GetStatusBar

    DESCRIPTION:
\*****************************************************************************/
CStatusBar * CFtpMenu::_GetStatusBar(void)
{
    return GetCStatusBarFromDefViewSite(_punkSite);
}


/*****************************************************************************\
     FUNCTION: FileSizeCountItemCB
 
    DESCRIPTION:
        This function will download the specified item and it's contents if it
    is a directory.
\*****************************************************************************/
HRESULT FileSizeCountItemCB(LPVOID pvFuncCB, HINTERNET hint, LPCITEMIDLIST pidlFull, BOOL * pfValidhinst, LPVOID pvData)
{
    PROGRESSINFO * pProgInfo = (PROGRESSINFO *) pvData;
    HRESULT hr = S_OK;

    if (pProgInfo->ppd && pProgInfo->ppd->HasUserCancelled())
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

    if (SUCCEEDED(hr))
    {
        // Is this a dir/folder that we need to recurse into?
        if (FILE_ATTRIBUTE_DIRECTORY & FtpPidl_GetAttributes(pidlFull))
            hr = EnumFolder((LPFNPROCESSITEMCB) pvFuncCB, hint, pidlFull, NULL, pfValidhinst, pvData);
        else
            pProgInfo->uliBytesTotal.QuadPart += FtpPidl_GetFileSize(pidlFull);
    }

    return hr;
}


HRESULT UpdateDownloadProgress(PROGRESSINFO * pProgInfo, LPCITEMIDLIST pidlFull, LPCWSTR pwzTo, LPCWSTR pwzFileName)
{
    HRESULT hr;
    WCHAR wzTemplate[MAX_PATH];
    WCHAR wzStatusText[MAX_PATH];
    WCHAR wzFrom[MAX_PATH];
    LPITEMIDLIST pidlParent = ILClone(pidlFull);

    if (pidlParent)
    {
        ILRemoveLastID(pidlParent);
        FtpPidl_GetDisplayName(pidlParent, wzFrom, ARRAYSIZE(wzFrom));
        ILFree(pidlParent);
    }

    // Give the directories some weight because the user may be copying tons of empty directories.
    EVAL(SUCCEEDED(pProgInfo->ppd->SetProgress64(pProgInfo->uliBytesCompleted.QuadPart, pProgInfo->uliBytesTotal.QuadPart)));

    // Generate the string "Downloading <FileName>..." status string
    EVAL(LoadStringW(HINST_THISDLL, IDS_DOWNLOADING, wzTemplate, ARRAYSIZE(wzTemplate)));
    wnsprintfW(wzStatusText, ARRAYSIZE(wzStatusText), wzTemplate, pwzFileName);
    EVAL(SUCCEEDED(pProgInfo->ppd->SetLine(1, wzStatusText, FALSE, NULL)));

    // Generate the string "From <SrcFtpUrlDir> to <DestFileDir>" status string
    if (EVAL(SUCCEEDED(hr = CreateFromToStr(wzStatusText, ARRAYSIZE(wzStatusText), wzFrom, pwzTo))))
        EVAL(SUCCEEDED(hr = pProgInfo->ppd->SetLine(2, wzStatusText, FALSE, NULL)));    // Line one is the file being copied.

    return hr;
}


/*****************************************************************************\
    ConfirmDownloadReplace

    Callback procedure that checks if this file really ought to be
    copied.

    Returns S_OK if the file should be copied.
    Returns S_FALSE if the file should not be copied.

    - If the user cancelled, then say S_FALSE from now on.
    - If the user said Yes to All, then say S_OK.
    - If there is no conflict, then say S_OK.
    - If the user said No to All, then say S_FALSE.
    - Else, ask the user what to do.

    Note that the order of the tests above means that if you say
    "Yes to All", then we don't waste our time doing overwrite checks.

    _GROSS_:  NOTE! that we don't try to uniquify the name, because
    WinINet doesn't support the STOU (store unique) command, and
    there is no way to know what filenames are valid on the server.
\*****************************************************************************/
HRESULT ConfirmDownloadReplace(LPCWSTR pwzDestPath, LPCITEMIDLIST pidlSrcFTP, OPS * pOps, HWND hwnd, CFtpFolder * pff, CFtpDir * pfd, int nObjs, BOOL * pfDeleteRequired)
{
    HRESULT hr = S_OK;

    ASSERT(hwnd);
    *pfDeleteRequired = FALSE;
    if (*pOps == opsCancel)
        hr = S_FALSE;
    else if (*pOps == opsYesToAll)
    {
        *pfDeleteRequired = PathFileExistsW(pwzDestPath);
        hr = S_OK;
    }
    else 
    {
        if (PathFileExistsW(pwzDestPath))
        {
            // It exists, so worry.
            if (*pOps == opsNoToAll)
                hr = S_FALSE;
            else
            {
                FTP_FIND_DATA wfdSrc;
                WIN32_FIND_DATA wfdDest;
                HANDLE hfindDest;
                FILETIME ftUTC;

                *pfDeleteRequired = TRUE;
                hfindDest = FindFirstFileW(pwzDestPath, &wfdDest);

                ftUTC = wfdDest.ftLastWriteTime;
                FileTimeToLocalFileTime(&ftUTC, &wfdDest.ftLastWriteTime);   // UTC->LocalTime
                EVAL(S_OK == Win32FindDataFromPidl(pidlSrcFTP, (LPWIN32_FIND_DATA)&wfdSrc, FALSE, FALSE));
                if (EVAL(hfindDest != INVALID_HANDLE_VALUE))
                {
                    // BUGBUG/TODO: Do we need to set modal?
                    switch (FtpConfirmReplaceDialog(hwnd, &wfdSrc, &wfdDest, nObjs, pff))
                    {
                    case IDC_REPLACE_YESTOALL:
                        *pOps = opsYesToAll;
                        // FALLTHROUGH

                    case IDC_REPLACE_YES:
                        hr = S_OK;
                        break;

                    case IDC_REPLACE_NOTOALL:
                        *pOps = opsNoToAll;
                        // FALLTHROUGH

                    case IDC_REPLACE_NO:
                        hr = S_FALSE;
                        break;

                    default:
                        ASSERT(0);        // Huh?
                        // FALLTHROUGH

                    case IDC_REPLACE_CANCEL:
                        *pOps = opsCancel;
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                        break;
                    }
                    FindClose(hfindDest);
                }
            }
        }
    }

    return hr;
}


// The following struct is used when recursively downloading
// files/dirs from the FTP server after a "Download" verb.
typedef struct tagDOWNLOADSTRUCT
{
    LPCWSTR             pwzDestRootPath;    // Dir on FileSys of the Download Destination
    LPCITEMIDLIST       pidlRoot;           // Base URL of the Download Source
    DWORD               dwInternetFlags;    // Binary, ASCII, AutoDetect?
    HWND                hwndParent;         // hwnd for Confirm UI
    OPS                 ops;                // Do we cancel?
    CFtpFolder *        pff;
    CFtpDir *           pfd;

    // Progress
    PROGRESSINFO        progInfo;
} DOWNLOADSTRUCT;


/*****************************************************************************\
     FUNCTION: _CalcDestName
 
    DESCRIPTION:
        This recursive function starts at pwzDestDir as the dest FS path and
    pidlRoot as the src ftp path.  We need to construct pwzDestPath which
    is the current path.  This will be done by adding the relative path
    (pidlFull - pidlRoot) to pwzDestDir.  pidlFull can point to either a file
    or a directory.
 
    PARAMETERS: (Example. "C:\dir1\dir2\dir3\file.txt")
         pwzDestParentPath: "C:\dir1\dir2\dir3"
         pwzDestDir: "C:\dir1\dir2\dir3\file.txt"
         pwzDestFileName: "file.txt"

    Example. "C:\dir1\dir2\dir3\"
         pwzDestParentPath: "C:\dir1\dir2"
         pwzDestDir: "C:\dir1\dir2\dir3"
         pwzDestFileName: "dir3"
\*****************************************************************************/
HRESULT _CalcDestName(LPCWSTR pwzDestDir, LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlFull, LPWSTR pwzDestParentPath, DWORD cchDestParentPathSize,
                      LPWSTR pwzDestPath, DWORD cchDestPathSize, LPWSTR pwzDestFileName, DWORD cchDestFileNameSize)
{
    HRESULT hr = S_OK;
    WCHAR wzFtpPathTemp[MAX_PATH];
    WCHAR wzFSPathTemp[MAX_PATH];
    LPITEMIDLIST pidlRootIterate = (LPITEMIDLIST) pidlRoot;    // I promise to iterate only
    LPITEMIDLIST pidlFullIterate = (LPITEMIDLIST) pidlFull;    // I promise to iterate only

    // This one is easy.
    StrCpyNW(pwzDestFileName, FtpPidl_GetLastFileDisplayName(pidlFull), cchDestFileNameSize);  // The dest filename is easy.

    // Let's find the relative path between pidlRoot and pidlFull.
    while (!ILIsEmpty(pidlRootIterate) && !ILIsEmpty(pidlFullIterate) && FtpItemID_IsEqual(pidlRootIterate, pidlFullIterate))
    {
        pidlFullIterate = _ILNext(pidlFullIterate);
        pidlRootIterate = _ILNext(pidlRootIterate);
    }

    ASSERT(ILIsEmpty(pidlRootIterate) && !ILIsEmpty(pidlFullIterate));  // Asure pidlFull is a superset of pidlRoot
    LPITEMIDLIST pidlParent = ILClone(pidlFullIterate);

    if (pidlParent)
    {
        ILRemoveLastID(pidlParent); // Remove the item that will be created (file or dir)

        GetDisplayPathFromPidl(pidlParent, wzFtpPathTemp, ARRAYSIZE(wzFtpPathTemp), TRUE);   // Full path w/o last item.
        StrCpyNW(pwzDestParentPath, pwzDestDir, cchDestParentPathSize);  // Put the base dest.
        UrlPathToFilePath(wzFtpPathTemp, wzFSPathTemp, ARRAYSIZE(wzFSPathTemp));
        if (!PathAppendW(pwzDestParentPath, wzFSPathTemp))
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);    // Path too long, probably.

        ILFree(pidlParent);
    }

    if (SUCCEEDED(hr))
    {
        GetDisplayPathFromPidl(pidlFullIterate, wzFtpPathTemp, ARRAYSIZE(wzFSPathTemp), FALSE);   // Full Path including item.
        StrCpyNW(pwzDestPath, pwzDestDir, cchDestParentPathSize);  // Put the base dest.
        UrlPathToFilePath(wzFtpPathTemp, wzFSPathTemp, ARRAYSIZE(wzFSPathTemp));
        if (!PathAppendW(pwzDestPath, wzFSPathTemp))
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);    // Path too long, probably.
    }

    return hr;
}


// This defines the size of a directory measured by the amount of time it would take compared to a file.
#define VIRTUAL_DIR_SIZE        1000        // about 1k.

/*****************************************************************************\
     FUNCTION: DownloadItemStackPig
 
    DESCRIPTION:
        This function will download the specified item and it's contents if it
    is a directory.
\*****************************************************************************/
HRESULT DownloadItemStackPig(HINTERNET hint, LPCITEMIDLIST pidlFull, BOOL * pfValidhinst, DOWNLOADSTRUCT * pDownLoad, CFtpDir ** ppfd)
{
    HRESULT hr;
    WCHAR wzDestParentPath[MAX_PATH];       // If item is "C:\dir1\dir2copy\", the this is "C:\dir1"
    WCHAR wzDestPath[MAX_PATH];             // This is "C:\dir1\dir2copy\"
    WCHAR wzDestFileName[MAX_PATH];         // This is "dir2copy"

    hr = _CalcDestName(pDownLoad->pwzDestRootPath, pDownLoad->pidlRoot, pidlFull, wzDestParentPath, ARRAYSIZE(wzDestParentPath), wzDestPath, ARRAYSIZE(wzDestPath), wzDestFileName, ARRAYSIZE(wzDestFileName));
    if (SUCCEEDED(hr))
    {
        if (pDownLoad->progInfo.ppd)
            EVAL(SUCCEEDED(UpdateDownloadProgress(&(pDownLoad->progInfo), pidlFull, wzDestParentPath, wzDestFileName)));

        if (pDownLoad->progInfo.ppd && pDownLoad->progInfo.ppd->HasUserCancelled())
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        else
        {
            // Is this a dir/folder that we need to recurse into?
            if (FILE_ATTRIBUTE_DIRECTORY & FtpPidl_GetAttributes(pidlFull))
            {
                // Yes, so let's go...

                if (EVAL((PathFileExistsW(wzDestPath) && PathIsDirectoryW(wzDestPath)) ||
                            CreateDirectoryW(wzDestPath, NULL)))
                {
                    EVAL(SetFileAttributes(wzDestPath, FtpPidl_GetAttributes(pidlFull)));
                    hr = pDownLoad->pfd->GetFtpSite()->GetFtpDir(pidlFull, ppfd);
                    if (!EVAL(SUCCEEDED(hr)))
                        TraceMsg(TF_ERROR, "DownloadItemStackPig() GetFtpDir failed hr=%#08lx", hr);
                }
                else
                {
                    hr = E_FAIL;
                    TraceMsg(TF_ERROR, "DownloadItemStackPig() CreateDirectory or PathFileExists failed hr=%#08lx", hr);
                }
            }
            else
            {
                BOOL fDeleteRequired;
                ULARGE_INTEGER uliFileSize;

                pDownLoad->progInfo.dwCompletedInCurFile = 0;
                pDownLoad->progInfo.dwLastDisplayed = 0;

                hr = ConfirmDownloadReplace(wzDestPath, pidlFull, &(pDownLoad->ops), GetProgressHWnd(pDownLoad->progInfo.ppd, pDownLoad->hwndParent), pDownLoad->pff, pDownLoad->pfd, 1, &fDeleteRequired);
                if (S_OK == hr)
                {
                    if (fDeleteRequired)
                    {
                        if (!DeleteFileW(wzDestPath))
                            hr = HRESULT_FROM_WIN32(GetLastError());
                    }

                    // Don't copy the file if it's a SoftLink because of the possible
                    // recursion case.
                    if (EVAL(SUCCEEDED(hr)) && (0 != FtpPidl_GetAttributes(pidlFull)))
                    {
                        // Contemplate adding a callback function in order to feed the status bar.
                        hr = FtpGetFileExPidlWrap(hint, TRUE, pidlFull, wzDestPath, TRUE, FtpPidl_GetAttributes(pidlFull), pDownLoad->dwInternetFlags, (DWORD_PTR)&(pDownLoad->progInfo));
                        if (FAILED(hr))
                        {
                            if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
                            {
                                // We need to display the error now while the extended error info is still valid.
                                // This is because as we walk out of the resursive call, we will be calling
                                // FtpSetCurrentDirectory() which will wipe clean the extended error msg.
                                DisplayWininetError(pDownLoad->hwndParent, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DOWNLOADING, IDS_FTPERR_WININET, MB_OK, pDownLoad->progInfo.ppd);
                                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // Don't display any more error dialogs.
                            }
                        }
                        else
                        {
                            // The docs imply that (FILE_SHARE_READ | FILE_SHARE_WRITE) means that other callers need both, but
                            // I want them to be able to use either.
                            HANDLE hFile = CreateFileW(wzDestPath, GENERIC_WRITE, (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FtpPidl_GetAttributes(pidlFull), NULL);

                            // FtpGetFile() won't set the time/date correctly, so we will.
                            if (EVAL(INVALID_HANDLE_VALUE != hFile))
                            {
                                FILETIME ftLastWriteTime = FtpPidl_GetFileTime(ILFindLastID(pidlFull));

                                // Since the file time on the disk is stored in a time zone independent way (UTC)
                                // we have a problem because FTP WIN32_FIND_DATA is in the local time zone.  So we
                                // need to convert the FTP local time to UTC when we set the file.
                                // Note that we are using an optimization that uses the fact that FTP always
                                // has the same time for LastAccessTime, LastWriteTime, and CreationTime.
    //                                ASSERT(pwfd->ftCreationTime.dwLowDateTime = pwfd->ftLastAccessTime.dwLowDateTime = pwfd->ftLastWriteTime.dwLowDateTime);
    //                                ASSERT(pwfd->ftCreationTime.dwHighDateTime = pwfd->ftLastAccessTime.dwHighDateTime = pwfd->ftLastWriteTime.dwHighDateTime);

                                // priv.h has notes on how time works.
                                SetFileTime(hFile, &ftLastWriteTime, &ftLastWriteTime, &ftLastWriteTime);
                                CloseHandle(hFile);
                            }
                            SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, wzDestPath, NULL);
                        }
                    }
                }

                uliFileSize.QuadPart = FtpPidl_GetFileSize(pidlFull);
                pDownLoad->progInfo.uliBytesCompleted.QuadPart += uliFileSize.QuadPart;
            }
        }
    }

    if (pfValidhinst)
        *pfValidhinst = (pDownLoad->progInfo.hint ? TRUE : FALSE);

    return hr;
}


/*****************************************************************************\
     FUNCTION: DownloadItemCB
 
    DESCRIPTION:
        This function will download the specified item and it's contents if it
    is a directory.
\*****************************************************************************/
HRESULT DownloadItemCB(LPVOID pvFuncCB, HINTERNET hint, LPCITEMIDLIST pidlFull, BOOL * pfValidhinst, LPVOID pvData)
{
    DOWNLOADSTRUCT * pDownLoad = (DOWNLOADSTRUCT *) pvData;
    LPFNPROCESSITEMCB pfnProcessItemCB = (LPFNPROCESSITEMCB) pvFuncCB;
    CFtpDir * pfdNew = NULL;
    HRESULT hr = DownloadItemStackPig(hint, pidlFull, pfValidhinst, pDownLoad, &pfdNew);

    if (SUCCEEDED(hr) && pfdNew)    // pfdNew Maybe NULL if cancelled
    {
        CFtpDir * pfdOriginal = pDownLoad->pfd;

        pDownLoad->pfd = pfdNew;
        hr = EnumFolder(pfnProcessItemCB, hint, pidlFull, pDownLoad->pff->GetCWireEncoding(), pfValidhinst, pvData);
        pDownLoad->pfd = pfdOriginal;

        pfdNew->Release();
    }

    return hr;
}


// BUGBUG/TODO: First, make this work on pidls that Bind to IStorages.
//              Second, nuke the CDownloadDialog code.
HRESULT ShowDownloadDialog(HWND hwnd, LPTSTR pszPath, DWORD cchSize)
{
    TCHAR szMessage[MAX_URL_STRING];
    HRESULT hr;
    LPITEMIDLIST pidlDefault = NULL;
    LPITEMIDLIST pidlFolder = NULL;
    HKEY hkey = NULL;
    IStream * pstrm = NULL;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, SZ_REGKEY_MICROSOFTSOFTWARE, 0, (KEY_READ | KEY_WRITE), &hkey))
    {
        pstrm = SHOpenRegStream(hkey, SZ_REGKEY_FTPCLASS, SZ_REGVALUE_DOWNLOAD_DIR, STGM_READWRITE);
        if (pstrm)
            ILLoadFromStream(pstrm, &pidlDefault);  // Will return (NULL == pidlDefault) if the reg value is empty.
    }

    if (!pidlDefault && (SHELL_VERSION_W95NT4 == GetShellVersion()))   // If reg key is empty.
        pidlDefault = SHCloneSpecialIDList(NULL, CSIDL_PERSONAL, TRUE);

    EVAL(LoadString(HINST_THISDLL, IDS_DLG_DOWNLOAD_TITLE, szMessage, ARRAYSIZE(szMessage)));
    hr = BrowseForDir(hwnd, szMessage, pidlDefault, &pidlFolder);
    if (pstrm)
    {
        // Do we want to save the new pidl?
        if (S_OK == hr)
        {
            LARGE_INTEGER li = {0};
            ULARGE_INTEGER uli = {0};

            // rewind the stream to the beginning so that when we
            // add a new pidl it does not get appended to the first one
            pstrm->Seek(li, STREAM_SEEK_SET, &uli);
            ILSaveToStream(pstrm, pidlFolder);
        }

        pstrm->Release();
    }

    if (S_OK == hr)
    {
        ASSERT(cchSize >= MAX_PATH);        // This is an assumption SHGetPathFromIDList makes.
        hr = (SHGetPathFromIDList(pidlFolder, pszPath) ? S_OK : E_FAIL);
    }

    if (hkey)
        RegCloseKey(hkey);

    if (pidlDefault)
        ILFree(pidlDefault);

    if (pidlFolder)
        ILFree(pidlFolder);

    return hr;
}


/*****************************************************************************\
    FUNCTION: _InvokeDownloadVerb

    DESCRIPTION:
        The user just selected file(s) and/or folder(s) and selected the
    "download" verb.  We need to:
    1. Display UI to ask the user for the destination directory.
    2. Download each item (pidl) into that directory.
\*****************************************************************************/
HRESULT CFtpMenu::_InvokeDownloadVerb(LPCMINVOKECOMMANDINFO pici)
{
    if (ZoneCheckPidlAction(_punkSite, URLACTION_SHELL_FILE_DOWNLOAD, m_pff->GetPrivatePidlReference(), (PUAF_DEFAULT | PUAF_WARN_IF_DENIED)))
    {
        TCHAR szDestDir[MAX_PATH];
//      DWORD dwDownloadType;
        HRESULT hr = ShowDownloadDialog(pici->hwnd, szDestDir, ARRAYSIZE(szDestDir));

        if (S_OK == hr)
        {
            HANDLE hThread;

            while (m_pszDownloadDir)
                Sleep(0);   // Wait until the other thread is done.

            Str_SetPtr(&m_pszDownloadDir, szDestDir);
//          m_dwDownloadType = dwDownloadType;

            AddRef();       // The thread will hold a ref.
            DWORD dwHack;   // Win95 fails CreateThread() if pdwThreadID is NULL.
            hThread = CreateThread(NULL, 0, CFtpMenu::DownloadThreadProc, this, 0, &dwHack);
            if (!hThread)
            {
                // Failed to create the thread.
                Release();       // The thread will hold a ref.
                Str_SetPtr(&m_pszDownloadDir, NULL);        // Clear this value so other thread an use it.
            }
            else
                Sleep(100);   // Give the thread a second to copy the variables.
        }
    }

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _DownloadThreadProc

    DESCRIPTION:
\*****************************************************************************/
DWORD CFtpMenu::_DownloadThreadProc(void)
{
    if (EVAL(m_pfd))
    {
        TCHAR szUrl[MAX_URL_STRING];
        WCHAR wzDestDir[MAX_PATH];
        HINTERNET hint;
        LPITEMIDLIST pidlRoot = ILClone(m_pfd->GetPidlReference());
        DOWNLOADSTRUCT downloadStruct = {wzDestDir, pidlRoot, m_dwDownloadType, m_hwnd, opsPrompt, m_pff, m_pfd, 0, 0, 0, 0, 0};
        CFtpPidlList * pflHfpl = NULL;      // We need a copy because the caller may select other files and execute a verb during the download.
        HRESULT hrOleInit = SHCoInitialize();
        HRESULT hr = HRESULT_FROM_WIN32(ERROR_INTERNET_CANNOT_CONNECT);

        IUnknown_Set(&pflHfpl, m_pflHfpl);
        StrCpyNW(wzDestDir, m_pszDownloadDir, ARRAYSIZE(wzDestDir));
        Str_SetPtr(&m_pszDownloadDir, NULL);        // Clear this value so other thread an use it.
        
        m_pfd->GetHint(NULL, NULL, &hint, _punkSite, m_pff);
        if (hint)
        {
            BOOL fReleaseHint = TRUE;

            // BUGBUG: Do we need to pass punkEnableModless?
            // Is the disk ready? (Floppy, CD, net share)
            if (SUCCEEDED(SHPathPrepareForWriteWrapW(m_hwnd, NULL, wzDestDir, FO_COPY, SHPPFW_DEFAULT)))    // Check and prompt if necessary.
            {
                hr = UrlCreateFromPidl(pidlRoot, SHGDN_FORPARSING, szUrl, ARRAYSIZE(szUrl), ICU_ESCAPE | ICU_USERNAME, FALSE);
                if (EVAL(SUCCEEDED(hr)))
                {
                    PROGRESSINFO progInfo;
                    progInfo.uliBytesCompleted.QuadPart = 0;
                    progInfo.uliBytesTotal.QuadPart = 0;

                    downloadStruct.progInfo.hint = hint;
                    downloadStruct.progInfo.ppd = CProgressDialog_CreateInstance(IDS_COPY_TITLE, IDA_FTPDOWNLOAD);
                    if (downloadStruct.progInfo.ppd)
                    {
                        HWND hwndParent = NULL;
                        WCHAR wzProgressDialogStr[MAX_PATH];

                        // If the caller was nice enough to SetSite() with their punk, I will be nice enough to make
                        // their window may progress dialog's parent window.
                        IUnknown_GetWindow(_punkSite, &hwndParent);
                        if (!hwndParent)
                            hwndParent = m_hwnd;

                        // We give a NULL punkEnableModless because we don't want to go modal.
                        downloadStruct.progInfo.ppd->StartProgressDialog(hwndParent, NULL, PROGDLG_AUTOTIME, NULL);
                        // Tell the user we are calculating how long it will take.
                        if (EVAL(LoadStringW(HINST_THISDLL, IDS_PROGRESS_DOWNLOADTIMECALC, wzProgressDialogStr, ARRAYSIZE(wzProgressDialogStr))))
                            EVAL(SUCCEEDED(downloadStruct.progInfo.ppd->SetLine(2, wzProgressDialogStr, FALSE, NULL)));
                        InternetSetStatusCallbackWrap(hint, TRUE, FtpProgressInternetStatusCB);
                        progInfo.ppd = downloadStruct.progInfo.ppd;
                    }

                    hr = pflHfpl->RecursiveEnum(pidlRoot, FileSizeCountItemCB, hint, (LPVOID) &progInfo);
                    if (downloadStruct.progInfo.ppd)
                    {
                        // Reset because RecursiveEnum(FileSizeCountItemCB) can take a long time and the estimated time
                        // is based on the time between ::StartProgressDialog() and the first
                        // ::SetProgress() call.
                        EVAL(SUCCEEDED(downloadStruct.progInfo.ppd->Timer(PDTIMER_RESET, NULL)));
                    }

                    if (SUCCEEDED(hr))
                    {
                        downloadStruct.progInfo.uliBytesCompleted.QuadPart = progInfo.uliBytesCompleted.QuadPart;
                        downloadStruct.progInfo.uliBytesTotal.QuadPart = progInfo.uliBytesTotal.QuadPart;

                        pflHfpl->UseCachedDirListings(TRUE);    // Get the perf advantage now because we just updated the cache a few lines up.
                        hr = pflHfpl->RecursiveEnum(pidlRoot, DownloadItemCB, hint, (LPVOID) &downloadStruct);
                    }
                    if (downloadStruct.progInfo.ppd)
                    {
                        EVAL(SUCCEEDED(downloadStruct.progInfo.ppd->StopProgressDialog()));
                        downloadStruct.progInfo.ppd->Release();
                    }

                    if (!downloadStruct.progInfo.hint)
                        fReleaseHint = FALSE;
                }
            }
            else
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // Err msg already displayed

            if (fReleaseHint)
                m_pfd->ReleaseHint(hint);
        }

        if (FAILED(hr) && (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)))
        {
            int nResult = DisplayWininetError(m_hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DOWNLOADING, IDS_FTPERR_WININET, MB_OK, NULL);
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // Don't display any more error dialogs.
        }
        ILFree(pidlRoot);
        IUnknown_Set(&pflHfpl, NULL);
        SHCoUninitialize(hrOleInit);
    }

    Release();  // This thread is holding a ref.
    return 0;
}


HRESULT CFtpMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    UINT idc;
    HRESULT hres = E_FAIL;

    if (pici->cbSize < sizeof(*pici))
        return E_INVALIDARG;

    if (HIWORD(pici->lpVerb))
    {
        int ivi;
        idc = (UINT)-1;
        for (ivi = 0; ivi < IVI_MAX; ivi++)
        {
            TCHAR szVerb[MAX_PATH];

            SHAnsiToTChar(pici->lpVerb, szVerb, ARRAYSIZE(szVerb));
            if (!StrCmpI(c_rgvi[ivi].ptszCmd, szVerb))
            {
                // Yes, the command is equal to the verb str, so this is the one.
                idc = c_rgvi[ivi].idc;
                break;
            }
        }
    }
    else
        idc = LOWORD(pici->lpVerb);

    switch (idc)
    {
    case IDC_ITEM_NEWFOLDER:
        hres = _InvokeNewFolderVerb(pici);
    break;

    case IDC_LOGIN_AS:
        hres = _InvokeLoginAsVerb(pici);
    break;

    case IDC_ITEM_OPEN:
    case IDC_ITEM_EXPLORE:
        hres = _EnumInvoke(pici, _ApplyOne, c_rgvi[IVI_REQ + idc].ptszCmd);
    break;

    case IDC_ITEM_DOWNLOAD:
        hres = _InvokeDownloadVerb(pici);
    break;

    case IDM_SHARED_FILE_DELETE:        // SFVIDM_FILE_DELETE
        hres = _InvokeDeleteVerb(pici);
        break;

    case IDM_SHARED_FILE_RENAME:        // SFVIDM_FILE_RENAME
        hres = _InvokeRename(pici);
        break;

    case IDM_SHARED_EDIT_COPY:          // SFVIDM_EDIT_COPY
        hres = _InvokeCutCopy(DFM_CMD_COPY, pici);
        break;

    case IDM_SHARED_EDIT_CUT:           // SFVIDM_EDIT_CUT
        hres = _InvokeCutCopy(DFM_CMD_MOVE, pici);
        break;

    //  _UNOBVIOUS_:  Yes, this is not a typo.  You might think I
    //  should have written SFVIDM_EDIT_PASTE, but you would be wrong.
    case SHARED_EDIT_PASTE:
        //  What's more annoying is that I also have to list
        //  IDM_SHARED_EDIT_PASTE, as a hack, because the "convert a
        //  name to an ID" loop above will cook up IDM_SHARED_EDIT_PASTE
        //  as the matching ID.
    case IDM_SHARED_EDIT_PASTE:
        hres = _InvokePaste(pici);
        break;

    case IDC_ITEM_BKGNDPROP:     // Properties for the background folder.
    case IDM_SHARED_FILE_PROP:   // Same as SFVIDM_FILE_PROPERTIES
        TraceMsg(TF_FTP_OTHER, "Properties!");
        hres = CFtpProp_DoProp(m_pflHfpl, m_pff, m_hwnd);
        break;

    case IDM_SORTBYNAME:
    case IDM_SORTBYSIZE:
    case IDM_SORTBYTYPE:
    case IDM_SORTBYDATE:
        ASSERT(m_hwnd);
        ShellFolderView_ReArrange(m_hwnd, CONVERT_IDMID_TO_COLNAME(idc));
        hres = S_OK;
        break;

    default:
        TraceMsg(TF_FTP_OTHER, "InvokeCommand");
        hres = E_INVALIDARG;
        break;
    }

    return hres;
}


/*****************************************************************************
 *
 *    CFtpMenu_Create
 *
\*****************************************************************************/

HRESULT CFtpMenu_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, HWND hwnd, REFIID riid, LPVOID * ppvObj, BOOL fFromCreateViewObject)
{
    HRESULT hr;
    CFtpMenu * pfm;

    *ppvObj = NULL;

    hr = CFtpMenu_Create(pff, pflHfpl, hwnd, fFromCreateViewObject, &pfm);
    if (EVAL(SUCCEEDED(hr)))
    {
        hr = pfm->QueryInterface(riid, ppvObj);
        pfm->Release();
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    return hr;
}


/**********************************************************************\
    FUNCTION: GetFtpDirFromFtpFolder

    DESCRIPTION:
        If an ftp folder is opened to an ftp server root (ftp://wired/),
    and the user clicks on the icon in the caption bar, pff will have an
    empty pidl.  This will cause us to return NULL.
\**********************************************************************/
CFtpDir * GetFtpDirFromFtpFolder(CFtpFolder * pff, CFtpPidlList * pflHfpl)
{
    LPCITEMIDLIST pidl = pff->GetPrivatePidlReference();
    if (!pidl || ILIsEmpty(pidl))
        return NULL;

    return pff->GetFtpDirFromPidl(pidl);
}


BOOL CanRenameAndDelete(CFtpFolder * pff, CFtpPidlList * pidlList, DWORD * pdwSFGAO)
{
    BOOL fResult = TRUE;

    //  If talking about yourself, you can't delete or rename.
    //  (Rename isn't allowed because SetNameOf doesn't like "self".)
    if (pidlList->GetCount() == 0)
        fResult = FALSE;
    else if (pidlList->GetCount() == 1)
    {
        LPITEMIDLIST pidl = GetPidlFromFtpFolderAndPidlList(pff, pidlList);

        // We can't rename or delete FTP servers, so check to see if it is one.
        if (FtpID_IsServerItemID(FtpID_GetLastIDReferense(pidl)))
            fResult = FALSE;
        ILFree(pidl);
    }

    return fResult;
}

/*****************************************************************************
 *
 *    CFtpMenu_Create
 *
\*****************************************************************************/
HRESULT CFtpMenu_Create(CFtpFolder * pff, CFtpPidlList * pidlList, HWND hwnd, BOOL fFromCreateViewObject, CFtpMenu ** ppfcm)
{
    HRESULT hr = E_FAIL;
    // It's ok if this is NULL
    CFtpDir * pfd = GetFtpDirFromFtpFolder(pff, pidlList);

    ASSERT(ppfcm);
    *ppfcm = new CFtpMenu();
    if (EVAL(*ppfcm))
    {
        //  We must AddRef the moment we copy them, else
        //  Finalize will get extremely upset.
        //
        //  NOTE! that we rely on the fact that GetAttributesOf
        //  will barf on complex pidls!
        (*ppfcm)->m_pff = pff;
        if (pff)
            pff->AddRef();

        IUnknown_Set(&(*ppfcm)->m_pflHfpl, pidlList);
        IUnknown_Set(&(*ppfcm)->m_pfd, pfd);
        (*ppfcm)->m_sfgao = SFGAO_CAPABILITYMASK | SFGAO_FOLDER;
        (*ppfcm)->m_hwnd = hwnd;
        (*ppfcm)->m_fBackground = fFromCreateViewObject;

        if (!CanRenameAndDelete(pff, pidlList, &((*ppfcm)->m_sfgao)))
            (*ppfcm)->m_sfgao &= ~(SFGAO_CANDELETE | SFGAO_CANRENAME);  // Clear those two bits.

        LPCITEMIDLIST * ppidl = (*ppfcm)->m_pflHfpl->GetPidlList();
        if (ppidl)
        {
            hr = (*ppfcm)->m_pff->GetAttributesOf((*ppfcm)->m_pflHfpl->GetCount(), 
                                        ppidl, &(*ppfcm)->m_sfgao);
            (*ppfcm)->m_pflHfpl->FreePidlList(ppidl);
        }

        if (!(EVAL(SUCCEEDED(hr))))
            IUnknown_Set(ppfcm, NULL);    // Unable to get attributes
    }
    else
        hr = E_OUTOFMEMORY;

    if (pfd)
        pfd->Release();

    ASSERT_POINTER_MATCHES_HRESULT(*ppfcm, hr);
    return hr;
}



/****************************************************\
    Constructor
\****************************************************/
CFtpMenu::CFtpMenu() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pflHfpl);
    ASSERT(!m_pff);
    ASSERT(!m_pfd);
    ASSERT(!m_sfgao);
    ASSERT(!m_hwnd);

    LEAK_ADDREF(LEAK_CFtpContextMenu);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpMenu::~CFtpMenu()
{
    IUnknown_Set(&m_pflHfpl, NULL);
    IUnknown_Set(&m_pff, NULL);
    IUnknown_Set(&m_pfd, NULL);
    IUnknown_Set(&_punkSite, NULL);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpContextMenu);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpMenu::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpMenu::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IContextMenu))
    {
        *ppvObj = SAFECAST(this, IContextMenu*);
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = SAFECAST(this, IObjectWithSite*);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CFtpMenu::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}
