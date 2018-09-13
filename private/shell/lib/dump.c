//
// Debug dump functions for common ADTs
//
// This file should be #included by your DLL.  It is not part of
// stocklib.lib because it requires linking to certain guid libs,
// and DLLs like COMCTL32 do not do this.  (Therefore, COMCTL32
// doesn't #include this file, but still links to stocklib.)
//
//

#include <intshcut.h>       // For error values
#include <sherror.h>

#ifdef DEBUG

/*
 * macro for simplifying result to string translation, assumes result string
 * pointer pcsz
 */

#define STRING_CASE(val)               case val: pcsz = TEXT(#val); break


//
//  Debug value-to-string mapping functions
//


/*----------------------------------------------------------
Purpose: Return the string form of the clipboard format.

Returns: pointer to a static string
Cond:    --
*/
LPCTSTR 
Dbg_GetCFName(
    UINT ucf)
{
    LPCTSTR pcsz;
    static TCHAR s_szCFName[MAX_PATH];

    switch (ucf)
    {
        STRING_CASE(CF_TEXT);
        STRING_CASE(CF_BITMAP);
        STRING_CASE(CF_METAFILEPICT);
        STRING_CASE(CF_SYLK);
        STRING_CASE(CF_DIF);
        STRING_CASE(CF_TIFF);
        STRING_CASE(CF_OEMTEXT);
        STRING_CASE(CF_DIB);
        STRING_CASE(CF_PALETTE);
        STRING_CASE(CF_PENDATA);
        STRING_CASE(CF_RIFF);
        STRING_CASE(CF_WAVE);
        STRING_CASE(CF_UNICODETEXT);
        STRING_CASE(CF_ENHMETAFILE);
        STRING_CASE(CF_HDROP);
        STRING_CASE(CF_LOCALE);
        STRING_CASE(CF_MAX);
        STRING_CASE(CF_OWNERDISPLAY);
        STRING_CASE(CF_DSPTEXT);
        STRING_CASE(CF_DSPBITMAP);
        STRING_CASE(CF_DSPMETAFILEPICT);
        STRING_CASE(CF_DSPENHMETAFILE);

    default:
        if (! GetClipboardFormatName(ucf, s_szCFName, SIZECHARS(s_szCFName)))
            lstrcpy(s_szCFName, TEXT("UNKNOWN CLIPBOARD FORMAT"));
        pcsz = s_szCFName;
        break;
    }

    ASSERT(pcsz);

    return(pcsz);
}


LPCTSTR 
Dbg_GetHRESULTName(
    HRESULT hr)
{
    LPCTSTR pcsz;
    static TCHAR s_rgchHRESULT[] = TEXT("0x12345678");

    switch (hr)
        {
        STRING_CASE(S_OK);
        STRING_CASE(S_FALSE);

        STRING_CASE(DRAGDROP_S_CANCEL);
        STRING_CASE(DRAGDROP_S_DROP);
        STRING_CASE(DRAGDROP_S_USEDEFAULTCURSORS);

        STRING_CASE(E_UNEXPECTED);
        STRING_CASE(E_NOTIMPL);
        STRING_CASE(E_OUTOFMEMORY);
        STRING_CASE(E_INVALIDARG);
        STRING_CASE(E_NOINTERFACE);
        STRING_CASE(E_POINTER);
        STRING_CASE(E_HANDLE);
        STRING_CASE(E_ABORT);
        STRING_CASE(E_FAIL);
        STRING_CASE(E_ACCESSDENIED);

        STRING_CASE(CLASS_E_NOAGGREGATION);

        STRING_CASE(CO_E_NOTINITIALIZED);
        STRING_CASE(CO_E_ALREADYINITIALIZED);
        STRING_CASE(CO_E_INIT_ONLY_SINGLE_THREADED);

        STRING_CASE(DV_E_DVASPECT);
        STRING_CASE(DV_E_LINDEX);
        STRING_CASE(DV_E_TYMED);
        STRING_CASE(DV_E_FORMATETC);

        STRING_CASE(E_FLAGS);

        STRING_CASE(URL_E_INVALID_SYNTAX);
        STRING_CASE(URL_E_UNREGISTERED_PROTOCOL);

        STRING_CASE(IS_E_EXEC_FAILED);

        STRING_CASE(E_FILE_NOT_FOUND);
        STRING_CASE(E_PATH_NOT_FOUND);

    default:
        wsprintf(s_rgchHRESULT, TEXT("%#lx"), hr);
        pcsz = s_rgchHRESULT;
        break;
        }

    ASSERT(IS_VALID_STRING_PTR(pcsz, -1));

    return(pcsz);
}


