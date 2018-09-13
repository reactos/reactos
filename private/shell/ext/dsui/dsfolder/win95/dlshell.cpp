#include "pch.h"
#include "dlshell.h"
#include "shguidp.h"
#pragma hdrstop


//
// Functions lifted from various bits of shell32
//

CLIPFORMAT g_acfIDLData[ICF_MAX] = { CF_HDROP, 0 };

#define RCF(x)  RegisterClipboardFormat(x)

STDAPI_(void) IDLData_InitializeClipboardFormats(void)
{
    if (g_cfHIDA == 0)
    {
        g_cfHIDA                 = RCF(CFSTR_SHELLIDLIST);
        g_cfOFFSETS              = RCF(CFSTR_SHELLIDLISTOFFSET);
        g_cfNetResource          = RCF(CFSTR_NETRESOURCES);
        g_cfFileContents         = RCF(CFSTR_FILECONTENTS);         // "FileContents"
        g_cfFileGroupDescriptorA = RCF(CFSTR_FILEDESCRIPTORA);      // "FileGroupDescriptor"
        g_cfFileGroupDescriptorW = RCF(CFSTR_FILEDESCRIPTORW);      // "FileGroupDescriptor"
        g_cfPrivateShellData     = RCF(CFSTR_SHELLIDLISTP);
        g_cfFileName             = RCF(CFSTR_FILENAMEA);            // "FileName"
        g_cfFileNameW            = RCF(CFSTR_FILENAMEW);            // "FileNameW"
        g_cfFileNameMap          = RCF(CFSTR_FILENAMEMAP);          // "FileNameMap"
        g_cfFileNameMapW         = RCF(CFSTR_FILENAMEMAPW);         // "FileNameMapW"
        g_cfPrinterFriendlyName  = RCF(CFSTR_PRINTERGROUP);
        g_cfHTML                 = RCF(TEXT("HTML Format"));
        g_cfPreferredDropEffect  = RCF(CFSTR_PREFERREDDROPEFFECT);  // "Preferred DropEffect"
        g_cfPerformedDropEffect  = RCF(CFSTR_PERFORMEDDROPEFFECT);  // "Performed DropEffect"
        g_cfPasteSucceeded       = RCF(CFSTR_PASTESUCCEEDED);       // "Paste Succeeded"
        g_cfShellURL             = RCF(CFSTR_SHELLURL);             // "Uniform Resource Locator"
        g_cfInDragLoop           = RCF(CFSTR_INDRAGLOOP);           // "InShellDragLoop"
        g_cfDragContext          = RCF(CFSTR_DRAGCONTEXT);          // "DragContext"
        g_cfTargetCLSID          = RCF(TEXT("TargetCLSID"));        // who the drag drop went to
    }

}

