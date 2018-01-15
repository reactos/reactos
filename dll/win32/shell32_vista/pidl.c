#include "precomp.h"

HRESULT WINAPI SHGetNameFromIDList(PCIDLIST_ABSOLUTE pidl, SIGDN sigdnName, PWSTR *ppszName)
{
    IShellFolder *psfparent;
    LPCITEMIDLIST child_pidl;
    STRRET disp_name;
    HRESULT ret;

    TRACE("%p 0x%08x %p\n", pidl, sigdnName, ppszName);

    *ppszName = NULL;
    ret = SHBindToParent(pidl, &IID_IShellFolder, (void**)&psfparent, &child_pidl);
    if(SUCCEEDED(ret))
    {
        switch(sigdnName)
        {
                                                /* sigdnName & 0xffff */
        case SIGDN_NORMALDISPLAY:               /* SHGDN_NORMAL */
        case SIGDN_PARENTRELATIVEPARSING:       /* SHGDN_INFOLDER | SHGDN_FORPARSING */
        case SIGDN_PARENTRELATIVEEDITING:       /* SHGDN_INFOLDER | SHGDN_FOREDITING */
        case SIGDN_DESKTOPABSOLUTEPARSING:      /* SHGDN_FORPARSING */
        case SIGDN_DESKTOPABSOLUTEEDITING:      /* SHGDN_FOREDITING | SHGDN_FORADDRESSBAR*/
        case SIGDN_PARENTRELATIVEFORADDRESSBAR: /* SIGDN_INFOLDER | SHGDN_FORADDRESSBAR */
        case SIGDN_PARENTRELATIVE:              /* SIGDN_INFOLDER */

            disp_name.uType = STRRET_WSTR;
            ret = IShellFolder_GetDisplayNameOf(psfparent, child_pidl,
                                                sigdnName & 0xffff,
                                                &disp_name);
            if(SUCCEEDED(ret))
                ret = StrRetToStrW(&disp_name, pidl, ppszName);

            break;

        case SIGDN_FILESYSPATH:
            *ppszName = CoTaskMemAlloc(sizeof(WCHAR)*MAX_PATH);
            if(SHGetPathFromIDListW(pidl, *ppszName))
            {
                TRACE("Got string %s\n", debugstr_w(*ppszName));
                ret = S_OK;
            }
            else
            {
                CoTaskMemFree(*ppszName);
                ret = E_INVALIDARG;
            }
            break;

        case SIGDN_URL:
        default:
            FIXME("Unsupported SIGDN %x\n", sigdnName);
            ret = E_FAIL;
        }

        IShellFolder_Release(psfparent);
    }
    return ret;
}

/*************************************************************************
 * SHGetIDListFromObject             [SHELL32.@]
 */
HRESULT WINAPI SHGetIDListFromObject(IUnknown *punk, PIDLIST_ABSOLUTE *ppidl)
{
    IPersistIDList *ppersidl;
    IPersistFolder2 *ppf2;
    IDataObject *pdo;
    IFolderView *pfv;
    HRESULT ret;

    if(!punk)
        return E_NOINTERFACE;

    *ppidl = NULL;

    /* Try IPersistIDList */
    ret = IUnknown_QueryInterface(punk, &IID_IPersistIDList, (void**)&ppersidl);
    if(SUCCEEDED(ret))
    {
        TRACE("IPersistIDList (%p)\n", ppersidl);
        ret = IPersistIDList_GetIDList(ppersidl, ppidl);
        IPersistIDList_Release(ppersidl);
        if(SUCCEEDED(ret))
            return ret;
    }

    /* Try IPersistFolder2 */
    ret = IUnknown_QueryInterface(punk, &IID_IPersistFolder2, (void**)&ppf2);
    if(SUCCEEDED(ret))
    {
        TRACE("IPersistFolder2 (%p)\n", ppf2);
        ret = IPersistFolder2_GetCurFolder(ppf2, ppidl);
        IPersistFolder2_Release(ppf2);
        if(SUCCEEDED(ret))
            return ret;
    }

    /* Try IDataObject */
    ret = IUnknown_QueryInterface(punk, &IID_IDataObject, (void**)&pdo);
    if(SUCCEEDED(ret))
    {
        IShellItem *psi;
        TRACE("IDataObject (%p)\n", pdo);
        ret = SHGetItemFromDataObject(pdo, DOGIF_ONLY_IF_ONE,
                                      &IID_IShellItem, (void**)&psi);
        if(SUCCEEDED(ret))
        {
            ret = SHGetIDListFromObject((IUnknown*)psi, ppidl);
            IShellItem_Release(psi);
        }
        IDataObject_Release(pdo);

        if(SUCCEEDED(ret))
            return ret;
    }

    /* Try IFolderView */
    ret = IUnknown_QueryInterface(punk, &IID_IFolderView, (void**)&pfv);
    if(SUCCEEDED(ret))
    {
        IShellFolder *psf;
        TRACE("IFolderView (%p)\n", pfv);
        ret = IFolderView_GetFolder(pfv, &IID_IShellFolder, (void**)&psf);
        if(SUCCEEDED(ret))
        {
            /* We might be able to get IPersistFolder2 from a shellfolder. */
            ret = SHGetIDListFromObject((IUnknown*)psf, ppidl);
        }
        IFolderView_Release(pfv);
        return ret;
    }

     return ret;
 }
