//+---------------------------------------------------------------------
//
//   File:      cutil.cxx
//
//  Contents:   Utility functions for CSite, move out from csite.cxx
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_DOBJ_HXX_
#define X_DOBJ_HXX_
#include "dobj.hxx"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_OLEDLG_H_
#define X_OLEDLG_H_
#include <oledlg.h>
#endif

MtDefine(DoInsertObjectUI_plptszResult, Utilities, "DoInsertObjectUI *plptszResult")

extern "C" const CLSID CLSID_HTMLImg;
extern "C" const CLSID CLSID_HTMLButtonElement;
//extern "C" const CLSID CLSID_HTMLInputTextElement;
extern "C" const CLSID CLSID_HTMLTextAreaElement;
//extern "C" const CLSID CLSID_HTMLOptionButtonElement;
extern "C" const CLSID CLSID_HTMLListElement;
extern "C" const CLSID CLSID_HTMLMarqueeElement;
extern "C" const CLSID CLSID_HTMLDivPosition;
extern "C" const CLSID CLSID_HTMLHRElement;
extern "C" const CLSID CLSID_HTMLIFrame;
//extern "C" const CLSID CLSID_HTMLInputButtonElement;
//extern "C" const CLSID CLSID_HTMLInputFileElement;
extern "C" const CLSID CLSID_HTMLFieldSetElement;
extern "C" const CLSID CLSID_HTMLParaElement;
extern "C" const CLSID CLSID_HTMLInputElement;

static const struct HTMLSTRMAPPING
{
    UINT        cmd;
    const CLSID *     pClsid;
    LPCSTR      lpsz;
}
s_aHtmlMaps[] =
{   
    { 0,                    NULL,                           "<OBJECT %s></OBJECT>"},
    { IDM_IMAGE,            &CLSID_HTMLImg,                 "<IMG%s>"},
    { IDM_BUTTON,           &CLSID_HTMLButtonElement,       "<BUTTON%s></BUTTON>"},
    //{ IDM_RICHTEXT,         &CLSID_HTMLRichtextElement,     "<RICHTEXT%s></RICHTEXT"},
    { IDM_TEXTBOX,          &CLSID_HTMLInputElement,        "<INPUT TYPE = TEXT%s>"},
    { IDM_TEXTAREA,         &CLSID_HTMLTextAreaElement,     "<TEXTAREA%s></TEXTAREA>"},
    { IDM_CHECKBOX,         NULL,                           "<INPUT TYPE = CHECKBOX%s>"},   
    { IDM_RADIOBUTTON,      &CLSID_HTMLInputElement,        "<INPUT TYPE = RADIO%s>"},
    { IDM_DROPDOWNBOX,      &CLSID_HTMLListElement,         "<SELECT%s> </SELECT>"},
    { IDM_LISTBOX,          NULL,                           "<SELECT MULTIPLE%s> </SELECT>"},
    { IDM_MARQUEE,          &CLSID_HTMLMarqueeElement,      "<MARQUEE%s></MARQUEE>"},
    { IDM_1D,               &CLSID_HTMLDivPosition,         "<DIV STYLE=POSITION:RELATIVE%s></DIV>"},
    { IDM_HORIZONTALLINE,   &CLSID_HTMLHRElement,           "<HR%s>"},
    { IDM_IFRAME,           &CLSID_HTMLIFrame,              "<IFRAME%s></IFRAME>"},
    { IDM_INSINPUTBUTTON,   &CLSID_HTMLInputElement,        "<INPUT TYPE = BUTTON%s>"},
    { IDM_INSINPUTRESET,    NULL,                           "<INPUT TYPE = RESET%s>"},
    { IDM_INSINPUTSUBMIT,   NULL,                           "<INPUT TYPE = SUBMIT%s>"},
    { IDM_INSINPUTUPLOAD,   &CLSID_HTMLInputElement,        "<INPUT TYPE = FILEUPLOAD%s>"},
    { IDM_INSINPUTIMAGE,    &CLSID_HTMLInputElement,        "<INPUT TYPE = IMAGE%s>"},
    { IDM_INSINPUTHIDDEN,   NULL,                           "<INPUT TYPE = HIDDEN%s>"},
    { IDM_INSINPUTPASSWORD, NULL,                           "<INPUT TYPE = PASSWORD%s>"},
    { IDM_INSFIELDSET,      &CLSID_HTMLFieldSetElement,     "<FIELDSET%s></FIELDSET>"},
    { IDM_PARAGRAPH,        &CLSID_HTMLParaElement,         "<P%s>"},
    { IDM_NONBREAK,         NULL,                           "&nbsp;"}
};

