//=================================================================
//
//   File:      omrect.hxx
//
//  Contents:   COMRect and COMRectCollection classes
//
//
//=================================================================

#ifndef I_OMRECT_HXX_
#define I_OMRECT_HXX_
#pragma INCMSG("--- Beg 'omrect.hxx'")

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#define _hxx_
#include "omrect.hdl"

MtExtern(COMRect)
MtExtern(COMRectCollection)
MtExtern(COMRectCollection_aryRects_pv)

//+------------------------------------------------------------
//
//  Class : COMRect
//
//-------------------------------------------------------------

class COMRect : public CBase,  public IHTMLRect
{
    DECLARE_CLASS_TYPES(COMRect, CBase)
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(COMRect))

    COMRect(RECT *pRect);

    inline RECT *GetRect() { return &_Rect;};

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(COMRect)
    DECLARE_DERIVED_DISPATCH(CBase)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IHTMLOMRect methods
    #define _COMRect_
    #include "omrect.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

    RECT _Rect;
};


DECLARE_CPtrAry(CAryOmRects, COMRect *, Mt(Mem), Mt(COMRectCollection_aryRects_pv))


//+------------------------------------------------------------
//
//  Class : COMRectCollection
//
//-------------------------------------------------------------

class COMRectCollection : public CCollectionBase, public IHTMLRectCollection
{
public:
    DECLARE_CLASS_TYPES(COMRectCollection, CCollectionBase)
    DECLARE_MEMALLOC_NEW_DELETE(Mt(COMRectCollection))

    ~COMRectCollection();

    // Returns memeber COMRect pointer array pointer
    inline CAryOmRects *GetAryPtr() { return &_aryRects;}

    // Get the rectangels from given array of Ractangles
    HRESULT SetRects(CDataAry<RECT> *pSrcRect);

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(COMRect)
    DECLARE_DERIVED_DISPATCH(CCollectionBase)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IHTMLRectCollection methods
    #define _COMRectCollection_
    #include "omrect.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

    virtual long FindByName(LPCTSTR pszName, BOOL fCaseSensitive = TRUE );
    virtual LPCTSTR GetName(long lIdx);
    virtual HRESULT GetItem( long lIndex, VARIANT *pvar );

    CAryOmRects   _aryRects;

};

#pragma INCMSG("--- End 'omrect.hxx'")
#else
#pragma INCMSG("*** Dup 'omrect.hxx'")
#endif