/*
 ** _ILResize
 *
 *  PARAMETERS:
 *      cbExtra is the amount to add to cbRequired if the block needs to grow,
 *      or it is 0 if we want to resize to the exact size
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

LPITEMIDLIST _ILResize(LPITEMIDLIST pidl, UINT cbRequired, UINT cbExtra)
{
    LPITEMIDLIST pidlsave = pidl;
    if (pidl==NULL)
    {
        pidl = _ILCreate(cbRequired+cbExtra);
    }
    else if (!cbExtra || SHGetSize(pidl) < cbRequired)
    {
        pidl = (LPITEMIDLIST)SHRealloc(pidl, cbRequired+cbExtra);
    }
    return pidl;
}


//===========================================================================
// IDLARRAY stuff
//===========================================================================

#define HIDA_GetPIDLFolder(pida)        (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

STDAPI_(HIDA) HIDA_Create(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST * apidl)
{
    HIDA hida;
#if _MSC_VER == 1100
// BUGBUG: Workaround code generate bug in VC5 X86 compiler (12/30 version).
    volatile
#endif
    UINT i;
    UINT offset = SIZEOF(CIDA) + SIZEOF(UINT)*cidl;
    UINT cbTotal = offset + ILGetSize(pidlFolder);
    for (i=0; i<cidl ; i++) {
        cbTotal += ILGetSize(apidl[i]);
    }

    hida = GlobalAlloc(GPTR, cbTotal);  // This MUST be GlobalAlloc!!!
    if (hida)
    {
        LPIDA pida = (LPIDA)hida;       // no need to lock

        LPCITEMIDLIST pidlNext;
        pida->cidl = cidl;

        for (i=0, pidlNext=pidlFolder; ; pidlNext=apidl[i++])
        {
            UINT cbSize = ILGetSize(pidlNext);
            pida->aoffset[i] = offset;
            MoveMemory(((LPBYTE)pida)+offset, pidlNext, cbSize);
            offset += cbSize;

            ASSERT(ILGetSize(HIDA_GetPIDLItem(pida,i-1)) == cbSize);

            if (i==cidl)
                break;
        }

        ASSERT(offset == cbTotal);
    }

    return hida;
}

STDAPI_(void) HIDA_Free(HIDA hida)
{
    GlobalFree(hida);
}

HIDA HIDA_Create2(void *pida, UINT cb)
{
    HIDA hida = GlobalAlloc(GPTR, cb);
    if (hida)
    {
        MoveMemory(hida, pida, cb);
    }
    return hida;
}

STDAPI_(HIDA) HIDA_Clone(HIDA hida)
{
    UINT cbTotal = GlobalSize(hida);
    HIDA hidaCopy = GlobalAlloc(GPTR, cbTotal);
    if (hidaCopy)
    {
        MoveMemory(hidaCopy, GlobalLock(hida), cbTotal);
        GlobalUnlock(hida);
    }
    return hidaCopy;
}

STDAPI_(UINT) HIDA_GetCount(HIDA hida)
{
    UINT count = 0;
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida)
    {
        count = pida->cidl;
        GlobalUnlock(hida);
    }
    return count;
}

STDAPI_(UINT) HIDA_GetIDList(HIDA hida, UINT i, LPITEMIDLIST pidlOut, UINT cbMax)
{
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida)
    {
        LPCITEMIDLIST pidlFolder = HIDA_GetPIDLFolder(pida);
        LPCITEMIDLIST pidlItem   = HIDA_GetPIDLItem(pida, i);
        UINT cbFolder  = ILGetSize(pidlFolder)-SIZEOF(USHORT);
        UINT cbItem = ILGetSize(pidlItem);
        if (cbMax < cbFolder+cbItem) {
            if (pidlOut) {
                pidlOut->mkid.cb = 0;
            }
        } else {
            MoveMemory(pidlOut, pidlFolder, cbFolder);
            MoveMemory(((LPBYTE)pidlOut)+cbFolder, pidlItem, cbItem);
        }
        GlobalUnlock(hida);

        return (cbFolder+cbItem);
    }
    return 0;
}

//
// This one reallocated pidl if necessary. NULL is valid to pass in as pidl.
//
STDAPI_(LPITEMIDLIST) HIDA_FillIDList(HIDA hida, UINT i, LPITEMIDLIST pidl)
{
    UINT cbRequired = HIDA_GetIDList(hida, i, NULL, 0);
    pidl = _ILResize(pidl, cbRequired, 32); // extra 32-byte if we realloc
    if (pidl)
    {
        HIDA_GetIDList(hida, i, pidl, cbRequired);
    }

    return pidl;
}

STDAPI_(LPITEMIDLIST) IDA_ILClone(LPIDA pida, UINT i)
{
    if (i < pida->cidl)
        return ILCombine(HIDA_GetPIDLFolder(pida), HIDA_GetPIDLItem(pida, i));
    return NULL;
}

LPITEMIDLIST HIDA_ILClone(HIDA hida, UINT i)
{
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida)
    {
        LPITEMIDLIST pidl = IDA_ILClone(pida, i);
        GlobalUnlock(hida);
        return pidl;
    }
    return NULL;
}


//
// SHAlloc and others
//

EXTERN_C void* MySHAlloc(ULONG cb)
{
    IMalloc *pmem;
    SHGetMalloc(&pmem);
    return pmem->Alloc(cb);
}

EXTERN_C void* MySHRealloc(LPVOID pv, ULONG cbNew)
{
    IMalloc *pmem;
    SHGetMalloc(&pmem);
    return pmem->Realloc(pv, cbNew);
}

EXTERN_C void MySHFree(LPVOID pv)
{
    IMalloc *pmem;
    SHGetMalloc(&pmem);
    pmem->Free( pv);
}

EXTERN_C ULONG MySHGetSize(LPVOID pv)
{
    IMalloc *pmem;
    SHGetMalloc(&pmem);
    return pmem->GetSize(pv);
}

//
// IDList helpers
//

EXTERN_C LPITEMIDLIST _MyILCreate(UINT cbSize)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)SHAlloc(cbSize);
    if (pidl)
        memset(pidl, 0, cbSize);      // zero-init for external task allocator

    return pidl;
}

//
// deal with the versioning of this structure...
//

EXTERN_C void CopyInvokeInfo(CMINVOKECOMMANDINFOEX *pici, const CMINVOKECOMMANDINFO *piciIn)
{
    ASSERT(piciIn->cbSize >= SIZEOF(CMINVOKECOMMANDINFO));

    ZeroMemory(pici, SIZEOF(CMINVOKECOMMANDINFOEX));
    memcpy(pici, piciIn, min(SIZEOF(CMINVOKECOMMANDINFOEX), piciIn->cbSize));
    pici->cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
}


//=============================================================================
// HDXA stuff
//=============================================================================
//
//  This function enumerate all the context menu handlers and let them
// append menuitems. Each context menu handler will create an object
// which support IContextMenu interface. We call QueryContextMenu()
// member function of all those IContextMenu object to let them append
// menuitems. For each IContextMenu object, we create ContextMenuInfo
// struct and append it to hdxa (which is a dynamic array of ContextMenuInfo).
//
//  The caller will release all those IContextMenu objects, by calling
// its Release() member function.
//
// Arguments:
//  hdxa            -- Handler of the dynamic ContextMenuInfo struct array
//  pdata           -- Specifies the selected items (files)
//  hkeyShellEx     -- Specifies the reg.dat class we should enumurate handlers
//  hkeyProgID      -- Specifies the program identifier of the selected file/directory
//  pszHandlerKey   -- Specifies the reg.dat key to the handler list
//  pidlFolder      -- Specifies the folder (drop target)
//  hmenu           -- Specifies the menu to be modified
//  uInsert         -- Specifies the position to be insert menuitems
//  idCmdFirst      -- Specifies the first menuitem ID to be used
//  idCmdLast       -- Specifies the last menuitem ID to be used
//
// Returns:
//  The first menuitem ID which is not used.
//
// History:
//  02-25-93 SatoNa     Created
//
//  06-30-97 lAmadio    Modified to add ID mapping support.

UINT HDXA_AppendMenuItems(HDXA hdxa, IDataObject *pdtobj,
                        UINT nKeys, const HKEY *ahkeyClsKeys,
                        LPCITEMIDLIST pidlFolder,
                        HMENU hmenu, UINT uInsert,
                        UINT  idCmdFirst,  UINT idCmdLast,
                        UINT fFlags,
                        HDCA hdca)
{
    QCMINFO qcm = {hmenu,uInsert,idCmdFirst,idCmdLast,NULL};
    return HDXA_AppendMenuItems2(hdxa, pdtobj,nKeys,ahkeyClsKeys,pidlFolder,&qcm,fFlags,hdca,NULL);
}

UINT HDXA_AppendMenuItems2(HDXA hdxa, IDataObject *pdtobj,
                        UINT nKeys, const HKEY *ahkeyClsKeys,
                        LPCITEMIDLIST pidlFolder,
                        QCMINFO* pqcm,
                        UINT fFlags,
                        HDCA hdca,
                        IUnknown* pSite)
{
    int idca;
    const UINT idCmdBase = pqcm->idCmdFirst;
    UINT idCmdFirst = pqcm->idCmdFirst;
    ASSERT(pqcm != NULL);

    // Apparently, somebody has already called into here with this object.  We
    // need to keep the ID ranges separate, so we'll put the new ones at the
    // end.
    // BUGBUG: If QueryContextMenu is called too many times, we will run out of
    // ID range and not add anything.  We could try storing the information
    // used to create each pcm (HKEY, GUID, and fFlags) and reuse some of them,
    // but then we would have to worry about what if the number of commands
    // grows and other details; this is just not worth the effort since
    // probably nobody will ever have a problem.  The rule of thumb is to
    // create an IContextMenu, do the QueryContextMenu and InvokeCommand, and
    // then Release it.
    idca = DSA_GetItemCount(hdxa);
    if (idca > 0)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, idca-1);
        idCmdFirst += pcmi->idCmdMax;
    }

    //
    // Note that we need to reverse the order because each extension
    // will intert menuitems "above" uInsert.
    //
    for (idca = DCA_GetItemCount(hdca) - 1; idca >= 0; idca--)
    {
        int nCurKey;
        IShellExtInit *psei = NULL;
        IContextMenu *pcm = NULL;
        IObjectWithSite* pows = NULL;

        TCHAR szCLSID[GUIDSTR_MAX];
        TCHAR szRegKey[GUIDSTR_MAX + 40];
        DWORD dwType;
        DWORD dwSize;
        DWORD dwExtType;

        const CLSID* pclsid = DCA_GetItem(hdca, idca);
        GetStringFromGUID(*pclsid, szCLSID, ARRAYSIZE(szCLSID));
        Trace(TEXT("szCLSID for index %d is %s"), idca, szCLSID);

        //
        // Let's avoid creating an instance (loading the DLL) when:
        //  1. fFlags has CMF_DEFAULTONLY and
        //  2. CLSID\clsid\MayChangeDefault does not exist
        //
        if (fFlags & CMF_DEFAULTONLY)
        {
            if (pclsid && (*pclsid) != CLSID_ShellFileDefExt)
            {
                wsprintf(szRegKey, TEXT("CLSID\\%s\\shellex\\MayChangeDefaultMenu"), szCLSID);
                Trace(TEXT("szRegKey %s"), szRegKey);

                if (RegQueryValue(HKEY_CLASSES_ROOT, szRegKey, NULL, NULL) != ERROR_SUCCESS)
                {
                    continue;
                }
            }
        }

        for (nCurKey = 0; nCurKey < (int)nKeys; nCurKey++)
        {
            HRESULT hres;
            UINT citems;
            BOOL fQIBug;

            if (!psei && FAILED(DCA_CreateInstance(hdca, idca, IID_IShellExtInit, (void **)&psei)))
            {
                Trace(TEXT("Failed when calling DCA_CreateInstance for index %d"), idca);
                break;
            }

            // Try all the class keys in order
            hres = psei->Initialize(pidlFolder, pdtobj, ahkeyClsKeys[nCurKey]);
            if (FAILED(hres))
            {
                TraceMsg("Calling IShellExtInit::Initialize failed");
                continue;
            }

            // APPCOMPAT: PGP50 can only be QIed for IContextMenu, IShellExtInit, and IUnknown.
            fQIBug = lstrcmp(szCLSID, SZ_PGP50_CONTEXTMENU) == 0;

            if (pSite)
            {
                if (!fQIBug)
                    IUnknown_SetSite((IUnknown*)psei, pSite);
            }

            // Only get the pcm after initializing
            if (!pcm && FAILED(psei->QueryInterface(IID_IContextMenu, (void **)&pcm)))
            {
                TraceMsg("Failed to QI for the IContextMenu iface");
                continue;   // break?
            }
            
            wsprintf(szRegKey, TEXT("CLSID\\%s"), szCLSID);
            Trace(TEXT("%s"), szRegKey);                

            dwSize = SIZEOF(DWORD);            

            if (SHGetValue(HKEY_CLASSES_ROOT, szRegKey, TEXT("flags"),&dwType, (BYTE*)&dwExtType, &dwSize) == ERROR_SUCCESS &&
                dwType == REG_DWORD &&
                pqcm->pIdMap != NULL &&
                dwExtType < pqcm->pIdMap->nMaxIds)
            {
                //Explanation:
                //Here we are trying to add a context menu extension to an already 
                //existing menu, owned by the sister object of DefView. We used the callback
                //to get a list of extension "types" and their place withing the menu, relative
                //to IDs that the sister object inserted already. That object also told us 
                //where to put extensions, before or after the ID. Since they are IDs and not
                //positions, we have to convert using GetMenuPosFromID.
                TraceMsg("Extending an exisitng menu via QueryContextMenu");
                hres = pcm->QueryContextMenu(
                    pqcm->hmenu, 
                    GetMenuPosFromID(pqcm->hmenu,pqcm->pIdMap->pIdList[dwExtType].id) +
                    ((pqcm->pIdMap->pIdList[dwExtType].fFlags & QCMINFO_PLACE_AFTER)? 1: 0),  
                    idCmdFirst, 
                    pqcm->idCmdLast, fFlags);
            }
            else
            {
                TraceMsg("Calling QueryContextMenu");
                hres = pcm->QueryContextMenu(pqcm->hmenu, pqcm->indexMenu, idCmdFirst, pqcm->idCmdLast, fFlags);
            }


            citems = HRESULT_CODE(hres);
            Trace(TEXT("citems %d"), citems);

            if (SUCCEEDED(hres) && citems)
            {
                ContextMenuInfo cmi;
                cmi.pcm = pcm;
                cmi.fQIBug = fQIBug;
                cmi.idCmdFirst = idCmdFirst - idCmdBase;
                cmi.idCmdMax   = cmi.idCmdFirst + citems;

                if (DSA_AppendItem(hdxa, &cmi) == -1)
                {
                    // There is no "clean" way to remove menu items, so
                    // we should check the add to the DSA before adding the
                    // menu items
                    TraceMsg("Failed to add the menu item, info to the DSA");                        
                }
                else
                {
                    TraceMsg("AddRef'ing the pcm object");
                    pcm->AddRef();
                }
                idCmdFirst += citems;

                //
                //  keep going if it is our internal handler
                //

                if ((*(CLSID*)DCA_GetItem(hdca, idca)) != CLSID_ShellFileDefExt)
                    break;      // not out handler stop

                pcm->Release();
                pcm = NULL;

                psei->Release();
                psei = NULL;

                continue;       // next hkey
            }
        }

        if (pcm)
            pcm->Release();

        if (psei)
            psei->Release();
    }

    Trace(TEXT("idCmdFirst on exit is %08x"), idCmdFirst);
    return idCmdFirst;
}

//
// Function:    HDXA_LetHandlerProcessCommand, public (not exported)
//
//  This function is called after the user select one of add-in menu items.
// This function calls IncokeCommand method of corresponding context menu
// object.
//
//  hdxa            -- Handler of the dynamic ContextMenuInfo struct array
//  idCmd           -- Specifies the menu item ID
//  hwndParent      -- Specifies the parent window.
//  pszWorkingDir   -- Specifies the working directory.
//
// Returns:
//  IDCMD_PROCESSED, if InvokeCommand method is called; idCmd, otherwise
//
// History:
//  03-03-93 SatoNa     Created
//
UINT HDXA_LetHandlerProcessCommand(HDXA hdxa, LPCMINVOKECOMMANDINFOEX pici)
{
    int icmi;
    UINT idCmd = (UINT)pici->lpVerb;

    //
    // One of add-in menuitems is selected. Let the context
    // menu handler process it.
    //
    for (icmi = 0; icmi < DSA_GetItemCount(hdxa); icmi++)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, icmi);
        //
        // Check if it is for this context menu handler.
        //
        // Notes: We can't use InRange macro because idCmdFirst might
        //  be equal to idCmdLast.
        // if (InRange(idCmd, pcmi->idCmdFirst, pcmi->idCmdMax-1))
        if (HIWORD(pici->lpVerb))
        {
            if (SUCCEEDED(pcmi->pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)pici)))
            {
                idCmd = IDCMD_PROCESSED;
                break;
            }
        }
        else if ((idCmd >= pcmi->idCmdFirst) && (idCmd < pcmi->idCmdMax))
        {
            //
            // Yes, it is. Let it handle this menuitem.
            //
            CMINVOKECOMMANDINFOEX ici;

            CopyInvokeInfo(&ici, (CMINVOKECOMMANDINFO *)pici);

            ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - pcmi->idCmdFirst);

            if (SUCCEEDED(pcmi->pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici)))
            {
                idCmd = IDCMD_PROCESSED;
            }
            break;
        }
    }

    if (idCmd != IDCMD_PROCESSED)
    {
        DebugMsg(DM_ERROR, TEXT("filemenu.c - ERROR: Nobody processed (%d)"), idCmd);
    }

    return idCmd;
}

HRESULT HDXA_GetCommandString(HDXA hdxa, UINT idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hres = E_INVALIDARG;
    int icmi;
    LPTSTR pCmd = (LPTSTR)idCmd;

    if (!hdxa)
        return E_INVALIDARG;

    //
    // One of add-in menuitems is selected. Let the context
    // menu handler process it.
    //
    for (icmi = 0; icmi < DSA_GetItemCount(hdxa); icmi++)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, icmi);

        if (HIWORD(idCmd))
        {
            // This must be a string command; see if this handler wants it
            if (pcmi->pcm->GetCommandString(idCmd, uType,
                                            pwReserved, pszName, cchMax) == NOERROR)
            {
                return NOERROR;
            }
        }
        //
        // Check if it is for this context menu handler.
        //
        // Notes: We can't use InRange macro because idCmdFirst might
        //  be equal to idCmdLast.
        // if (InRange(idCmd, pcmi->idCmdFirst, pcmi->idCmdMax-1))
        else if (idCmd >= pcmi->idCmdFirst && idCmd < pcmi->idCmdMax)
        {
            //
            // Yes, it is. Let it handle this menuitem.
            //
            hres = pcmi->pcm->GetCommandString(idCmd-pcmi->idCmdFirst, uType, pwReserved, pszName, cchMax);
            break;
        }
    }

    return hres;
}

HRESULT HDXA_FindByCommand(HDXA hdxa, UINT idCmd, REFIID riid, void **ppv)
{
    if (hdxa)
    {
        int icmi;
        for (icmi = 0; icmi < DSA_GetItemCount(hdxa); icmi++)
        {
            ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, icmi);

            if (idCmd >= pcmi->idCmdFirst && idCmd < pcmi->idCmdMax)
            {
                // APPCOMPAT: PGP50 can only be QIed for IContextMenu, IShellExtInit, and IUnknown.
                *ppv = NULL;    // bug nt power toy does not properly null out in error cases...
                if (!pcmi->fQIBug)
                    return pcmi->pcm->QueryInterface(riid, ppv);
                else
                    return E_FAIL;
            }
        }
    }
    *ppv = NULL;
    return E_FAIL;
}

//
// This function releases all the IContextMenu objects in the dynamic
// array of ContextMenuInfo,
//
void HDXA_DeleteAll(HDXA hdxa)
{
    if (hdxa)
    {
        int icmi;
        //
        //  Release all the IContextMenu objects, then destroy the DSA.
        //
        for (icmi = 0; icmi < DSA_GetItemCount(hdxa); icmi++)
        {
            ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, icmi);
            IContextMenu *pcm = pcmi->pcm;
            if (pcm)
            {
                pcm->Release();
            }
        }
        DSA_DeleteAllItems(hdxa);
    }
}

// This function releases all the IContextMenu objects in the dynamic
// array of ContextMenuInfo, then destroys the dynamic array.

void HDXA_Destroy(HDXA hdxa)
{
    if (hdxa)
    {
        HDXA_DeleteAll(hdxa);
        DSA_Destroy(hdxa);
    }
}

