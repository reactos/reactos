//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       src\core\cdbase\basemso.cxx
//
//  Contents:   Implementation of IOleCommandTarget
//
//  Classes:    CBase
//
//  Functions:
//
//  History:    12-Sep-95   JuliaC    Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_PRIVCID_H_
#define X_PRIVCID_H_
#include "privcid.h"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_DWNNOT_H_
#define X_DWNNOT_H_
#include "dwnnot.h"
#endif

DeclareTag(tagMsoCommandTarget, "IOleCommandTarget", "IOleCommandTarget methods in CBase")


//+-------------------------------------------------------------------------
//
//  IOleCommandTarget implementation
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CBase::IsCmdGroupSupported, static
//
//  Synopsis:   Determine if the given command group is supported.
//
//--------------------------------------------------------------------------

BOOL
CBase::IsCmdGroupSupported(const GUID *pguidCmdGroup)
{
    return pguidCmdGroup == NULL || *pguidCmdGroup == CGID_MSHTML ||
            *pguidCmdGroup == CGID_ShellDocView || 
            *pguidCmdGroup == CGID_IWebBrowserPriv ||
            *pguidCmdGroup == CGID_ShortCut ||
            *pguidCmdGroup == CGID_Explorer ||
            *pguidCmdGroup == CGID_DownloadHost;
}

// The following enum is defined by classic MSHTML
enum 
{
    HTMLID_FIND         = 1,
    HTMLID_VIEWSOURCE   = 2,
    HTMLID_OPTIONS      = 3,
    NAMELESS_ENUM_Last_Enum
};


struct MAP 
{ 
    short idm; 
    USHORT usCmdID; 
};

static MAP amapCommandSet95[] =
{
    IDM_OPEN, OLECMDID_OPEN,
    IDM_NEW, OLECMDID_NEW,
    IDM_SAVE, OLECMDID_SAVE,
    IDM_SAVEAS, OLECMDID_SAVEAS,
    IDM_SAVECOPYAS, OLECMDID_SAVECOPYAS,
    IDM_PRINT, OLECMDID_PRINT,
    IDM_PRINTPREVIEW, OLECMDID_PRINTPREVIEW,
    IDM_PAGESETUP, OLECMDID_PAGESETUP,
    IDM_SPELL, OLECMDID_SPELL,
    IDM_PROPERTIES, OLECMDID_PROPERTIES,
    IDM_CUT, OLECMDID_CUT,
    IDM_COPY, OLECMDID_COPY,
    IDM_PASTE, OLECMDID_PASTE,
    IDM_PASTESPECIAL, OLECMDID_PASTESPECIAL,
    IDM_UNDO, OLECMDID_UNDO,
    IDM_REDO, OLECMDID_REDO,
    IDM_SELECTALL, OLECMDID_SELECTALL,
    IDM_CLEARSELECTION, OLECMDID_CLEARSELECTION,
    IDM_STOP, OLECMDID_STOP,
    IDM_REFRESH, OLECMDID_REFRESH,
    IDM_STOPDOWNLOAD, OLECMDID_STOPDOWNLOAD,
    IDM_ENABLE_INTERACTION, OLECMDID_ENABLE_INTERACTION,
    OLECMDID_ONUNLOAD, OLECMDID_ONUNLOAD,
    IDM_INFOVIEW_ZOOM, OLECMDID_ZOOM,
    IDM_INFOVIEW_GETZOOMRANGE, OLECMDID_GETZOOMRANGE,
    OLECMDID_DONTDOWNLOADCSS, OLECMDID_DONTDOWNLOADCSS,
};

static MAP amapPersistence[] =
{
    IDM_ONPERSISTSHORTCUT,     CMDID_INTSHORTCUTCREATE,
    IDM_SAVEASTHICKET,         CMDID_SAVEASTHICKET,
};

static MAP amapDownloadHost[] =
{
    IDM_DWNH_SETDOWNLOAD,      DWNHCMDID_SETDOWNLOADNOTIFY,
};

