//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccWind.hxx
//
//  Contents:   Accessible Window object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCWIND_HXX_
#define I_ACCWIND_HXX_
#pragma INCMSG("--- Beg 'accwind.hxx'")

#ifndef X_ACCBASE_HXX_
#define X_ACCBASE_HXX_
#include "accbase.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

class CAccWindow : public CAccBase
{
    typedef CAccBase   super;

    virtual STDMETHODIMP_(ULONG) PrivateAddRef();
    virtual STDMETHODIMP_(ULONG) PrivateRelease();

     STDMETHOD (PrivateQueryInterface)(REFIID, void **);

public:
    virtual STDMETHODIMP get_accParent(IDispatch ** ppdispParent);
    
    virtual STDMETHODIMP get_accChildCount(long* pChildCount);
    
    virtual STDMETHODIMP get_accChild(VARIANT varChild, IDispatch ** ppdispChild);
    
    virtual STDMETHODIMP get_accName(VARIANT varChild, BSTR* pbstrName);
    
    virtual STDMETHODIMP get_accValue(VARIANT varChild, BSTR* pbstrValue);
    
    virtual STDMETHODIMP get_accDescription(VARIANT varChild, BSTR* pbstrDescription);

    virtual STDMETHODIMP get_accRole(VARIANT varChild, VARIANT *pvarRole);

    virtual STDMETHODIMP get_accState(VARIANT varChild, VARIANT *pvarState); 

    virtual STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, 
                                                    BSTR* pbstrKeyboardShortcut);

    virtual STDMETHODIMP get_accFocus(VARIANT * pvarFocusChild);

    virtual STDMETHODIMP get_accSelection(VARIANT * pvarSelectedChildren);

    virtual STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR* pbstrDefaultAction);

    virtual STDMETHODIMP accSelect( long flagsSel, VARIANT varChild);

    virtual STDMETHODIMP accLocation(   long* pxLeft, long* pyTop, 
                                        long* pcxWidth, long* pcyHeight, 
                                                            VARIANT varChild);

    virtual STDMETHODIMP accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);

    virtual STDMETHODIMP accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);

    virtual STDMETHODIMP accDoDefaultAction(VARIANT varChild);

    virtual STDMETHODIMP put_accValue(VARIANT varChild, BSTR bstrValue);
    
    //Overwrite this helper function since the window has simpler access to
    //the CDoc.
    virtual CDoc* GetAccDoc(){return _pDoc; }          

    virtual HRESULT GetEnumerator( IEnumVARIANT ** ppEnum);

    HRESULT GetClientAccObj( CAccBase ** ppAccChild );

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccWindow( CDoc* pDocInner );
    
    ~CAccWindow();

    CMarkupPointer  _elemBegin;
    CMarkupPointer  _elemEnd;
       
protected:
    HRESULT AccObjFromWindow( HWND hWnd, void ** ppAccObj );


    CDoc            *_pDoc;         // pointer to the document we are hanging off.
};

#pragma INCMSG("--- End 'accwind.hxx'")
#else
#pragma INCMSG("*** Dup 'accwind.hxx'")
#endif