/*----------------------------------------------------------
Purpose: Dump propvariant types

Returns: 
Cond:    --
*/
LPCTSTR 
Dbg_GetVTName(
    VARTYPE vt)
{
    LPCTSTR pcsz;
    static TCHAR s_szT[] = TEXT("0x12345678");

    switch (vt)
        {
        STRING_CASE(VT_EMPTY);
        STRING_CASE(VT_NULL);
        STRING_CASE(VT_I2);
        STRING_CASE(VT_I4);
        STRING_CASE(VT_R4);
        STRING_CASE(VT_R8);
        STRING_CASE(VT_CY);
        STRING_CASE(VT_DATE);
        STRING_CASE(VT_BSTR);
        STRING_CASE(VT_ERROR);
        STRING_CASE(VT_BOOL);
        STRING_CASE(VT_VARIANT);
        STRING_CASE(VT_UI1);
        STRING_CASE(VT_UI2);
        STRING_CASE(VT_UI4);
        STRING_CASE(VT_I8);
        STRING_CASE(VT_UI8);
        STRING_CASE(VT_LPSTR);
        STRING_CASE(VT_LPWSTR);
        STRING_CASE(VT_FILETIME);
        STRING_CASE(VT_BLOB);
        STRING_CASE(VT_STREAM);
        STRING_CASE(VT_STORAGE);
        STRING_CASE(VT_STREAMED_OBJECT);
        STRING_CASE(VT_STORED_OBJECT);
        STRING_CASE(VT_BLOB_OBJECT);
        STRING_CASE(VT_CLSID);
        STRING_CASE(VT_ILLEGAL);
        STRING_CASE(VT_CF);

    default:
        wsprintf(s_szT, TEXT("%#lx"), vt);
        pcsz = s_szT;
        break;
        }

    ASSERT(IS_VALID_STRING_PTR(pcsz, -1));

    return(pcsz);
}


#define STRING_RIID(val)               { &val, TEXT(#val) }

//
//  Alphabetical order, please.
//