static MAP amapShellDocView[] =
{
    IDM_SHDV_FINALTITLEAVAIL,  SHDVID_FINALTITLEAVAIL,
    IDM_SHDV_MIMECSETMENUOPEN, SHDVID_MIMECSETMENUOPEN,
    IDM_SHDV_FONTMENUOPEN,     SHDVID_FONTMENUOPEN,
    IDM_SHDV_PRINTFRAME,       SHDVID_PRINTFRAME,
    IDM_SHDV_PUTOFFLINE,       SHDVID_PUTOFFLINE,
    IDM_SHDV_GOBACK,           SHDVID_GOBACK,
    IDM_SHDV_GOFORWARD,        SHDVID_GOFORWARD,
    IDM_SHDV_CANGOBACK,        SHDVID_CANGOBACK,
    IDM_SHDV_CANGOFORWARD,     SHDVID_CANGOFORWARD,
    IDM_SHDV_CANSUPPORTPICS,   SHDVID_CANSUPPORTPICS,
    IDM_SHDV_CANDEACTIVATENOW, SHDVID_CANDEACTIVATENOW,
    IDM_SHDV_DEACTIVATEMENOW,  SHDVID_DEACTIVATEMENOW,
    IDM_SHDV_NODEACTIVATENOW,  SHDVID_NODEACTIVATENOW,
#ifndef WIN16
    IDM_SHDV_SETPENDINGURL,    SHDVID_SETPENDINGURL,
    IDM_SHDV_ISDRAGSOURCE,     SHDVID_ISDRAGSOURCE,
    IDM_SHDV_DOCFAMILYCHARSET, SHDVID_DOCFAMILYCHARSET,
    IDM_SHDV_DOCCHARSET,       SHDVID_DOCCHARSET,
    IDM_SHDV_GETMIMECSETMENU,  SHDVID_GETMIMECSETMENU,
    IDM_SHDV_GETFONTMENU,      SHDVID_GETFONTMENU,
    IDM_SHDV_GETDOCDIRMENU,    SHDVID_GETDOCDIRMENU,
#endif
    IDM_SHDV_CANDOCOLORSCHANGE,SHDVID_CANDOCOLORSCHANGE,
    IDM_SHDV_ONCOLORSCHANGE,   SHDVID_ONCOLORSCHANGE,
    IDM_SHDV_ADDMENUEXTENSIONS,SHDVID_ADDMENUEXTENSIONS, // Context Menu Extensions
    IDM_SHDV_PAGEFROMPOSTDATA, SHDVID_PAGEFROMPOSTDATA,
};

static MAP amapExplorer[] =
{
    IDM_SHDV_GETFRAMEZONE,     SBCMDID_MIXEDZONE, 
};

static MAP amapClassicMSHTML[] =
{
    IDM_FIND,                  HTMLID_FIND,
    IDM_VIEWSOURCE,            HTMLID_VIEWSOURCE,
    IDM_OPTIONS,               HTMLID_OPTIONS,
};


//+-------------------------------------------------------------------------
//
//  Method:     CBase::IDMFromCmdID, static
//
//  Synopsis:   Compute menu item identifier from command set and command id.
//
//--------------------------------------------------------------------------

int
CBase::IDMFromCmdID(const GUID *pguidCmdGroup, ULONG ulCmdID)
{
    MAP *pmap;
    int  cmap;

    if (pguidCmdGroup == NULL)
    {
        pmap = amapCommandSet95;
        cmap = ARRAY_SIZE(amapCommandSet95);
    }
    else if (*pguidCmdGroup == CGID_MSHTML)
    {

        // Command identifiers in the Forms3 command set map
        // directly to menu item identifiers.
        return ulCmdID;

    }
    else if (*pguidCmdGroup == CGID_ShellDocView)
    {
        pmap = amapShellDocView;
        cmap = ARRAY_SIZE(amapShellDocView);
    }
    else if (*pguidCmdGroup == CGID_IWebBrowserPriv)
    {
        pmap = amapClassicMSHTML;
        cmap = ARRAY_SIZE(amapClassicMSHTML);
    }
    else if (*pguidCmdGroup == CGID_Explorer)
    {
        pmap = amapExplorer;
        cmap = ARRAY_SIZE(amapExplorer);
    }
    else if (*pguidCmdGroup == CGID_ShortCut)
    {
        pmap = amapPersistence;
        cmap = ARRAY_SIZE(amapPersistence);
    }
    else if (*pguidCmdGroup == CGID_DownloadHost)
    {
        pmap = amapDownloadHost;
        cmap = ARRAY_SIZE(amapDownloadHost);
    }
    else
    {
        return IDM_UNKNOWN;
    }

    for (; --cmap >= 0; pmap++)
    {
        if (pmap->usCmdID == ulCmdID)
            return pmap->idm;
    }

    return IDM_UNKNOWN;
}


//+-------------------------------------------------------------------------
//
//  Method:     OLECMDIDFromIDM
//
//  Synopsis:   Compute standard command id from an idm.
//
//--------------------------------------------------------------------------

