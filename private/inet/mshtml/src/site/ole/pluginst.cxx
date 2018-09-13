//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       pluginst.cxx
//
//  Contents:   CPluginSite
//
//  Netscape plugin site code.  Works closely with plugin.ocx.
//
//  FYI:  A plugin is recorded in the HTML as:
//
//      <embed src="empty.vts" width=550 height=295 othername=otherval> 
//
//  Thus the plugin parameters are all recorded as tag attributes.  Contrast
//  this with an ActiveX imbedding which is stored as:
//     <OBJECT CLASSID = "clsid:01000000-840E-11CE-99BF-00AA0047D4FD">
//         <PARAM NAME="_Version" VALUE="65536">
//         <PARAM NAME="_ExtentX" VALUE="4445">
//         <PARAM NAME="_ExtentY" VALUE="3196">
//         <PARAM NAME="_StockProps" VALUE="13"></OBJECT>
//
//  Here is the IActiveXPlugin interface we use to communicate with plugin.ocx
//     interface IActiveXPlugin : IDispatch 
//     {
//        // Properties
//        // Methods
//        void Load([in]BSTR bstrUrl, [in]int bDeleteRegistry);
//        void AddParam([in]BSTR bstrName, [in]BSTR bstrValue);
//        void Show(void);
//        void Hide(void);
//        void Clear(void);
//        // get the Plugin's IDispatch, if any, otherwise return NULL
//        [propget] HRESULT dispatch([out, retval] IDispatch** retval);
//  
//     };
/*  Here is the plugin HTML spec from www.netscape.com:

Embedded plug-ins are loaded by Navigator when the user encounters an HTML page 
with an embedded object with a MIME type registered by a plug-in. When loaded, 
an embedded plug-in is displayed as part of the HTML document in a rectangular 
subpart of the page. This is similar to how a GIF or JPEG image is embedded, 
except that the plug-in can be live and respond to user events (such as mouse
 clicks). 

Plug-in objects are embedded in an HTML page by using the EMBED tag. The syntax
 of the EMBED tag is: 

<EMBED attributes> ... </EMBED>      [**** Tomsn note: the </EMBED> is often omitted]


Use the following attributes with the EMBED tag: 

HEIGHT="value" defines the horizontal location of the plug-in in the HTML page. 
    The unit of measurement is optionally defined by the UNITS attribute. 

HIDDEN="value" indicates whether the plug-in is visible on the page. 
    The value can be either true (the default) or false. A value of true 
    overrides the values of HEIGHT and WIDTH, making the plug-in zero-sized. 
    Always explicitly set HIDDEN=true to make an invisible plug-in (rather 
    than simply setting the HEIGHT and WIDTH to zero). 

PALETTE="value" indicates the mode of the plug-in's color palette. 
    The value can be either foreground or background (the default). 
    The palette mode is only relevant on the Windows platform. 

PLUGINSPAGE="URL" indicates the location of instructions on installing 
    the plug-in. The value URL is a standard uniform resource locator. 
    The URL is used by the assisted installation process if the plug-in 
    registered for the MIME type of this EMBED tag is not found. 

SRC="URL" optionally indicates the location of the plug-in data file.
    The value URL is a standard uniform resource locator. The MIME type of 
    the file (typically based on the file-name suffix) determines which 
    plug-in is loaded to handle this EMBED tag. Either the SRC attribute 
    or the TYPE attribute is required in an EMBED tag. 

TYPE="type" optionally indicates the MIME type of the EMBED tag, which 
    in turn determines which plug-in is loaded to handle this EMBED tag. 
    Either the SRC attribute or the TYPE attribute is required in an EMBED tag. 
    Use TYPE instead of SRC for plug-ins that require no data (for example, 
    a plug-in that draws an analog clock) or plug-ins that fetch all their 
    data dynamically. 

WIDTH="value " optionally defines the vertical location of the plug-in in 
    the HTML page. The unit of measurement is optionally defined by the 
    UNITS attribute. 

UNITS="value" defines the measurement unit used by the HEIGHT and WIDTH 
    attributes. The value can be either pixels (the default) or en 
    (half the point size). 

In addition to these standard attributes, plug-ins may optionally have private 
attributes to communicate specialized information between the HTML page and 
the plug-in code. Navigator ignores all non-standard attributes when parsing 
the HTML, but passes all attributes to the plug-in, allowing the plug-in to 
examine the attribute list for any private attributes that may modify its behavior. 

For example, a plug-in that displays video could have private attributes 
to determine whether the plug-in should automatically start playing the video, 
and whether the video should automatically loop on playback. Thus an example 
EMBED tag could be: 

<EMBED SRC="myavi.avi" WIDTH=320 HEIGHT=200 AUTOSTART=true LOOP=true>

Navigator would interpret the SRC tag to load the data file and determine the 
MIME type of the data, and the WIDTH and HEIGHT tags to size the area of the 
page handled by the plug-in to be 320 by 200 pixels. Navigator would simply 
ignore private attributes AUTOSTART and LOOP and pass them to the plug-in with 
the rest of the attributes. The plug-in could then scan its list of attributes 
to see if it should automatically start the video and loop it on playback. 
 */
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_PLUGINST_HXX_
#define X_PLUGINST_HXX_
#include "pluginst.hxx"
#endif

#ifndef X_PROPBAG_HXX_
#define X_PROPBAG_HXX_
#include "propbag.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_INETREG_H_
#define X_INETREG_H_
#include "inetreg.h"    // for REGSTR_VAL_CODEDOWNLOAD & stuff
#endif

