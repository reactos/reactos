//+------------------------------------------------------------------------
//
//  File:       EdAdorn.hxx
//
//  Contents:   Edit Adorner Classes.
//
//      
//  History: 07-18-98 - marka Created
//
//-------------------------------------------------------------------------

#ifndef _EDADORN_HXX_
#define _EDADORN_HXX_ 1

MtExtern( CGrabHandleAdorner )
MtExtern( CActiveControlAdorner )
MtExtern( CCursor )

const USHORT CT_ADJ_LEFT      =   1;
const USHORT CT_ADJ_TOP       =   2;
const USHORT CT_ADJ_RIGHT     =   4;
const USHORT CT_ADJ_BOTTOM    =   8;
const USHORT CT_ADJ_ALL       =  15;

class CSelectionManager;

//+------------------------------------------------------------------------
//
//  Class:      CCurs (Curs)
//
//  Purpose:    System cursor stack wrapper class.  Creating one of these
//              objects pushes a new system cursor (the wait cursor, the
//              arrow, etc) on top of a stack; destroying the object
//              restores the old cursor.
//
//  Interface:  Constructor     Pushes a new cursor
//              Destructor      Pops the old cursor back
//
//-------------------------------------------------------------------------
class CCursor
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCursor));
    CCursor(LPCTSTR idr);
    ~CCursor(void);
    void Show();
private:
    HCURSOR     _hcrs;
    HCURSOR     _hcrsOld;
};


class CEditAdorner : public IElementAdorner 
{
    protected:
        virtual ~CEditAdorner(); // Use DestroyAdorner call instead.
            
    public:
    
        CEditAdorner( IHTMLElement* pIElement , IHTMLDocument2 * _pIDoc );


        
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

        STDMETHOD ( Draw ) ( 
            HDC hDc,
            RECT *pElementBounds) = 0 ;
        
        STDMETHOD(  HitTestPoint) ( 
            POINT* hitPoint,
            RECT *pAdornerBounds,
            BOOL *fHitTestResult,
            ADORNER_HTI *peAdornerHTI ) = 0 ;
        
        STDMETHOD ( GetSize ) ( 
                SIZE* pElementSize, 
                SIZE* pAdornerSize ) = 0 ;
        
        STDMETHOD ( GetPosition ) ( 
                POINT* pElementPosition, 
                POINT* pAdornerPosition ) = 0 ;

        STDMETHOD ( OnPositionSet) () = 0;
        
        HRESULT CreateAdorner();

        HRESULT DestroyAdorner();

        HRESULT UpdateAdorner();

        HRESULT ScrollIntoView();
       
        HRESULT InvalidateAdorner();
        
        VOID SetNotifyManagerOnPositionSet( BOOL fNotify)
        {
            _fNotifyManagerOnPositionSet = fNotify;
        }

        VOID SetManager( CSelectionManager * pManager );

        VOID NotifyManager();

        
    protected:        
        HRESULT ScrollIntoViewInternal();
        
        BOOL IsAdornedElementPositioned();

        IHTMLDocument2 * _pIDoc;    // Weak-ref of the Document that we live in.
        
        void SetBoundsInternal();
        
        RECT _rcBounds;      // Store the bounds on creation.

        CSelectionManager*                _pManager;

        IHTMLElement* _pIElement;      // Cookie of the Element we're attached to.

        BOOL                            _fPositionSet:1;
        BOOL                            _fScrollIntoViewOnPositionSet:1;
        int                             _ctOnPositionSet ;
        
    private:


        
        LONG _cRef;                 // Ref-Count

        DWORD_PTR _adornerCookie;      // Cookie that AddElementAdorner() gives us back on creation.

        BOOL                            _fNotifyManagerOnPositionSet:1;        


};

class CBorderAdorner : public CEditAdorner
{
    protected:
        virtual ~CBorderAdorner();
    public:
        
        CBorderAdorner( IHTMLElement* pIElement , IHTMLDocument2 * _pIDoc );



            
        STDMETHOD ( GetSize ) ( 
                SIZE* pElementSize, 
                SIZE* ppAdornerSize );
        
        STDMETHOD ( GetPosition ) ( 
                POINT* pElementPosition, 
                POINT* ppAdornerPosition );

        STDMETHOD ( OnPositionSet) ();
        
        virtual int GetInset() = 0 ;

};



class CGrabHandleAdorner : public CBorderAdorner
{

    private:



    protected:
        ~CGrabHandleAdorner();

    public:
         DECLARE_MEMCLEAR_NEW_DELETE(Mt(CGrabHandleAdorner));
         
         CGrabHandleAdorner( IHTMLElement* pIElement , IHTMLDocument2 * _pIDoc, BOOL fLocked );
         
