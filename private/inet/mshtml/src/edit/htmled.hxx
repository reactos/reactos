//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Class:      CHTMLEditor
//
//  Contents:   Definition of CHTMLEditor interfaces. This class is used to 
//                dispatch IOleCommandTarget commands to Tridnet range objects.
//
//  History:    7-Jan-98   raminh  Created
//----------------------------------------------------------------------------
#ifndef _HTMLED_HXX_
#define _HTMLED_HXX_ 1

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

class CCommandTable;
class CSelectionManager;
class CMshtmlEd;
class CSpringLoader;
struct COMPOSE_SETTINGS;
class CStringCache;

enum TRACKER_NOTIFY;

HRESULT OldCompare( IMarkupPointer *, IMarkupPointer *, int * );

extern "C" const CLSID  CLSID_HTMLEditor;

MtExtern(CHTMLEditor);

//
// Used by CHTMLEditor::AdjustPointer
//

enum ADJPTROPT
{
    ADJPTROPT_None                  = 0,
    ADJPTROPT_AdjustIntoURL         = 1,        // If at URL boundary, don't move outside
    ADJPTROPT_DontExitPhrase        = 2         // Don't exit phrase elements (like span's, bold, etc.) while adjusting to text
};

class ATL_NO_VTABLE CHTMLEditor : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CHTMLEditor, &CLSID_HTMLEditor>,
    public IHTMLEditor,
    public IHTMLEditingServices,
    public IOleCommandTarget
{
    public:

        DECLARE_REGISTRY_RESOURCEID( IDR_HTMLEDITOR )
        DECLARE_NOT_AGGREGATABLE(CHTMLEditor)
        
        ~CHTMLEditor();
    
        BEGIN_COM_MAP(CHTMLEditor)
            COM_INTERFACE_ENTRY(IHTMLEditor)
            COM_INTERFACE_ENTRY(IHTMLEditingServices)
            COM_INTERFACE_ENTRY(IOleCommandTarget)
        END_COM_MAP()


        ///////////////////////////////////////////////////////
        // IHTMLEditor methods
        ///////////////////////////////////////////////////////
        
        STDMETHOD ( Initialize ) (
            IUnknown * pUnkDoc,
            IUnknown * pUnkView );

        STDMETHOD ( HandleMessage ) ( 
            SelectionMessage* pSelectionMessage ,
            DWORD* pdwFollowUpAction )  ;
            
        STDMETHOD ( SetEditContext )  (
            BOOL fEditable ,
            BOOL fSetSelection,
            BOOL fParentEditable,
            IMarkupPointer* pStartPointer,
            IMarkupPointer* pEndPointer,
            BOOL fNoScope ); 

        STDMETHOD ( GetSelectionType) (
            SELECTION_TYPE * eSelectionType );

        STDMETHOD ( Notify ) (
            SELECTION_NOTIFICATION eSelectionNotification,
            IUnknown *pUnknown, 
            DWORD* pdwFollowUpActionFlag,
            DWORD dword ) ;

        STDMETHOD ( GetRangeCommandTarget ) (
            IUnknown* pContext,
            IUnknown ** ppUnkTarget );
        
        STDMETHOD ( GetElementToTabFrom ) (
            BOOL            fForward,
            IHTMLElement**  ppElement,
            BOOL *          pfFindNext);

        STDMETHOD ( IsElementSiteSelected ) (IHTMLElement* pIElement);

        STDMETHOD( IsEditContextUIActive ) () ;
        
        ///////////////////////////////////////////////////////
        // IHTMLEditingServices Methods
        ///////////////////////////////////////////////////////

        STDMETHOD ( Delete ) ( 
            IMarkupPointer* pStart, 
            IMarkupPointer* pEnd,
            BOOL fAdjustPointers );

        STDMETHOD ( DeleteCharacter ) (
            IMarkupPointer * pPointer, 
            BOOL fLeftBound, 
            BOOL fWordMode,
            IMarkupPointer * pBoundary );
        
        STDMETHOD ( Paste ) ( 
            IMarkupPointer* pStart, 
            IMarkupPointer* pEnd, 
            BSTR bstrText = NULL );   

        STDMETHOD ( InsertSanitizedText ) ( 
            IMarkupPointer* pIPointerInsertHere, 
            OLECHAR * pstrText,
            BOOL fDataBinding);

        STDMETHOD ( UrlAutoDetectCurrentWord ) (
            IMarkupPointer* pWord );

        STDMETHOD ( UrlAutoDetectRange ) (
            IMarkupPointer* pStart,
            IMarkupPointer* pEnd );

        STDMETHOD ( ShouldUpdateAnchorText ) (
            OLECHAR * pstrHref,
            OLECHAR * ptrAnchorText,
            BOOL    * pfResult );

        STDMETHOD ( PasteFromClipboard ) ( 
            IMarkupPointer* pStart, 
            IMarkupPointer* pEnd,
            IDataObject * pDO );   
            
        STDMETHOD ( Select ) ( 
            IMarkupPointer* pStart, 
            IMarkupPointer* pEnd ,
            SELECTION_TYPE eType,
            DWORD * pdwFollowUpCode );   
            
        STDMETHOD ( LaunderSpaces ) ( 
            IMarkupPointer  * pStart,
            IMarkupPointer  * pEnd  );

        STDMETHOD ( IsPointerInSelection ) (
            IMarkupPointer* pMarkupPointer ,
            BOOL *  pfPointInSelection,
            POINT * pptGlobal,
            IHTMLElement* pIElementOver );

        STDMETHOD ( AdjustPointerForInsert ) ( 
                                    IMarkupPointer * pWhereIThinkIAm , 
                                    BOOL fFurtherInDocument, 
                                    BOOL fNotAtBol ,
                                    IMarkupPointer* pConstraintStart,
                                    IMarkupPointer* pConstraintEnd);

        STDMETHOD ( FindSiteSelectableElement ) (IMarkupPointer* pPointerStart,
                                                 IMarkupPointer* pPointerEnd, 
                                                 IHTMLElement** ppIHTMLElement);  

        STDMETHOD ( IsElementSiteSelectable ) ( IHTMLElement* pIHTMLElement);                                                           

        STDMETHOD ( IsElementUIActivatable ) ( IHTMLElement* pIHTMLElement);  
        
        ///////////////////////////////////////////////////////
        // IOleCommandTarget methods
        ///////////////////////////////////////////////////////

        STDMETHOD (QueryStatus)(
            const GUID * pguidCmdGroup,
            ULONG cCmds,
            OLECMD rgCmds[],
            OLECMDTEXT * pcmdtext);

        STDMETHOD (Exec)(
            const GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);

        ///////////////////////////////////////////////////////
        // Constructor
        ///////////////////////////////////////////////////////
        
        CHTMLEditor() ;

        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHTMLEditor));
        
        ///////////////////////////////////////////////////////
        // Utilitites
        ///////////////////////////////////////////////////////
        
        IHTMLDocument2* GetDoc() { return _pDoc; }

        IUnknown* GetUnkDoc() { return _pUnkDoc; }

        IUnknown* GetUnkView() { return _pUnkView; }

        IMarkupServices* GetMarkupServices() { return _pMarkupServices; }

        IHTMLViewServices* GetViewServices() { return _pViewServices; }

