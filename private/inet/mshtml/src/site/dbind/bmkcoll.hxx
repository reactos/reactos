#ifndef _BMKCOLL_HXX_
#define _BMKCOLL_HXX_

#include <headers.hxx>
#include <collbase.hxx>
#include <oledb.h>
#include <adoint.h>
typedef Recordset15 IADORecordset;    // beats me why ADO doesn't use I...

#define _hxx_
#include <bmkcoll.hdl>

MtExtern(CBookmarkCollection);
MtExtern(CBookmarkCollection_aryBookmarks_pv);

//+==================================================================
//
//  Class:      CBookmarkCollection
//
//+==================================================================

class CBookmarkCollection : public CBase, public IHTMLBookmarkCollection
{
    DECLARE_CLASS_TYPES(CBookmarkCollection, CBase);
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBookmarkCollection));

    CBookmarkCollection() {}
    ~CBookmarkCollection();

    HRESULT Init(const HROW *rghRows, ULONG cRows, IADORecordset *pADO);
    
    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CBookmarkCollection);
    DECLARE_DERIVED_DISPATCH(CBase);
    DECLARE_PRIVATE_QI_FUNCS(CBase);

    // IHTMLBookmarkCollection methods
    #define _CBookmarkCollection_
    #include <bmkcoll.hdl>

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    DECLARE_CDataAry(CAryBookmarks, VARIANT, Mt(Mem), Mt(CBookmarkCollection_aryBookmarks_pv));
    CAryBookmarks  _aryBookmarks;
};

#endif _BMKCOLL_HXX_
