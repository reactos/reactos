//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       htmled.cxx
//
//  Contents:   Implementation of IHTMLEditor interface inisde of mshtmled.dll
//
//  Classes:    CHTMLEditor
//
//  History:    07-21-98 - johnbed - created
//-------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_CHARCMD_HXX_
#define _X_CHARCMD_HXX_
#include "charcmd.hxx"
#endif

#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif

#ifndef _X_PASTECMD_HXX_
#define _X_PASTECMD_HXX_
#include "pastecmd.hxx"
#endif

#ifndef _X_COPYCMD_HXX_
#define _X_COPYCMD_HXX_
#include "copycmd.hxx"
#endif

#ifndef _X_CUTCMD_HXX_
#define _X_CUTCMD_HXX_
#include "cutcmd.hxx"
#endif

#ifndef _X_DLGCMD_HXX_
#define _X_DLGCMD_HXX_
#include "dlgcmd.hxx"
#endif

#ifndef _X_INSCMD_HXX_
#define _X_INSCMD_HXX_
#include "inscmd.hxx"
#endif

#ifndef _X_SELCMD_HXX_
#define _X_SELCMD_HXX_
#include "selcmd.hxx"
#endif

#ifndef _X_MISCCMD_HXX_
#define _X_MISCCMD_HXX_
#include "misccmd.hxx"
#endif

#ifndef X_EDTRACK_HXX_
#define X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_DIMM_HXX_
#define X_DIMM_HXX_
#include "dimm.h"
#endif

#define SID_SHTMLEditHost IID_IHTMLDragEditHost

using namespace EdUtil;

class CSpringLoader;

MtDefine(CHTMLEditor, Utilities, "HTML Editor")
MtDefine(CHTMLEditor_pComposeSettings, CHTMLEditor, "CHTMLEditor::_pComposeSettings")

DeclareTag(tagEditingTrackExecFailures, "Edit", "Track exec failures")

#ifndef NO_IME
extern CRITICAL_SECTION g_csActiveIMM ; 
extern int g_cRefActiveIMM ;  
extern IActiveIMMApp * g_pActiveIMM;
#endif

HRESULT
OldCompare( IMarkupPointer * p1, IMarkupPointer * p2, int * pi )
{
    HRESULT hr = S_OK;
    BOOL    fResult;
    
    hr = THR_NOTRACE( p1->IsEqualTo( p2, & fResult ) );
    if ( FAILED( hr ) )
        goto Cleanup;
        
    if (fResult)
    {
        *pi = 0;
        goto Cleanup;
    }

    hr = THR_NOTRACE( p1->IsLeftOf( p2, & fResult ) );
    if ( FAILED( hr ) )
        goto Cleanup;
        
    *pi = fResult ? -1 : 1;

Cleanup:

    RRETURN( hr );
}

//////////////////////////////////////////////////////////////////////////////////
//    CHTMLEditor Constructor, Destructor and Initialization
//////////////////////////////////////////////////////////////////////////////////

CHTMLEditor::CHTMLEditor()
{
    Assert( _pDoc == NULL );
    Assert( _pUnkDoc == NULL );
    Assert( _pUnkView == NULL );
    Assert( _pMarkupServices == NULL );
    Assert( _pViewServices == NULL );
    Assert( _pComposeSettings == NULL);
    Assert( _pSelMan == NULL );
    Assert( _pCommandTarget == NULL );
    Assert( _fGotActiveIMM == FALSE ) ;
    Assert( _pStringCache == NULL );    
}

CHTMLEditor::~CHTMLEditor()
{
    DeleteCommandTable();

    delete _pSelMan;
    delete _pCommandTarget;
    delete _pComposeSettings;
    delete _pStringCache;

    ReleaseInterface( _pDoc );
    _pDoc = NULL;

    ReleaseInterface( _pMarkupServices );
    _pMarkupServices = NULL;

    ReleaseInterface( _pViewServices );
    _pViewServices = NULL;

#if DBG == 1
    ReleaseInterface( _pEditDebugServices );
    _pEditDebugServices = NULL;
#endif
    if (g_hEditLibInstance)
    {
        FreeLibrary(g_hEditLibInstance);
        g_hEditLibInstance = NULL;
    }

#ifndef NO_IME
    if (_fGotActiveIMM)
    {
        EnterCriticalSection(&g_csActiveIMM);
        Assert(g_cRefActiveIMM > 0);
        Assert(g_pActiveIMM != NULL);
        if (--g_cRefActiveIMM == 0)
            ClearInterface(&g_pActiveIMM);
        LeaveCriticalSection(&g_csActiveIMM);
    }
#endif
}

//+========================================================================
//
// Method: Initialize
//
// Synopsis: Set the Document's interfaces for this instance of HTMLEditor
//           Called from Formkrnl when it CoCreates us.
//
//           We are actually passed a COmDocument, which uses weak ref's for
//           the interfaces we are QIing for. If we need to add an interface
//           in the future, make sure you add it to COmDocument (omdoc.cxx)
//           QueryInterface
//
//-------------------------------------------------------------------------

HRESULT
CHTMLEditor::Initialize( IUnknown * pUnkDoc, IUnknown * pUnkView )
{
    HRESULT hr;
    Assert( pUnkDoc );
    Assert( pUnkView );
    _pUnkDoc = pUnkDoc;
    _pUnkView = pUnkView;
    IServiceProvider * pDocServiceProvider = NULL;
    
    IFC(_pUnkDoc->QueryInterface( IID_IHTMLDocument2 , (void **) &_pDoc ));
    IFC( _pUnkDoc->QueryInterface( IID_IMarkupServices , (void **) &_pMarkupServices ));
    IFC( _pUnkView->QueryInterface( IID_IHTMLViewServices , (void **) &_pViewServices ));

#if DBG == 1
    //
    // We don't fail here. It can happen that a Non-Debug Mshtml calls into a debug mshtmled.
    // In which case failure is bad. Instead - every debug method - just checks we have a non-null
    // _pEditDebugServices.
    //
    IGNORE_HR( _pUnkDoc->QueryInterface( IID_IEditDebugServices, ( void**) & _pEditDebugServices));
#endif

    IFC( InitCommandTable() );

    // Create the selection manager
    _pSelMan = new CSelectionManager( this );
    if( _pSelMan == NULL )
        goto Error;

    _pCommandTarget = new CMshtmlEd( this );
    if( _pCommandTarget == NULL )
        goto Error;

    _pStringCache = new CStringCache(IDS_CACHE_BEGIN, IDS_CACHE_END);
    if( _pStringCache == NULL )
        goto Error;

    // Need to delay loading of the string table because we can't LoadLibrary in process attach.
    // This is not delayed to the command level because checking if the table is already loaded
    // is expensive.
    CGetBlockFmtCommand::LoadStringTable(g_hInstance);    

#ifndef NO_IME
    EnterCriticalSection(&g_csActiveIMM);
    if (g_pActiveIMM == NULL)
    {
        IGNORE_HR( GetViewServices()->GetActiveIMM(&g_pActiveIMM) );
    } 

    if (g_pActiveIMM != NULL)
    {
        _fGotActiveIMM = TRUE;
        g_cRefActiveIMM += 1;
    }
    LeaveCriticalSection(&g_csActiveIMM);     
#endif

    //
    // Set the Edit Host - if there is one.
    //
#ifdef EDIT_DRAG_HOST    
    IFC( _pDoc->QueryInterface(IID_IServiceProvider, (void**) & pDocServiceProvider ));
    IGNORE_HR( pDocServiceProvider->QueryService( SID_SHTMLEditHost, IID_IHTMLDragEditHost, ( void** ) & _pIHTMLDragEditHost ));
#endif


Cleanup:
    ReleaseInterface( pDocServiceProvider );
    return hr;

Error:
    return( E_OUTOFMEMORY );
    
}


//----------------------------------------------------------
//
//  Initialize the CommandTable
//
//
//     *** NOTE: Insert frequently used commands first ***
//     This will keep them closer to the top of the tree and make lookups 
//     faster
//   
//==========================================================

