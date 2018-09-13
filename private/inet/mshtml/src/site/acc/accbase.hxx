//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccBase.hxx
//
//  Contents:   Accessible base object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCBASE_HXX_
#define I_ACCBASE_HXX_
#pragma INCMSG("--- Beg 'accbase.hxx'")

#ifndef X_WINABLE_H_
#define X_WINABLE_H_
#include "winable.h"
#endif

#ifndef X_OLEACC_H_
#define X_OLEACC_H_
#include "oleacc.h"
#endif

// Forward declarations
//--------------------------
class CElement;
class CAccElement;

MtExtern(CAccBase)

//=================================================================
//
//  CLASS : CAccBase
//
//=================================================================

class CAccBase 
:   public CBase,
    public IAccessible , 
    public IOleWindow
{
    typedef CBase   super;
    
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAccBase))

    // IUnknown 
    DECLARE_PLAIN_IUNKNOWN(CAccBase);

    // IDispatch
	STDMETHOD (GetTypeInfoCount)( UINT *pctInfo );

	STDMETHOD (GetTypeInfo)(	unsigned int iTInfo,
								LCID lcid,
								ITypeInfo FAR* FAR* ppTInfo);

	STDMETHOD (GetIDsOfNames)(	REFIID riid,
								OLECHAR FAR* FAR* rgszNames,
								unsigned int cNames,
								LCID lcid,
								DISPID FAR* rgDispId);

	STDMETHOD (Invoke)(	DISPID dispIdMember,
						REFIID riid,
						LCID lcid,
						WORD wFlags,
						DISPPARAMS FAR* pDispParams,
						VARIANT FAR* pVarResult,
						EXCEPINFO FAR* pExcepInfo,
						unsigned int FAR* puArgErr);

    STDMETHOD (PrivateQueryInterface)(REFIID, void **);

    virtual STDMETHODIMP_( ULONG ) PrivateAddRef() = 0;
    virtual STDMETHODIMP_( ULONG ) PrivateRelease() = 0;

    //IAccessible Methods
    virtual STDMETHODIMP    get_accParent(IDispatch ** ppdispParent) = 0;
                            
    virtual STDMETHODIMP    get_accChildCount(long* pChildCount) = 0;
                            
    virtual STDMETHODIMP    get_accChild(VARIANT varChild, IDispatch ** ppdispChild) = 0;
                            
    virtual STDMETHODIMP    get_accName(VARIANT varChild, BSTR* pbstrName) = 0;
                            
    virtual STDMETHODIMP    get_accValue(VARIANT varChild, BSTR* pbstrValue) = 0;
                            
    virtual STDMETHODIMP    get_accDescription(VARIANT varChild, BSTR* pbstrDescription) = 0;
                            
    virtual STDMETHODIMP    get_accRole(VARIANT varChild, VARIANT *pvarRole) = 0;
                            
    virtual STDMETHODIMP    get_accState(VARIANT varChild, VARIANT *pvarState) = 0; 
                            
    STDMETHODIMP get_accHelp(VARIANT varChild, BSTR* pbstrHelp);
                            
    STDMETHODIMP get_accHelpTopic(BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic);
                            
    virtual STDMETHODIMP    get_accKeyboardShortcut(VARIANT varChild, 
                                                      BSTR* pbstrKeyboardShortcut) = 0;
                            
    virtual STDMETHODIMP    get_accFocus(VARIANT * pvarFocusChild) = 0 ;
                            
    virtual STDMETHODIMP    get_accSelection(VARIANT * pvarSelectedChildren) =0;
                            
    virtual STDMETHODIMP    get_accDefaultAction(VARIANT varChild, BSTR* pbstrDefaultAction) = 0;
                            
    virtual STDMETHODIMP    accSelect( long flagsSel, VARIANT varChild) = 0;
                            
    virtual STDMETHODIMP    accLocation(   long* pxLeft, long* pyTop, 
                                          long* pcxWidth, long* pcyHeight, 
                                                              VARIANT varChild) = 0;
                            
    virtual STDMETHODIMP    accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt) = 0;
                            
    virtual STDMETHODIMP    accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint) = 0;
                            
    virtual STDMETHODIMP    accDoDefaultAction(VARIANT varChild) = 0;
                            
    STDMETHODIMP    put_accName(VARIANT varChild, BSTR bstrName);
                            
    virtual STDMETHODIMP    put_accValue(VARIANT varChild, BSTR bstrValue) = 0;

    //IOleWindow Methods
    STDMETHODIMP  GetWindow( HWND* phwnd );
    STDMETHODIMP  ContextSensitiveHelp( BOOL fEnterMode );

    //--------------------------------------------------
    // list/child access methods
    //--------------------------------------------------

    //return the CDoc that the object is in. ( IOleWindow::GetWindow uses this )
    virtual CDoc* GetAccDoc() = 0;

    //--------------------------------------------------
    // Helper Methods
    //--------------------------------------------------
    long GetRole() { return(_lRole); }
    void SetRole( long lRole ){ _lRole = lRole; }
    
    // return the ITypeInfo pointer for the IAccessible interface. 
    HRESULT EnsureAccTypeInfo( ITypeInfo ** ppTI );

    HRESULT ValidateChildID( VARIANT *pVar );
    
    HRESULT GetAccParent( CElement * pElem, CAccBase ** ppAccParent );

    virtual CElement* GetElement() { return NULL; };

    // Return an IEnumVariant pointer to the child enumerator of this object.
    virtual HRESULT GetEnumerator( IEnumVARIANT ** ppEnum) = 0;

protected:

    DECLARE_CLASSDESC_MEMBERS;
    
//--------------------------------------------------
//  Member Data
//--------------------------------------------------
    long    _lRole;         // Acc. Role : set during derived object construction.
    long    _lState;        // state : set in derived class .

};


#pragma INCMSG("--- End 'accbase.hxx'")
#else
#pragma INCMSG("*** Dup 'accbase.hxx'")
#endif

