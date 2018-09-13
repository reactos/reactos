#include "headers.hxx"

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_DLGCMD_HXX_
#define _X_DLGCMD_HXX_
#include "dlgcmd.hxx"
#endif

#ifndef _X_HTMLED_HXX_
#define _X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

using namespace EdUtil;

MtDefine(CDialogCommand, EditCommand, "CDialogCommand")

//
// s_dlgInfo[] table provides the required mapping between IDM's for supported dialogs
// and their corresponding resource id as well as undo text.
// BUGBUG: raminh
//         Undo will be tracked by the object model, so idsUndoText should go away
//

static const struct DialogInfo
{
    UINT    idm;
    UINT    idsUndoText;
    TCHAR * szidr;
}
s_dlgInfo[] =
{
    {IDM_REPLACE,       IDS_EDUNDOGENERICTEXT,   IDR_REPLACEDIALOG},
    {IDM_PARAGRAPH,     IDS_EDUNDOGENERICTEXT,   IDR_FORPARDIALOG},
    {IDM_FONT,          IDS_EDUNDOGENERICTEXT,   IDR_FORCHARDIALOG},
    {IDM_IMAGE,         IDS_EDUNDONEWCTRL,       IDR_INSIMAGEDIALOG},
    {IDM_HYPERLINK,     IDS_EDUNDOGENERICTEXT,   IDR_EDLINKDIALOG}
    // Not implemented in IE40
    //{IDM_GOTO,          0,                     IDR_GOBOOKDIALOG},
    //{IDM_BOOKMARK,      IDS_UNDOGENERICTEXT,   IDR_EDBOOKDIALOG},
};

//
// Forward references
//

HRESULT LoadProcedure(DYNPROC *pdynproc);

void DeinitDynamicLibraries();

//+---------------------------------------------------------------------------
//
//  CDialogCommand Constructor
//
//----------------------------------------------------------------------------

CDialogCommand::CDialogCommand(DWORD cmdId, CHTMLEditor * ped ) : CCommand( cmdId, ped )
{
}


//+---------------------------------------------------------------------------
//
//  CDialogCommand Destructor
//
//----------------------------------------------------------------------------

CDialogCommand::~CDialogCommand()
{
}

//+---------------------------------------------------------------------------
//
//  CDialogCommand Exec
//
//----------------------------------------------------------------------------

HRESULT 
CDialogCommand::PrivateExec( DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut )
{
    HRESULT         hr = OLECMDERR_E_NOTSUPPORTED;
    HWND            myHWND;
    IDispatch  *    pDisp = NULL;
    VARIANT         varDoc; 
    VARIANT         varReturn;
    OLECMD          cmd;

    Assert( nCmdexecopt != OLECMDEXECOPT_DONTPROMPTUSER || pvarargIn == NULL);

    hr = THR( PrivateQueryStatus(&cmd, NULL) );
    if (hr)
        goto Cleanup;

    if (cmd.cmdf == MSOCMDSTATE_DISABLED)
        return E_FAIL;

    hr = GetEditor()->GetUnkDoc()->QueryInterface( IID_IDispatch, (void**) &pDisp );
    if (hr)                                                       
        goto Cleanup;

    VariantInit(&varDoc);
    V_VT(&varDoc) = VT_DISPATCH;
    V_DISPATCH(&varDoc) = pDisp;

    hr = THR( GetEditor()->GetViewServices()->GetViewHWND( &myHWND ) );
    if( hr )
        goto Cleanup;

    //
    // Note that cmdId's for dialogs have been negated.
    // Here we're un-negating them so that ShowEditDialog can find the proper resource.
    //
    hr = THR( ShowEditDialog( ~_cmdId, &varDoc, myHWND, &varReturn, GetEditor()->GetMarkupServices() ));
    
Cleanup:
    ReleaseInterface(pDisp);
    RRETURN ( hr );
}


//+---------------------------------------------------------------------------
//
//  CDialogCommand::QueryStatus
//
//----------------------------------------------------------------------------