typedef LPTSTR (*PFILEHELPER) (LPTSTR, LPTSTR, int, int *);


HRESULT
ClsidParamStrFromClsid (CLSID * pClsid, LPTSTR ptszParam, int cbParam)
{
    HRESULT     hr;

    hr = THR(Format (
        0, ptszParam, cbParam,
        _T("CLASSID = \"clsid<0g>"),
        pClsid
    ));
    if (!OK(hr))
        goto Cleanup;

    if ( (_T('{') != ptszParam[16+0]) || (_T('}') != ptszParam[16+37]) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    ptszParam[16+0 ] = _T(':');
    ptszParam[16+37] = _T('"');

Cleanup:
    RRETURN (hr);
}


HRESULT
ClsidParamStrFromClsidStr (LPTSTR ptszClsid, LPTSTR ptszParam, int cbParam)
{
    HRESULT     hr;

    hr = THR(Format (
        0, ptszParam, cbParam,
        _T("CLASSID = \"clsid<0s>"),
        ptszClsid
    ));
    if (!OK(hr))
        goto Cleanup;

    if ( (_T('{') != ptszParam[16+0]) || (_T('}') != ptszParam[16+37]) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    ptszParam[16+0 ] = _T(':');
    ptszParam[16+37] = _T('"');

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Function:   HtmlTagStrFromParam
//
//  Synopsis:   If it is intrinic control, create html tag and return S_OK
//
//-------------------------------------------------------------------------
HRESULT
HtmlTagStrFromParam (LPTSTR lptszInsertParam, int *pnFound)
{
    HRESULT             hr = E_FAIL;
    int                 i;
    TCHAR               aszClsid[CLSID_STRLEN + 1];
    CLSID               clsid;
    TCHAR  *            pch;

    Assert(pnFound);

    if (!lptszInsertParam)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Get clsid string from lptszInsertParam

    pch = _tcsstr(lptszInsertParam, _T(":"));
    if (!pch)
        goto Cleanup;

    _tcscpy(aszClsid, pch);

    aszClsid[0] = _T('{');
    aszClsid[CLSID_STRLEN - 1] = _T('}');
    aszClsid[CLSID_STRLEN] = 0;

    hr = CLSIDFromString(aszClsid, &clsid);
    if (hr)
        goto Cleanup;

    // find mapping
    for (i = 0; i < ARRAY_SIZE(s_aHtmlMaps); i++)
    {
        if (!s_aHtmlMaps[i].pClsid)
            continue;

        if (clsid.Data1 != s_aHtmlMaps[i].pClsid->Data1)
            continue;

        if(IsEqualCLSID(clsid, *s_aHtmlMaps[i].pClsid))
        {
            // match is found
            *pnFound = i;
            hr = S_OK;
            goto Cleanup;
        }
    }

    // No match is found
     *pnFound = -1;

Cleanup:
    RRETURN(hr);
}

#ifndef NO_OLEUI

//--------------------------------------------------------------------
// OleUIMetafilePictIconFree
//
// Purpose:
//  Deletes the metafile contained in a METAFILEPICT structure and
//  frees the memory for the structure itself.
//
// Parameters:
//  hMetaPict       HGLOBAL metafilepict structure created in
//                  OleUIMetafilePictFromIconAndLabel
//
// Return Value:
//  None
//-------------------------------------------------------------------

STDAPI_(void) OleUIMetafilePictIconFree(HGLOBAL hMetaPict)
{
    LPMETAFILEPICT      pMF;

    if (NULL==hMetaPict)
        return;

    pMF=(LPMETAFILEPICT)GlobalLock(hMetaPict);

    if (NULL!=pMF)
    {
        if (NULL!=pMF->hMF)
            DeleteMetaFile(pMF->hMF);
    }

    GlobalUnlock(hMetaPict);
    GlobalFree(hMetaPict);
}

//+----------------------------------------------------------------------------
//
//  Function:     DoInsertObjectUI
//
//  Synopsis:   executes UI for Insert / Object
//
//  Arguments   [out] dwResult      specifies what was specified in UI
//              [out] plptszResult  specifies classid, file name, etc., depending
//                                  on dwResult flags
//
//  Returns:    S_OK                OK hit in UI, plptszResult set
//              S_FALSE             CANCEL hit in UI, NULL == *plptszResult
//              other               failure
//
//-----------------------------------------------------------------------------

DYNLIB g_dynlibOLEDLG = { NULL, NULL, "OLEDLG.DLL" };

DYNPROC s_dynprocOleUIInsertObjectA =
         { NULL, &g_dynlibOLEDLG, "OleUIInsertObjectA" };

HRESULT
DoInsertObjectUI (CElement * pElement, DWORD * pdwResult, LPTSTR * plptszResult)
{
    HRESULT                 hr = S_OK;
    OLEUIINSERTOBJECTA      ouio;
    CHAR                    szFile[MAX_PATH] = "";
    TCHAR                   wszFile[MAX_PATH];
    UINT                    uRC;

    *pdwResult = 0;
    *plptszResult = NULL;

    // zero ouio
    memset(&ouio, 0, sizeof(ouio));

    // and fill it out
    ouio.cbStruct = sizeof(ouio);
    ouio.dwFlags =
            IOF_DISABLELINK |       // BUBUG Remove when form supports links.
            IOF_SELECTCREATENEW |
            IOF_DISABLEDISPLAYASICON |
            IOF_HIDECHANGEICON |
            IOF_VERIFYSERVERSEXIST |
            IOF_SHOWINSERTCONTROL;
    ouio.hWndOwner = pElement->Doc()->InPlace()->_hwnd;
    ouio.lpszFile = szFile;
    ouio.cchFile = ARRAY_SIZE(szFile);

    hr = THR(LoadProcedure(&s_dynprocOleUIInsertObjectA));
    if (!OK(hr))
        goto Cleanup;

    uRC = (*(UINT (STDAPICALLTYPE *)(LPOLEUIINSERTOBJECTA))
            s_dynprocOleUIInsertObjectA.pfn)(&ouio);

    hr = (OLEUI_OK     == uRC) ? S_OK :
         (OLEUI_CANCEL == uRC) ? S_FALSE :
                                 E_FAIL;
    if (S_OK != hr)
        goto Cleanup;

    Assert((ouio.dwFlags & IOF_SELECTCREATENEW) ||
           (ouio.dwFlags & IOF_SELECTCREATEFROMFILE) ||
           (ouio.dwFlags & IOF_SELECTCREATECONTROL));

    *pdwResult = ouio.dwFlags;

    if (*pdwResult & (IOF_SELECTCREATENEW|IOF_SELECTCREATECONTROL))
    {
        const int cbResult = 128;

        *plptszResult = new(Mt(DoInsertObjectUI_plptszResult)) TCHAR[cbResult];
        hr = (*plptszResult) ? S_OK : E_OUTOFMEMORY;
        if (!OK(hr))
            goto Cleanup;

        ClsidParamStrFromClsid (&ouio.clsid, *plptszResult, cbResult);
    }
    else // if (*pdwResult & IOF_SELECTCREATEFROMFILE)
    {
        int     nFile;

        nFile = MultiByteToWideChar(CP_ACP, 0,
            szFile, -1,
            wszFile, ARRAY_SIZE(wszFile));

        if (0 == nFile)
        {
            hr = HRESULT_FROM_WIN32(GetLastWin32Error());
            Assert (!OK(hr));
            goto Cleanup;
        }

        *plptszResult = new(Mt(DoInsertObjectUI_plptszResult)) TCHAR[nFile+1];
        hr = (*plptszResult) ? S_OK : E_OUTOFMEMORY;
        if (!OK(hr))
            goto Cleanup;

        _tcscpy ((LPTSTR)(*plptszResult), wszFile);
    }
Cleanup:
    if (ouio.hMetaPict)
        OleUIMetafilePictIconFree(ouio.hMetaPict);

    if (!OK(hr))
    {
        ClearErrorInfo();
        PutErrorInfoText(EPART_ACTION, IDS_EA_INSERT_CONTROL);
        pElement->CloseErrorInfo(hr);
    }

    RRETURN1 (hr, S_FALSE);
}
#endif  // NO_OLEUI


#define MAXINSERTSTRINGLEN  256
#define MAXINSERTPARAMLEN   (MAXINSERTSTRINGLEN - 30)

//+------------------------------------------------------------------------
//
//  Function:   CreateDataObjectFromIDM
//
//  Synopsis:   creates an object which contains html string
//              specifying object corresponding to cmd insert
//              command
//
//  Arguments:  [in] cmd            insert object command
//              [out] ppDataObj     IDataObject
//
//-------------------------------------------------------------------------

// this function could be optimized to make b-search or even direct access

HRESULT CreateHtmlDOFromIDM (UINT cmd, LPTSTR lptszInsertParam, IDataObject ** ppHtmlDO)
{
    CHAR                lpszObject[MAXINSERTSTRINGLEN];
    CHAR                lpszParam[MAXINSERTPARAMLEN];

    HRESULT             hr = S_OK;
    int                 i;
    int                 nFound = -1;

    *ppHtmlDO = NULL;

    if (IDM_INSERTOBJECT == cmd)
    {
        // Check if it is intrinic control, if so, return html tag.
        hr = THR(HtmlTagStrFromParam(lptszInsertParam, &nFound));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // find mapping
        nFound = -1;

        for (i = 0; i < ARRAY_SIZE(s_aHtmlMaps); i++)
        {
            if (s_aHtmlMaps[i].cmd == cmd )
            {
                nFound = i;
                break;
            }
        }

        Assert(nFound != -1);
    }


    // 0 is a special case, it must not be found. It is reserved to insert an OBJECT tag
    Assert(nFound != 0);

    if(nFound != -1)
    {
        if(lptszInsertParam)
        {
            TCHAR lpszStringToInsert[MAXINSERTSTRINGLEN];

            if(_tcslen(lptszInsertParam) > MAXINSERTPARAMLEN)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            if(IDM_IMAGE == cmd)
                _tcscpy(lpszStringToInsert, _T(" SRC=\""));
             else
                _tcscpy(lpszStringToInsert, _T(" ID=\""));

            // Append the parameter value
            _tcscat(lpszStringToInsert, lptszInsertParam);

            // Append the closing quote
            _tcscat(lpszStringToInsert, _T("\""));
            
            // The parameter is in unicode so we need to convert is to ANSII
            i = WideCharToMultiByte(CP_ACP, 0, lpszStringToInsert, -1, lpszParam, ARRAY_SIZE(lpszParam), NULL, NULL);
            if(i == 0)
            {
                hr = HRESULT_FROM_WIN32(GetLastWin32Error());
                goto Cleanup;
            }
        }
        else
        {
            // Insert an empty argument
            lpszParam[0] = 0;
        }
    }
    else
    {
         // Insert an object tag
         nFound = 0;

        // The parameter is in unicode so we need to convert is to ANSII
        i = WideCharToMultiByte(CP_ACP, 0, lptszInsertParam, -1, lpszParam, ARRAY_SIZE(lpszParam), NULL, NULL);
        if(i == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastWin32Error());
            goto Cleanup;
        }
    }
  
    i = wsprintfA(lpszObject, s_aHtmlMaps[nFound].lpsz, lpszParam);
    // Documentation says that we have an error when the returned value is less then the format control
    //  string length. It is not true, because in out case we convet <IMG%s> to <IMG>.
    // I am checking for 3 because the strings in our table have at least <>
    if(i < 3)
    {
        hr = HRESULT_FROM_WIN32(GetLastWin32Error());
        goto Cleanup;
    }

    hr = THR(CDataObject::CreateFromStr(lpszObject, ppHtmlDO));

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Function:   ObjectParamStrFromDO
//
//  Synopsis:   creates object param string which will then be inserted
//              into html like this: <OBJECT %s> </OBJECT>
//
//  Arguments:  [in] pDO                data object to convert
//              [in] prc                rectangle which specifies size of the object
//                                      if NULL, no size specified
//                                      if not NULL and left <= right and top <= bottom,
//                                      (right-left, bottom-top) specify the size
//              [in-out] lptszParam     output string
//              [in] cbParam            max len of output string
//              [out] pfNeedToCreateFromDO
//                                      true if this object need to
//                                      be created from a data object
//
//-------------------------------------------------------------------------

HRESULT
ObjectParamStrFromDO (IDataObject * pDO, RECT * prc,
                      LPTSTR lptszParam, int cbParam)
{
    HRESULT         hr = S_OK;
    TCHAR           tszParam[pdlUrlLen + 10] = _T("");
    TCHAR           tszHW[64] = _T("");
    TCHAR           tszClsid[CLSID_STRLEN + 1] = _T("");

    // tszParam used for both file name and clsid param, so make sure it will
    // be able to contain CLSID param
    Assert (64 < MAX_PATH);
    TCHAR *pFormatStr = _T("style=\"WIDTH = <0d>px HEIGHT = <1d>px TOP = <2d>px LEFT = <3d>px\" ");

    // set HEIGHT & WIDTH attrs if any
    if (prc && (prc->left <= prc->right) && (prc->top <= prc->bottom) )
    {
        hr = THR(Format (
            0, tszHW, ARRAY_SIZE (tszHW),
            pFormatStr,
            prc->right - prc->left, prc->bottom - prc->top,
            prc->top, prc->left ) );
        if (!OK(hr))
            goto Cleanup;
    }

    //
    if (OK(GetcfCLSIDFmt(pDO, tszClsid)))
    {
        hr = THR(ClsidParamStrFromClsidStr(tszClsid, tszParam, ARRAY_SIZE(tszParam)));
        if (!OK(hr))
            goto Cleanup;
    }
    else
    {
        tszParam[0] = 0;
        
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = THR(Format (
        0, lptszParam, cbParam,
        _T("<0s><1s>"),
        tszHW, tszParam
    ));

Cleanup:

    RRETURN1 (hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetShortcutFileName
//
//  Synopsis:   Generate file name for URL shortcut
//
//  Arguments:  [pchUrl]    URL to be used for creating the shortcut (input)
//              [pchName]   Description of the URL (input)
//              [pchOut]    Buffer (must be of size MAX_PATH or more) to return
//                          file name (output)
//
//  Notes:      We should throw away this code and use the shell function
//              GetShortcutFileName() instead as and when that becomes
//              available as an exported function.
//
//----------------------------------------------------------------------------

BOOL
GetShortcutFileName(const TCHAR * pchUrl, const TCHAR * pchName, TCHAR * pchOut)
{
    TCHAR   achBuf[MAX_PATH-4];

    Assert(pchUrl && *pchUrl);
    Assert(pchOut);

    achBuf[0] = 0;

    // Use name/title if one is supplied
    if (pchName && *pchName)
    {
        _tcsncpy(achBuf, pchName, MAX_PATH - 5);

        // Force this to be null-terminated, since _tcsncpy does not always do so.
        achBuf[MAX_PATH - 5] = 0;
        
        // Remove leading space
        StrTrim(achBuf, _T(" \t\r\n"));
    }

    // Otherwise, use the URL
    if (!achBuf[0])
    {
        _tcsncpy(achBuf, PathFindFileName(pchUrl), MAX_PATH - 5);

        // Force this to be null-terminated, since _tcsncpy does not always do so.
        achBuf[MAX_PATH - 5] = 0;
        
        // Remove leading space
        StrTrim(achBuf, _T(" \t\r\n"));
    }

    // Validate the file name
    if (!PathCanonicalize(pchOut, achBuf))
    {
        return FALSE;
    }

    // Replace any remaining invalid characters with '-'
    while (*pchOut)
    {
        if (PathGetCharType(*pchOut) & (GCT_INVALID | GCT_WILD | GCT_SEPARATOR))
        {
            *pchOut = _T('-');
        }
        pchOut++;
    }


    _tcscat(pchOut, _T(".url"));
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateLinkDataObject
//
//  Synopsis:   Create a link data object as an Internet Shortcut
//
//  Arguments:  [pchUrl]    URL to be used for creating the shortcut (input)
//              [pchName]   Description of the URL (input)
//              [ppLink]    Pointer to the link data object created (output)
//
//  Notes:      Mostly copied from IE3.
//
//----------------------------------------------------------------------------
HRESULT
CreateLinkDataObject(const TCHAR *              pchUrl,
                     const TCHAR *              pchName,
                     IUniformResourceLocator ** ppLink)
{
#if defined(WIN16) || defined(WINCE)
    return E_FAIL;
#else
    HRESULT         hr = S_OK;
    IShellLink *    pisl = NULL;
    TCHAR           achFileName[MAX_PATH];

    if (!pchUrl || !*pchUrl || !ppLink)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppLink = NULL;
    hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                           IID_IUniformResourceLocator, (void **)ppLink);
    if (hr)
        goto Cleanup;

    hr = (*ppLink)->SetURL(pchUrl, 0);
    if (hr)
    {
        // Translate URL-specific failure into generic failure.
        if (hr == URL_E_INVALID_SYNTAX)
            hr = E_FAIL;

        goto Cleanup;
    }

    hr = (*ppLink)->QueryInterface(IID_IShellLink, (void **)&pisl);
    if (hr != S_OK)
        goto Cleanup;

    if (!GetShortcutFileName(pchUrl, pchName, achFileName))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IGNORE_HR(pisl->SetDescription(achFileName));
    ReleaseInterface(pisl);

Cleanup:
    RRETURN(hr);
#endif
}


//+---------------------------------------------------------------------------
//
//  Function:   CopyFileToClipboard
//
//  Synopsis:   Supports CF_HDROP format
//
//  Arguments:  [pchPath]   Fully expanded file path (input)
//              [pDO]       Pointer to the data object (input)
//
//  Notes:      Mostly copied from IE3.
//
//----------------------------------------------------------------------------
HRESULT
CopyFileToClipboard(const TCHAR * pchPath, CGenDataObject * pDO)
{
    HRESULT             hr = S_OK;
#ifndef WIN16
    ULONG               cchPath, cchAnsi;
    HGLOBAL             hgDropFiles = NULL;
    DROPFILES *         pdf;
    char *              pchDropPath;

    if (!pchPath || !*pchPath || !pDO)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    cchPath = _tcslen(pchPath);

    // Convert file path to ANSI. While NT shell can handle both
    // UNICODE and ANSI, Win95 shell can handle only ANSI.
    cchAnsi = WideCharToMultiByte(CP_ACP, 0, pchPath, cchPath + 1,
                NULL, 0, NULL, NULL);
    if (cchAnsi <= 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // (+ 2) for double null terminator.
    // GPTR does zero-init for us
    hgDropFiles = GlobalAlloc(GPTR,
                    sizeof(DROPFILES) + cchAnsi + 1);
    if (!hgDropFiles)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pdf = (DROPFILES *)hgDropFiles;
    if (!pdf)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    pdf->pFiles = sizeof(DROPFILES);
    // pdf->fWide = FALSE; // ZeroInit takes care of this

    pchDropPath = (char *)(pdf + 1);
    WideCharToMultiByte(CP_ACP, 0, pchPath, cchPath + 1,
                pchDropPath, cchAnsi+1, NULL, NULL);

    hr = pDO->AppendFormatData(CF_HDROP, hgDropFiles);
    if (hr)
        goto Cleanup;

    // Don't free hgDropFiles on subsequent error now that
    // hgDropFiles has been added to the data object.
    // IDataObject::Release() will.
    hgDropFiles = NULL;

Cleanup:
    if (hgDropFiles)
        GlobalFree(hgDropFiles);
#endif // !WIN16
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// Function: IsTabKey
//
// Test if CMessage is a TAB KEYDOWN message.
//
//-----------------------------------------------------------------------------
BOOL
IsTabKey(CMessage * pMessage)
{
    BOOL fTabOrder = FALSE;

    if (pMessage->message == WM_KEYDOWN)
    {
        if (pMessage->wParam == VK_TAB)
        {
            fTabOrder = (pMessage->dwKeyState & FCONTROL) ? (FALSE) : (TRUE);
        }
    }

    return fTabOrder;
}

//+---------------------------------------------------------------------------
//
// Function: IsFrameTabKey
//
// Test if CMessage is a CTRL+TAB or F6 KEYDOWN message.
//
//-----------------------------------------------------------------------------
BOOL
IsFrameTabKey(CMessage * pMessage)
{
    BOOL fResult = FALSE;

    if (pMessage->message == WM_KEYDOWN)
    {
        if ((pMessage->wParam == VK_TAB && (pMessage->dwKeyState & FCONTROL))
                || (pMessage->wParam == VK_F6))
        {
            fResult = TRUE;
        }
    }
    return fResult;
}