#ifndef X_PLUGIN_I_H_
#define X_PLUGIN_I_H_
#include "plugin_i.h"   // IActiveXPlugin interface definition.
#endif

#define _cxx_
#include "pluginst.hdl"

MtDefine(CPluginSite, Elements, "CPluginSite")
MtDefine(MapExtnToKnownControl, CPluginSite, "MapExtnToKnownControl (code base)")
MtDefine(CPluginSiteCreateObject, CPluginSite, "CPluginSite::CreateObject");

// These are the clsids for the actual plugin.ocx control:
const GUID CDECL IID_IActiveXPlugin =
{  0x06DD38D1L,0xD187,0x11CF,{ 0xA8,0x0D,0x00,0xC0,0x4F,0xD7,0x4A,0xD8}};

const GUID CDECL CLSID_ActiveXPlugin =
  { 0x06DD38D3L,0xD187,0x11CF, {0xA8,0x0D,0x00,0xC0,0x4F,0xD7,0x4A,0xD8}};

const CElement::CLASSDESC CPluginSite::s_classdesc =
{
    {
        &CLSID_HTMLEmbed,              // _pclsid
        0,                             // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                        // _pcpi
        ELEMENTDESC_NEVERSCROLL    |
        ELEMENTDESC_OLESITE,           // _dwFlags
        &IID_IHTMLEmbedElement,        // _piidDispinterface
        &s_apHdlDescs,                 // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLEmbedElement, // _pfnTearOff
    NULL,                              // _pAccelsDesign
    NULL                               // _pAccelsRun
};


HRESULT 
CPluginSite::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    *ppElement = new CPluginSite(pDoc);

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPluginSite::get_BaseHref
//
//  Synopsis:   Returns the base href for this object tag.
//              Uses the helper provided by COleSite.
//              BTW, this is the only abstract property in pluginst.pdl,
//              therefore this is the only get/set routine in this src file.
//
//----------------------------------------------------------------------------

HRESULT
CPluginSite::get_BaseHref(BSTR *pbstr)
{
    RRETURN( SetErrorInfo(GetBaseHref( pbstr )) );
}


//+---------------------------------------------------------------------------
//
//  Member:     CPluginSite::CreateObject
//
//  Synopsis:   Called by our super when it comes time to actually create
//              the ActiveX Plugin control.
//
//              We gather the parameters from two places: 1) CUnknownPair
//              list which hase the <embed unknown1=this unknown2=that>
//              type pairs, and 2) _pAA attributes list is is accessed via
//              all our custom get_XXX() routines.  This list contains all the
//              known properties such as <embed width=4444 src=a_plugin.vts>.
//              These properties are available to vbscript code.
//
//              We transmit the properties to plugin.ocx via two routes: 1)
//              Load( pPropertyBag ).  This way the control gets all the known
//              properties up front at load time.  Eventially we may support
//              IPersistPropertyBag2 which will let the control find *all* properties,
//              known and unknown, at load time.  2) Call pActiveXPlugin->Load()
//              for every propety, know and unknown.
//
//              What this gives us is a uniform presentation of properties to 
//              plugin.ocx.  It can choose to use them at the Load( pPropertyBag ) time,
//              at the pActiveXPlugin->Load() time, or a mixture of the two.
//----------------------------------------------------------------------------

BOOL TryAsActiveXControl(CDoc *pDoc, LPCTSTR pszFile, LPTSTR pszClassId, 
 LPTSTR *ppszCodeBase, LPCTSTR szMimeTypeIn);

// This routine is exported by urlmon and should appear in a public header
// someday:
STDAPI FindMimeFromData(
                        LPBC pBC,                   // bind context - can be NULL
                        LPCWSTR pwzUrl,             // url - can be null 
                        LPVOID pBuffer,             // buffer with data to sniff - can be null (pwzUrl must be valid)
                        DWORD cbSize,               // size of buffer
                        LPCWSTR pwzMimeProposed,    // proposed mime if - can be null
                        DWORD dwMimeFlags,          // will be determined
                        LPWSTR *ppwzMimeOut,        // the suggested mime
                        DWORD dwReserved);          // must be 0

BOOL GetMimeTypeFromUrl(LPCTSTR url, TCHAR *mime);

HRESULT
CPluginSite::CreateObject()
{
    HRESULT             hr = E_FAIL;
    LPCTSTR             pszSrc = NULL;
    OLECREATEINFO       info;
    TCHAR               szClassId[2*CLSID_STRLEN];
    TCHAR               szMime[MAX_PATH];
    LPCTSTR             pszMime = NULL;
    LPTSTR              pszCodeBase = NULL;
    CDoc *              pDoc = Doc();

    GWKillMethodCall((COleSite *)this, ONCALL_METHOD(COleSite, DeferredCreateObject, deferredcreateobject), 0);

    pDoc->AddRef();

    //
    // If user settings or other factors outside us prohibit
    // ActiveX controls and Plugins from running, then we
    // fail this create entirely:
    //

    if (!AllowCreate(CLSID_ActiveXPlugin))
        goto Cleanup;

    // If current document was obtained by POST action, the src attribute is
    // incorrect.  Use path to the cached data file instead.
    //
    // BUGBUG:  What if the cached file is still being downloaded??  Can we
    //          still hand this to the plugin/control?  Or should we wait until
    //          the file is fully downloaded?  (philco)
    if (pDoc->_fFullWindowEmbed && pDoc->_pDwnPost)
    {
        SetAAsrc(pDoc->GetPluginCacheFilename());
    }

    _fHidden = GetAAhidden();

    pszSrc = GetAAsrc();
    pszMime = GetAAtype();  // 'type' attribute is the mime type.

    if( !pszSrc && !pszMime )
        goto Cleanup;

    // Make sure we have at least an empty param bag allocated:
    hr = THR(EnsureParamBag());
    if (hr)
        goto Cleanup;

    Assert(_pParamBag);
    info.pPropBag = _pParamBag;
    info.pPropBag->AddRef();

#if 0
    // This URLMON call does not do a thorough enough job - it stops
    // just short of contacting the server to dertermine the mime type.
    // Thus we do the call just below this.
    FindMimeFromData( NULL, pszSrc, NULL, 0, NULL, 0, &pszMime, 0 );
#endif

    // Give the plugin the fully qualified URL of the plugin src file:
    if( pszSrc )
    {
        TCHAR   cBuf[pdlUrlLen];
        pDoc->ExpandUrl(pszSrc, ARRAY_SIZE(cBuf), cBuf, this);
        if (!pDoc->ValidateSecureUrl(cBuf, FALSE, FALSE))
        {
            // If unsecure, NULLify URL
            pszSrc = NULL;
            Assert(!_pszFullUrl);
        }
        else
        {
            // plugin.ocx has buffer overrun issues!!!  Do NOT allow
            // a string greater than INTERNET_MAX_URL_LENGTH to be
            // passed.  We're ensuring this is truncated here as the
            // lower-impact fix since plugin.ocx hasn't been changed
            // in quite some time.
            cBuf[INTERNET_MAX_URL_LENGTH-1] = _T('\0');
            MemAllocString(Mt(CPluginSiteCreateObject), cBuf, &_pszFullUrl);
            if (_pszFullUrl == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
    }

    // Track down the mime type.
    // If it is not given explicitly via an attribute of the <embed> tag, 
    // then query the server about what it thinks the mime type is.
    if( !pszMime && GetMimeTypeFromUrl(_pszFullUrl, szMime) )
    {
        // Some servers lie about the mime type and return "text/plain" to
        // confuse us.  Therefore we ignore such uninformative returns.
        if( StrCmpIC( szMime, _T("text/plain") ) && StrCmpIC( szMime, _T("text/html") ))
        {
            pszMime = szMime;
            hr = SetAAtype(pszMime);
            if( FAILED( hr ) )
                goto Cleanup;
        }
        else
        {
            // One last chance.  If this is a full-window embed, use the 
            // content type reported to the binding used for the plugin data file.
            if (pDoc->IsFullWindowEmbed())
            {
                pszMime = pDoc->GetPluginContentType();
                hr = SetAAtype(pszMime);
                if( FAILED( hr ) )
                    goto Cleanup;
            }
        }
    }


    // First decide if we should actually create an ActiveX control
    // to handle this data.  Notice pszCodeBase is an OUT param which
    // if allocated gets freed in the Cleanup.
    if( TryAsActiveXControl( pDoc, pszSrc, szClassId, &pszCodeBase, pszMime ) )
    {   //
        // Create ActiveX control directly code path....
        //
        hr = CLSIDFromHtmlString( szClassId, &info.clsid );
        if (FAILED(hr))
        {
            hr = THR(CLSIDFromString(szClassId, &info.clsid));
            if (FAILED(hr))
                goto Cleanup;
        }
        
        MemReplaceString(Mt(OleCreateInfo), pszCodeBase, &info.pchSourceUrl);

        // Set particular parameters based on what we know from other
        // places:
        if( pszCodeBase )
        {
            hr = SetAAcodeBase(pszCodeBase);
            if( FAILED( hr ) )
                goto Cleanup;
        }

        // Save them again so all that new stuff gets propagated to the
        // property bag:
        hr = THR(SaveAttributes(_pParamBag, FALSE));
        if (hr)
            goto Cleanup;
    
        // Right now super:: is COleSite.  If that ever changes you'd
        // better eyeball this whole routine closely.
        _fUsingActiveXControl = TRUE;
        hr = super::CreateObject( &info );
        // Be aware that that CreateObject() call is asynchronous and
        // COleSite will finish creating the object on the CreateObjectNow()
        // callback from CCodeLoad.
    }
    else 
    {   //
        // Normal plugin.ocx plugin handling code path...
        //
        info.clsid = CLSID_ActiveXPlugin;
    
        // This saves all, known & unknown attributes into the property bag:
        hr = THR(SaveAttributes(_pParamBag, FALSE));
        if (hr)
            goto Cleanup;
    
        _fUsingActiveXControl = FALSE;
        hr = THR(super::CreateObject(&info));
        if (hr)
            goto Cleanup;
        // Be aware that that CreateObject() call is asynchronous and
        // WE will finish creating the object on the CreateObjectNow()
        // callback from CCodeLoad.
    }
    
Cleanup:
    pDoc->Release();
    MemFreeString(pszCodeBase);
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CPluginSite::PostLoad
//
//  Synopsis:   Called after the OLE site loads it's component in the normal
//              way.  Used to perform special actions after loading.
//
//---------------------------------------------------------------
HRESULT
CPluginSite::PostLoad()
{
    IActiveXPlugin *    pIActiveXPlugin = NULL;
    HRESULT             hr = S_OK;

    if( !_fUsingActiveXControl ) 
    {
        hr = QueryControlInterface(IID_IActiveXPlugin, (LPVOID*)&pIActiveXPlugin);
        if (hr)
            goto Cleanup;

        // _pszFullUrl may be NULL & that's OK.
        pIActiveXPlugin->Load( _pszFullUrl, FALSE );
    }

  Cleanup:
    ReleaseInterface(pIActiveXPlugin);
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CPluginSite::Passivate
//
//  Synopsis:   Called when main CSite reference count (_ulRefs) drops
//              to zero. Do not call this method directly.  Use
//              IUnknown::Release instead.
//
//---------------------------------------------------------------

void
CPluginSite::Passivate()
{
    MemFree( _pszFullUrl );
    _pszFullUrl = NULL;

    ReleaseParamBag();

    super::Passivate();
}


//+---------------------------------------------------------------------------
//
//  Member:     CPluginSite::Save
//
//  Synopsis:   Call PLUGIN.OCX to save its parambag, and look for special printing keyword
//              IE5/60771
//
//----------------------------------------------------------------------------

HRESULT
CPluginSite::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr = S_OK;
    DISPID  expandoDISPID = DISPID_UNKNOWN;

    
    if (!fEnd && pStreamWrBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC))
    {
        CDoc * pDoc = Doc();

        // Only create expando if no other print jobs are pending, so we don't have to
        // freeze the document for too long.
        if (!pDoc || !pDoc->_fSaveTempfileForPrinting || pDoc->PrintJobsPending())
            goto Cleanup;

        if (OK(ExchangeParamBag(FROMCONTROL)))
        {
            // If we have a marshaled punk, add an expando for it.
            //
            VARIANT   var;
            PROPBAG2  propbag;
            TCHAR     szBuf[16];

            memset(&propbag, 0, sizeof(PROPBAG2));
            propbag.vt = VT_I4;
            propbag.pstrName = L"_Marshaled_pUnk";

            // Look for a parameter named "_Marshaled_pUnk"
            //
            if (OK(_pParamBag->Read(1, &propbag, NULL, &var, &hr)) && OK(hr))
            {
                // I think we need it to be a string?
                //
                if (var.vt == VT_I4)
                {
                    wsprintf(szBuf, L"%d", var.ulVal);
                    VariantClear(&var);

                    // Create an expando

                    hr = THR_NOTRACE(AddExpando(L"_Marshaled_pUnk", &expandoDISPID));
                    if (hr)
                        goto Cleanup;

                    expandoDISPID = expandoDISPID - DISPID_EXPANDO_BASE +
                        DISPID_ACTIVEX_EXPANDO_BASE;

                    hr = THR(AddString(
                            expandoDISPID,
                            szBuf,
                            CAttrValue::AA_Expando));
                    if (hr)
                        goto Cleanup;

                    // Let DoPrint know we now rely on the original document to stick around.
                    pDoc->GetRootDoc()->_fPrintedDocSavedPlugins = TRUE;
                }
            }

        }
    }

Cleanup:
    hr = THR(super::Save(pStreamWrBuff, fEnd));

    // If we added an expando to save the punk, remove it now.
    if (expandoDISPID != DISPID_UNKNOWN)
        FindAAIndexAndDelete(expandoDISPID, CAttrValue::AA_Expando);
    
    ReleaseParamBag();
    RRETURN1(hr, S_FALSE);
}


//=================================================================
//===== THIS HELPER CODE STOLEN FROM IE3.0 PLUGIN.OCX FILE DIALOGS.CPP
//=================================================================

//////////////////////////////////////////////////////////////////////////
//
//    GetMimeTypeFromUrl                                         [PUBLIC]
//
//  Ideally we would not have synchronous calls like this into wininet
// on the GUI thread.  They should be made asynchronous and use a callback
// to return the data when available.  The callback support in wininet, 
// however, makes cross thread calls to the callback function.  We can not
// use that.  The asynchronous support in urlmon.dll, which does the correct
// same-thread callback behavior, does not have an interface for just querying
// the mime type.  You must download the data and you then get the mime type
// as a side effect.  We don't want to download the data at this time.
//
// To work around the possibility of freezing the GUI thread while waiting for
// a response from the internet server, we set the timeout to be a relatively
// low 10 seconds.  
//
BOOL GetMimeTypeFromUrl(LPCTSTR url, TCHAR *mime)
{
   BOOL        bRet = FALSE;
#ifdef WIN16
   // BUGWIN16
   TraceTag((tagError, "GetMimeTypeFromUrl - pluginst.cxx, need to plugin ours !!"));
#else
   HINTERNET   hInternet=NULL;                 // opened connection
   HINTERNET   hRequest=NULL;                  // opened request
   DWORD       dwSize  = MAX_PATH;        // size of buffer returned
   DWORD       dwTimeout = 10000;   // 10 seconds.

   Assert( mime );

   if (NULL == url)
      goto Cleanup;

   if (NULL == (hInternet = InternetOpen(_T("contype"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)))
      goto Cleanup;

   InternetSetOption( hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, (LPVOID)&dwTimeout, sizeof(dwTimeout) );
   InternetSetOption( hInternet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, (LPVOID)&dwTimeout, sizeof(dwTimeout) );
   InternetSetOption( hInternet, INTERNET_OPTION_SEND_TIMEOUT, (LPVOID)&dwTimeout, sizeof(dwTimeout) );
   InternetSetOption( hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, (LPVOID)&dwTimeout, sizeof(dwTimeout) );

   if (NULL == (hRequest  = InternetOpenUrl(hInternet, url, _T("Accept: */*"), (ULONG)-1, 0, 0)))
      goto Cleanup;

   if (!HttpQueryInfo(hRequest, HTTP_QUERY_CONTENT_TYPE, mime, &dwSize, NULL))
      goto Cleanup;

   if (dwSize > 0)
      bRet = TRUE;

  Cleanup:
   if( hRequest )
      InternetCloseHandle(hRequest);
   if( hInternet )
      InternetCloseHandle(hInternet);

#endif
   return (bRet);
}

//=================================================================
//===== THIS DIALOG CODE STOLEN FROM IE3.0 SOURCE FILES DLG_PLUG.*
//=================================================================

typedef struct {
    LPCTSTR  szExt;
    LPCTSTR  szMIMEtype;
} DLGPLUGDATA, *LPDLGPLUGDATA;

INT_PTR CALLBACK OCXOrPluginDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPDLGPLUGDATA lpPlug;
            lpPlug = (LPDLGPLUGDATA) lParam;

            // turn checkbox on
            CheckDlgButton(hDlg, IDC_PLUGIN_UPGRADE_CHECK, TRUE);

            // set dialog text for MIME and Extention
            SetDlgItemText(hDlg, IDC_PLUGIN_UPGRADE_MIME_TYPE, 
                lpPlug->szMIMEtype);
            SetDlgItemText(hDlg, IDC_PLUGIN_UPGRADE_EXTENSION, 
                lpPlug->szExt);
        }
        return FALSE;

    case WM_DESTROY:
        return TRUE;
/*
    case WM_HELP:            // F1
        ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
        HELP_WM_HELP, (DWORD)(LPSTR)mapIDCsToIDHs);
        break;

    case WM_CONTEXTMENU:        // right mouse click
        ResWinHelp( (HWND) wParam, IDS_HELPFILE,
        HELP_CONTEXTMENU, (DWORD)(LPSTR)mapIDCsToIDHs);
        break;
*/
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {

            case IDYES:
                if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                {
                    EndDialog(hDlg, IDYES);
                    return TRUE;
                }
                break;

            case IDCANCEL:
                if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                {
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }
                break;

            case IDNO:
                if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                {
                    if (IsDlgButtonChecked(hDlg, IDC_PLUGIN_UPGRADE_CHECK))
                        EndDialog(hDlg, IDCANCEL );
                    else
                        EndDialog(hDlg, IDNO);
                    return TRUE;
                }
                break;
        }

    }

    return FALSE;
}

//=================================================================
//===== THIS CODE STOLEN FROM IE3.0 SOURCE FILE HTML.C
//=================================================================

const static TCHAR * szKNOWNCONTROLS = 
    _T("Software\\Microsoft\\Internet Explorer\\EmbedExtnToClsidMappings\\");

// MapExtnToKnownControl
// Looks up a registry mapping for Extn to Clsid + CODEBASE (if available)
// Returns: 
//  TRUE: if found a clsid for extn
//  FALSE: not found or any error
BOOL MapExtnToKnownControl(TCHAR *fileExt, TCHAR *szClassId, TCHAR **ppszCodeBase)
{
    DWORD Size = MAX_PATH;
    DWORD dwType =0;
    LONG lResult = ERROR_SUCCESS;

    const static TCHAR * szCODEBASE = _T("CODEBASE");

    TCHAR szKey[MAX_PATH];
    TCHAR szCodeBase[pdlUrlLen];
    HKEY hKeyExt = 0;

    BOOL fRet = FALSE;

    wcscpy(szKey, szKNOWNCONTROLS);
    wcscat(szKey, fileExt);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, 
                        KEY_READ, &hKeyExt);

    if (lResult == ERROR_SUCCESS) {

        Size = MAX_PATH;
        lResult = RegQueryValueEx(hKeyExt, NULL, NULL, &dwType, 
                            (LPBYTE)szClassId, &Size);

        if (lResult == ERROR_SUCCESS) {

            fRet = TRUE;

            Size = pdlUrlLen;
            lResult = RegQueryValueEx(hKeyExt, szCODEBASE, NULL, &dwType, 
                                (LPBYTE)szCodeBase, &Size);

            if (lResult == ERROR_SUCCESS) {
                HRESULT hr = MemAllocString( Mt(MapExtnToKnownControl), szCodeBase, ppszCodeBase );
                if( hr )
                    fRet = FALSE;
            }
        }

        if (hKeyExt)
            RegCloseKey(hKeyExt);
    }


    return fRet;
}

