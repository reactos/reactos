//+------------------------------------------------------------------------
//
//  File:       InsCmd.cxx
//
//  Contents:   CInsertCommand, CInsertObjectCommand Class implementation
//
//-------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_INSCMD_HXX_
#define _X_INSCMD_HXX_
#include "inscmd.hxx"
#endif

#ifndef _X_HTMLED_HXX_
#define _X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef X_OLEDLG_H_
#define X_OLEDLG_H_
#include <oledlg.h>
#endif

extern HRESULT HtmlStringToSignaturedHGlobal (HGLOBAL * phglobal, const TCHAR * pStr, long cch);

using namespace EdUtil;

MtDefine(CInsertCommand, EditCommand, "CInsertCommand")
MtDefine(CInsertObjectCommand, EditCommand, "CInsertObjectCommand")
MtDefine(CInsertParagraphCommand, EditCommand, "CInsertParagraphCommand")

//
// Forward references
//

HRESULT LoadProcedure(DYNPROC *pdynproc);

void DeinitDynamicLibraries();

int edWsprintf(LPTSTR pstrOut, LPCTSTR pstrFormat, LPCTSTR pstrParam);


//+---------------------------------------------------------------------------
//
//  CInsertCommand Constructor
//
//----------------------------------------------------------------------------

CInsertCommand::CInsertCommand(DWORD cmdId,
                               ELEMENT_TAG_ID etagId,
                               LPTSTR pstrAttribName,
                               LPTSTR pstrAttribValue,  
                                                           CHTMLEditor * pEd )
: CCommand( cmdId, pEd )
{
    _tagId = etagId;

    _bstrAttribName  = pstrAttribName  ? SysAllocString( pstrAttribName  ) : NULL;
    _bstrAttribValue = pstrAttribValue ? SysAllocString( pstrAttribValue ) : NULL;
}


//+---------------------------------------------------------------------------
//
//  CInsertCommand Destructor
//
//----------------------------------------------------------------------------

CInsertCommand::~CInsertCommand()
{
    if (_bstrAttribName)
        SysFreeString( _bstrAttribName );
    
    if (_bstrAttribValue)
        SysFreeString( _bstrAttribValue );
}


//+---------------------------------------------------------------------------
//
//  CInsertCommand::SetAttributeValue
//
//----------------------------------------------------------------------------

void
CInsertCommand::SetAttributeValue(LPTSTR pstrAttribValue)
{
    Assert(pstrAttribValue);

    if (_bstrAttribValue)
        SysFreeString( _bstrAttribValue );
    
    _bstrAttribValue = SysAllocString( pstrAttribValue );
}


//+---------------------------------------------------------------------------
//
//  CInsertCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CInsertCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT          hr = S_OK;
    IHTMLElement   * pElement = NULL;
    int              iSegmentCount, i;
    IMarkupPointer * pStart = NULL;
    IMarkupPointer * pEnd = NULL;
    IMarkupServices * pMarkupServices = GetMarkupServices();
    ISegmentList * pSegmentList = NULL;
    CSpringLoader * psl = GetSpringLoader();
    OLECMD          cmd;
    CUndoUnit       undoUnit(GetEditor());

    IFC( PrivateQueryStatus(&cmd, NULL) );
    if (cmd.cmdf == MSOCMDSTATE_DISABLED)
        return E_FAIL;

    IFC( GetSegmentList( &pSegmentList ));
        
    IFC( pSegmentList->GetSegmentCount( & iSegmentCount, NULL ) );
    IFC( pMarkupServices->CreateMarkupPointer( & pStart ) );
    IFC( pMarkupServices->CreateMarkupPointer( & pEnd ) );

    IFC( undoUnit.Begin(IDS_EDUNDONEWCTRL) );

    for ( i = 0; i < iSegmentCount; i++ )
    {
        BOOL              fResult;
        BOOL              fRepositionSpringLoader = FALSE;
        
        IFC( MovePointersToSegmentHelper(GetViewServices(), pSegmentList, i, &pStart, &pEnd) );

        if (_tagId == TAGID_HR && psl)
            fRepositionSpringLoader = psl->IsSpringLoadedAt(pStart);

        if ( pvarargIn )
        {
            CVariant var;
            
            IFC( VariantChangeTypeSpecial( & var, pvarargIn, VT_BSTR ) );
            
            IFC( ApplyCommandToSegment( pStart, pEnd, V_BSTR( & var ) ) );
        }
        else
        {
            IFC( ApplyCommandToSegment( pStart, pEnd, NULL ) );
        }

        //
        // Collapse the pointers after the insertion point
        //

        IFC( pStart->IsRightOf( pEnd, & fResult ) );

        if ( fResult )
        {
            IFC( pEnd->MoveToPointer( pStart ) );
        }
        else
        {
            IFC( pStart->MoveToPointer( pEnd ) );
        }

        // Reposition springloader after insertion (63304).
        if (fRepositionSpringLoader)
            psl->Reposition(pEnd);

        // BUGBUG: Need to call MoveSegmentToPointers.
    }