struct 
    {
    REFIID riid;
    LPCTSTR psz;
    } const c_mpriid[] = 
{
        STRING_RIID(IID_IAddressList),
        STRING_RIID(IID_IAdviseSink),
#ifdef __IAddressList_INTERFACE_DEFINED__
        STRING_RIID(IID_IAddressList),
#endif
        STRING_RIID(IID_IAugmentedShellFolder),
        STRING_RIID(IID_IAugmentedShellFolder2),
#ifdef __IAuthenticate_INTERFACE_DEFINED__
        STRING_RIID(IID_IAuthenticate),
#endif
#ifdef __IBandSiteHelper_INTERFACE_DEFINED__
        STRING_RIID(IID_IBandSiteHelper),
#endif
#ifdef __IBandProxy_INTERFACE_DEFINED__
        STRING_RIID(IID_IBandProxy),
#endif
#ifdef __IBindStatusCallback_INTERFACE_DEFINED__
        STRING_RIID(IID_IBindStatusCallback),
#endif
        STRING_RIID(IID_IBrowserBand),
#ifdef __IBrowserService_INTERFACE_DEFINED__
        STRING_RIID(IID_IBrowserService),
#endif
        STRING_RIID(IID_IBrowserService2),
        STRING_RIID(IID_IConnectionPoint),
#ifdef __IConnectionPointCB_INTERFACE_DEFINED__
        STRING_RIID(IID_IConnectionPointCB),
#endif
        STRING_RIID(IID_IConnectionPointContainer),
        STRING_RIID(IID_IContextMenu),
        STRING_RIID(IID_IContextMenu2),
        STRING_RIID(IID_IContextMenuCB),
        STRING_RIID(IID_IContextMenuSite),
        STRING_RIID(IID_IDataObject),
        STRING_RIID(IID_IDeskBand),
        STRING_RIID(IID_IDispatch),
        STRING_RIID(IID_IDocFindBrowser),
        STRING_RIID(IID_IDocFindFileFilter),
#ifdef __IDocHostUIHandler_INTERFACE_DEFINED__
        STRING_RIID(IID_IDocHostUIHandler),
#endif
        STRING_RIID(IID_IDockingWindowFrame),
        STRING_RIID(IID_IDockingWindow),
        STRING_RIID(IID_IDockingWindowSite),
#ifdef __IDocNavigate_INTERFACE_DEFINED__
        STRING_RIID(IID_IDocNavigate),
#endif
        STRING_RIID(IID_IDocViewSite),
        STRING_RIID(IID_IDropTarget),
        STRING_RIID(IID_IDropTargetBackground),
#ifdef __IEFrameAuto_INTERFACE_DEFINED__
        STRING_RIID(IID_IEFrameAuto),
#endif        
        STRING_RIID(IID_IEnumUnknown),
        STRING_RIID(IID_IErrorInfo),
#ifdef __IExpDispSupport_INTERFACE_DEFINED__
        STRING_RIID(IID_IExpDispSupport),
#endif
#ifdef __IExpDispSupportOC_INTERFACE_DEFINED__
        STRING_RIID(IID_IExpDispSupportOC),
#endif
        STRING_RIID(IID_IExplorerToolbar),
        STRING_RIID(IID_IExtractIcon),
        STRING_RIID(IID_IExternalConnection),
        STRING_RIID(IID_FavoriteMenu),
#ifdef __IHistSFPrivate_INTERFACE_DEFINED__
        STRING_RIID(IID_IHistSFPrivate),
#endif
#ifdef __IHlink_INTERFACE_DEFINED__
        STRING_RIID(IID_IHlink),
#endif
#ifdef __IHlinkFrame_INTERFACE_DEFINED__
        STRING_RIID(IID_IHlinkFrame),
#endif
#ifdef __IHlinkSite_INTERFACE_DEFINED__
        STRING_RIID(IID_IHlinkSite),
#endif
#ifdef __IHlinkTarget_INTERFACE_DEFINED__
        STRING_RIID(IID_IHlinkTarget),
#endif
#ifdef __IHttpNegotiate_INTERFACE_DEFINED__
        STRING_RIID(IID_IHttpNegotiate),
#endif
#ifdef __IHttpSecurity_INTERFACE_DEFINED__
        STRING_RIID(IID_IHttpSecurity),
#endif
        STRING_RIID(IID_IInputObject),
        STRING_RIID(IID_IInputObjectSite),
        STRING_RIID(IID_IIsWebBrowserSB),
        STRING_RIID(IID_IMenuBand),
#ifdef __IMRU_INTERFACE_DEFINED__
        STRING_RIID(IID_IMRU),
#endif
#ifdef __INavigationStack_INTERFACE_DEFINED__
        STRING_RIID(IID_INavigationStack),
#endif
#ifdef __INavigationStackItem_INTERFACE_DEFINED__
        STRING_RIID(IID_INavigationStackItem),
#endif
        STRING_RIID(IID_INewShortcutHook),
#ifdef __IObjectCache_INTERFACE_DEFINED__
        STRING_RIID(IID_IObjectCache),
#endif
#ifdef __IObjectSafety_INTERFACE_DEFINED__
        STRING_RIID(IID_IObjectSafety),
#endif
        STRING_RIID(IID_IOleClientSite),
        STRING_RIID(IID_IOleCommandTarget),
        STRING_RIID(IID_IOleContainer),
        STRING_RIID(IID_IOleControl),
        STRING_RIID(IID_IOleControlSite),
        STRING_RIID(IID_IOleDocument),
        STRING_RIID(IID_IOleDocumentSite),
        STRING_RIID(IID_IOleDocumentView),
        STRING_RIID(IID_IOleInPlaceActiveObject),
        STRING_RIID(IID_IOleInPlaceFrame),
        STRING_RIID(IID_IOleInPlaceSite),
        STRING_RIID(IID_IOleInPlaceObject),
        STRING_RIID(IID_IOleInPlaceUIWindow),
        STRING_RIID(IID_IOleObject),
        STRING_RIID(IID_IOleWindow),
        STRING_RIID(IID_IPersist),
        STRING_RIID(IID_IPersistFolder),
#ifdef __IPersistMoniker_INTERFACE_DEFINED__
        STRING_RIID(IID_IPersistMoniker),
#endif
        STRING_RIID(IID_IPersistPropertyBag),
        STRING_RIID(IID_IPersistStorage),
        STRING_RIID(IID_IPersistStream),
        STRING_RIID(IID_IPersistStreamInit),
        STRING_RIID(IID_IPersistString),
        STRING_RIID(IID_IProvideClassInfo),
        STRING_RIID(IID_IPropertyNotifySink),
        STRING_RIID(IID_IPropertySetStorage),
        STRING_RIID(IID_IPropertyStorage),
        STRING_RIID(IID_IProxyShellFolder),
        STRING_RIID(IID_IServiceProvider),
        STRING_RIID(IID_ISetWinHandler),
        STRING_RIID(IID_IShellBrowser),
        STRING_RIID(IID_IShellChangeNotify),
        STRING_RIID(IID_IShellDetails),
        STRING_RIID(IID_IShellDetails2),
        STRING_RIID(IID_IShellExtInit),
        STRING_RIID(IID_IShellFolder),
        STRING_RIID(IID_IShellIcon),
        STRING_RIID(IID_IShellLink),
        STRING_RIID(IID_IShellLinkDataList),
        STRING_RIID(IID_IShellMenu),
        STRING_RIID(IID_IShellMenuCallback),
        STRING_RIID(IID_IShellPropSheetExt),
#ifdef __IShellService_INTERFACE_DEFINED__
        STRING_RIID(IID_IShellService),
#endif
        STRING_RIID(IID_IShellView),
        STRING_RIID(IID_IShellView2),
#ifdef __ITargetEmbedding_INTERFACE_DEFINED__
        STRING_RIID(IID_ITargetEmbedding),
#endif
#ifdef __ITargetFrame2_INTERFACE_DEFINED__
        STRING_RIID(IID_ITargetFrame2),
#endif
#ifdef __ITargetFramePriv_INTERFACE_DEFINED__
        STRING_RIID(IID_ITargetFramePriv),
#endif
        STRING_RIID(IID_ITravelEntry),
        STRING_RIID(IID_ITravelLog),
        STRING_RIID(IID_IUniformResourceLocator),
        STRING_RIID(IID_IUnknown),
        STRING_RIID(IID_IViewObject),
        STRING_RIID(IID_IViewObject2),
        STRING_RIID(IID_IWebBrowser),
        STRING_RIID(IID_IWebBrowser2),
        STRING_RIID(IID_IWebBrowserApp),
        STRING_RIID(IID_IWinEventHandler),
};