// BUGBUG: CastadoE has a bug assigned to design the dialog box for this
// UI and get a developer to make the code change.

BOOL UserPrefersControlOverPlugin(CDoc *pDoc, TCHAR const *szName, TCHAR *fileExt)
{

    BOOL fRet;
    TCHAR szKey[MAX_PATH];
    int Choice;
    DLGPLUGDATA   Plug;
    HWND hwndParent;

    Plug.szExt = fileExt;
    Plug.szMIMEtype = szName;

    {
        CDoEnableModeless   dem(pDoc);
        
        hwndParent = dem._hwnd;
        Choice = DialogBoxParam(GetResourceHInst(),
                              MAKEINTRESOURCE(IDD_PLUGIN_UPGRADE),
                              hwndParent,
                              &OCXOrPluginDlgProc,
                              (LPARAM)&Plug);
    }
    
    switch (Choice) {

    case IDYES:
        fRet = TRUE;
        break;
        
    case IDNO:

        // never ask again for this data type/extn
        // remove the entry from the known controls list in the registry
        wcscpy(szKey, szKNOWNCONTROLS);
        wcscat(szKey, fileExt);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szKey);
        
    case IDCANCEL:
    default:

        fRet = FALSE;
        break;
    }

    return fRet;
}

#ifndef WIN16
#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"  // for string conversions.
#endif
#endif // !WIN16