Cleanup:
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pElement );
    RRETURN ( hr );
}


//+---------------------------------------------------------------------------
//
//  CInsertCommand::QueryStatus
//
//----------------------------------------------------------------------------

HRESULT
CInsertCommand::PrivateQueryStatus( 
        OLECMD * pCmd,
        OLECMDTEXT * pcmdtext )
{
    HRESULT             hr;
    CSelectionManager   *pSelMan = NULL;
    ELEMENT_TAG_ID      tagId;
    
    IFR( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        RRETURN(hr);

    pCmd->cmdf = MSOCMDSTATE_UP;

    // Make sure the edit context isn't a button
    if (GetEditor())
    {
        pSelMan = GetEditor()->GetSelectionManager();
        if (pSelMan && pSelMan->IsEditContextSet() && pSelMan->GetEditableElement())
        {
            IFR( GetMarkupServices()->GetElementTagId(pSelMan->GetEditableElement(), &tagId) );
            if (tagId == TAGID_INPUT || tagId == TAGID_BUTTON)
            {
                pCmd->cmdf = MSOCMDSTATE_DISABLED;                
            }
        }
    }
    
        
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Function:   MapObjectToIntrinsicControl
//
//  Synopsis:   Find a match for pClsId within s_aryIntrinsicsClsid[]
//              If a match is found the IDM command corresponding to 
//              the matched intrinsic control is returned in pdwCmdId
//              otherwise 0 is returned.
//
//-------------------------------------------------------------------------

void
CInsertObjectCommand::MapObjectToIntrinsicControl (CLSID * pClsId, DWORD * pdwCmdId)
{
    int i;

    *pdwCmdId = 0;

    for (i = 0; i < ARRAY_SIZE(s_aryIntrinsicsClsid); i++)
    {
        if ( pClsId->Data1 != s_aryIntrinsicsClsid[i].pClsId->Data1 )
            continue;

        if( IsEqualCLSID( *pClsId, *s_aryIntrinsicsClsid[i].pClsId ) )
        {
            // Match is made, congratulations!
            *pdwCmdId = s_aryIntrinsicsClsid[i].CmdId;
            break;
        }
    }
}

//+------------------------------------------------------------------------
//
//  Function:   SetAttributeFromClsid
//
//  Synopsis:   Sets the attribute value for <OBJECT CLASSID=...> using the 
//              class id.
//
//-------------------------------------------------------------------------
HRESULT
CInsertObjectCommand::SetAttributeFromClsid (CLSID * pClsid )
{   
    HRESULT     hr = S_OK;
    TCHAR       pstrClsid[40];
    int         cch;
    TCHAR       pstrParam[ 128 ];

    //
    // Get the string value of pClsid
    //
    cch = StringFromGUID2( *pClsid, pstrClsid, ARRAY_SIZE(pstrClsid) );
    if (!cch)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Construct the correct syntax for CLASSID attribute value
    //
    cch = edWsprintf(pstrParam, _T("clsid%s"), pstrClsid);
    
    if ( (_T('{') != pstrParam[5+0]) || (_T('}') != pstrParam[5+37]) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }   
    pstrParam[5+0 ] = _T(':');
    pstrParam[5+37] = 0;

    //
    // Set _varAttribValue 
    //

    SetAttributeValue( pstrParam );

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------
// Function:    OleUIMetafilePictIconFree
//
// Synopsis:    Deletes the metafile contained in a METAFILEPICT structure and
//              frees the memory for the structure itself.
//
//
//--------------------------------------------------------------------

void
CInsertObjectCommand::OleUIMetafilePictIconFree( HGLOBAL hMetaPict )
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
//  Function:   HandleInsertObjectDialog
//
//  Synopsis:   Executes UI for Insert Object
//
//  Arguments   [in]  hwnd          hwnd passed from Trident
//              [out] dwResult      result of user action specified in UI
//              [out] pstrResult    classid, file name, etc., depending
//                                  on dwResult flags
//
//  Returns:    S_OK                OK hit in UI, pstrResult set
//              S_FALSE             CANCEL hit in UI, NULL == *pstrResult
//              other               failure
//
//-----------------------------------------------------------------------------

DYNLIB g_dynlibOLEDLG = { NULL, NULL, "OLEDLG.DLL" };

DYNPROC s_dynprocOleUIInsertObjectA =
         { NULL, &g_dynlibOLEDLG, "OleUIInsertObjectA" };

HRESULT
CInsertObjectCommand::HandleInsertObjectDialog (HWND hwnd, DWORD * pdwResult, DWORD * pdwIntrinsicCmdId)
{
    HRESULT                 hr = S_OK;
    OLEUIINSERTOBJECTA      ouio;
    CHAR                    szFile[MAX_PATH] = "";
    UINT                    uRC;

    *pdwIntrinsicCmdId = 0;
    *pdwResult = 0;

    //
    // Initialize ouio
    //
    memset(&ouio, 0, sizeof(ouio));
    ouio.cbStruct = sizeof(ouio);
    ouio.dwFlags =
            IOF_DISABLELINK |    
            IOF_SELECTCREATENEW |
            IOF_DISABLEDISPLAYASICON |
            IOF_HIDECHANGEICON |
            IOF_VERIFYSERVERSEXIST |
            IOF_SHOWINSERTCONTROL;
    ouio.hWndOwner = hwnd;
    ouio.lpszFile = szFile;
    ouio.cchFile = ARRAY_SIZE(szFile);

    //
    // Bring up the OLE Insert Object Dialog
    //
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

    //
    // Process what the user wanted
    //
    Assert((ouio.dwFlags & IOF_SELECTCREATENEW) ||
           (ouio.dwFlags & IOF_SELECTCREATEFROMFILE) ||
           (ouio.dwFlags & IOF_SELECTCREATECONTROL));

    *pdwResult = ouio.dwFlags;

    if ( *pdwResult & IOF_SELECTCREATENEW )
    {
        //
        // For create new object, set the CLASSID=... attribute
        //
        SetAttributeFromClsid ( & ouio.clsid );
    }
    else if ( *pdwResult & IOF_SELECTCREATECONTROL )
    {
        //
        // For create control, first check to see whether the selected
        // control maps to an HTML intrinsic control
        // If there is a match PrivateExec will fire the appropriate IDM
        // insert command based on pdwInstrinsicCmdId, otherwise an 
        // <OBJECT> tag will be inserted.
        //
        MapObjectToIntrinsicControl( & ouio.clsid, pdwIntrinsicCmdId );
        if (! *pdwIntrinsicCmdId )
        {
            SetAttributeFromClsid ( & ouio.clsid );
        }
    }
    // BUGBUG: IOF_SELECTCREATEFROMFILE is not supported

Cleanup:
    if (ouio.hMetaPict)
        OleUIMetafilePictIconFree(ouio.hMetaPict);

    DeinitDynamicLibraries();

    RRETURN1 (hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  CInsertObjectCommand Exec
//
//----------------------------------------------------------------------------

HRESULT 
CInsertObjectCommand::PrivateExec( DWORD nCmdexecopt,
                                   VARIANTARG * pvarargIn,
                                   VARIANTARG * pvarargOut )
{
    HRESULT     hr;
    HWND        hwndParent;
    DWORD       dwResult;
    DWORD       dwIntrinsicCmdId;
    CCommand *  theCommand = NULL;
    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    ISegmentList* pSegmentList = NULL;
    
    Assert( nCmdexecopt != OLECMDEXECOPT_DONTPROMPTUSER );

    IFC( GetMarkupServices()->CreateMarkupPointer( &pStart ));
    IFC( GetMarkupServices()->CreateMarkupPointer( &pEnd ));
    IFC( GetSegmentList( &pSegmentList ));
    IFC( MovePointersToSegmentHelper(GetViewServices(), pSegmentList, 0, &pStart, &pEnd)); // BUGBUG multiple selection
    IFC( pStart->SetGravity( POINTER_GRAVITY_Left ));
    IFC( pEnd->SetGravity( POINTER_GRAVITY_Right ));
    
    //
    // Get the parent handle
    //
    hr = THR( GetEditor()->GetViewServices()->GetViewHWND( &hwndParent ) );
    if( hr )
        goto Cleanup;

    //
    // Bring up the dialog and handle user selections
    //
    hr = THR( HandleInsertObjectDialog( hwndParent, &dwResult, & dwIntrinsicCmdId ) );
    if (hr != S_OK)
    {
        // hr can be S_FALSE, indicating cancelled dialog
        hr = S_OK;
        goto Cleanup;
    }

    //
    // Either insert a new <OBJECT> tag or instantiate an intrinsic
    // control based on the results from HandleInsertObjectDialog()
    // s_aryIntrinsicsClsid[] table is used to map Forms3 controls
    // to their corresponding HTML tags, denoted by the CmdId field. 
    // This table enables us to handle the scenario where user picks
    // a Forms3 control using the Insert Object Dialog. In this case
    // rather than inserting an <OBJECT> with the specified class id,
    // we instantiate the corresponding HTML tag.
    //

    if (dwResult & (IOF_SELECTCREATENEW | IOF_SELECTCREATECONTROL) )
    {
        if (! dwIntrinsicCmdId)
        {
            // 
            // Delegate to super class to insert an <OBJECT> tag
            //
            hr = CInsertCommand::PrivateExec( nCmdexecopt, pvarargIn, pvarargOut );
        }
        else   
        {
            //
            // Delegate to the insertcommand denoted by dwIntrinsicCmdId
            // to insert an intrinsic control
            //
            theCommand = GetEditor()->GetCommandTable()->Get( dwIntrinsicCmdId );

            if ( theCommand )
            {
                hr = theCommand->Exec( nCmdexecopt, pvarargIn, pvarargOut, _pcmdtgt );
            }
            else
            {
                hr = OLECMDERR_E_NOTSUPPORTED;
            }

        }
    }
    else if (dwResult & IOF_SELECTCREATEFROMFILE)
    {
        // BUGBUG (alexz): when async download from file for <OBJECT> tag is implemented,
        // this can be done such that html like <OBJECT SRC = "[pstrResult]"> </OBJECT>
        // is used to create the object.
        // Currently the feature left disabled.
        hr = OLECMDERR_E_NOTSUPPORTED;
        goto Cleanup;
    }

    if ( FAILED(hr) )
        goto Cleanup;

    //
    // Site Select the inserted object
    //
    if ( hr == S_OK )
    {
        DWORD code = 0;
        GetEditor()->Select( pStart, pEnd, SELECTION_TYPE_Control, &code );
        Assert(code == 0 );
    }
    
Cleanup:
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    RRETURN(hr);
}
//+---------------------------------------------------------------------------
//
//  CInsertCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CInsertCommand::ApplyCommandToSegment( IMarkupPointer * pStart,
                                       IMarkupPointer * pEnd,
                                       TCHAR *          pchVar,
                                       BOOL             fRemove /* = TRUE */ )
{
    HRESULT           hr = S_OK;
    CStr              strAttributes;
    IMarkupServices * pMarkupServices = GetMarkupServices();
    IHTMLElement *    pIHTMLElement = NULL;

    Assert( pStart );
    
    if (pchVar && *pchVar)
    {
        IFC( strAttributes.Append( _T(" ") ) );
        IFC( strAttributes.Append( _cmdId == IDM_IMAGE ? _T("src") : _T("id") ) );
        IFC( strAttributes.Append( _T("=") ) );
        IFC( strAttributes.Append( _cmdId == IDM_IMAGE ? _T("\"") : _T("") ) );
        IFC( strAttributes.Append( pchVar ) );
        IFC( strAttributes.Append( _cmdId == IDM_IMAGE ? _T("\"") : _T("") ) );
    }

    if (_bstrAttribName && _bstrAttribValue)
    {
        IFC( strAttributes.Append( _T(" ") ) );
        IFC( strAttributes.Append( _bstrAttribName ) );
        IFC( strAttributes.Append( _T("=") ) );
        IFC( strAttributes.Append( _bstrAttribValue ) );
    }

    if (_cmdId == IDM_INSINPUTSUBMIT)
    {
        IFC( strAttributes.Append( _T(" ") ) );
        IFC( strAttributes.Append( _T("value='Submit Query'") ) );
    }

    if (fRemove)
    {
        IFC( pMarkupServices->Remove( pStart, pEnd ) );
    }

    IFC( ClingToText( pEnd, LEFT, pStart ) );
    IFC( ClingToText( pStart, RIGHT, pEnd ) );

    if (_cmdId == IDM_NONBREAK)
    {
        OLECHAR ch = WCH_NBSP;
        IFC( GetViewServices()->InsertMaximumText( &ch, 1, pEnd ) );
    }
    else
    {
        IFC( pMarkupServices->CreateElement( _tagId, strAttributes, & pIHTMLElement ) );
        IFC( InsertElement( pMarkupServices, pIHTMLElement, pStart, pEnd ) );
    }

    if (_tagId == TAGID_BR)
    {
        SP_ISegmentList spSegmentList;
        INT             iSegmentCount;
        SELECTION_TYPE  eSelectionType;
        
        // Make sure caret fNotAtBOL and fAtLogicalBOL are set correctly
        IFR( GetSegmentList(&spSegmentList) );
        if (spSegmentList != NULL)
        {
            IFR( spSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );
            if (eSelectionType == SELECTION_TYPE_Caret)
            {
                CEditTracker *pEditTracker = GetEditor()->GetSelectionManager()->GetActiveTracker();

                if (pEditTracker)
                {
                    IFR( pEditTracker->SetNotAtBOL(FALSE) );
                    IFR( pEditTracker->SetAtLogicalBOL(TRUE) );
                }
            }
        }
    }
    else if ( GetEditor()->IsElementSiteSelectable( pIHTMLElement) == S_OK &&
              GetEditor()->IsContextEditable() && 
              _cmdId != IDM_HORIZONTALLINE ) // don't do this for HR's to make OE happy.
    {
        //
        // Site Select the inserted object
        //
        DWORD code = 0;
        IFC( pStart->MoveAdjacentToElement( pIHTMLElement , ELEM_ADJ_BeforeBegin ));
        IFC( pEnd->MoveAdjacentToElement( pIHTMLElement , ELEM_ADJ_AfterEnd ));
        IFC( GetEditor()->Select( pStart, pEnd, SELECTION_TYPE_Control, &code ));
        Assert(code == 0 );   
    }

Cleanup:

    ClearInterface( & pIHTMLElement );

    RRETURN( hr );
}

BOOL 
CInsertCommand::IsValidOnControl()
{
    ELEMENT_TAG_ID  eTag;
    SP_IHTMLElement spElement;
    BOOL            fValid = FALSE;
    HRESULT         hr;
    SP_ISegmentList spSegmentList;
    INT             iSegmentCount;
    SELECTION_TYPE  eSelectionType;

    IFC( GetSegmentList( &spSegmentList ));        
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, & eSelectionType ) );

    if (eSelectionType != SELECTION_TYPE_Control || iSegmentCount == 0)
        goto Cleanup;

    IFC( GetSegmentElement(spSegmentList, 0, &spElement) );

    if (! spElement)
        goto Cleanup;

    IFC( GetMarkupServices()->GetElementTagId( spElement, & eTag ));
    fValid = ( eTag == TAGID_IMG && _cmdId == IDM_IMAGE );

Cleanup:
    return fValid;
}


//+---------------------------------------------------------------------------
//
//  CInsertParagraphCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CInsertParagraphCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT             hr;
    CUndoUnit           undoUnit(GetEditor());
    SP_IMarkupPointer   spStart, spEnd;
    SP_ISegmentList     spSegmentList;
    INT                 i;
    INT                 iSegmentCount;
    CStr                strPara;
    OLECMD              cmd;  
    HGLOBAL             hGlobal = 0;

    IFR( PrivateQueryStatus(&cmd, NULL) );
    if (cmd.cmdf == MSOCMDSTATE_DISABLED)
        return E_FAIL;

    IFR( GetSegmentList( &spSegmentList ));        
    IFR( spSegmentList->GetSegmentCount(&iSegmentCount, NULL ) );
    
    IFR( undoUnit.Begin(IDS_EDUNDONEWCTRL) );

    IFR( GetMarkupServices()->CreateMarkupPointer(&spStart) );
    IFR( GetMarkupServices()->CreateMarkupPointer(&spEnd) );
    for ( i = 0; i < iSegmentCount; i++ )    {
        
        IFR( MovePointersToSegmentHelper(GetViewServices(), spSegmentList, i, &(spStart.p), &(spEnd.p)) );

        if ( pvarargIn )
        {
            CVariant var;
            
            IFR( VariantChangeTypeSpecial(&var, pvarargIn, VT_BSTR) );
            IFR( strPara.Set(_T("<P id=\"")) );
            IFR( strPara.Append(V_BSTR(&var)) );
            IFR( strPara.Append(_T("\"></P>")) );
        }
        else
        {
            IFR( strPara.Set(_T("<P></P>")) );
        }
        
        IFR( HtmlStringToSignaturedHGlobal(&hGlobal, strPara, _tcslen(strPara)) );
        hr = THR( GetViewServices()->DoTheDarnPasteHTML(spStart, spEnd, hGlobal) ); 
        GlobalFree(hGlobal);
        IFR(hr);
    }

    return S_OK;
}