struct 
{
    IID iid;
    TCHAR szGuid[GUIDSTR_MAX];
} s_guid[50] = {0};

LPCTSTR
Dbg_GetREFIIDName(
    REFIID riid)
{
    int i;

    // search the known list
    for (i = 0; i < ARRAYSIZE(c_mpriid); i++)
        {
        if (IsEqualIID(riid, c_mpriid[i].riid))
            return c_mpriid[i].psz;
        }

    // get a display name for the the first few unknown requests
    for (i = 0; i < ARRAYSIZE(s_guid); i++)
        {
        if (TEXT('{') /*}*/ == s_guid[i].szGuid[0])
            {
            if (IsEqualIID(riid, &s_guid[i].iid))
                return s_guid[i].szGuid;
            }
        else
            {
            s_guid[i].iid = *riid;
            SHStringFromGUID(riid, s_guid[i].szGuid, ARRAYSIZE(s_guid[0].szGuid));
            return s_guid[i].szGuid;
            }
        }

    return TEXT("Unknown REFIID");
}

//***
// NOTE
//  must be called *after* Dbg_GetREFIIDName (since that is what creates entry)
void *
Dbg_GetREFIIDAtom(
    REFIID riid)
{
    int i;

    for (i = 0; i < ARRAYSIZE(c_mpriid); i++)
        {
        if (IsEqualIID(riid, c_mpriid[i].riid))
            return (void *) c_mpriid[i].riid;
        }

    // get a display name for the the first few unknown requests
    for (i = 0; i < ARRAYSIZE(s_guid); i++)
        {
        if (TEXT('{') /*}*/ == s_guid[i].szGuid[0])
            {
            if (IsEqualIID(riid, &s_guid[i].iid))
                return (void *) &s_guid[i].iid;
            }
        else
            {
            return NULL;
            }
        }

    return NULL;
}

#endif  // DEBUG