         STDMETHOD ( Draw ) ( 
            HDC hDc,
            RECT *pElementBounds);

        STDMETHOD(  HitTestPoint) ( 
            POINT *hitPoint,
            RECT *pAdornerBounds,
            BOOL *fHitTestResult,
            ADORNER_HTI* peAdornerHTI);
            
         BOOL IsInResizeHandle( POINT ppt, ADORNER_HTI* pGrabHandle = NULL, RECT * pRectBounds = NULL );

         BOOL IsInMoveArea( POINT pt );
         
         BOOL IsInAdorner( POINT pt );

         virtual BOOL IsEditable();
         
         LPCTSTR GetResizeHandleCursorId(ADORNER_HTI inAdorner);
         
         VOID BeginResize( POINT pt, HWND hwnd );

         VOID DuringResize( POINT pt, BOOL fForceErase = FALSE );

         VOID EndResize( POINT pt , RECT* pNewSize );
#if 0
         VOID BeginMove( POINT pt, HWND hwnd, RECT* pElementRect = NULL );
         
         VOID DuringMove( POINT pt, BOOL fForceErase = FALSE, RECT* pRectMove = NULL );

         VOID EndMove( POINT, POINT* newPoint, BOOL fDrawFeedback = TRUE, RECT* pDrawRect =NULL );

         void CalcRectMove(RECT * prc, POINT pt);
#endif
         VOID ShowCursor(ADORNER_HTI inGrabHandle );
         
         BOOL IsLocked()
         {
            return _fLocked;
         }


         
    protected: 

        HRESULT SetLocked();
        
        HBRUSH GetFeedbackBrush();

        HBRUSH GetHatchBrush();
        
        int GetInset() ;      
        
        void DrawGrabHandles(HDC hdc, RECT *prc );

        void DrawGrabBorders(HDC hdc, RECT *prc, BOOL fHatch);

        //
        // Utility Routines for drawing.
        //

        void GetGrabRect(ADORNER_HTI htc, RECT * prcOut, RECT * prcIn);

        //void DrawDefaultFeedbackRect(HDC hDC, RECT * prc);

        static void PatBltRectH(HDC hDC, RECT * prc, RECT* prectExclude, int cThick, DWORD dwRop);

        static void PatBltRectV(HDC hDC, RECT * prc, RECT* prectExclude, int cThick, DWORD dwRop);              

        static void PatBltRect(HDC hDC, RECT * prc,RECT* prectExclude, int cThick, DWORD dwRop);
        
        void CalcRect(RECT * prc, POINT pt);
        
        void RectFromPoint(RECT * prc, POINT pt);

        VOID GetControlBounds(RECT* pRect);
        
#if 0
        void SnapRect(RECT * prc);
#endif
        void DrawFeedbackRect( RECT* prc );
        
        char _resizeAdorner; // the place we're resizing from

        RECT                            _rc;            // the Last Rc drawn
        RECT                            _rcFirst;       // the first RC when resizing starts
        USHORT                          _adj;           // the current adjustment.
        POINT                           _ptFirst;       // the Point where we begin resizing from
        
        HBRUSH                          _hbrFeedback;   // cached feedback brush
        HBRUSH                          _hbrHatch;      // cached Hatch brush
        RECT                            _rcControl;
        BOOL                            _fFeedbackVis: 1;   // Is the last feedback rect drawn visible ?
        BOOL                            _fDrawNew:     1;   // Do we draw the new Rect ?
        BOOL                            _fIsPositioned: 1;  // Is this element Positioned 
        BOOL                            _fLocked:1;       // Is this element Locked ?
        ADORNER_HTI                    _currentCursor; // ID of Current Cursor.

        CCursor*                        _pCursor;
        
#if DBG ==1
        BOOL                            _fInResize:1;
        BOOL                            _fInMove:1;
#endif
};

class CActiveControlAdorner: public CGrabHandleAdorner
{
    protected:
        ~CActiveControlAdorner();
        
    public:
    
         CActiveControlAdorner( IHTMLElement* pIElement , IHTMLDocument2 * _pIDoc, BOOL fLocked );        
         
         STDMETHOD ( Draw ) ( 
            HDC hDc,
            RECT *pElementBounds);
     
};

class CSelectedControlAdorner: public CGrabHandleAdorner
{
    protected:
        ~CSelectedControlAdorner();
        
    public:
    
         CSelectedControlAdorner( IHTMLElement* pIElement , IHTMLDocument2 * _pIDoc, BOOL fLocked );        

         BOOL IsEditable();
         
         STDMETHOD ( Draw ) ( 
            HDC hDc,
            RECT *pElementBounds);
     
};


#endif