//
// FindPlugin - delegates this call to the plugin OCX
//
BOOL FindPlugin(TCHAR *szFileExt, TCHAR *szName, TCHAR const *szMime)
{
   typedef (WINAPI *PFN_FINDPLUGIN)(char *ext, char *name, char *mime);
   PFN_FINDPLUGIN pfnFindPlugin;
   BOOL fRet = FALSE;

   HMODULE hLib = LoadLibraryEx(_T("plugin.ocx"), NULL, 0);
   if (hLib == NULL)
   {
       goto Exit;
   }

    // Wowza - I wish there was a FindPluginW() available....
   pfnFindPlugin = (PFN_FINDPLUGIN)GetProcAddress(hLib, "FindPluginA");
   if (pfnFindPlugin == NULL)
   {
       goto Exit;
   }

   //BUGWIN16: shortcut, CStrIn is not defined for several reasons.
   // we need to fix that before enabling this.
#ifndef WIN16
   {
        CStrIn strinFileExt(szFileExt), strinName(szName), strinMime(szMime);
        fRet = pfnFindPlugin(strinFileExt, strinName, strinMime);
   }
#endif // !WIN16
Exit:
   if (hLib)
      FreeLibrary(hLib);

   return fRet;
}

//
// Checks the registry to see if we should prefer a plugin over a certain
// control for a certain file extension. This was put in because some ActiveX
// controls do not fully support rendering the file extensions they claim.
//
// For example ActiveMovie doesn't support QuickTime VR .mov files.
//
BOOL PreferPluginOverControl(TCHAR *szFileExt, TCHAR *szClassId)
{
    HKEY hk;
    TCHAR szKeyName[MAX_PATH];
    TCHAR *pszRealClassId;

    //
    // The real classid name begins at szClassId + strlen("clsid:")
    //
    Assert(szClassId && wcslen(szClassId) > 6);
    pszRealClassId = szClassId + 6;

    // notice tricky braces '}' below due to bizarre look of the
    // pszRealClsid we get:
    Format( 0, szKeyName, MAX_PATH, _T("CLSID\\{<0s>\\EnablePlugin\\<1s>"), pszRealClassId, szFileExt);
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKeyName, 0, KEY_READ, &hk) ==
        ERROR_SUCCESS)
    {
        RegCloseKey(hk); // Close the key immediately, just test for existance
        
        //
        // Now we know we have a control that doesn't handle this file extension
        // fully. So we make the expensive call to find out if there is a 
        // plugin that can handle this file extension. The sad assumption is 
        // that any plugins will handle this file type better than this control
        //
        if (FindPlugin(szFileExt, NULL, NULL))
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL PreferControlOverPlugin(CDoc *pDoc, TCHAR *fileExt, TCHAR const *pszMime)
{
    TCHAR name[MAX_PATH];
    HKEY hkey = NULL;
    TCHAR szDoCodeDownload[10]; // should only be "yes" or "no"
    DWORD cb;
    BOOL fRet = TRUE;

    name[0] = _T('\0');  // start out fresh and clean.

    if (!FindPlugin(fileExt, name, pszMime))
    {
        goto Exit;
    }

    // check to see if the user has set codedown load to "no"
    if (RegOpenKeyEx(HKEY_CURRENT_USER, TSZWININETPATH, (DWORD) NULL,
            KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
    {
        cb = sizeof(szDoCodeDownload);
        if (RegQueryValueEx(hkey, REGSTR_VAL_CODEDOWNLOAD, NULL,
                NULL, (LPBYTE)szDoCodeDownload, &cb) == ERROR_SUCCESS)
        {
            goto SkipDefault;
        }
    }

    // something went wrong... use default
    wcscpy(szDoCodeDownload, REGSTR_VAL_CODEDOWNLOAD_DEF);

SkipDefault:
    
    if (hkey)
        RegCloseKey(hkey);

    // if "no" codedownload... don't ask the question
    if (!_tcsicmp(szDoCodeDownload, TEXT("no")))
    {
        fRet = FALSE;
        goto Exit;  // return FALSE
    }

    // there is a plugin for this extn/mime on the system
    // we have to pop up a courtesy dialog to the user to check if they 
    // want to use the plugin or download the control

    fRet = UserPrefersControlOverPlugin(pDoc, pszMime, fileExt);

Exit:
    return fRet;
}


// TryAsActiveXControl
//
// try to locate an activeX control 
// if absent, check for known control extension to clsid mapping
// if absent or present and user wanted to use a plugin anyway
// fall to the plugin. Otherwise we go to the 
// object code with the clsid/codebase form the registry
#define REGKEY_DISABLEACTIVEX \
 _T("Software\\Microsoft\\Internet Explorer\\Plugins\\DisableActiveXControls")

int GetFileExtension(LPCTSTR szFilename, TCHAR **szExtension);
BOOL IsActiveXControl(LPCTSTR szFile, TCHAR *szClassId, TCHAR *fileExt, LPCTSTR szMimeTypeIn);

BOOL TryAsActiveXControl(CDoc *pDoc, LPCTSTR pszFile, LPTSTR pszClassId, 
 LPTSTR *ppszCodeBase, LPCTSTR szMimeTypeIn)
{
    TCHAR    *fileExt = NULL;    // file extension
    BOOL    fRet     = FALSE;    // return value
    HKEY    hKey     = NULL;     // reg key
    LONG    cbData     = 0;      // bytes to read
    TCHAR   szKey[MAX_PATH];
    HRESULT    hr;               // return code

    // Determine if searching for activex controls has been disabled...
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_DISABLEACTIVEX, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS == hr)
    {
       cbData = MAX_PATH;
       RegQueryValue(hKey, NULL, szKey, &cbData);
       RegCloseKey(hKey);
       if (StrCmpIC(szKey, _T("true")) == 0)
          return (FALSE);
    }

    // Need to parse the szFile to get the extension.
    // If this fails we carry on since we can also use the mimetype below.
    if( pszFile )
        GetFileExtension(pszFile, &fileExt);

    if (fileExt && _tcslen(fileExt) > 50)
    {
        fRet = FALSE;
        goto exit;
    }
    
    fRet = IsActiveXControl(pszFile, pszClassId,fileExt,szMimeTypeIn);

    // If we have no file extension then the processing below is not
    // applicable and we return the fRet just obtained.
    if (!fileExt)
        goto exit;

    //
    // If there is an ActiveXControl that we might want to use for this EMBED
    // tag, we check if there is a plugin that can handle this file extension
    // and if we prefer this plugin over this control. If so return FALSE;
    //
    if (fRet)
    {
        if(PreferPluginOverControl(fileExt, pszClassId))
        {
            fRet = FALSE; // set return value to FALSE so will use the PLUGIN
        }
        else
        {
            ;    // Do nothing as fRet is already TRUE so will use the CONTROL
        }

        //
        // Return result in fRet as we know we have a control or plugin to use.
        //
        goto exit;
    }

    // check for a Known control availabe

    fRet = MapExtnToKnownControl(fileExt, pszClassId, ppszCodeBase);

    if (!fRet)
        goto exit;    // if not found then use the plugin if any

    // check for a plugin, and if found then
    // put up a courtesy dialog before proceeding
    fRet = PreferControlOverPlugin(pDoc, fileExt, szMimeTypeIn);

exit:
    if (fileExt)
        FormsFreeString(fileExt);

    return fRet;
}

//////////////////////////////////////////////////////////////////////////
//
//    ClsidFromMime  - Look up a mime type in the registry and find the
//                     activeX control's clsid.
//
//         szMimeType  - IN points at obj mime type, MAY BE NULL.
//         szClassId   - [RETURN] String pointing to the class id.
//   ----------->BEWARE: string contains a leading '{' char as is found
//                       in the registry.  Caller must get rid of it.
//                       We do null out the trailing '}' char. 
//
//       HRESULT        - [RETURN] If we mapped to control.
//
//    Looks up mimetype->clsid

HRESULT ClsidFromMime( TCHAR *szClassId, LPCTSTR szMimeType)
{
    HKEY     hKeyProgId  = NULL;  // registry key for prog id
    LONG     cbData;              // buffer size
    HRESULT  hr;                  // last return from Reg*** function
    DWORD    dwRet;
    TCHAR    pszRegKey[MAX_PATH];   // class id for the object

    Assert( szMimeType );

    hr = Format(0,pszRegKey,MAX_PATH,_T("MIME\\Database\\Content Type\\<0s>"), szMimeType); 
    if (FAILED(hr))
        goto CleanupHR;

    // Open the registry entry for the prog id
    dwRet = RegOpenKeyEx(HKEY_CLASSES_ROOT, pszRegKey, 0, KEY_READ, &hKeyProgId);
    if (ERROR_SUCCESS != hr )
        goto Cleanup;

    cbData = MAX_PATH;
    dwRet = RegQueryValueEx(hKeyProgId, _T("CLSID"), NULL, NULL, (LPBYTE)szClassId, (DWORD*)&cbData);

    // Get rid of the trailing '}'
    if ( ERROR_SUCCESS == dwRet && cbData >= 2 )
        szClassId[cbData - 2] = _T('\0');

  Cleanup:
    hr = HRESULT_FROM_WIN32( dwRet );

  CleanupHR:
    if (hKeyProgId != NULL)
        RegCloseKey(hKeyProgId);

    return( hr );
}
 
//////////////////////////////////////////////////////////////////////////
//
//    IsActiveXControl - only used by EMBED tag.
//
//       szFile      - The file to check to determine if we have a control. MAY BE NULL.
//       szClassId   - [RETURN] String pointing to the class id.
//       fileExt     - IN points at extn in szFile MAY BE NULL.
//       szMimeTypeIn- IN points at obj mime type, MAY BE NULL.
//
//       BOOL        - [RETURN] If we mapped to control.
//
//    Given a source URL or file to look at, we will search to determine
//    if the object is an ActiveX Control.
//
//    Looks up file extension->mimetype->clsid then if that fails
//                  extension->progid->clsid then if that fails
//                  mimetype->clsid
//    We give priority to the file extension mime type mapping over the
//    passed in mime type since this is closer to the original IE3
//    behavior which only used the file extension.
//
BOOL IsActiveXControl(LPCTSTR szFile, TCHAR *szClassId, TCHAR *fileExt, LPCTSTR szMimeTypeIn)
{
   HKEY     hKeyFileExt = NULL;  // opened registry key
   HKEY     hKeyProgId  = NULL;  // registry key for prog id
   HKEY     hKeyClassId = NULL;  // registry key for class id
   LONG     cbData;              // buffer size
   HRESULT  hr;                  // last return from Reg*** function
   HKEY     hKeyControl = NULL;  // registry key for control under clsid
   TCHAR    pszControl[MAX_PATH];   // path to control key
   TCHAR    pszMimeTypeOrProgId[MAX_PATH];  // prog id for the object
   TCHAR    pszClassId[MAX_PATH];   // class id for the object

    *pszMimeTypeOrProgId = _T('\0');

    if (fileExt)
    {
        // Try to open the key HKEY_CLASSES_ROOT\fileExt. to get mime type.
        //    Example HKEY_CLASSES_ROOT\.xls
        hr = RegOpenKeyEx(HKEY_CLASSES_ROOT, fileExt, 0, KEY_READ, &hKeyFileExt);
        if (ERROR_SUCCESS == hr)
        {
            cbData     = MAX_PATH;

            // We found the file extension in the registry... now, we
            // want to get the mime type, if present
            hr = RegQueryValueEx(hKeyFileExt, _T("Content Type"), NULL, NULL, (LPBYTE)pszMimeTypeOrProgId, (DWORD*)&cbData);
            if (ERROR_SUCCESS == hr)
            {
                hr = ClsidFromMime( pszClassId, pszMimeTypeOrProgId );
                if( !hr ) 
                    goto GotCLSID;
            }
        }
    }

    //
    // just fall thru and try the ext->progid->clsid lookup
    //
    cbData = MAX_PATH;   
    // now, try the progid for the object.
    hr = RegQueryValue(hKeyFileExt, NULL, pszMimeTypeOrProgId, &cbData);
    if (ERROR_SUCCESS != hr || cbData <= 0)
        goto TryMime;
   
    // Now that we have the prog id, let's go and get the CLSID.
    pszMimeTypeOrProgId[cbData-1] = _T('\0'); // append the NULL character to our string

    // Open the registry entry for the prog id
    hr = RegOpenKeyEx(HKEY_CLASSES_ROOT, pszMimeTypeOrProgId, 0, KEY_READ, &hKeyProgId);
    if (ERROR_SUCCESS != hr)
        goto TryMime;

    // Check to see if the object has a CLSID key.
    hr = RegOpenKeyEx(hKeyProgId, _T("CLSID"), 0, KEY_READ, &hKeyClassId);
    if (ERROR_SUCCESS != hr)
        goto TryMime;

    cbData = MAX_PATH;
    hr = RegQueryValue(hKeyClassId, NULL, pszClassId, &cbData);
    if (ERROR_SUCCESS != hr || cbData <= 0)
        goto TryMime;

    // In the object tag, the class id can not have the { }. So, we will remove the
    // brackets when we return the value of the class id.
    Format(0, pszControl, MAX_PATH, _T("CLSID\\<0s>\\Control"), pszClassId);
    hr = RegOpenKeyEx(HKEY_CLASSES_ROOT, pszControl, 0, KEY_READ, &hKeyControl);
    if (ERROR_SUCCESS == hr)
    {
        // The classid does refer to a control.  Get rid of the { } brackets in
        // the classid string:
        if( cbData >= 2 )
            pszClassId[cbData-2] = _T('\0');     // add the null to the end of the string where the }
    }
    else
    {
  TryMime:  
        // Try the passed in mime type directly.
        // if we don't have a mime type, then don't try to execute
        // the GotCLSID piece below, since pszClassId never gets set
        if (!szMimeTypeIn)
            goto Cleanup;   // hr is already an error code, so we'll return FALSE.

        hr = ClsidFromMime( pszClassId, szMimeTypeIn );
        if( hr ) 
            goto Cleanup;
    }

  GotCLSID:
    wcscpy(szClassId, _T("clsid:"));    // need to preface with "clsid:"
    wcscat(szClassId, pszClassId+1);  // copy the value to our out param
 
  Cleanup:
    // Close all open reg keys
    if (hKeyFileExt != NULL)
        RegCloseKey(hKeyFileExt);

    if (hKeyProgId != NULL)
        RegCloseKey(hKeyProgId);

    if (hKeyClassId != NULL)
        RegCloseKey(hKeyClassId);

    if (hKeyControl != NULL)
        RegCloseKey(hKeyControl);

    if (hr == ERROR_SUCCESS)
        return (TRUE);
    else
        return (FALSE);
}

//////////////////////////////////////////////////////////////////////////
//
//    GetFileExtension
//
//       szFilename     - [in] string containing the file name
//       szExtension    - [out] the file extension found
//                        WARNING - caller must free memory for szExtension
//
//       int            - [return] the number of characters in file extension
//
//    This function will return the file extension (including the .). And it 
//    will return the number of characters in the file extension.
//
int GetFileExtension(LPCTSTR szFilename, TCHAR **szExtension)
{
   int  count   = 0;          // number of characters in extension
   LPCTSTR szTemp = NULL;       // temp pointer to filename string
   LPCTSTR szEnd = NULL;

   Assert(szFilename);
   Assert(szExtension);

   if (szFilename == NULL || szExtension == NULL)
      return (count);

   // move temp pointer to end
   szTemp = szFilename + wcslen(szFilename);
   szEnd = szTemp;

   while (szTemp != szFilename)
   {
      // This function is essentially duplicated in at least
      // two other components.  The search behavior here is different
      // (RTL as opposed to the typical LTR), but I'm not going
      // to clean this up in a hotfix.
      // Consequently, when a query or bookmark char is found,
      // update it as our new end point for the extension string.
      if ((*szTemp == _T('?')) || (*szTemp == _T('#')))
      {
          szEnd = szTemp;
      }

      // when we have a period, we found the extension
      else if (*szTemp == _T('.'))
      {
        HRESULT hr = FormsAllocStringLen( szTemp, szEnd-szTemp, szExtension );
        if( FAILED( hr ) )
            count = 0;
        return (count);
      }
      --szTemp;
      ++count;
   }
   return (0);
}

//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange
//
//  Note:       Called after a property has changed to notify derived classes
//              of the change.  All properties (except those managed by the
//              derived class) are consistent before this call.
//
//              Also, fires a property change notification to the site.
//
//-------------------------------------------------------------------------

HRESULT
CPluginSite::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr;

    switch (dispid)
    {
        case DISPID_CPluginSite_hidden:
        {
            _fHidden = GetAAhidden();
            break;
        }
    }

    hr = THR(super::OnPropertyChange(dispid, dwFlags));

    RRETURN(hr);
}

