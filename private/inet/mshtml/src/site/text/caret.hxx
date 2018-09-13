//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       caret.hxx
//
//  Contents:   Definition of the CCaret class.
//
//  Classes:    CCaret
//
//----------------------------------------------------------------------------

#ifndef I_CARET_H_
#define I_CARET_H_
#pragma INCMSG( "--- Beg 'caret.hxx'" )


#define CCH_PENDINGUPDATE   10       // max number of pending characters before forced display update
#define JUMP_CX             8        // Number of pixels to jump right by in EnsureCaretVisible
#define JUMP_CY             8        // Number of pixels to jump up by in EnsureCaretVisible
        

MtExtern(CCaret);

class CElement;
class CDisplay;
class CLinePtr;

class CCaret :  public CVoid , public IHTMLCaret
{            
        // --------------------------------------------------
        // Constructor / Destructor
        // --------------------------------------------------
        
    public:
        DECLARE_MEMCLEAR_NEW_DELETE( Mt( CCaret ))
        
        CCaret( 
            CDoc *              pDoc );

        virtual ~CCaret();

        HRESULT Init();

        NV_DECLARE_ONCALL_METHOD(DeferredUpdateCaret, deferredupdatecaret, (DWORD_PTR dwContext));
        NV_DECLARE_ONCALL_METHOD(DeferredUpdateCaretScroll, deferredupdatecaretscroll, (DWORD_PTR dwContext));
        
        // --------------------------------------------------
        // IUnknown Interface
        // --------------------------------------------------

        STDMETHODIMP_(ULONG)
        AddRef( void ) ;

        STDMETHODIMP_(ULONG)
        Release( void ) ;

        STDMETHODIMP
        QueryInterface(
            REFIID              iid, 
            LPVOID *            ppv ) ;

        // --------------------------------------------------
        // IHTMLCursor Interface
        // --------------------------------------------------

        STDMETHODIMP 
        MoveCaretToPointer( 
            IMarkupPointer* pIMarkupPointerCaret,
            BOOL            fNotAtBOL,
            BOOL            fScrollIntoView,
            CARET_DIRECTION eDir );

        STDMETHODIMP
        MoveCaretToPointerEx(
            IMarkupPointer* pPosition,
            BOOL            fNotAtBOL,
            BOOL            fVisible,
            BOOL            fScrollIntoView,
            CARET_DIRECTION eDir );

        STDMETHODIMP 
        MovePointerToCaret( 
            IMarkupPointer* pIMarkupPointerCaret );

        STDMETHODIMP 
        IsVisible(
            BOOL *          pIsVisible );

        STDMETHODIMP 
        Show(
            BOOL            fScrollIntoView );

        STDMETHODIMP 
        Hide();

        STDMETHODIMP 
        InsertText(
            OLECHAR *       pText,
            LONG            lLen );

        STDMETHODIMP 
        InsertMarkup( 
            OLECHAR *       pMarkup );

        STDMETHODIMP 
        ScrollIntoView();

        STDMETHODIMP 
        GetElementContainer( 
            IHTMLElement ** ppElement );

        STDMETHODIMP
        GetLocation(
            POINT *         pPoint,
            BOOL            fTranslate );

        STDMETHODIMP
        UpdateCaret();
        
        STDMETHODIMP 
        SetOffset(
            LONG            lXDelta,
            LONG            lYDelta,
            LONG            lHDelta )
        {
            _dx = lXDelta;
            _dy = lYDelta;
            _dh = lHDelta;
            DeferUpdateCaret( FALSE );
            return S_OK;
        }

        STDMETHODIMP
        GetOffset(
            LONG *          plXDelta,
            LONG *          plYDelta,
            LONG *          plHDelta )
        {
            *plXDelta = _dx;
            *plYDelta = _dy;
            *plHDelta = _dh;
            return S_OK;
        }

        STDMETHODIMP
        GetNotAtBOL( BOOL * pfNotAtBOL )
        {
            *pfNotAtBOL = _fNotAtBOL;
            return( S_OK );
        }

        STDMETHODIMP
        SetNotAtBOL( BOOL  fNotAtBOL )
        {
            _fNotAtBOL = fNotAtBOL;
            return( S_OK );
        }

        STDMETHODIMP
        LoseFocus();

        
        HRESULT
        UpdateCaret(
            BOOL            fScrollIntoView,
            BOOL            fForceScroll,
            CDocInfo *      pdci = NULL );

        BOOL 
        IsPositioned();
            
        HRESULT
        BeginPaint();

        HRESULT
        EndPaint();

        LONG GetCp();
            
        CTreeNode * GetNodeContainer(DWORD dwFlags);

    private:
        // --------------------------------------------------
        // Hidden Constructor / Destructor
        // --------------------------------------------------

        CCaret() {};
    
        // --------------------------------------------------
        // Static Variables
        // --------------------------------------------------
        
        static const LONG         _xInvisiblePos;
        static const LONG         _yInvisiblePos;
        static const UINT         _HeightInvisible;
        static const UINT         _xCSCaretShift;     // COMPLEXSCRIPT Number of pixels to shift bitmap caret to properly align
        
        // --------------------------------------------------
        // Instance Variables
        // --------------------------------------------------

        LONG                    _cRef;
        CDoc *                  _pDoc;                       // The Document in which we live
        CMarkupPointer *        _pMPCaret;                   // Markup Pointer denoting the position of the caret

        BOOL                    _fVisible               : 1; // Should I show the caret?
        BOOL                    _fNotAtBOL              : 1; // Is the caret in the infamous EOL position
        BOOL                    _fMoveForward           : 1; // is the caret moving logically forward?
        BOOL                    _fCanPostDeferred       : 1; // Is it valid to post deferred updates? Not when I'm dying
        BOOL                    _fTyping                : 1; // Has typing occured since my last update
        BOOL                    _fUpdateEndPaint        : 1; // We recieved a deferred update during paint
        BOOL                    _fUpdateScrollEndPaint  : 1; // We recieved a deferred update and scroll during paint
        POINT                   _Location;
        LONG                    _fPainting;                  // Are we painting?
        LONG                    _width;
        LONG                    _height;
        LONG                    _dx;
        LONG                    _dy;
        LONG                    _dh;
        HBITMAP                 _hbmpCaret;                  // COMPLEXSCRIPT caret for foreign language support

        // --------------------------------------------------
        // Helper Functions
        // --------------------------------------------------

        CFlowLayout * GetFlowLayout();

        HRESULT UpdateScreenCaret( BOOL fScrollIntoView , BOOL fIsIME );
        
        HRESULT CalcScreenCaretPos();

        void CreateCSCaret(
            LCID                    curKbd);

        void DeferUpdateCaret( BOOL fScrollOnNextUpdate );

        BOOL DocHasFocus();
};


class CPaintCaret
{
public:
    CPaintCaret(CCaret * pCaret )
    {
        _pCaret = pCaret;
        if (_pCaret)
        {
            _pCaret->BeginPaint();
        }
    }

    ~CPaintCaret()
    {
        if (_pCaret)
        {
            _pCaret->EndPaint();
        }
    }

private:
    CCaret *    _pCaret;
};

#endif // I_CARET_H_