#if DBG == 1
        IEditDebugServices* GetEditDebugServices() { return _pEditDebugServices; }
#endif       

        CCommandTable* GetCommandTable() { return _pCommandTable; }

        BOOL IsContextEditable();

        //
        // Block related helpers
        //

        HRESULT FindBlockElement( 
            IMarkupPointer   *     pPointer,
            IHTMLElement    **     ppBlockElement );

        HRESULT FindOrInsertBlockElement( 
            IHTMLElement     *pElement, 
            IHTMLElement     **pBlockElement,
            IMarkupPointer   *pCaret = NULL,
            BOOL             fStopAtBlockquote = FALSE);
        
        CSelectionManager* GetSelectionManager() { return _pSelMan ; }

        IMarkupPointer* GetStartEditContext();

        IMarkupPointer* GetEndEditContext();

        HRESULT IsInEditContext( IMarkupPointer* pPointer, BOOL* pfInEditContext );        
        
        CSpringLoader             * GetPrimarySpringLoader();
        struct COMPOSE_SETTINGS * GetComposeSettings(BOOL fDontExtract = FALSE);
        struct COMPOSE_SETTINGS * EnsureComposeSettings();

        HRESULT     InsertElement(
            IHTMLElement *          pElement, 
            IMarkupPointer *        pStart, 
            IMarkupPointer *        pEnd );


        HRESULT     GetSiteContainer(
            IHTMLElement *          pStart,
            IHTMLElement **         ppSite,
            BOOL *                  pfText          = NULL,
            BOOL *                  pfMultiLine     = NULL,
            BOOL *                  pfScrollable    = NULL );


        HRESULT     GetTextSiteContainer(
            IHTMLElement *          pStart,
            IHTMLElement **         ppTextSite      = NULL,
            BOOL *                  pfMultiLine     = NULL,
            BOOL *                  pfScrollable    = NULL );


        HRESULT     GetBlockContainer(
            IHTMLElement *          pStart,
            IHTMLElement **         ppElement );


        HRESULT     GetSiteContainer(
            IMarkupPointer *        pPointer,
            IHTMLElement **         ppSite,
            BOOL *                  pfText          = NULL,
            BOOL *                  pfMultiLine     = NULL,
            BOOL *                  pfScrollable    = NULL );


        HRESULT     GetTextSiteContainer(
            IMarkupPointer *        pPointer,
            IHTMLElement **         ppTextSite      = NULL,
            BOOL *                  pfMultiLine     = NULL,
            BOOL *                  pfScrollable    = NULL );


        HRESULT     GetBlockContainer(
            IMarkupPointer *        pPointer,
            IHTMLElement **         ppElement );

            
        HRESULT     AdjustPointer(
            IMarkupPointer *        pPointer,           // Pointer to Adjust
            BOOL                    pfNotAtBOL,         // NotAtBOL (what line am I on?)
            Direction               eBlockDirection,    // Direction to adjust into site and block
            Direction               eTextDirection,     // Direction to adjust to text
            IMarkupPointer *        pLeftBoundary,
            IMarkupPointer *        pRightBoundary,
            DWORD                   dwOptions  = ADJPTROPT_None );
            
        BOOL        IsNonTextBlock(
            ELEMENT_TAG_ID          eTag );

        TCHAR *GetCachedString(UINT uiString);
            
        HRESULT HandleEnter(
            IMarkupPointer *pCaret,
            IMarkupPointer **ppNewInsertPos,
            CSpringLoader  *psl = NULL,
            BOOL            fExtraDiv = FALSE );
            
        HRESULT InsertSanitizedText(
            TCHAR *             pStr,
            IMarkupPointer*     pStart,
            IMarkupServices*    pMarkupServices,
            CSpringLoader *     psl,
            BOOL                fDataBinding);

        // Overloaded AdjustPointerForInsert from IHTMLEditingServices

        HRESULT AdjustPointerForInsert ( 
            IMarkupPointer * pWhereIThinkIAm , 
            BOOL fFurtherInDocument, 
            BOOL fNotAtBol ,
            IMarkupPointer* pConstraintStart,
            IMarkupPointer* pConstraintEnd,
            BOOL fStayOutsideUrl);

        BOOL IsInDifferentEditableSite(IMarkupPointer* pPointer);
        
        HRESULT GetSegmentElement(ISegmentList *pSegmentList, INT i, IHTMLElement **ppElement);

        HRESULT IsPhraseElement(IHTMLElement *pElement);

        HRESULT MovePointersToEqualContainers( IMarkupPointer* pIInner, IMarkupPointer* pIOuter );

        BOOL EqualContainers( IMarkupContainer* pIMarkup1, IMarkupContainer* pIMarkup2 );
        
    protected:
    
        HRESULT     AdjustIntoTextSite(
            IMarkupPointer *        pPointer,
            Direction               eDir );
        
        HRESULT     AdjustIntoBlock(
            IMarkupPointer *        pPointer,
            Direction               eDir,
            BOOL *                  pfHitText,
            BOOL                    fAdjustOutOfBody,
            IMarkupPointer *        pLeftBoundary,
            IMarkupPointer *        pRightBoundary );

        HRESULT     AdjustIntoPhrase(
            IMarkupPointer  *pPointer,
            Direction       eTextDir,
            BOOL            fDontExitPhrase );
            
        HRESULT InitCommandTable();
        HRESULT DeleteCommandTable();
        HRESULT InsertLineBreak( IMarkupPointer * pStart, BOOL fAcceptsHTML );

    private:
    
        ///////////////////////////////////////////////////////
        // Instance Variables
        ///////////////////////////////////////////////////////
        
        IHTMLDocument2*                     _pDoc;                   // The Doc we live in.
        IUnknown*                           _pUnkDoc;                // The IUnknown doc we were passed
        IUnknown*                           _pUnkView;               // The IUnknown view we were passed
        IMarkupServices*                    _pMarkupServices;        // Markup Services
        IHTMLViewServices*                  _pViewServices;          // View Services
 
#if DBG == 1        
        IEditDebugServices*                 _pEditDebugServices;     // Editing Debug Services
#endif        
        CSelectionManager*                  _pSelMan;                // The selection manager
        struct COMPOSE_SETTINGS*            _pComposeSettings;       // The compose settings
        CCommandTable*                      _pCommandTable;          // The command table
        CMshtmlEd*                          _pCommandTarget;         // The command target for selection
        BOOL                                _fGotActiveIMM;          // Did we get an IMM ?
        CStringCache                        *_pStringCache;          // Cache for LoadString calls
};

#endif // _HTMLED_HXX_