BOOL
OLECMDIDFromIDM(int idm, ULONG *pulCmdID)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(amapCommandSet95); i++)
    {
        if (amapCommandSet95[i].idm == idm)
        {
            *pulCmdID = amapCommandSet95[i].usCmdID;
            return TRUE;
        }
    }

    return FALSE;
}


// Table used to to translate command names to command IDs and get command value return types
// Notice that the last element defines the default cmdID and vt returned when the item is not found
CBase::CMDINFOSTRUCT CBase::cmdTable[] =
{
        _T("CreateBookmark"),       IDM_BOOKMARK,
        _T("CreateLink"),           IDM_HYPERLINK,
        _T("InsertImage"),          IDM_IMAGE,
        _T("Bold"),                 IDM_BOLD,
        _T("BrowseMode"),           IDM_BROWSEMODE,
        _T("EditMode"),             IDM_EDITMODE,
        _T("InsertButton"),         IDM_BUTTON,
        _T("InsertIFrame"),         IDM_IFRAME,
        _T("InsertInputButton"),    IDM_INSINPUTBUTTON,
        _T("InsertInputCheckbox"),  IDM_CHECKBOX,
        _T("InsertInputImage"),     IDM_INSINPUTIMAGE,
        _T("InsertInputRadio"),     IDM_RADIOBUTTON,
        _T("InsertInputText"),      IDM_TEXTBOX,
        _T("InsertSelectDropdown"), IDM_DROPDOWNBOX,
        _T("InsertSelectListbox"),  IDM_LISTBOX,
        _T("InsertTextArea"),       IDM_TEXTAREA,
#ifdef NEVER        
        _T("InsertHtmlArea"),       IDM_HTMLAREA,
#endif        
        _T("Italic"),               IDM_ITALIC,
        _T("SizeToControl"),        IDM_SIZETOCONTROL,
        _T("SizeToControlHeight"),  IDM_SIZETOCONTROLHEIGHT,
        _T("SizeToControlWidth"),   IDM_SIZETOCONTROLWIDTH,
        _T("Underline"),            IDM_UNDERLINE,
        _T("Copy"),                 IDM_COPY,
        _T("Cut"),                  IDM_CUT,
        _T("Delete"),               IDM_DELETE,
        _T("Print"),                IDM_EXECPRINT,
        _T("JustifyCenter"),        IDM_JUSTIFYCENTER,
        _T("JustifyFull"),          IDM_JUSTIFYFULL,
        _T("JustifyLeft"),          IDM_JUSTIFYLEFT,
        _T("JustifyRight"),         IDM_JUSTIFYRIGHT,
        _T("JustifyNone"),          IDM_JUSTIFYNONE,
        _T("Paste"),                IDM_PASTE,
        _T("PlayImage"),            IDM_DYNSRCPLAY,
        _T("StopImage"),            IDM_DYNSRCSTOP,
        _T("InsertInputReset"),     IDM_INSINPUTRESET,
        _T("InsertInputSubmit"),    IDM_INSINPUTSUBMIT,
        _T("InsertInputFileUpload"),IDM_INSINPUTUPLOAD,
        _T("InsertFieldset"),       IDM_INSFIELDSET,
        _T("Unselect"),             IDM_CLEARSELECTION,
        _T("BackColor"),            IDM_BACKCOLOR,
        _T("ForeColor"),            IDM_FORECOLOR,
        _T("FontName"),             IDM_FONTNAME,
        _T("FontSize"),             IDM_FONTSIZE,
        _T("FormatBlock"),          IDM_BLOCKFMT,
        _T("Indent"),               IDM_INDENT,
        _T("InsertMarquee"),        IDM_MARQUEE,
        _T("InsertOrderedList"),    IDM_ORDERLIST,
        _T("InsertParagraph"),      IDM_PARAGRAPH,
        _T("InsertUnorderedList"),  IDM_UNORDERLIST,
        _T("Outdent"),              IDM_OUTDENT,
        _T("Redo"),                 IDM_REDO,
        _T("Refresh"),              IDM_REFRESH,
        _T("RemoveParaFormat"),     IDM_REMOVEPARAFORMAT,
        _T("RemoveFormat"),         IDM_REMOVEFORMAT,
        _T("SelectAll"),            IDM_SELECTALL,
        _T("StrikeThrough"),        IDM_STRIKETHROUGH,
        _T("Subscript"),            IDM_SUBSCRIPT,            
        _T("Superscript"),          IDM_SUPERSCRIPT,
        _T("Undo"),                 IDM_UNDO,
        _T("Unlink"),               IDM_UNLINK,
        _T("InsertHorizontalRule"), IDM_HORIZONTALLINE,
        _T("UnBookmark"),           IDM_UNBOOKMARK,
        _T("OverWrite"),            IDM_OVERWRITE,
        _T("InsertInputPassword"),  IDM_INSINPUTPASSWORD,
        _T("InsertInputHidden"),    IDM_INSINPUTHIDDEN,
        _T("DirLTR"),               IDM_DIRLTR,
        _T("DirRTL"),               IDM_DIRRTL,
        _T("BlockDirLTR"),          IDM_BLOCKDIRLTR,
        _T("BlockDirRTL"),          IDM_BLOCKDIRRTL,
        _T("InlineDirLTR"),         IDM_INLINEDIRLTR,
        _T("InlineDirRTL"),         IDM_INLINEDIRRTL,
        _T("SaveAs"),               IDM_SAVEAS,
        _T("Open"),                 IDM_OPEN,
        _T("Stop"),                 IDM_STOP,
        NULL,                       0
};