HRESULT 
CDialogCommand::PrivateQueryStatus( 
	OLECMD rgCmds[],
    OLECMDTEXT * pcmdtext )
{
    HRESULT         hr;
    SP_ISegmentList spSegmentList;
    INT             iSegmentCount;
    SELECTION_TYPE  eSelectionType;

    rgCmds->cmdf = MSOCMDSTATE_UP; // up by default
    
    //
    // If hyperlink, allow on a control.  Otherwise, return disabled for a control.
    //
    
    if ((~_cmdId) == IDM_HYPERLINK)
        return S_OK;

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );

    if (eSelectionType == SELECTION_TYPE_Control)    
    {
        if ( (~_cmdId) != IDM_IMAGE )
        {
            rgCmds->cmdf = MSOCMDSTATE_DISABLED;
        }
        else
        {
            ELEMENT_TAG_ID  eTag;
            SP_IHTMLElement spElement;
    
            IFC( GetSegmentElement(spSegmentList, 0, &spElement) );

            if (! spElement)
                goto Cleanup;

            IFC( GetMarkupServices()->GetElementTagId( spElement, & eTag ));
            if ( eTag != TAGID_IMG )
            {
                rgCmds->cmdf = MSOCMDSTATE_DISABLED;
            }
        }
    }

    if (((~_cmdId) == IDM_FONT) && !CanAcceptHTML(spSegmentList))
    {
        rgCmds->cmdf = MSOCMDSTATE_DISABLED;      
    }

Cleanup:    
    RRETURN( hr );
}


//+------------------------------------------------------------------------
//
//  Function:   CreateResourceMoniker
//
//  Synopsis:   Creates a new moniker based off a resource file & rid
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CDialogCommand::CreateResourceMoniker(
    HINSTANCE hInst,
    TCHAR *pchRID,
    IMoniker **ppmk)
{
    HRESULT         hr = S_OK;
    TCHAR           ach[pdlUrlLen];
    HINSTANCE       hinstDll;	//DLL containing the Pluggable UI html for dialog boxes

    IFC( GetEditResourceLibrary (&hinstDll) );

    _tcscpy(ach, _T("res://"));

    if (!GetModuleFileName(
            hinstDll,
            ach + _tcslen(ach),
            pdlUrlLen - _tcslen(ach) - 1))
    {
        hr = GetLastWin32Error();
        goto Cleanup;
   }

#ifdef UNIX
    {
        TCHAR* p = _tcsrchr(ach, _T('/'));
        if (p)
    {
            int iLen = _tcslen(++p);
            memmove(ach + 6, p, sizeof(TCHAR) * iLen);
            ach[6 + iLen] = _T('\0');
    }
    }
#endif

    _tcscat(ach, _T("/"));
    _tcscat(ach, pchRID);

    hr = THR(CreateURLMoniker(NULL, ach, ppmk));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   ShowEditDialog
//
//  Synopsis:   Given an IDM, brings up the corresponding dialog using
//              the C API ShowHTMLDialog()
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

DYNLIB g_dynlibMSHTML = { NULL, NULL, "MSHTML.DLL" };

DYNPROC s_dynprocShowHTMLDialog = { NULL, &g_dynlibMSHTML, "ShowHTMLDialog" };

HRESULT
CDialogCommand::ShowEditDialog(UINT idm, VARIANT * pvarExecArgIn, 
                               HWND hwndParent, VARIANT * pvarArgReturn, 
                               IMarkupServices * pMarkupServices)
{
    HRESULT             hr = S_OK;
    int                 i;
    HINSTANCE           hinst = NULL;
    IMoniker *          pMoniker = NULL;
    SHOWHTMLDIALOGFN *  pfnShowHTMLDialog;
    CUndoUnit           undoUnit( GetEditor() );

    // find resource id string
    for (i = 0; i < ARRAY_SIZE(s_dlgInfo); ++i)
    {
        if (idm == s_dlgInfo[i].idm)
            break;
    }
    Assert(i < ARRAY_SIZE(s_dlgInfo));

    // Load the C API procedure from Mshtml.dll
    hr = THR(LoadProcedure(&s_dynprocShowHTMLDialog));
    if (!OK(hr))
        goto Cleanup;

    hinst = s_dynprocShowHTMLDialog.pdynlib->hinst;
    if (!hinst)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // Make the resource moniker
    hr = THR(CreateResourceMoniker(
            hinst,
            s_dlgInfo[i].szidr,
            &pMoniker));
    if (hr)
        goto Cleanup;

    // Begin the undo unit
    hr = THR( undoUnit.Begin( s_dlgInfo[i].idsUndoText ) );
    if (hr)
        goto Cleanup;

    // bring up the dialog
    pfnShowHTMLDialog = (SHOWHTMLDIALOGFN*)(s_dynprocShowHTMLDialog.pfn);
    hr = (*pfnShowHTMLDialog)(hwndParent, pMoniker, pvarExecArgIn, NULL, pvarArgReturn);
    if (hr)
        goto Cleanup;

Cleanup:
    DeinitDynamicLibraries();
    RRETURN( hr );
}