// TODO (johnbed) if memory/perf overhead of having this per instance is too great,
//                  move this to a static table and call in process attach. All static
//                  tables should be moved here since these are globally accessable.
HRESULT
CHTMLEditor::InitCommandTable()
{
    CCommand * pCmd = NULL;
    COutdentCommand * pOutdentCmd = NULL;

    _pCommandTable = new CCommandTable(60);
    if( _pCommandTable == NULL )
        goto Error;

    //+----------------------------------------------------
    // CHAR FORMAT COMMANDS 
    //+----------------------------------------------------

    pCmd = new CCharCommand( IDM_BOLD, TAGID_STRONG, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CCharCommand( IDM_ITALIC, TAGID_EM, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CCharCommand( IDM_STRIKETHROUGH, TAGID_STRIKE, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CCharCommand( IDM_SUBSCRIPT, TAGID_SUB, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );
    
    pCmd = new  CCharCommand( IDM_SUPERSCRIPT, TAGID_SUP, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CCharCommand( IDM_UNDERLINE, TAGID_U, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CFontSizeCommand( IDM_FONTSIZE, TAGID_FONT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CFontNameCommand( IDM_FONTNAME, TAGID_FONT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CForeColorCommand( IDM_FORECOLOR, TAGID_FONT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CBackColorCommand( IDM_BACKCOLOR, TAGID_FONT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CAnchorCommand( IDM_HYPERLINK, TAGID_A, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CAnchorCommand( IDM_BOOKMARK, TAGID_A, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CRemoveFormatCommand( IDM_REMOVEFORMAT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );
    
    pCmd = new CUnlinkCommand( IDM_UNLINK, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );
    
    pCmd = new CUnlinkCommand( IDM_UNBOOKMARK, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // BLOCK FORMAT COMMANDS 
    //+----------------------------------------------------

    pCmd = new  CIndentCommand( IDM_INDENT, TAGID_BLOCKQUOTE , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pOutdentCmd = new  COutdentCommand( IDM_OUTDENT, TAGID_BLOCKQUOTE , this );
    if( pOutdentCmd == NULL )
        goto Error;
    _pCommandTable->Add( (CCommand*) pOutdentCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYCENTER, TAGID_CENTER, _T("center"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYLEFT, TAGID_NULL, _T("left"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYGENERAL, TAGID_NULL, _T("left"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYRIGHT, TAGID_NULL, _T("right"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYNONE, TAGID_NULL, _T(""), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYFULL, TAGID_NULL, _T("justify"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CGetBlockFmtCommand( IDM_GETBLOCKFMTS, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CLocalizeEditorCommand( IDM_LOCALIZEEDITOR, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CListCommand( IDM_ORDERLIST, TAGID_OL, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CListCommand( IDM_UNORDERLIST, TAGID_UL, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CBlockFmtCommand( IDM_BLOCKFMT , TAGID_NULL, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CBlockDirCommand( IDM_BLOCKDIRLTR, TAGID_NULL, _T("ltr"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CBlockDirCommand( IDM_BLOCKDIRRTL, TAGID_NULL, _T("rtl"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // DIALOG COMMANDS 
    //+----------------------------------------------------

    // BUGBUG - these UINT casts/inversion needs to change

    pCmd = new CDialogCommand( UINT(~IDM_HYPERLINK) , this ); 
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CDialogCommand( UINT(~IDM_IMAGE) , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CDialogCommand( UINT(~IDM_FONT) , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );


    //+----------------------------------------------------
    // INSERT COMMANDS 
    //+----------------------------------------------------

    pCmd = new CInsertCommand( IDM_BUTTON, TAGID_BUTTON, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_TEXTBOX, TAGID_INPUT, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_TEXTAREA, TAGID_TEXTAREA, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_MARQUEE, TAGID_MARQUEE, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_HORIZONTALLINE, TAGID_HR, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_IFRAME, TAGID_IFRAME, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSFIELDSET, TAGID_FIELDSET, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertParagraphCommand( IDM_PARAGRAPH, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_IMAGE, TAGID_IMG, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_DROPDOWNBOX, TAGID_SELECT, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );
    
    pCmd = new CInsertCommand( IDM_LISTBOX, TAGID_SELECT,  _T("MULTIPLE"), _T("TRUE") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_1D, TAGID_DIV, _T("POSITION:RELATIVE"), NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_CHECKBOX, TAGID_INPUT, _T("type"), _T("checkbox") , this );   
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_RADIOBUTTON, TAGID_INPUT, _T("type"), _T("radio") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTBUTTON, TAGID_INPUT, _T("type"), _T("button") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTRESET, TAGID_INPUT, _T("type"), _T("reset") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTSUBMIT, TAGID_INPUT, _T("type"), _T("submit") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTUPLOAD, TAGID_INPUT, _T("type"), _T("file") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTHIDDEN, TAGID_INPUT, _T("type"), _T("hidden") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTPASSWORD, TAGID_INPUT, _T("type"), _T("password") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTIMAGE, TAGID_INPUT, _T("type"), _T("image") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSERTSPAN, TAGID_SPAN, _T("class"), _T("") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_LINEBREAKLEFT, TAGID_BR, _T("clear"), _T("left"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_LINEBREAKRIGHT, TAGID_BR, _T("clear"), _T("right"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_LINEBREAKBOTH, TAGID_BR, _T("clear"), _T("all"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_LINEBREAKNORMAL, TAGID_BR, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_NONBREAK, TAGID_NULL, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );
    
    //+----------------------------------------------------
    // INSERTOBJECT COMMAND
    //+----------------------------------------------------
    
    pCmd = new CInsertObjectCommand( IDM_INSERTOBJECT, TAGID_OBJECT, _T( "CLASSID" ), NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // SELECTION COMMANDS 
    //+----------------------------------------------------

    pCmd = new CSelectAllCommand( IDM_SELECTALL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CClearSelectionCommand( IDM_CLEARSELECTION , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // DELETE COMMAND
    //+----------------------------------------------------

    pCmd = new CDeleteCommand( IDM_DELETE , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CDeleteCommand( IDM_DELETEWORD , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // CUT COMMAND
    //+----------------------------------------------------

    pCmd = new CCutCommand( IDM_CUT , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );


    //+----------------------------------------------------
    // COPY COMMAND
    //+----------------------------------------------------

    pCmd = new CCopyCommand( IDM_COPY , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );


    //+----------------------------------------------------
    // PASTE COMMAND
    //+----------------------------------------------------
    
    pCmd = new CPasteCommand( IDM_PASTE , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );


    //+----------------------------------------------------
    // MISCELLANEOUS OTHER COMMANDS 
    //+----------------------------------------------------

    pCmd = new CAutoDetectCommand( IDM_AUTODETECT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new COverwriteCommand( IDM_OVERWRITE, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CComposeSettingsCommand( IDM_COMPOSESETTINGS, this );
    if (pCmd == NULL)
        goto Error;
    _pCommandTable->Add( pCmd );

    return( S_OK );
    
Error:
    return( E_OUTOFMEMORY );
}


HRESULT
CHTMLEditor::DeleteCommandTable()
{
    delete _pCommandTable;
    return(S_OK);
}


//////////////////////////////////////////////////////////////////////////////////
//    CHTMLEditor IHTMLEditor Implementation
//////////////////////////////////////////////////////////////////////////////////

HRESULT
CHTMLEditor::HandleMessage( 
    SelectionMessage* pSelectionMessage ,
    DWORD* pdwFollowUpAction )
{
    if(!_pSelMan)
        return E_FAIL;

    HRESULT hr = THR( _pSelMan->HandleMessage( pSelectionMessage, pdwFollowUpAction ) );
    return( hr );
}
    
HRESULT
CHTMLEditor::SetEditContext(
            BOOL fEditable ,
            BOOL fSetSelection,
            BOOL fParentEditable,
            IMarkupPointer* pStartPointer,
            IMarkupPointer* pEndPointer,
            BOOL fNoScope ) 
{
    HRESULT hr ;
    
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( _pSelMan->SetEditContext( fEditable, fSetSelection, fParentEditable, pStartPointer, pEndPointer, fNoScope  ));

Cleanup:
    return( hr );
}
    

HRESULT
CHTMLEditor::GetSelectionType(
    SELECTION_TYPE * eSelectionType )
{
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        return( E_FAIL) ;
    }
    
    RRETURN( _pSelMan->GetSelectionType( eSelectionType ) );
}
    

HRESULT
CHTMLEditor::Notify(
    SELECTION_NOTIFICATION eSelectionNotification,
    IUnknown *pUnknown, 
    DWORD* pdwFollowUpActionFlag,
    DWORD dword )
{
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        return ( E_FAIL );
    }
    return( THR( _pSelMan->Notify( eSelectionNotification, pUnknown, pdwFollowUpActionFlag, dword ) ));
}
    

HRESULT
CHTMLEditor::GetRangeCommandTarget(
    IUnknown *  pContext,
    IUnknown ** ppUnkTarget )
{
    // TODO: (johnbed) smarten this up so we just maintain a pool of available
    // command targets instead of create/destroy.
    HRESULT hr = S_OK;
    IUnknown * punk = NULL;
    CMshtmlEd * pt = new CMshtmlEd( this );

    if( pt == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    hr = THR( pt->Initialize( pContext ));
    if( hr )
        goto Cleanup;
        
    hr = THR( pt->QueryInterface( IID_IUnknown , (void **) &punk ));
    if( hr )
        goto Cleanup;
        
    *ppUnkTarget = punk;

Cleanup:

    return( hr );
}    

HRESULT
CHTMLEditor::GetElementToTabFrom(
            BOOL            fForward,
            IHTMLElement**  ppElement,
            BOOL *          pfFindNext)
{
    HRESULT hr = S_OK;
    SELECTION_TYPE eSelType;
    IHTMLElement * pElement = NULL;
    ISelectionRenderingServices * psrs = NULL;
            
    Assert( ppElement );
    if( ppElement == NULL )
        goto Cleanup;

    Assert(pfFindNext);
    if (!pfFindNext)
        goto Cleanup;

    *ppElement = NULL;
    *pfFindNext = TRUE;
    eSelType = _pSelMan->GetSelectionType();

    if( eSelType == SELECTION_TYPE_Control )
    {
        CEditTracker * pTracker = _pSelMan->GetActiveTracker();
        Assert( pTracker );
        CControlTracker * pCtrlTracker = (CControlTracker *) pTracker;
        pElement = pCtrlTracker->GetControlElement();
        pElement->AddRef();
    }
    else
    {
        CEditPointer epLeft( this );
        CEditPointer epRight( this );
        INT lSegmentCount = 0;

        IFC( _pViewServices->GetCurrentSelectionRenderingServices( &psrs ) );
        IFC( psrs->GetSegmentCount( &lSegmentCount , &eSelType ));        

        if( lSegmentCount == 0 )
        {
            SP_IHTMLCaret spCaret;
            BOOL fPositioned;
            // There is no selection, try to use the physical caret

            hr = THR( _pViewServices->GetCaret( & spCaret ));
            hr = THR( spCaret->MovePointerToCaret( epRight ));
            hr = THR( epRight->IsPositioned( & fPositioned ));

            if( fPositioned )
            {

            }
            else
            {
                // no caret to fall back on - go use the edit context
                if( fForward )
                {
                    IFC( epRight.MoveToPointer( _pSelMan->GetStartEditContext()));
                }
                else
                {
                    IFC( epRight.MoveToPointer( _pSelMan->GetEndEditContext()));
                }
            }
            
        }
        else
        {
            if (fForward)
            {
                // Move to the last segment added to the list
                IFC( psrs->MovePointersToSegment( lSegmentCount - 1 , epLeft, epRight ));
            }
            else
            {
                // Move to the first segment added to the list. Also, store the left
                // ptr in epRight because epRight is what we use for all further
                // computation.
                IFC( psrs->MovePointersToSegment( 0 , epRight, epLeft ));
            }
        }

        // Find the next element to go to when tabbing from caret(epRight)
        {
            IHTMLElement *          pElementCaret   = NULL;
            MARKUP_CONTEXT_TYPE     context         = CONTEXT_TYPE_None;

            // First find the element that contains caret
            IFC(_pViewServices->CurrentScopeOrSlave(epRight, &pElementCaret));
            if (!pElementCaret)
                goto Cleanup;

            // Now, scan upto the first scope boundary
            for (;;)
            {
                Assert(!pElement);
                if (fForward)
                {
                    IFC(_pViewServices->RightOrSlave(epRight, TRUE, &context, &pElement, NULL, NULL));
                }
                else
                {
                    IFC(_pViewServices->LeftOrSlave(epRight, TRUE, &context, &pElement, NULL, NULL));
                }
                if (    context == CONTEXT_TYPE_EnterScope
                    ||  context == CONTEXT_TYPE_ExitScope
                    ||  context == CONTEXT_TYPE_NoScope
                   )
                {
                    break;
                }
                ClearInterface(&pElement);
                if (context == CONTEXT_TYPE_None)
                    break;
            }
            if (pElement != pElementCaret)
            {
                *pfFindNext = FALSE;
            }
            ReleaseInterface(pElementCaret);
        }
    }
    
Cleanup:

    if( SUCCEEDED(hr) && pElement )
    {
        hr = THR( pElement->QueryInterface( IID_IHTMLElement , (LPVOID *) ppElement ));
    }

    ReleaseInterface( pElement );
    ReleaseInterface( psrs );
    
    RRETURN( hr );
}


//////////////////////////////////////////////////////////////////////////////////
//    CHTMLEditor IHTMLEditingServices Implementation
//////////////////////////////////////////////////////////////////////////////////


HRESULT 
CHTMLEditor::Delete ( 
    IMarkupPointer*     pStart, 
    IMarkupPointer*     pEnd,
    BOOL fAdjustPointers)
{
    CDeleteCommand * pDeleteCommand;

    pDeleteCommand = (CDeleteCommand *) GetCommandTable()->Get( IDM_DELETE );
    Assert( pDeleteCommand );

    RRETURN( THR(pDeleteCommand->Delete( pStart, pEnd, fAdjustPointers )));
}       


HRESULT 
CHTMLEditor::DeleteCharacter (
    IMarkupPointer * pPointer, 
    BOOL fLeftBound, 
    BOOL fWordMode,
    IMarkupPointer * pBoundary )
{
    CDeleteCommand * pDeleteCommand;

    pDeleteCommand = (CDeleteCommand *) GetCommandTable()->Get( IDM_DELETE );
    Assert( pDeleteCommand );

    RRETURN( THR(pDeleteCommand->DeleteCharacter( pPointer, fLeftBound, fWordMode, pBoundary )));
}       


HRESULT 
CHTMLEditor::Paste( 
    IMarkupPointer* pStart, 
    IMarkupPointer* pEnd, 
    BSTR bstrText )
{
    CPasteCommand * pPasteCommand;

    pPasteCommand = (CPasteCommand *) GetCommandTable()->Get( IDM_PASTE );
    Assert( pPasteCommand );

    RRETURN( THR( pPasteCommand->Paste( pStart, pEnd, GetPrimarySpringLoader(), bstrText )));      

}       


HRESULT 
CHTMLEditor::PasteFromClipboard( 
    IMarkupPointer* pStart, 
    IMarkupPointer* pEnd,
    IDataObject* pDO )
{
    CPasteCommand  * pPasteCommand = NULL;

    pPasteCommand = (CPasteCommand *) GetCommandTable()->Get( IDM_PASTE );
    Assert( pPasteCommand );

    RRETURN( THR(pPasteCommand->PasteFromClipboard( pStart, pEnd , pDO, GetPrimarySpringLoader())));    
}       

HRESULT 
CHTMLEditor::Select( 
    IMarkupPointer* pStart, 
    IMarkupPointer* pEnd ,
    SELECTION_TYPE eType,
    DWORD * pdwFollowUpCode )
{
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        return E_FAIL;
    }
    //
    // Stop doing a set edit context range here, assume that its been set in Trident.
    //

    RRETURN ( _pSelMan->Select( pStart, pEnd, eType, pdwFollowUpCode ));
    
}     
  

HRESULT 
CHTMLEditor::IsPointerInSelection( 
    IMarkupPointer* pMarkupPointer, 
    BOOL * pfPointInSelection,
    POINT * pptGlobal,
    IHTMLElement* pIElementOver )
{
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        return( E_FAIL );
    }
    RRETURN ( _pSelMan->IsPointerInSelection( pMarkupPointer, pfPointInSelection, pptGlobal, pIElementOver  ));
}     


HRESULT 
CHTMLEditor::AdjustPointerForInsert ( 
                                    IMarkupPointer * pWhereIThinkIAm , 
                                    BOOL fFurtherInDocument, 
                                    BOOL fNotAtBol ,
                                    IMarkupPointer* pConstraintStart,
                                    IMarkupPointer* pConstraintEnd )
{
    HRESULT hr = S_OK;
    hr = THR( AdjustPointer( pWhereIThinkIAm, fNotAtBol, fFurtherInDocument ? RIGHT : LEFT, fNotAtBol ? LEFT : RIGHT, 
                             ( pConstraintStart == NULL ) ? _pSelMan->GetStartEditContext() : pConstraintStart,
                             ( pConstraintEnd == NULL ) ? _pSelMan->GetEndEditContext() : pConstraintEnd, POSCARETOPT_None ) );
    RRETURN ( hr );
}

HRESULT 
CHTMLEditor::AdjustPointerForInsert ( 
                                    IMarkupPointer * pWhereIThinkIAm , 
                                    BOOL fFurtherInDocument, 
                                    BOOL fNotAtBol ,
                                    IMarkupPointer* pConstraintStart,
                                    IMarkupPointer* pConstraintEnd,
                                    BOOL fStayOutsideUrl)
{
    HRESULT hr = S_OK;
    hr = THR( AdjustPointer( pWhereIThinkIAm, fNotAtBol, fFurtherInDocument ? RIGHT : LEFT, fNotAtBol ? LEFT : RIGHT, 
                             ( pConstraintStart == NULL ) ? _pSelMan->GetStartEditContext() : pConstraintStart,
                             ( pConstraintEnd == NULL ) ? _pSelMan->GetEndEditContext() : pConstraintEnd, fStayOutsideUrl ? POSCARETOPT_None : ADJPTROPT_AdjustIntoURL ));
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: FindSiteSelectableElement
//
// Synopsis: Traverse through a range of pointers and find the site selectable element if any
//
//------------------------------------------------------------------------------------


HRESULT
CHTMLEditor::FindSiteSelectableElement (
                                     IMarkupPointer* pPointerStart,
                                     IMarkupPointer* pPointerEnd, 
                                     IHTMLElement** ppIHTMLElement)  
{
    HRESULT hr = S_OK;
    IHTMLElement* pICurElement = NULL;
    IHTMLElement* pISiteSelectableElement = NULL;
    IMarkupPointer* pPointer = NULL;
    
    int iWherePointer = 0;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
    BOOL fFound = FALSE;
    BOOL fValidForSelection  = FALSE;    
    BOOL fSiteSelectable = FALSE;
    
#if DBG == 1
    IFC( OldCompare( pPointerStart, pPointerEnd, & iWherePointer ));
    Assert( iWherePointer != LEFT );
#endif

    IFC( GetMarkupServices()->CreateMarkupPointer( & pPointer ));
    IFC( pPointer->MoveToPointer( pPointerStart));

    for(;;)
    {
        IFC( pPointer->Right( TRUE, & eContext, & pICurElement, NULL, NULL));
        IFC( OldCompare( pPointer, pPointerEnd, & iWherePointer));

        if ( iWherePointer == LEFT )
        {       
            goto Cleanup;
        }                    
        switch( eContext)
        {
            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_NoScope:
            {
                Assert( pICurElement);

                fSiteSelectable = IsElementSiteSelectable( pICurElement ) == S_OK ;
                if ( fSiteSelectable )
                {
                    if (! fFound )
                    {
                        fValidForSelection = TRUE;
                        IFC( pPointer->MoveAdjacentToElement( pICurElement, ELEM_ADJ_AfterEnd ));
                        ReplaceInterface( & pISiteSelectableElement, pICurElement);
                    }
                    else 
                    {                
                        //
                        // If we find a more than one site selectable elment assume we are a text selection
                        // this assumption breaks for MultipleSelection
                        //
                        fValidForSelection = FALSE;
                        goto Cleanup;
                    }
                }                    
            }
            break;
            
            case CONTEXT_TYPE_Text:
            case CONTEXT_TYPE_None:
            {
                if ( fFound )
                    fValidForSelection = FALSE;
                goto Cleanup;                    
            }

        }
        ClearInterface( & pICurElement );
    }

    
Cleanup:
    ReleaseInterface( pICurElement);
    ReleaseInterface( pPointer );

    if ( hr == S_OK )
    {
        if ( fValidForSelection )
        {
            ReplaceInterface( ppIHTMLElement, pISiteSelectableElement );
        }
        else
            hr = S_FALSE;
    }        
    ReleaseInterface( pISiteSelectableElement );
    return ( hr );
}

//+====================================================================================
//
// Method: IsElementSiteSelectable
//
// Synopsis: Tests to see if a given element is site selectable (Doesn't drill up to the Edit
//           Context. - test is just for THIS Element
//
//------------------------------------------------------------------------------------


HRESULT
CHTMLEditor::IsElementSiteSelectable( IHTMLElement* pIElement )
{
    HRESULT hr = S_OK;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    BOOL fSiteSelectable = FALSE;
    
    IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
    
    fSiteSelectable = CControlTracker::IsThisElementSiteSelectable(
                            _pSelMan,
                            eTag, 
                            pIElement );
Cleanup:

    if ( fSiteSelectable )
        hr = S_OK;
    else
        hr = S_FALSE;
    RRETURN1( hr , S_FALSE );
}

//+====================================================================================
//
// Method: IsElementUIActivatable
//
// Synopsis: Tests to see if a given element is UI Activatable
//
//------------------------------------------------------------------------------------

HRESULT
CHTMLEditor::IsElementUIActivatable( IHTMLElement* pIElement )
{
    HRESULT hr;
    
    hr =  _pSelMan->IsElementUIActivatable( pIElement ) ? S_OK : S_FALSE ; 

    RRETURN1( hr , S_FALSE );
}


BOOL
SkipCRLF ( TCHAR ** ppch )
{
    TCHAR   ch1;
    TCHAR   ch2;
    BOOL    fSkippedCRLF = FALSE;
    //
    // this function returns TRUE if a replacement of one or two chars
    // with br spacing is possible.  
    //

    //
    // is there a CR or LF?
    //

    ch1 = **ppch;
    if (ch1 != _T('\r') && ch1 != _T('\n'))
        goto Cleanup;

    //
    // Move pch forward and see of there is another CR or LF 
    // that can be grouped with the first one.
    //
    fSkippedCRLF = TRUE;
    ++(*ppch);
    ch2 = **ppch;

    if ((ch2 == _T('\r') || ch2 == _T('\n')) && ch1 != ch2)
        (*ppch)++;

Cleanup:
    
    return fSkippedCRLF;
}


HRESULT
CHTMLEditor::InsertLineBreak( IMarkupPointer * pStart, BOOL fAcceptsHTML )
{
    IMarkupPointer  * pEnd = NULL;
    IHTMLElement    * pIElement = NULL;
    HRESULT           hr;
    
    IFC( GetMarkupServices()->CreateMarkupPointer( & pEnd ) );
    IFC( pEnd->MoveToPointer( pStart ) );

    //
    // If pStart is in an element that accepts the truth, then insert a BR there
    // otherwise insert a carriage return
    //
    if (fAcceptsHTML)
    {
        IFC( GetMarkupServices()->CreateElement( TAGID_BR, NULL, & pIElement) );        
        IFC( InsertElement( pIElement, pStart, pEnd ) );
    }
    else
    {
        IFC( GetViewServices()->InsertMaximumText( _T("\r"), 1, pStart ) );
    }

Cleanup:
    ReleaseInterface( pIElement );
    ReleaseInterface( pEnd );
    RRETURN( hr );
}


HRESULT
CHTMLEditor::InsertSanitizedText(
    TCHAR *             pStr,
    IMarkupPointer*     pStart,
    IMarkupServices*    pMarkupServices,
    CSpringLoader *     psl,
    BOOL                fDataBinding)
{
    const TCHAR chNBSP  = WCH_NBSP;
    const TCHAR chSpace = _T(' ');
    const TCHAR chTab   = _T('\t');

    HRESULT   hr = S_OK;;
    TCHAR     pchInsert[ TEXTCHUNK_SIZE + 1 ];
    TCHAR     chNext;
    int       cch = 0;
    IHTMLElement        *   pFlowElement = NULL;
    BOOL                    fContainer, fAcceptsHTML;
    IHTMLViewServices   *   pViewServices = GetViewServices();
    IMarkupPointer      *   pmpTemp = NULL;
    BOOL                    fMultiLine = TRUE;
    POINTER_GRAVITY         eGravity;

    // Remember the gravity so we can restore it at Cleanup
    IFC( pStart->Gravity(& eGravity ) );

    // we may be passed a null pStr if, for example, we are pasting an empty clipboard.
    
    if( pStr == NULL )
        goto Cleanup;

    IFC( pStart->SetGravity( POINTER_GRAVITY_Right ) );

    //
    // Determine whether we accept HTML or not
    //
    IFC( pViewServices->GetFlowElement(pStart, &pFlowElement) );

    if (! pFlowElement)
    {
        //
        // Elements that do not accept HTML, e.g. TextArea, always have a flow layout.
        // If the element does not have a flow layout then it might have been created
        // using the DOM (see bug 42685). Set fAcceptsHTML to true.
        //
        fAcceptsHTML = TRUE;
    }
    else
    {
        IFC( pViewServices->IsContainerElement(pFlowElement, &fContainer, &fAcceptsHTML) );

        IFC( pViewServices->IsMultiLineFlowElement(pFlowElement, &fMultiLine) );
    }

    if (*pStr && psl)
        IGNORE_HR(psl->Fire(pStart));

    chNext = *pStr;

    while( chNext )
    {   
        //
        // If the first character is a space, 
        // it must turn into an nbsp if pStart is 
        // at the beginning of a block/layout or after a <BR>
        //    
        if ( fAcceptsHTML && chNext == chSpace )
        {
            CEditPointer    ePointer( this );
            DWORD           dwBreak;

            IFC( ePointer.MoveToPointer( pStart ) );
            IGNORE_HR( ePointer.Scan(  LEFT,
                            BREAK_CONDITION_Block |
                            BREAK_CONDITION_Site |
                            BREAK_CONDITION_Control |
                            BREAK_CONDITION_NoScopeBlock |
                            BREAK_CONDITION_Text ,
                            &dwBreak ) );

            if ( ePointer.CheckFlag( dwBreak, BREAK_CONDITION_ExitBlock ) ||
                 ePointer.CheckFlag( dwBreak, BREAK_CONDITION_ExitSite  ) ||
                 ePointer.CheckFlag( dwBreak, BREAK_CONDITION_NoScopeBlock ))
            {
                // Change the first character to an nbsp
                chNext = chNBSP;
            }
        }

        for ( cch = 0 ; chNext != 0 && chNext != _T('\r') && chNext != _T('\n') ; )
        {
            if ( fAcceptsHTML )
            {
                //
                // Launder spaces if we accept html
                //
                switch ( chNext )
                {
                case chSpace:
                    if ( *(pStr + 1) == _T( ' ' ) )
                    {
                        chNext = chNBSP;
                    }
                    break;

                case chNBSP:
                    break;

                case chTab:
                    chNext = chNBSP;
                    break;
                }
            }

            pchInsert[ cch++ ] = chNext;
            
            chNext = *++pStr;
            
            if (cch == TEXTCHUNK_SIZE)
            {
                // pchInsert is full, empty the text into the tree
                pchInsert[ cch ] = 0;
                IFC( pViewServices->InsertMaximumText( pchInsert, TEXTCHUNK_SIZE, pStart ) );
                cch = 0;
            }
        }
        
        pchInsert[ cch ] = 0;

        if (! *pStr)
        {
            if (cch)
            {
                // Insert processed text into the tree and bail
                IFC( pViewServices->InsertMaximumText( pchInsert, cch, pStart ) );
            }
            break;
        }

        Verify( SkipCRLF( &pStr ) );        

        IFC( pViewServices->InsertMaximumText( pchInsert, -1, pStart ) );

        if ( !fAcceptsHTML && !fMultiLine )
        {
            // 
            // We're done because this is not a multi line control and
            // the first line of text has already been inserted.
            // 
            goto Cleanup;
        }

        if ( SkipCRLF( &pStr ))
        {
            // We got two CRLFs. If we are in data binding, simply
            // insert two BRs. If we are not in databinding, insert
            // a new paragraph.
            if (fDataBinding || !fAcceptsHTML)
            {
                IFC( InsertLineBreak( pStart, fAcceptsHTML ) );
                IFC( InsertLineBreak( pStart, fAcceptsHTML ) );
            }
            else
            {
                ClearInterface( & pmpTemp );
                IFC( HandleEnter( pStart, & pmpTemp, NULL, TRUE ) );
                if (pmpTemp)
                {
                    IFC( pStart->MoveToPointer( pmpTemp ) );
                }
            }
        }
        else
        {
            IFC( InsertLineBreak( pStart, fAcceptsHTML ) );
        }

        chNext = *pStr;
    }

Cleanup:
    ReleaseInterface( pmpTemp );
    ReleaseInterface( pFlowElement );
    IGNORE_HR( pStart->SetGravity( eGravity ) );

    RRETURN ( hr );
}

//+====================================================================================
//
// Method: IsElementSiteSelectable
//
// Synopsis: Tests to see if a given element is site selectable (Doesn't drill up to the Edit
//           Context. - test is just for THIS Element
//
//------------------------------------------------------------------------------------


HRESULT
CHTMLEditor::IsElementSiteSelected( IHTMLElement* pIElement )
{
    HRESULT hr = S_OK;

    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        return( E_FAIL );
    }
    
    hr = _pSelMan->IsElementSiteSelected( pIElement );
    
    RRETURN1( hr , S_FALSE );
}

//+====================================================================================
//
// Method: IsEditContextUIActive
//
// Synopsis: Is the edit context Ui active ( does it have the hatched border ?).
//           If it does - return S_OK
//                      else return S_FALSE
//
//------------------------------------------------------------------------------------

HRESULT
CHTMLEditor::IsEditContextUIActive()
{
    HRESULT hr;

    if ( !_pSelMan )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = _pSelMan->HasFocusAdorner() ? S_OK : S_FALSE;

Cleanup:
    RRETURN1( hr, S_FALSE );
}    

HRESULT 
CHTMLEditor::InsertSanitizedText( 
    IMarkupPointer * pIPointerInsertHere, 
    OLECHAR *        pstrText,
    BOOL             fDataBinding)
{
    HRESULT           hr = S_OK;
    
    if (!pIPointerInsertHere)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Now, do the insert
    //
    
    hr = THR( InsertSanitizedText( pstrText, pIPointerInsertHere, _pMarkupServices, NULL, fDataBinding ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}       

HRESULT
CHTMLEditor::UrlAutoDetectCurrentWord( 
    IMarkupPointer * pWord )
{
    HRESULT hr;

    if( !pWord )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( AutoUrl_DetectCurrentWord( _pMarkupServices, pWord, NULL, NULL ) );
    if( hr )
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CHTMLEditor::UrlAutoDetectRange(
    IMarkupPointer * pStart,
    IMarkupPointer * pEnd )
{
    HRESULT hr;

    if( !pStart || !pEnd )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( AutoUrl_DetectRange( _pMarkupServices, pStart, pEnd ) );
    if( hr )
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

    
HRESULT
CHTMLEditor::ShouldUpdateAnchorText (
        OLECHAR * pstrHref,
        OLECHAR * pstrAnchorText,
        BOOL    * pfResult )
{
    return ( AutoUrl_ShouldUpdateAnchorText (pstrHref, pstrAnchorText, pfResult ) );
}


HRESULT 
CHTMLEditor::LaunderSpaces ( 
    IMarkupPointer  * pStart,
    IMarkupPointer  * pEnd  )
{
    CDeleteCommand * pDeleteCommand;

    pDeleteCommand = (CDeleteCommand *) GetCommandTable()->Get( IDM_DELETE );
    Assert( pDeleteCommand );

    RRETURN( THR(pDeleteCommand->LaunderSpaces( pStart, pEnd )));
}    



//////////////////////////////////////////////////////////////////////////////////
// CHTMLEditor IOleCommandTarget Implementation
//////////////////////////////////////////////////////////////////////////////////

HRESULT
CHTMLEditor::QueryStatus(
    const GUID * pguidCmdGroup,
    ULONG cCmds,
    OLECMD rgCmds[],
    OLECMDTEXT * pcmdtext)
{
    HRESULT hr = E_FAIL;
    ISelectionRenderingServices * psrs = NULL;
    ISegmentList * pcontext = NULL;
    
    hr = THR( _pViewServices->GetCurrentSelectionRenderingServices( &psrs ) );
    if( hr )
        goto Cleanup;

    hr = THR( psrs->QueryInterface( IID_ISegmentList , (void **) &pcontext ));
    if( hr || pcontext == NULL)
        goto Cleanup;

    Assert(_pCommandTarget);
    hr = THR( _pCommandTarget->Initialize( pcontext ));
    if( hr )
        goto Cleanup;

    hr = THR_NOTRACE( _pCommandTarget->QueryStatus( pguidCmdGroup, cCmds, rgCmds, pcmdtext ));

Cleanup:
    ReleaseInterface( psrs );
    ReleaseInterface( pcontext );
    return( hr );    
}    

HRESULT
CHTMLEditor::Exec(
    const GUID * pguidCmdGroup,
    DWORD nCmdID,
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut)
{
    HRESULT hr = E_FAIL;
    ISelectionRenderingServices * psrs = NULL;
    ISegmentList * pcontext = NULL;
    IHTMLCaret * pc = NULL;
    
    hr = THR( _pViewServices->GetCurrentSelectionRenderingServices( &psrs ) );
    if( hr )
        goto Cleanup;

    hr = THR( psrs->QueryInterface( IID_ISegmentList , (void **) &pcontext ));
    if( hr || pcontext == NULL)
        goto Cleanup;

    Assert(_pCommandTarget);
    hr = THR( _pCommandTarget->Initialize( pcontext ));
    if( hr )
        goto Cleanup;

    hr = THR_NOTRACE( _pCommandTarget->Exec( pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut ));

#if DBG==1
    if (IsTagEnabled(tagEditingTrackExecFailures))
    {
        if (FAILED(hr))
        {
            CHAR szBuf[1000];

            wsprintfA(szBuf, "CHTMLEditor::Exec failed: nCmdId=%d, nCmdexecopt=0x%x, pvarargIn=0x%x, pvarargOut=0x%x",
                    nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            AssertSz(0, szBuf);
        }
    }
#endif    
    if( hr )
        goto Cleanup;

    if(!_pSelMan)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface( pc );
    ReleaseInterface( psrs );
    ReleaseInterface( pcontext );    
    return( hr );
}    

//////////////////////////////////////////////////////////////////////////////////
// CHTMLEditor Public Utilities
//////////////////////////////////////////////////////////////////////////////////


BOOL 
CHTMLEditor::IsContextEditable()
{ 
    return _pSelMan->IsContextEditable();
}

//
// marka - BUGBUG - why are there 2 FindBlockElement routines ( another one in edutil ?)
//
HRESULT 
CHTMLEditor::FindBlockElement( 
    IMarkupPointer   *     pPointer,
    IHTMLElement    **     ppBlockElement )
{
    HRESULT             hr = S_FALSE;
    IHTMLElement      * pElement = NULL;
    IHTMLElement      * pOldElement = NULL;
    BOOL                fIsBlockElement;
    ELEMENT_TAG_ID      tagId;

    *ppBlockElement = NULL;

    hr = THR( GetViewServices()->CurrentScopeOrSlave(pPointer, & pElement ) );
    if (hr)
        goto Cleanup;

    if (! pElement)
        goto Cleanup;

    *ppBlockElement = pElement;
    pElement->AddRef();

    do
    {
        hr = _pMarkupServices->GetElementTagId ( *ppBlockElement, & tagId );
        if (tagId == TAGID_BODY || tagId == TAGID_HTML)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        hr = _pViewServices->IsBlockElement(*ppBlockElement, & fIsBlockElement); 
        if (FAILED(hr) || fIsBlockElement)
            goto Cleanup;
            
        pOldElement = *ppBlockElement;                
        hr = THR(pOldElement->get_parentElement(ppBlockElement));
        pOldElement->Release();
        if (FAILED(hr))
            goto Cleanup;        
    }
    while (*ppBlockElement);

Cleanup: 
    ReleaseInterface( pElement );
    if (hr)
        ClearInterface( ppBlockElement );

    return hr;        
}


CSpringLoader *
CHTMLEditor::GetPrimarySpringLoader()
{
    Assert(_pCommandTarget);
    return _pCommandTarget->GetSpringLoader();
}

HRESULT 
CHTMLEditor::InsertElement(IHTMLElement *pElement, IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    RRETURN( EdUtil::InsertElement(GetMarkupServices(), pElement, pStart, pEnd) );
}

//////////////////////////////////////////////////////////////////////////////////
// CHTMLEditor Compose Settings Methods
//////////////////////////////////////////////////////////////////////////////////

struct COMPOSE_SETTINGS *
CHTMLEditor::GetComposeSettings(BOOL fDontExtract /*=FALSE*/)
{
    if (!fDontExtract)
        CComposeSettingsCommand::ExtractLastComposeSettings(this, _pComposeSettings != NULL);

    return _pComposeSettings;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLEditor::EnsureComposeSettings
//
//  Synopsis:   This function ensures the allocation of the compose font buffer
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

COMPOSE_SETTINGS *
CHTMLEditor::EnsureComposeSettings()
{
    if (!_pComposeSettings)
    {
        _pComposeSettings = (struct COMPOSE_SETTINGS *)MemAllocClear(Mt(CHTMLEditor_pComposeSettings), sizeof(struct COMPOSE_SETTINGS));

        if (_pComposeSettings)
            _pComposeSettings->_fComposeSettings = FALSE;
    }

    return _pComposeSettings;
}




HRESULT     
CHTMLEditor::GetSiteContainer(
    IHTMLElement *          pStart,
    IHTMLElement **         ppSite,
    BOOL *                  pfText,             /*=NULL*/
    BOOL *                  pfMultiLine,        /*=NULL*/
    BOOL *                  pfScrollable )      /*=NULL*/
{
    HRESULT hr = E_FAIL;
    BOOL fSite = FALSE;
    BOOL fText = FALSE;
    BOOL fMultiLine = FALSE;
    BOOL fScrollable = FALSE;

    SP_IHTMLElement spElement;

    Assert( pStart != NULL && ppSite != NULL );
    if( pStart == NULL || ppSite == NULL )
        goto Cleanup;

    *ppSite = NULL;
    spElement = pStart;

    while( ! fSite && spElement != NULL )
    {
        IFC( _pViewServices->IsSite( spElement, &fSite, &fText, &fMultiLine, &fScrollable ));

        if( ! fSite )
        {
            SP_IHTMLElement  spParent;
            IFC( spElement->get_parentElement( &spParent ));
            spElement = spParent;
        }
    }
    
Cleanup:

    if( fSite )
    {
        hr = S_OK;
        
        *ppSite = spElement;
        (*ppSite)->AddRef();
        
        if( pfText != NULL )
            *pfText = fText;

        if( pfMultiLine != NULL )
            *pfMultiLine = fMultiLine;

        if( pfScrollable != NULL )
            *pfScrollable = fScrollable;
    }

    RRETURN( hr );
}


HRESULT     
CHTMLEditor::GetTextSiteContainer(
    IHTMLElement *          pStart,
    IHTMLElement **         ppTextSite,
    BOOL *                  pfMultiLine,        /*=NULL*/
    BOOL *                  pfScrollable )      /*=NULL*/
{
    
    HRESULT hr = E_FAIL;
    BOOL fSite = FALSE;
    BOOL fText = FALSE;
    BOOL fMultiLine = FALSE;
    BOOL fScrollable = FALSE;
    BOOL fFound = FALSE;

    SP_IHTMLElement spElement;

    Assert( pStart != NULL && ppTextSite != NULL );
    if( pStart == NULL || ppTextSite == NULL )
        goto Cleanup;
        
    *ppTextSite = NULL;
    spElement = pStart;
    
    while( ! fFound && spElement != NULL )
    {
        IFC( _pViewServices->IsSite( spElement, &fSite, &fText, &fMultiLine, &fScrollable ));
        fFound = fSite && fText;
        
        if( ! fFound )
        {
            SP_IHTMLElement spParent;
            IFC( spElement->get_parentElement( &spParent ));
            spElement = spParent;
        }
    }
    
Cleanup:

    if( fFound )
    {
        hr = S_OK;
        *ppTextSite = spElement;
        (*ppTextSite)->AddRef();
        
        if( pfMultiLine != NULL )
            *pfMultiLine = fMultiLine;

        if( pfScrollable != NULL )
            *pfScrollable = fScrollable;
    }

    RRETURN( hr );
}


HRESULT
CHTMLEditor::GetBlockContainer(
    IHTMLElement *          pStart,
    IHTMLElement **         ppElement )
{
    HRESULT hr = E_FAIL;
    BOOL fFound = FALSE;
    SP_IHTMLElement spElement;

    Assert( pStart && ppElement );
    if( pStart == NULL || ppElement == NULL )
        goto Cleanup;

    *ppElement = NULL;
    spElement = pStart;

    while( ! fFound && spElement != NULL )
    {
        IFC( _pViewServices->IsBlockElement( spElement, &fFound ));
        
        if( ! fFound )
        {
            SP_IHTMLElement spParent;
            IFC( spElement->get_parentElement( &spParent ));
            spElement = spParent;
        }
    }
    
Cleanup:
    if( fFound )
    {
        hr = S_OK;
        *ppElement = spElement;
        (*ppElement)->AddRef();
    }
        
    RRETURN( hr );
}


HRESULT     
CHTMLEditor::GetSiteContainer(
    IMarkupPointer *        pPointer,
    IHTMLElement **         ppSite,
    BOOL *                  pfText,             /*=NULL*/
    BOOL *                  pfMultiLine,        /*=NULL*/
    BOOL *                  pfScrollable )      /*=NULL*/
{
    HRESULT hr = E_FAIL;
    SP_IHTMLElement spElement;

    Assert( pPointer != NULL && ppSite != NULL );
    if( pPointer == NULL || ppSite == NULL )
        goto Cleanup;

    IFC( GetViewServices()->CurrentScopeOrSlave(pPointer, &spElement ));
    
    if( spElement )
        IFC( GetSiteContainer( spElement, ppSite, pfText, pfMultiLine, pfScrollable ));
    
Cleanup:
    RRETURN( hr );
}


HRESULT     
CHTMLEditor::GetTextSiteContainer(
    IMarkupPointer *        pPointer,
    IHTMLElement **         ppTextSite,
    BOOL *                  pfMultiLine,        /*=NULL*/
    BOOL *                  pfScrollable )      /*=NULL*/
{
    HRESULT hr = E_FAIL;
    SP_IHTMLElement spElement;

    Assert( pPointer != NULL && ppTextSite != NULL );
    if( pPointer == NULL || ppTextSite == NULL )
        goto Cleanup;

    IFC( GetViewServices()->CurrentScopeOrSlave(pPointer, &spElement ));
    
    if( spElement )
        IFC( GetTextSiteContainer( spElement, ppTextSite, pfMultiLine, pfScrollable ));
    
Cleanup:
    RRETURN( hr );
}


HRESULT
CHTMLEditor::GetBlockContainer(
    IMarkupPointer *        pPointer,
    IHTMLElement **         ppElement )
{
    HRESULT hr = E_FAIL;
    SP_IHTMLElement spElement;

    Assert( pPointer != NULL && ppElement != NULL );
    if( pPointer == NULL || ppElement == NULL )
        goto Cleanup;

    IFC( GetViewServices()->CurrentScopeOrSlave(pPointer, &spElement ));
    
    if( spElement )
        IFC( GetBlockContainer( spElement, ppElement ));
    
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsInDifferentEditableSite
//
// Synopsis: Are we in a different site-selectable object from the edit context
//
//------------------------------------------------------------------------------------


BOOL
CHTMLEditor::IsInDifferentEditableSite(IMarkupPointer* pPointer)
{
    HRESULT hr = S_OK;
    BOOL fDifferent = FALSE;
    IHTMLElement* pIFlowElement = NULL ;

    IFC( GetViewServices()->GetFlowElement( pPointer, & pIFlowElement ));
    if ( pIFlowElement )
    {
        if ( ! SameElements( 
                             pIFlowElement, 
                             GetSelectionManager()->GetEditableElement()) && 
            IsElementSiteSelectable( pIFlowElement) == S_OK  )
        {
            fDifferent = TRUE;
        }
    }
    
Cleanup:
    ReleaseInterface( pIFlowElement);
    
    return( fDifferent );
}

HRESULT
CHTMLEditor::AdjustPointer(
    IMarkupPointer *        pPointer,
    BOOL                    infNotAtBOL,
    Direction               eBlockDir,
    Direction               eTextDir,
    IMarkupPointer *        pLeftBoundary,
    IMarkupPointer *        pRightBoundary,
    DWORD                   dwOptions /* = ADJPTROPT_None */ )
{
    HRESULT hr = S_OK;

    ELEMENT_TAG_ID eTag = TAGID_NULL;
    CEditPointer ep( this );        // allocate a new MP
    CEditPointer epSave( this );    // allocate a place saver
    BOOL fTextSite = FALSE;
    BOOL fBlockHasLayout = FALSE;
    BOOL fNotAtBOL = infNotAtBOL;
    BOOL fAtLogBOL = ! fNotAtBOL;
    DWORD dwSearch = 0;
    DWORD dwFound = 0;
    BOOL fStayOutOfUrl = ! CheckFlag( dwOptions , ADJPTROPT_AdjustIntoURL );    // If the adjust into url flag is not set, we stay out of the url
    BOOL fDontExitPhrase = CheckFlag( dwOptions , ADJPTROPT_DontExitPhrase ); // If the don't exit phrase flag is set, we don't exit phrase elements while adjusting to text
    
    SP_IHTMLElement spIBlock;
    SP_IHTMLElement spISite;        
    
    CEditPointer epTest(this);
    CEditPointer epLB(this);
    CEditPointer epRB(this);

    IFC( ep.MoveToPointer( pPointer ));
    IFC( epLB.MoveToPointer( pPointer ));
    IFC( epRB.MoveToPointer( pPointer ));
    IFC( GetViewServices()->MoveMarkupPointer( epLB, LAYOUT_MOVE_UNIT_CurrentLineStart, -1, &fNotAtBOL, &fAtLogBOL ));
    fNotAtBOL = infNotAtBOL;
    fAtLogBOL = ! infNotAtBOL;
    IFC( GetViewServices()->MoveMarkupPointer( epRB, LAYOUT_MOVE_UNIT_CurrentLineEnd, -1, &fNotAtBOL, &fAtLogBOL ));
    
    //
    // We want to be sure that the pointer is located inside a valid
    // site. For instance, if we are in a site that can't contain text,
    // trying to cling to text within that site is a waste of time.
    // In that case, we have to scan entering sites until we find one
    // that can contain text. If we can't find one going in our move 
    // direction, reverse and look the other way. By constraining 
    // using Scan and only entering layouts, we prevent many problems 
    // (especially with tables)
    //

    hr = THR( GetSiteContainer( ep, &spISite, &fTextSite ));

    //
    // BUGBUG. AdjPointerForInsert barfs on selects. Talk to John about the
    // "right" way to fix this.
    // We dont really allow typing in a select right now - so you're in the "right" 
    // place.
    //
    IFC( GetMarkupServices()->GetElementTagId( spISite, & eTag ));
    if ( eTag == TAGID_SELECT )
        goto Cleanup;
        
    //
    // Is our pointer within a text site?
    //

    if( ! fTextSite )
    {
        hr = AdjustIntoTextSite( ep, eBlockDir );
        
        if( FAILED( hr ))
        {
            hr = AdjustIntoTextSite( ep, Reverse( eBlockDir ));
        }
        
        if( FAILED( hr ))
        {
            hr = E_FAIL;
            goto Cleanup;
        }     
        
        // Our line may have moved
        IFC( epLB.MoveToPointer( ep ));
        IFC( epRB.MoveToPointer( ep ));
        fNotAtBOL = infNotAtBOL;
        fAtLogBOL = ! infNotAtBOL;
        IFC( GetViewServices()->MoveMarkupPointer( epLB, LAYOUT_MOVE_UNIT_CurrentLineStart, -1, &fNotAtBOL, &fAtLogBOL ));
        fNotAtBOL = infNotAtBOL;
        fAtLogBOL = ! infNotAtBOL;
        IFC( GetViewServices()->MoveMarkupPointer( epRB, LAYOUT_MOVE_UNIT_CurrentLineEnd, -1, &fNotAtBOL, &fAtLogBOL ));
    }

    //
    // Constrain the pointer
    //
    
    if( pLeftBoundary )
    {
        BOOL fAdj = FALSE;
        IFC( pLeftBoundary->IsRightOf( epLB, &fAdj ));

        if( fAdj )
            IFC( epLB->MoveToPointer( pLeftBoundary ));
    }

    if( pRightBoundary )
    {
        BOOL fAdj = FALSE;
        IFC( pRightBoundary->IsLeftOf( epRB, &fAdj ));

        if( fAdj )
            IFC( epRB->MoveToPointer( pRightBoundary ));
    }        
    
    IFC( ep.SetBoundary( epLB, epRB ));
    IFC( ep.Constrain() );
    
    //
    // Now that we are assured that we are in a nice cozy text site, we would
    // like to be in a block element, if possible.
    //
    hr = THR( GetBlockContainer( ep, &spIBlock ));
    
    if( spIBlock )
    {
        IFC( _pMarkupServices->GetElementTagId( spIBlock, &eTag ));
    }

    IFC( _pViewServices->IsSite( spIBlock , & fBlockHasLayout , NULL , NULL , NULL ));

    if( FAILED( hr ) || spIBlock == NULL || IsNonTextBlock( eTag ) || fBlockHasLayout )
    {
        BOOL fHitText = FALSE;
        
        hr = AdjustIntoBlock( ep, eBlockDir, &fHitText, TRUE, epLB, epRB );
        
        if( FAILED( hr ))
        {
            hr = AdjustIntoBlock( ep, Reverse( eBlockDir ), &fHitText, FALSE, epLB, epRB );
        }
        
        if( FAILED( hr ))
        {
            // Not so bad. Just cling to text...
            hr = S_OK;
        }
        
        if( fHitText )
        {
            goto Done;
        }
    }

    //
    // If we are block adjusting left, we can move exactly one NoscopeBlock to the left
    //
    
    if( eBlockDir == LEFT )
    {
        IFC( epSave.MoveToPointer( ep ));
        dwSearch =  BREAK_CONDITION_OMIT_PHRASE;
        dwFound = BREAK_CONDITION_None;
        hr = THR( ep.Scan( LEFT, dwSearch, &dwFound ));

        if( ! ep.CheckFlag( dwFound , BREAK_CONDITION_NoScopeBlock ))
        {
            IFC( ep.MoveToPointer( epSave ));
        }
    }

    //
    // Search for text
    //

    IFC( AdjustIntoPhrase(ep, eTextDir, fDontExitPhrase) );

Done:
    //
    // See if we need to exit from an URL boundary
    //

    IFC( epTest->MoveToPointer(ep) );
    IFC( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) );
    eTextDir = LEFT; // remember the direction of scan
    
    if (!epTest.CheckFlag(dwFound,  BREAK_CONDITION_ExitAnchor))
    {
        IFC( epTest->MoveToPointer(ep) );
        IFC( epTest.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) );
        eTextDir = RIGHT ;// remember the direction of scan
    }
    
    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_ExitAnchor))
    {
        if (!fStayOutOfUrl)
        {
            CEditPointer epText(this);
            
            // Make sure we are adjacent to text
            IFC( epText->MoveToPointer(ep) );
            IFC( epText.Scan(Reverse(eTextDir), BREAK_CONDITION_OMIT_PHRASE, &dwFound) );
            
            fStayOutOfUrl = !epText.CheckFlag(dwFound, BREAK_CONDITION_Text)
                            && !epText.CheckFlag(dwFound, BREAK_CONDITION_Anchor);
        }
        if (fStayOutOfUrl)
        {
            IFC( ep->MoveToPointer(epTest) );

            // Move into adjacent phrase elements if we can       
            IFC( AdjustIntoPhrase(ep, eTextDir, fDontExitPhrase) );
        }
    }

    IFC( pPointer->MoveToPointer( ep ));
        
Cleanup:

    RRETURN( hr );
}


HRESULT
CHTMLEditor::AdjustIntoTextSite(
    IMarkupPointer *        pPointer,
    Direction               eDir )
{
    HRESULT hr = E_FAIL;
    BOOL fDone = FALSE;
    BOOL fFound = FALSE;
    DWORD dwSearch = BREAK_CONDITION_Site;
    DWORD dwFound = BREAK_CONDITION_None;
    
    CEditPointer ep( this );
    SP_IHTMLElement spISite;
        
    IFC( ep.MoveToPointer( pPointer ));
    
    while( ! fDone && ! fFound )
    {
        dwFound = BREAK_CONDITION_None;
        hr = THR( ep.Scan( eDir, dwSearch, &dwFound ));

        if( ep.CheckFlag( dwFound, BREAK_CONDITION_Site ))
        {
            // Did we enter a text site in the desired direction?
            hr = THR( GetSiteContainer( ep, &spISite, &fFound ));
        }
        else
        {
            // some other break condition, we are done
            fDone = TRUE;
        }
    }

Cleanup:
    if( fFound )
    {
        hr = THR( pPointer->MoveToPointer( ep ));
    }
    else
    {
        hr = E_FAIL;
    }
    
    return( hr );
}    


HRESULT
CHTMLEditor::AdjustIntoBlock(
    IMarkupPointer *        pPointer,
    Direction               eDir,
    BOOL *                  pfHitText,
    BOOL                    fAdjustOutOfBody,
    IMarkupPointer *        pLeftBoundary,
    IMarkupPointer *        pRightBoundary )
{
    HRESULT hr = E_FAIL;

    // Stop if we hit text, enter or exit a site, or
    // hit an intrinsic control. If we enter or exit
    // a block, make sure we didn't "leave" the current 
    // line.

    BOOL fDone = FALSE;
    BOOL fFound = FALSE;
    BOOL fHitText = FALSE;
    
    DWORD dwSearch = BREAK_CONDITION_OMIT_PHRASE;
    DWORD dwFound = BREAK_CONDITION_None;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    TCHAR chFound = 0;
            
    CEditPointer ep( this );
    SP_IHTMLElement spIBlock;
    
    Assert( pPointer && pfHitText );
    if( pPointer == NULL || pfHitText == NULL )
        goto Cleanup;
        
    IFC( ep.MoveToPointer( pPointer ));
    IFC( ep.SetBoundary( pLeftBoundary, pRightBoundary ));
    IFC( ep.Constrain() );

    while( ! fDone )
    {
        dwFound = BREAK_CONDITION_None;
        IFC( ep.Scan( eDir , dwSearch, &dwFound, NULL, NULL, &chFound, NULL ));

        //
        // Did we hit text or a text like object?
        //

        if( ep.CheckFlag( dwFound , BREAK_CONDITION_TEXT ))
        {
            // just passed over text, back up and we done!
            
            LONG lChars = 1;
            MARKUP_CONTEXT_TYPE eCtxt = CONTEXT_TYPE_None;
            hr = THR( ep.Move( Reverse( eDir ), TRUE, &eCtxt, NULL, &lChars, NULL ));
            
            if( hr == E_HITBOUNDARY )
            { 
                fDone = TRUE;
                goto Cleanup;
            }
            
            IFC( pPointer->MoveToPointer( ep ));

            fHitText = TRUE;
            fFound = TRUE;
            fDone = TRUE;
            
            Assert( ! FAILED( hr ));
            
            goto Cleanup;
        }

        //
        // Did we hit a site boundary?
        //
        
        else if( ep.CheckFlag( dwFound, BREAK_CONDITION_Site ))
        {
            fDone = TRUE;
        }

        //
        // Did we enter a block?
        //
        
        else if( ep.CheckFlag( dwFound , BREAK_CONDITION_EnterBlock ))
        {   
            // Entered a Block
            hr = THR( GetBlockContainer( ep, &spIBlock ));

            if(( ! FAILED( hr )) && ( spIBlock != NULL ))
            {
                IFC( _pMarkupServices->GetElementTagId( spIBlock, &eTag ));

                if(( ! IsNonTextBlock( eTag )) && ( ! ( fAdjustOutOfBody && eTag == TAGID_BODY )))
                {
                    // found a potentially better block - check to see if eitehr breakonempty is set or the block contains text
                    BOOL fBOESet = FALSE;
                    IFC( _pViewServices->IsInflatedBlockElement( spIBlock , &fBOESet ));

                    if( fBOESet )
                    {
                        // it really is better - move our pointer there and set the sucess flag!!!
                        IFC( pPointer->MoveToPointer( ep ));
                        fFound = TRUE; // but, keep going!
                    }

                    // if BOE is not set, we have to keep going to determine if the block has content
                }
            }
            else
            {
                // didn't find a better block and we can go no farther... we are probably in the root
                fDone = TRUE;
            }
        }

        //
        // Did we exit a block - probably not a good sign, but we should keep going for now...
        //
        
        else if( ep.CheckFlag( dwFound, BREAK_CONDITION_ExitBlock ))
        {
            // we just booked out of a block element. things may be okay
        }

        else // hit something like boundary,  or error 
        {
            fDone = TRUE;
        }
    }

Cleanup:
    
    if( pfHitText )
    {
        *pfHitText = fHitText;
    }

    if( ! fFound )
        hr = E_FAIL;
    
    return( hr );
}

HRESULT
CHTMLEditor::AdjustIntoPhrase(IMarkupPointer  *pPointer, Direction eTextDir, BOOL fDontExitPhrase )
{
    HRESULT         hr;
    DWORD           dwFound;
    DWORD           dwSearch = BREAK_CONDITION_OMIT_PHRASE;        
    
    CEditPointer    epSave(this);
    CEditPointer    ep(this, pPointer);

    if( fDontExitPhrase )
    {
        dwSearch = dwSearch + BREAK_CONDITION_ExitPhrase;
    }
    
    IFC( epSave.MoveToPointer( ep ));    
    hr = THR( ep.Scan( eTextDir, dwSearch, &dwFound ));    
                        
    if( ! ep.CheckFlag( dwFound , BREAK_CONDITION_TEXT ))
    {
        //
        // We didn't find any text, lets look the other way
        //

        eTextDir = Reverse( eTextDir ); // Reverse our direction, keeping track of which way we are really going        
        IFC( ep.MoveToPointer( epSave )); // Go back to where we started...
        hr = THR( ep.Scan( eTextDir, dwSearch, &dwFound ));
    }

    if( ep.CheckFlag( dwFound , BREAK_CONDITION_TEXT ))
    {
        //
        // We found something, back up one space
        //
    
        LONG lChars = 1;
        MARKUP_CONTEXT_TYPE eCtxt = CONTEXT_TYPE_None;
        hr = THR( ep.Move( Reverse( eTextDir ), TRUE, &eCtxt, NULL, &lChars, NULL ));
        if( hr == E_HITBOUNDARY )
        {
            Assert( hr != E_HITBOUNDARY );
            goto Cleanup;
        }

    }
    else
    {
        IFC( ep.MoveToPointer(epSave) );
    }

Cleanup:
    RRETURN(hr);
}    

BOOL     
CHTMLEditor::IsNonTextBlock(
    ELEMENT_TAG_ID          eTag )
{
    BOOL fIsNonTextBlock = FALSE;
        
    switch( eTag )
    {
        case TAGID_NULL:
        case TAGID_UL:
        case TAGID_OL:
        case TAGID_DL:
        case TAGID_DIR:
        case TAGID_MENU:
        case TAGID_FORM:
        case TAGID_FIELDSET:
        case TAGID_TABLE:
        case TAGID_THEAD:
        case TAGID_TBODY:
        case TAGID_TFOOT:
        case TAGID_COL:
        case TAGID_COLGROUP:
        case TAGID_TC:
        case TAGID_TH:
        case TAGID_TR:
            fIsNonTextBlock = TRUE;        
            break;
       
        default:
            fIsNonTextBlock = FALSE;
    }
    
    return( fIsNonTextBlock );    
}

TCHAR *
CHTMLEditor::GetCachedString(UINT uiStringId)
{
    TCHAR *pchResult = NULL;

    if (_pStringCache)
    {
        pchResult = _pStringCache->GetString(uiStringId);
    }

    return pchResult;
}

// TODO: hook this into CRemoveFormatCommand
HRESULT
RemoveEmptyCharFormat(IMarkupServices *pMarkupServices, IHTMLElement **ppElement, BOOL fTopLevel, CSpringLoader *psl)
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spLeft;
    SP_IMarkupPointer   spRight;
    SP_IMarkupPointer   spWalk;
    ELEMENT_TAG_ID      tagId;
    BOOL                bEqual;
    BOOL                bHeading = FALSE;
    SP_IHTMLElement     spNewElement;

    IFC( pMarkupServices->GetElementTagId(*ppElement, &tagId) );
    
    switch (tagId)
    {
    case TAGID_H1:
    case TAGID_H2:
    case TAGID_H3:
    case TAGID_H4:
    case TAGID_H5:
    case TAGID_H6:
        if (!fTopLevel)
            goto Cleanup; // don't try heading reset if not in top level

        bHeading = TRUE;
        // fall through

    case TAGID_B:
    case TAGID_STRONG:
    case TAGID_U:
    case TAGID_EM:
    case TAGID_I:
    case TAGID_FONT:
    case TAGID_STRIKE:
    case TAGID_SUB:
    case TAGID_SUP:

        IFC( pMarkupServices->CreateMarkupPointer(&spLeft) );
        IFC( spLeft->MoveAdjacentToElement(*ppElement, ELEM_ADJ_AfterBegin ) );

        IFC( pMarkupServices->CreateMarkupPointer(&spRight) );
        IFC( spRight->MoveAdjacentToElement(*ppElement, ELEM_ADJ_BeforeEnd ) );

        IFC( spLeft->IsEqualTo(spRight, &bEqual) );

        //
        // If the pointers are not equal, allow one nbsp to be inside.
        //

        if (!bEqual)
        {
            MARKUP_CONTEXT_TYPE mctContext;
            long                cch = 1; // Walk one character at a time.
            TCHAR               ch;

            IFC( pMarkupServices->CreateMarkupPointer(&spWalk) );
            IFC( spWalk->MoveToPointer(spLeft) );

            IFC( spWalk->Right(TRUE, &mctContext, NULL, &cch, &ch) );
            if (   mctContext == CONTEXT_TYPE_Text
                && cch && WCH_NBSP == ch )
            {
                IFC( spWalk->IsEqualTo(spRight, &bEqual) );
            }
        }

        if (bEqual)
        {
            if (bHeading)
            {
                IFC( CGetBlockFmtCommand::GetDefaultBlockTag(pMarkupServices, &tagId) );
                IFC( pMarkupServices->CreateElement(tagId, NULL, &spNewElement) );
                
                IFC( ReplaceElement(pMarkupServices, *ppElement, spNewElement, spLeft, spRight) );

                ReplaceInterface(ppElement, (IHTMLElement *)spNewElement);

                if (psl)
                {
                    IGNORE_HR( psl->SpringLoadComposeSettings(spLeft, TRUE) );
                }
            }   
            else
            {
                IFC( pMarkupServices->RemoveElement(*ppElement) );
                hr = S_FALSE;
            }

        }
        break;

    default:
         hr = S_OK; // nothing to remove
    }    

Cleanup:    
    RRETURN1(hr, S_FALSE);
}

HRESULT
CHTMLEditor::HandleEnter(
    IMarkupPointer  *pCaret, 
    IMarkupPointer  **ppInsertPosOut,
    CSpringLoader   *psl       /* = NULL */,
    BOOL             fExtraDiv /* = FALSE */)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;
    SP_IHTMLElement     spBlockElement;
    SP_IHTMLElement     spNewElement;
    SP_IHTMLElement     spDivElement;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    SP_IHTMLElement     spCurElement;
    ELEMENT_TAG_ID      tagId;
    ELEMENT_TAG_ID      tagIdDefaultBlock = TAGID_NULL;
    SP_IObjectIdentity  spIdent;
    BOOL                bListMode = FALSE;
    
    if (ppInsertPosOut)
        *ppInsertPosOut = NULL;

    //
    // Create helper pointers
    //

    IFC( GetMarkupServices()->CreateMarkupPointer(&spStart) );
    IFC( GetMarkupServices()->CreateMarkupPointer(&spEnd) );

    //
    // Walk up to get the block element
    //

    IFC( GetMarkupServices()->BeginUndoUnit( _T("Typing") ));

    IFC( pCaret->CurrentScope(&spElement) );
    IFC( FindListItem(GetMarkupServices(), spElement, &spBlockElement) );
    if (spBlockElement)
    {
        bListMode = TRUE;
    }
    else
    {
        CBlockPointer       bpCurrent(this);
        SP_IMarkupPointer   spTest;
        BOOL                bEqual;

        IFC( pCaret->CurrentScope(&spElement) );                       
        IFC( FindOrInsertBlockElement(spElement, &spBlockElement, pCaret, TRUE) );

        // If we are not after begin or before end, it is safe to flatten
        IFC( GetMarkupServices()->CreateMarkupPointer(&spTest) );
        IFC( spTest->MoveAdjacentToElement(spBlockElement, ELEM_ADJ_AfterBegin) );
        
        IFC( spTest->IsEqualTo(pCaret, &bEqual) );
        if (!bEqual)
        {
            IFC( spTest->MoveAdjacentToElement(spBlockElement, ELEM_ADJ_BeforeEnd) );
            IFC( spTest->IsEqualTo(pCaret, &bEqual) );
            if (!bEqual)
            {
                // We need to flatten the block element so we don't introduce overlap
                IFC( bpCurrent.MoveTo(spBlockElement) );
                if (bpCurrent.GetType() == NT_Block)
                {
                    // we need a pointer with right gravity but we can't change the gravity of the input pointer
                    IFC( bpCurrent.FlattenNode() );
            
                    IFC( pCaret->CurrentScope(&spElement) );
                    IFC( FindOrInsertBlockElement(spElement, &spBlockElement, pCaret) );            
                }
            }
        }        
    }

    //
    // Load the spring loader before processing the enter key to 
    // copy formats down
    //
    if (psl)
    {
        IFC( psl->SpringLoad(pCaret, SL_TRY_COMPOSE_SETTINGS) );
    }

    //
    //
    // Split the block element and all contained elements
    // Except Anchors
    //
    
    IFC( spBlockElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&spIdent) );
    IFC( pCaret->CurrentScope(&spCurElement) );

    //
    // fExtraDiv works when the default block element is a DIV.
    // Refer to comment farther down for more information.
    //
    if ( fExtraDiv )
    {
        IFC( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagIdDefaultBlock));
    }
    
    for (;;)
    {                    
        IFC( spStart->MoveAdjacentToElement(spCurElement, ELEM_ADJ_AfterBegin) );
        IFC( spEnd->MoveAdjacentToElement(spCurElement, ELEM_ADJ_BeforeEnd) );
        IFC( GetMarkupServices()->GetElementTagId(spCurElement, &tagId) );        
        IFC( CCommand::SplitElement(GetMarkupServices(), spCurElement, spStart, pCaret, spEnd, & spNewElement) );
        IFC( pCaret->MoveAdjacentToElement(spNewElement, ELEM_ADJ_BeforeBegin) );
        IFC( GetMarkupServices()->GetElementTagId(spNewElement, &tagId) );

        switch (tagId)
        {
            case TAGID_A:
                //
                // If we are splitting the anchor in the middle or end, the first element remains
                // an anchor while the second does not.  If we split the anchor at the start,
                // the second element remains an anchor while the first is deleted.
                //

                if (DoesSegmentContainText(GetMarkupServices(), GetViewServices(), spStart, pCaret))
                {
                    IFC( GetMarkupServices()->RemoveElement( spNewElement ));
                    spNewElement = NULL;

                    if (psl)
                    {
                        IFC( psl->SpringLoad(pCaret, SL_RESET | SL_TRY_COMPOSE_SETTINGS) );
                    }
                }
                else
                {
                    IFC( GetMarkupServices()->RemoveElement( spCurElement ));
                    spCurElement = NULL;
                }
                break;

            case TAGID_DIV:
                //
                // If the split block element was a div, and fExtraDiv
                // is TRUE, then (because naked divs have no inter-block 
                // spacing) insert am empty div to achieve the empty line effect.
                //
                if ( fExtraDiv && tagIdDefaultBlock == TAGID_DIV)
                {
                    // We correctly assume that pCaret is before the inserted element
                    IFC( GetMarkupServices()->CreateElement( TAGID_DIV, NULL, &spDivElement ));                    
                    IFC( InsertElement(spDivElement, pCaret, pCaret ));
                }
                IFC( GetViewServices()->InflateBlockElement(spNewElement) );
                break;                

            case TAGID_P:
            case TAGID_BLOCKQUOTE:
                IFC( GetViewServices()->InflateBlockElement(spNewElement) );
                break;

            case TAGID_LI:
                IFC( spNewElement->removeAttribute(_T("value"), 0, NULL) )
                break;
        }    
        
        if( spNewElement != NULL && psl )
        {
            if (spCurElement != NULL)
                hr = spIdent->IsEqualObject(spCurElement);
            else
                hr = S_FALSE; // can't be top level if we deleted it
                
            IFC( RemoveEmptyCharFormat(GetMarkupServices(), &(spNewElement.p), (hr == S_OK), psl) ); // pNewElement may morph
        }
        else
        {
            hr = S_OK;
        }
        
        if( spNewElement != NULL && S_OK == hr )
        {
            SP_IMarkupPointer spNewCaretPos;

            IFC( GetMarkupServices()->CreateMarkupPointer(&spNewCaretPos) );
            IFC( spNewCaretPos->MoveAdjacentToElement(spNewElement, ELEM_ADJ_AfterBegin) );
            IFC( LaunderSpaces(spNewCaretPos, spNewCaretPos) );
            
            if (ppInsertPosOut && !(*ppInsertPosOut))
            {
                IFC( GetMarkupServices()->CreateMarkupPointer(ppInsertPosOut) );
                IFC( (*ppInsertPosOut)->MoveToPointer(spNewCaretPos) );
            }
        }

        if (spCurElement != NULL)
        {
            hr = spIdent->IsEqualObject(spCurElement);
            if (S_OK == hr)
                break;
        }
        
        IFC( pCaret->CurrentScope(&spCurElement) );        
    } 

    Assert(spNewElement != NULL); // we must exit by hitting the block element
        
Cleanup:
    GetMarkupServices()->EndUndoUnit();

    // We should have found a new position for the caret while walking up
    AssertSz(ppInsertPosOut == NULL || (*ppInsertPosOut), "Can't find new caret position");
    RRETURN(hr);    
}

IMarkupPointer* 
CHTMLEditor::GetStartEditContext()
{
    return _pSelMan->GetStartEditContext();
}

IMarkupPointer*
CHTMLEditor::GetEndEditContext()
{
    return _pSelMan->GetEndEditContext();
}

HRESULT 
CHTMLEditor::GetSegmentElement(ISegmentList *pSegmentList, INT i, IHTMLElement **ppElement)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    
    *ppElement = NULL;    

    IFR( GetMarkupServices()->CreateMarkupPointer(&spStart) );
    IFR( GetMarkupServices()->CreateMarkupPointer(&spEnd) );

    IFR(MovePointersToSegmentHelper(GetViewServices(), pSegmentList, i, &(spStart.p), &(spEnd.p)));

    // Get the element
    IFR( GetViewServices()->RightOrSlave(spStart, FALSE, NULL, ppElement, NULL, NULL) );
    Assert(*ppElement);

    return S_OK;
}


HRESULT
CHTMLEditor::IsInEditContext( IMarkupPointer * pPointer, BOOL *pfInEdit )
{
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        return( E_FAIL );
    }
    return _pSelMan->IsInEditContext( pPointer, pfInEdit );
}

//////////////////////////////////////////////////////////////////////////////////
// CHTMLEditor Private Methods
//////////////////////////////////////////////////////////////////////////////////



HRESULT 
CHTMLEditor::FindOrInsertBlockElement( IHTMLElement     *pElement, 
                                       IHTMLElement     **ppBlockElement,
                                       IMarkupPointer   *pCaret /* = NULL */,
                                       BOOL             fStopAtBlockquote /* = FALSE */ )
{
    HRESULT             hr;
    SP_IHTMLElement     spParentElement;
    CEditPointer        epLeft(this);
    CEditPointer        epRight(this);
    SP_IHTMLElement     spBlockElement;
    BOOL                bBlockElement;
    BOOL                bLayoutElement;
    ELEMENT_TAG_ID      tagId;

    spBlockElement = pElement;

    do
    {
        IFC( GetViewServices()->IsLayoutElement(spBlockElement, &bLayoutElement) ); 

        if (bLayoutElement)
            break; // need to insert below
                    
        IFC( GetViewServices()->IsBlockElement(spBlockElement, &bBlockElement) ); 

        if (fStopAtBlockquote)
        {
            IFC( GetMarkupServices()->GetElementTagId(spBlockElement, &tagId) );
            if (tagId == TAGID_BLOCKQUOTE)
                break; // need to insert below
        }

        if (bBlockElement)
            goto Cleanup; // done

        IFC(spBlockElement->get_parentElement(&spParentElement));
        
        if (spParentElement != NULL)
            spBlockElement = spParentElement;
    }
    while (spParentElement != NULL);


    //
    // Need to insert a block element
    //

    if (pCaret)
    {
        DWORD dwSearch = BREAK_CONDITION_Block | BREAK_CONDITION_Site;
        DWORD dwFound;

        // Expand left
        IFC( epLeft->MoveToPointer(pCaret) );
        IFC( epLeft.Scan(LEFT, dwSearch, &dwFound) );
        if (epLeft.CheckFlag(dwFound, dwSearch))
            IFC( epLeft.Scan(RIGHT, dwSearch, &dwFound) );

        // Expand right
        IFC( epRight->MoveToPointer(pCaret) );
        IFC( epRight.Scan(RIGHT, dwSearch, &dwFound) );
        if (epRight.CheckFlag(dwFound, dwSearch))
            IFC( epRight.Scan(LEFT, dwSearch, &dwFound) );
    }
    else
    {
        // Just use the block element as the boundary
        IFC( epLeft->MoveAdjacentToElement(spBlockElement, ELEM_ADJ_AfterBegin) );
        IFC( epRight->MoveAdjacentToElement(spBlockElement, ELEM_ADJ_BeforeEnd) );        
    }

    IFC( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagId) );
    IFC( GetMarkupServices()->CreateElement(tagId, NULL, &spBlockElement) );
    IFC( GetViewServices()->InflateBlockElement(spBlockElement) );

    if (pCaret)
    {
        IFC( InsertBlockElement(GetMarkupServices(), spBlockElement, epLeft, epRight, pCaret) );
    }
    else
    {
        IFC( InsertElement(spBlockElement, epLeft, epRight) );
    }        

Cleanup:  
    if (ppBlockElement)
    {
        *ppBlockElement = spBlockElement;
        if (spBlockElement != NULL)
            spBlockElement->AddRef();
    }

    RRETURN(hr);
}


HRESULT
CHTMLEditor::IsPhraseElement(IHTMLElement *pElement)
{
    HRESULT         hr;
    BOOL            fNotPhrase = TRUE;

    // Make sure the element is not a site or block element
    IFC( GetViewServices()->IsSite(pElement, &fNotPhrase, NULL, NULL, NULL) );
    if (fNotPhrase)
        return FALSE;
        
    IFC( GetViewServices()->IsBlockElement(pElement, &fNotPhrase) );
    if (fNotPhrase)
        return FALSE;

    return TRUE;

Cleanup:
    return FALSE;
}



//+====================================================================================
//
// Method: EqualContainers
//
// Synopsis: Check for equality on 2 MarkupContainers
//
//------------------------------------------------------------------------------------

BOOL
CHTMLEditor::EqualContainers( IMarkupContainer* pIMarkup1, IMarkupContainer* pIMarkup2 )
{
    HRESULT hr = S_OK;
    IUnknown* pIObj1 = NULL;
    IUnknown* pIObj2 = NULL ;
    BOOL fSame = FALSE;
    
    IFC( pIMarkup1->QueryInterface( IID_IUnknown, (void**) & pIObj1));
    IFC( pIMarkup2->QueryInterface( IID_IUnknown, (void**) & pIObj2));

    fSame = pIObj1 == pIObj2 ;
    
Cleanup:
    ReleaseInterface( pIObj1 );
    ReleaseInterface( pIObj2 );
    
    return fSame;
}

//+====================================================================================
//
// Method: MovePointersToEquivalentContainers
//
// Synopsis: Move markup pointers that are in separate IMarkupContainers to the same markup containers
//           for the purposes of comparison. 
//
//           Done by drilling piInner up until you're in pIOuter (or you fail).
//
// RETURN:
//           S_OK if the inner pointer was able to be moved into the container of the OUTER
//           S_FALSE if this wasn't possible.
//------------------------------------------------------------------------------------

HRESULT
CHTMLEditor::MovePointersToEqualContainers( 
                        IMarkupPointer* pIInner, IMarkupPointer* pIOuter )
{
    HRESULT hr = S_OK;
    IMarkupContainer* pIInnerMarkup = NULL;
    IMarkupContainer* pIOuterMarkup = NULL;
    IHTMLElement* pIElement = NULL;
    IHTMLElement* pIOuterElement = NULL;
    BOOL fFound = FALSE;    
    IFC( pIInner->GetContainer( & pIInnerMarkup ));
    IFC( pIOuter->GetContainer( & pIOuterMarkup ));

    Assert(! EqualContainers( pIInnerMarkup, pIOuterMarkup ));

    while ( ! EqualContainers( pIInnerMarkup, pIOuterMarkup ))
    {
        ClearInterface( & pIElement);
        ClearInterface( & pIOuterElement );
        
        IFC( GetViewServices()->CurrentScopeOrSlave(pIInner, & pIElement ));
        
        //
        // GetElementForSelection only handles Inputs. This will have to be 
        // updated when more embedded masters are done for post IE5.
        //
        IFC( GetViewServices()->GetElementForSelection( pIElement, & pIOuterElement));
        IFC( pIInner->MoveAdjacentToElement( pIOuterElement, ELEM_ADJ_BeforeBegin ));

        ClearInterface( & pIInnerMarkup );
        IFC( pIInner->GetContainer( & pIInnerMarkup ));

        if ( EqualContainers( pIInnerMarkup, pIOuterMarkup ))
        {
            fFound = TRUE;
            break;
        }        

        if ( SameElements( pIElement, pIOuterElement ))
        {
            // 
            // drilling up did nothing - don't loop forever
            //
            break;
        }        
    }

    if ( fFound )
    {
        hr = S_OK;        
    }
    else
        hr = S_FALSE;

Cleanup:
    ReleaseInterface( pIInnerMarkup );
    ReleaseInterface( pIOuterMarkup );
    ReleaseInterface( pIElement);
    ReleaseInterface( pIOuterElement );
    
    RRETURN1( hr, S_FALSE );
}