// Translates command name into command ID. If the command is not found and the
//  command string starts with a digit the command number is used.
HRESULT 
CBase::CmdIDFromCmdName(BSTR bstrCmdName, ULONG *pcmdValue)
{
    int     i;
    HRESULT hr=S_OK;

    Assert(pcmdValue != NULL);
    *pcmdValue = 0;

    if(FormsIsEmptyString(bstrCmdName))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    i = 0;
    while(cmdTable[i].cmdName != NULL)
    {
        if(StrCmpIC(cmdTable[i].cmdName, bstrCmdName) == 0)
        {
            break;
        }
        i++;
    }
    if(cmdTable[i].cmdName != NULL)
    {
        // The command name was found, use the value from the table
        *pcmdValue = cmdTable[i].cmdID;
        if(*pcmdValue == 0)
            hr = E_INVALIDARG;
    }
    else
    {
        hr = E_INVALIDARG;
    }

Cleanup:
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandSupported
//
//  Synopsis:
//
//  Returns: returns true if given command (like bold) is supported
//----------------------------------------------------------------------------

HRESULT
CBase::queryCommandSupported(const BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    HRESULT         hr = S_OK;
    ULONG           uCmdId;

    if(pfRet == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = CmdIDFromCmdName(bstrCmdId, &uCmdId);
    if(hr == S_OK)
    {
        *pfRet = VB_TRUE;
    }
    else if(hr == E_INVALIDARG)
    {
        *pfRet = VB_FALSE;
        hr = S_OK;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandEnabled
//
//  Synopsis:
//
//  Returns: returns true if given command is currently enabled. For toolbar
//          buttons not being enabled means being grayed.
//----------------------------------------------------------------------------

HRESULT
CBase::queryCommandEnabled(const BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    HRESULT         hr = S_OK;
    DWORD           dwFlags;

    if(pfRet == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pfRet = VB_FALSE;

    hr = THR(QueryCommandHelper(bstrCmdId, &dwFlags, NULL));
    if(hr)
        goto Cleanup;

   if(dwFlags == MSOCMDSTATE_NINCHED ||
      dwFlags == MSOCMDSTATE_UP || dwFlags == MSOCMDSTATE_DOWN)
    {
        *pfRet = VB_TRUE;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandState
//
//  Synopsis:
//
//  Returns: returns true if given command is on. For toolbar buttons this
//          means being down. Note that a command button can be disabled
//          and also be down.
//----------------------------------------------------------------------------

HRESULT
CBase::queryCommandState(const BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    HRESULT         hr = S_OK;
    DWORD           dwFlags;

    if(pfRet == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pfRet = VB_FALSE;

    hr = THR(QueryCommandHelper(bstrCmdId, &dwFlags, NULL));
    if(hr)
        goto Cleanup;

   if(dwFlags == MSOCMDSTATE_DOWN)
    {
        *pfRet = VB_TRUE;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandIndeterm
//
//  Synopsis:
//
//  Returns: returns true if given command is in indetermined state.
//          If this value is TRUE the value returnd by queryCommandState
//          should be ignored.
//----------------------------------------------------------------------------

HRESULT
CBase::queryCommandIndeterm(const BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    HRESULT         hr = S_OK;
    DWORD           dwFlags;

    if(pfRet == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pfRet = VB_FALSE;

    hr = THR(QueryCommandHelper(bstrCmdId, &dwFlags, NULL));
    if(hr)
        goto Cleanup;

    if(dwFlags == MSOCMDSTATE_NINCHED)
    {
        *pfRet = VB_TRUE;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



//+---------------------------------------------------------------------------
//
//  Member:     queryCommandText
//
//  Synopsis:
//
//  Returns: Returns the text that describes the command (eg bold)
//----------------------------------------------------------------------------

HRESULT
CBase::queryCommandText(const BSTR bstrCmdId, BSTR *pcmdText)
{
    HRESULT     hr = S_OK;

    if(pcmdText == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pcmdText = NULL;

    hr = THR(QueryCommandHelper(bstrCmdId, NULL, pcmdText));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandValue
//
//  Synopsis:
//
//  Returns: Returns the  command value like font name or size.
//----------------------------------------------------------------------------

HRESULT
CBase::queryCommandValue(const BSTR bstrCmdId, VARIANT *pvarRet)
{
    HRESULT     hr = S_OK;
    DWORD       dwCmdId;

    if(pvarRet == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = VariantClear(pvarRet);
    if(hr)
        goto Cleanup;

    // Convert the command ID from string to number
    hr = THR(CmdIDFromCmdName(bstrCmdId, &dwCmdId));
    if(hr)
        goto Cleanup;

    // Set the appropriate variant type
    V_VT(pvarRet) = GetExpectedCmdValueType(dwCmdId);

    // Call QueryStatus instead of exec if the expected return value is boolean
    if(V_VT(pvarRet) == VT_BOOL)
    {
        MSOCMD msocmd;
        
        msocmd.cmdID = dwCmdId;
        msocmd.cmdf  = 0;

        hr = THR(QueryStatus(const_cast < GUID * > ( & CGID_MSHTML ),
                        1, &msocmd, NULL));
        if (hr)
            goto Cleanup;

        V_BOOL(pvarRet) = (msocmd.cmdf == MSOCMDSTATE_NINCHED || msocmd.cmdf == MSOCMDSTATE_DOWN)
                        ? VB_TRUE : VB_FALSE;
    }
    else
    {
        // Use exec to get the string on integer value

        // If GetExpectedCmdValueType returned a VT_BSTR we need to null out the value
        // VariantClear wont do that.  If the pvarRet passed in the VariantClear
        // the bstrVal would be bogus.
        V_BSTR(pvarRet) = NULL;

        hr = THR(Exec(const_cast < GUID * > ( & CGID_MSHTML )
                                ,dwCmdId, MSOCMDEXECOPT_DONTPROMPTUSER, NULL, pvarRet));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



//+---------------------------------------------------------------------------
//
//  Member:     queryCommandHelper
//
//  Synopsis:   This function is called by QueryCommandXXX functions and does the
//              command ID conversions and returns the flags or the text for given
//              command. Only one of the return parameters must be not NULL.
//
//  Returns:    S_OK if the value is returned
//----------------------------------------------------------------------------

struct MSOCMDTEXT_WITH_TEXT
{
    MSOCMDTEXT   header;
    WCHAR        text[FORMS_BUFLEN];
};

HRESULT
CBase::QueryCommandHelper(const BSTR bstrCmdId, DWORD *cmdf, BSTR *pcmdText)
{
    HRESULT                 hr = S_OK;
    MSOCMD                  msocmd;
    MSOCMDTEXT_WITH_TEXT    msocmdtext;

    Assert((cmdf == NULL && pcmdText != NULL) || (cmdf != NULL && pcmdText == NULL));

    // initialize the values so in case of error we return a NULL pointer
    if(pcmdText != NULL)
    {
        *pcmdText = NULL;
    }
    else
    {
        *cmdf = 0L;
    }

    // Fill the command structure converting the command ID from string to number
    hr = CmdIDFromCmdName(bstrCmdId, &(msocmd.cmdID));
    if(hr)
        goto Cleanup;
    msocmd.cmdf  = 0;

    if(pcmdText != NULL)
    {
        msocmdtext.header.cmdtextf = MSOCMDTEXTF_NAME;
        msocmdtext.header.cwBuf    = FORMS_BUFLEN;
        msocmdtext.header.cwActual = 0;
    }

    hr = THR(QueryStatus(const_cast < GUID * > ( & CGID_MSHTML ),
                    1, &msocmd, (MSOCMDTEXT *)&msocmdtext));
    if (hr)
        goto Cleanup;

    if(pcmdText != NULL)
    {
        // Ignore the  msocmd value, just return the text
        if(msocmdtext.header.cwActual > 0)
        {
             // Allocate the return string
            *pcmdText = SysAllocString(msocmdtext.header.rgwz);
            if(*pcmdText == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
    }
    else
    {
        // return the flags
        *cmdf = msocmd.cmdf;
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     execCommand
//
//  Synopsis:   Executes given command
//
//  Returns:
//----------------------------------------------------------------------------

HRESULT
CBase::execCommand(const BSTR bstrCmdId, VARIANT_BOOL showUI, VARIANT value)
{
    DWORD     dwCmdOpt;
    DWORD     dwCmdId;
    VARIANT * pValue = NULL;
    HRESULT   hr;
    
    // Translate the "show UI" flag into appropriate option
    dwCmdOpt = (showUI == VB_FALSE) 
            ? MSOCMDEXECOPT_DONTPROMPTUSER 
            : MSOCMDEXECOPT_PROMPTUSER;

    // Convert the command ID from string to number
    hr = THR_NOTRACE(CmdIDFromCmdName(bstrCmdId, &dwCmdId));
    if(hr)
        goto Cleanup;

    pValue = (V_VT(&value) == (VT_BYREF | VT_VARIANT)) ?
        V_VARIANTREF(&value) : &value;

    // Some functions do not check for empty or error type variants
    if (V_VT(pValue) == VT_ERROR || V_VT(pValue) == VT_EMPTY)
    {
        pValue = NULL;
    }

    hr = THR(Exec(const_cast < GUID * > ( & CGID_MSHTML )
                            ,dwCmdId, dwCmdOpt, pValue, NULL));

Cleanup:
    RRETURN(hr);
 }


//+---------------------------------------------------------------------------
//
//  Member:     execCommandShowHelp
//
//  Synopsis:
//
//  Returns:
//----------------------------------------------------------------------------

HRESULT
CBase::execCommandShowHelp(const BSTR bstrCmdId)
{
    HRESULT   hr;
    DWORD     dwCmdId;

    // Convert the command ID from string to number
    hr = CmdIDFromCmdName(bstrCmdId, &dwCmdId);
    if(hr)
        goto Cleanup;

    hr = THR(Exec(const_cast < GUID * > ( & CGID_MSHTML )
                    , dwCmdId, MSOCMDEXECOPT_SHOWHELP, NULL, NULL));

Cleanup:
    RRETURN(hr);
}



// Returns the expected VARIANT type of the command value (like VT_BSTR for font name)
VARTYPE CBase::GetExpectedCmdValueType(ULONG uCmdID)
{
    // We do not need to set the pvarOut, except for IDM_FONTSIZE. But we still
    // use this logic to determine if we need to use the queryStatus or exec
    // to get the command value
    if(uCmdID == IDM_FONTSIZE || uCmdID == IDM_FORECOLOR || uCmdID == IDM_BACKCOLOR)
    {
        return VT_I4;
    }

    if(uCmdID == IDM_BLOCKFMT || uCmdID == IDM_FONTNAME) 
    {
        return VT_BSTR;
    }

    return VT_BOOL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::ExecSetGetProperty and 
//              CBase::ExecSetGetKnownProp
//
//  Synopsis:   Helper functions for Exec(), it Set/Get a property for
//              control. pvarargIn, pvarargOut can both not NULL. they both
//              call CBase:: ExecSetGetHelper for the invoke logic.
//
//--------------------------------------------------------------------------
HRESULT
CBase::ExecSetGetProperty(
        VARIANTARG *    pvarargIn,      // In parameter
        VARIANTARG *    pvarargOut,     // Out parameter
        UINT            uPropName,      // property name
        VARTYPE         vt)             // Parameter type
{
    HRESULT         hr = S_OK;
    IDispatch *     pDispatch = NULL;
    DISPID          dispid;

    // Invalid parameters
    if (!pvarargIn && !pvarargOut)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // try to turn the uPropName into a dispid for the property
    hr = THR_NOTRACE(GetDispatchForProp(uPropName, &pDispatch, &dispid));
    if (hr)
        goto Cleanup;

    // call the helper to do the work
    hr = THR_NOTRACE(ExecSetGetHelper(pvarargIn, pvarargOut, pDispatch, dispid, vt));

Cleanup:
    ReleaseInterface(pDispatch);
    if (DISP_E_UNKNOWNNAME == hr)
        hr = OLECMDERR_E_NOTSUPPORTED; // we listen for this error code
    RRETURN(hr);
}

HRESULT
CBase::ExecSetGetKnownProp(
        VARIANTARG *    pvarargIn,      // In parameter
        VARIANTARG *    pvarargOut,     // Out parameter
        DISPID          dispidProp, 
        VARTYPE         vt)             // Parameter type
{
    HRESULT hr = S_OK;
    IDispatch *     pDispatch = NULL;

    // Invalid parameters?
    if (!pvarargIn && !pvarargOut)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // now get the IDispatch for *this*
    hr = THR_NOTRACE(PrivateQueryInterface(IID_IDispatch, (void **)&pDispatch));
    if (hr)
        goto Cleanup;

    // now call the helper to do the work
    hr = THR_NOTRACE(ExecSetGetHelper(pvarargIn, pvarargOut, pDispatch, dispidProp, vt));

Cleanup:
    ReleaseInterface(pDispatch);
    RRETURN(hr);
}


HRESULT
CBase::ExecSetGetHelper(
        VARIANTARG *    pvarargIn,      // In parameter
        VARIANTARG *    pvarargOut,     // Out parameter
        IDispatch  *    pDispatch,      // the IDispatch for *this*
        DISPID          dispid,         // the property dispid
        VARTYPE         vt)             // Parameter type
{
    HRESULT      hr  =S_OK;
    DISPPARAMS   dp = g_Zero.dispparams;         // initialized be zero.
    DISPID       dispidPut = DISPID_PROPERTYPUT; // Dispid of prop arg.

    // Set property
    if (pvarargIn)
    {
        // Fill in dp
        dp.rgvarg = pvarargIn;
        dp.rgdispidNamedArgs = &dispidPut;
        dp.cArgs = 1;
        dp.cNamedArgs = 1;

        hr = THR_NOTRACE(pDispatch->Invoke(
                dispid,
                IID_NULL,
                NULL,
                DISPATCH_PROPERTYPUT,
                &dp,
                NULL,
                NULL,
                NULL));

        if (hr)
            goto Cleanup;
    }

    // Get property
    if (pvarargOut)
    {
        // Get property requires different dp
        dp = g_Zero.dispparams;

        hr = THR_NOTRACE(pDispatch->Invoke(
                dispid,
                IID_NULL,
                NULL,
                DISPATCH_PROPERTYGET,
                &dp,
                pvarargOut,
                NULL,
                NULL));

        if (hr)
        {
            //
            // BUGBUG (EricVas) - This is a hack to prevent the member
            // not found from trickling up, causing a nasty message box to
            // appear
            //

            hr = OLECMDERR_E_DISABLED;

            goto Cleanup;
        }

        // Update the VT if necessary
        V_VT(pvarargOut) = vt;
    }

Cleanup:
    if (DISP_E_UNKNOWNNAME == hr)
        hr = OLECMDERR_E_NOTSUPPORTED; // we listen for this error code
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::ExecToggleCmd
//
//  Synopsis:   Helper function for exec(). It is used for cmdidBold,
//              cmdidItalic, cmdidUnderline. It always toggle property
//              value.
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------
HRESULT
CBase::ExecToggleCmd(UINT uPropName)       // Dipatch ID for command
{
    HRESULT         hr ;
    IDispatch *     pDispatch = NULL;
    DISPPARAMS      dp = g_Zero.dispparams;         // initialized be zero.
    DISPID          dispidPut = DISPID_PROPERTYPUT; // Dispid of prop arg.
    VARIANT         var;
    DISPID          dispid;

    hr = THR(GetDispatchForProp(uPropName, &pDispatch, &dispid));
    if (hr)
        goto Cleanup;

    VariantInit(&var);
    V_VT(&var) = VT_BOOL;

    // Get property value
    hr = THR_NOTRACE(pDispatch->Invoke(
            dispid,
            IID_NULL,
            NULL,
            DISPATCH_PROPERTYGET,
            &dp,
            &var,
            NULL,
            NULL));
    if (hr)
        goto Cleanup;

    // Toggle property value
    V_BOOL(&var) = !V_BOOL(&var);

    // Fill in dp
    dp.rgvarg = &var;
    dp.rgdispidNamedArgs = &dispidPut;
    dp.cArgs = 1;
    dp.cNamedArgs = 1;

    // Set new property value
    hr = THR_NOTRACE(pDispatch->Invoke(
            dispid,
            IID_NULL,
            NULL,
            DISPATCH_PROPERTYPUT,
            &dp,
            NULL,
            NULL,
            NULL));

Cleanup:
    ReleaseInterface(pDispatch);
    if (DISP_E_UNKNOWNNAME == hr)
        hr = OLECMDERR_E_NOTSUPPORTED; // we listen for this error code
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::QueryStatusProperty
//
//  Synopsis:   Helper function for QueryStatus(), it determines if a control
//              supports a property by checking whether you can get property. .
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------
HRESULT
CBase::QueryStatusProperty(
        MSOCMD *    pCmd,
        UINT        uPropName,
        VARTYPE     vt)
{
    HRESULT         hr;
    IDispatch *     pDispatch = NULL;
    CVariant        var;
    DISPPARAMS      dp = g_Zero.dispparams;         // initialized be zero.
    DISPID          dispid;

    hr = THR_NOTRACE(GetDispatchForProp(uPropName, &pDispatch, &dispid));
    if (hr)
        goto Cleanup;

    V_VT(&var) = vt;

    hr = THR_NOTRACE(pDispatch->Invoke(
            dispid,
            IID_NULL,
            NULL,
            DISPATCH_PROPERTYGET,
            &dp,
            &var,
            NULL,
            NULL));

Cleanup:
    if(!hr)
    {
        if(V_VT(&var) == VT_BOOL && V_BOOL(&var) == VB_TRUE)
            pCmd->cmdf = MSOCMDSTATE_DOWN;
        else
            pCmd->cmdf = MSOCMDSTATE_UP;
    }
    if (DISP_E_UNKNOWNNAME == hr)
        hr = OLECMDERR_E_NOTSUPPORTED; // we listen for this error code
    ReleaseInterface(pDispatch);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::ExecSetPropertyCmd
//
//  Synopsis:   Helper function for Exec(), It is used for SpecialEffect
//              Commands, Justify (TextAlign). For these commands, there are
//              not input parameter.
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------
HRESULT
CBase::ExecSetPropertyCmd(UINT uPropName, DWORD value)
{
    VARIANT     var;

    VariantInit(&var);
    V_VT(&var) = VT_I4;
    V_I4(&var) = value;

    return THR_NOTRACE(ExecSetGetProperty(&var, NULL, uPropName, VT_I4));
}

//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetDispatchForProp
//
//  Synopsis:   Helper function for flavours of Exec()-s and QueryStatus()-es
//
//--------------------------------------------------------------------------

HRESULT
CBase::GetDispatchForProp(UINT uPropName, IDispatch ** ppDisp, DISPID * pdispid)
{
    HRESULT     hr;
    TCHAR       achPropName[64];
    LPTSTR      pchPropName = achPropName;
    int         nLoadStringRes;

    *ppDisp = NULL;

    hr = THR_NOTRACE(PrivateQueryInterface(IID_IDispatch, (void **)ppDisp));
    if (hr)
        goto Cleanup;

    nLoadStringRes = LoadString(GetResourceHInst(), uPropName, achPropName, ARRAY_SIZE(achPropName));
    if (0 == nLoadStringRes)
    {
        hr = THR(GetLastWin32Error());
        Assert (!OK(hr));
        goto Cleanup;
    }

    hr = THR_NOTRACE((*ppDisp)->GetIDsOfNames(IID_NULL, &pchPropName, 1, g_lcidUserDefault, pdispid));

Cleanup:
    if (hr)
        ClearInterface (ppDisp);

    RRETURN (hr);
}

//+-------------------------------------------------------------------------
//
// Method:      CTQueryStatus
//
// Synopsis:    Call IOleCommandTarget::QueryStatus on an object.
//
//--------------------------------------------------------------------------

HRESULT
CTQueryStatus(
        IUnknown *pUnk,
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    IOleCommandTarget * pCommandTarget;
    HRESULT             hr;

    if (!pUnk)
    {
        Assert(0);
        RRETURN(E_FAIL);
    }

    hr = THR_NOTRACE(pUnk->QueryInterface(
            IID_IOleCommandTarget,
            (void**) &pCommandTarget));

    if (!hr)
    {
        hr = THR_NOTRACE(pCommandTarget->QueryStatus(
                pguidCmdGroup,
                cCmds,
                rgCmds,
                pcmdtext));
        pCommandTarget->Release();
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
// Method: CTExec
//
// Synopsis:    Call IOleCommandTarget::Exec on an object.
//
//--------------------------------------------------------------------------

HRESULT
CTExec(
        IUnknown *pUnk,
        const GUID * pguidCmdGroup,
        DWORD  nCmdID,
        DWORD  nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    IOleCommandTarget * pCommandTarget;
    HRESULT             hr;

    if (!pUnk)
    {
        Assert(0);
        RRETURN(E_FAIL);
    }

    hr = THR_NOTRACE(pUnk->QueryInterface(
            IID_IOleCommandTarget,
            (void**) &pCommandTarget));

    if (!hr)
    {
        hr = THR_NOTRACE(pCommandTarget->Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut));
        pCommandTarget->Release();
    }

    RRETURN(hr);
}
