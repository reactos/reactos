//-------------------------------------------------------------------------//
//
//  TreeItems.h
//
//-------------------------------------------------------------------------//

#ifndef __TREEITEMS_H__
#define __TREEITEMS_H__

#include "resource.h"
#include "Dictionary.h"
#include "PriorityLst.h"

//-------------------------------------------------------------------------//
//  Forwards
class  CPropertyTreeCtl ;
class  CPropertyTreeItem ;
class  CPropertyFolder ;
class  CProperty ;
class  CMetrics ;

//-------------------------------------------------------------------------//
const int CCH_PROPERTY_TEXT = MAX_PATH ;

//  Tier constants
//  BUGBUG: These should be calculated on tree view insertion in order
//  to support multilevel hierarchy.
enum PROPTREE_TIERS {
    NIL_TIER,
    FOLDER_TIER,      
    PROPERTY_TIER,   
} ;

//-------------------------------------------------------------------------//
class CPropertyTreeItem
//-------------------------------------------------------------------------//
{
public:
    //  Construction, destruction, initialization
    CPropertyTreeItem( CPropertyTreeCtl* pCtl ) ;
    virtual ~CPropertyTreeItem() {}
                                 
public:
    //  Utility Methods
    static  CPropertyTreeItem*  GetTreeItem( const TV_ITEM& tvi ) ;
    static  HINSTANCE           GetModuleInstance() ;
    static  HINSTANCE           GetResourceInstance() ;
    CPropertyTreeItem*          GetNext( PROPTREE_ITEM_TYPE type, BOOL bExpand ) const ;
    CPropertyTreeItem*          GetPrev( PROPTREE_ITEM_TYPE type, BOOL bExpand ) const ;

    operator HTREEITEM() const ;

    //  General function and access methods
    virtual USHORT     Type() const =0 ;

            LPCTSTR    GetName() const ;
            LPCTSTR    GetDisplayName() const ;
            LPCTSTR    GetDescription() const ;
    virtual ULONG      GetAccess() const =0 ;
 
    virtual int        ItemWidth( BOOL bTextOnly ) const;
 
    virtual void       SetName( LPCTSTR pszName ) ;
    virtual void       SetDisplayName( LPCTSTR pszDisplayName ) ;
    virtual void       SetDescription( LPCTSTR pszDescr ) ;
    virtual BOOL       IsCompositeMismatch() const { return FALSE ; }
    virtual BOOL       IsMultiline() ;
    static  LPTSTR     MultilineToSingleline( LPCTSTR pszValue ) ;

    virtual LPCTSTR    ValueText() ;
    virtual BOOL       OnValueChanging( CPropVariant& varNew ) ;
    virtual void       OnValueChanged( const CPropVariant& varNew ) ;
    virtual BOOL       OnValueRestore()     { return FALSE ; }

    //  Property source methods
    virtual HRESULT    AddSource( const VARIANT* pvarSrc, ULONG dwAccess, const PROPVARIANT* pVal ) ;
    virtual HRESULT    RemoveSource( const VARIANT* pvarSrc ) ;
    virtual LONG       SourceCount() const ;
    virtual BOOL       Reconcile( HWND hwndTree, LONG cMasterSourceCount ) ;

    //  Tree View methods
    virtual HTREEITEM  Insert( HWND hwndTree, HTREEITEM hParent, HTREEITEM hInsertAfter = TVI_LAST )=0 ;
    virtual void       Remove() ;
    virtual int        TierNumber() const = 0 ;
    virtual int        InsertChildren() ;
    virtual LRESULT    OnTreeItemCallback( TV_DISPINFO* pDI, BOOL& bHandled ) ;
    virtual LRESULT    Draw( LPNMTVCUSTOMDRAW, CMetrics* ) ;
    virtual void       HitTest( LPTVHITTESTINFO pHTI ) ;
    virtual BOOL       GetLabelRect( OUT LPRECT prc, BOOL fTextBox = FALSE ) ;
    virtual BOOL       GetValueRect( OUT LPRECT prc, BOOL fTextBox = FALSE ) ;
    virtual BOOL       OnKillFocus( UINT uAction = TVC_UNKNOWN, CPropertyTreeItem* pItemNew = NULL ) ;

    //  TreeView order index among siblings
    virtual void       SetOrder( int i )=0 ;
    virtual int        Order() const = 0 ;

    //  Array of sort direction indicators for child items.
    virtual HTREEITEM  GetSortTarget( int iCol )    { return m_hTreeItem ; }
    int*               ChildSortDir() ;

    //  Pend sort methods.
    virtual void       PendChildSort()   {} // indicates that the item's children should be resorted.
    virtual void       UnpendChildSort() {} // indicates that the item's children do not require a resort.
    virtual BOOL       ChildSortPending() const ; // Retrieves the child sort pending status of the item.

    //  In-Place Edit Control methods
    virtual HWND       EditControl()  { return NULL ; }
    virtual HWND       ShowEditControl( UINT nShowCmd, LPCRECT prcEdit, CMetrics* ) ;
    virtual void       RepositionEditControl( LPCRECT prcEdit, CMetrics* ) ;
    virtual LRESULT    OnEditControlCommand( UINT nID, UINT nCode, HWND hwndEdit, CMetrics*, BOOL& bHandled ) ;
    virtual LRESULT    OnEditControlNotify( NMHDR* pHdr, CMetrics*, BOOL& bHandled ) ;

    //  Miscellaneous methods:
    virtual void       DisplayQtipText( HWND hwndQtip ) ;

    //  Non-virtual TreeView implementation methods
protected:
    HTREEITEM          Insert( HWND hwndTree, TV_INSERTSTRUCT* pTVI ) ;
    BOOL               UpdateChildCount( int cChildren ) ;
    virtual BOOL       DrawValueText() ;
    CPropertyTreeCtl*  Control() ;
    const HWND&        TreeHwnd() const ;

    //  Data
protected:    
    //  1st dimension indices and upper bound for m_szText array.
    enum     {
        iName,
        iDisplayName,
        iDescr,
        cPropertyText   // array upper bound
    } ;

    TCHAR  m_szText[3][CCH_PROPERTY_TEXT] ;
    int    m_xLabel, m_cxLabel,     // start, width of label text box
           m_xValue, m_cxValue ;    // start, width of value text box.
    int    m_iChildSortDir[2] ;
    
private:
    CPropertyTreeCtl* m_pCtl ;
    HTREEITEM         m_hTreeItem ;
    static TCHAR      m_szCompositeMismatch[CCH_PROPERTY_TEXT] ;
} ;

//-------------------------------------------------------------------------//
class CPropertyFolder  : public CPropertyTreeItem
//-------------------------------------------------------------------------//
{
public:
    CPropertyFolder( CPropertyTreeCtl* pCtl, const PROPFOLDERITEM& folder ) ;
    virtual ~CPropertyFolder() ;

    //  Overrides of CPropertyTreeItem:
    virtual BOOL       Initialize() ;
    virtual USHORT     Type() const     { return PIT_FOLDER ; }
    virtual ULONG      GetAccess() const { return m_folderitem.dwAccess ; }
    virtual HRESULT    AddSource( const VARIANT* pvarSrc, ULONG dwAccess, const PROPVARIANT* pvarVal = NULL ) ;
    virtual HRESULT    RemoveSource( const VARIANT* pvarSrc ) ;
    virtual LONG       SourceCount() const ;
    virtual BOOL       Reconcile( HWND hwndTree, LONG cMasterSourceCount ) ;
    virtual HTREEITEM  Insert( HWND hwndTree, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST ) ;
    virtual int        InsertChildren() ;
    virtual int        TierNumber() const  { return FOLDER_TIER ; }
    virtual LRESULT    OnTreeItemCallback( TV_DISPINFO* pDI, BOOL& bHandled ) ;
    virtual void       SetOrder( int i )    { m_folderitem.iOrder = i ; }
    virtual HTREEITEM  GetSortTarget( int iCol ) ;
    virtual int        Order() const        { return m_folderitem.iOrder ; }
    virtual BOOL       ChildSortPending() const { return m_fSortPending ; }
    virtual void       PendChildSort()          { m_fSortPending = TRUE ; } 
    virtual void       UnpendChildSort()        { m_fSortPending = FALSE ; } 

protected:
    PROPFOLDERITEM          m_folderitem ;
    CSourceDictionaryLite   m_srcs ;
    BOOL                    m_fSortPending ;
} ;

//-------------------------------------------------------------------------//
class CProperty  : public CPropertyTreeItem
//-------------------------------------------------------------------------//
{
public:
    CProperty( CPropertyTreeCtl* pCtl, const PROPERTYITEM& item ) ;
    virtual ~CProperty() ;

    virtual BOOL        Initialize() ;

    const CPropVariant& Value() const               { return m_varComposite ; }
    virtual void        SetDirty( BOOL bDirty ) ;
    virtual BOOL        IsDirty() const             { return m_bDirty ; }
    virtual HRESULT     Apply() ;
    
    //  Overrides of CPropertyTreeItem
public:
    virtual USHORT      Type() const      { return PIT_PROPERTY ; }
    virtual ULONG       GetAccess() const { return m_propitem.dwAccess ; }
    virtual BOOL        IsCompositeMismatch() const { return m_bCompositeMismatch ; }
    virtual HTREEITEM   Insert( HWND hwndTree, HTREEITEM hParent, HTREEITEM hInsertAfter = TVI_LAST ) ;
    virtual LRESULT     OnTreeItemCallback( TV_DISPINFO* pDI, BOOL& bHandled ) ;
    virtual void        Remove() ;
    virtual int         TierNumber() const { return PROPERTY_TIER ; }
    virtual HRESULT     AddSource( const VARIANT* pvarSrc, ULONG dwAccess, const PROPVARIANT* pvarVal ) ;
    virtual HRESULT     RemoveSource( const VARIANT* pvarSrc ) ;
    virtual LONG        SourceCount() const ;
    virtual BOOL        Reconcile( HWND hwndTree, LONG cMasterSourceCount ) ;
    virtual LPCTSTR     ValueText() ;            // returns the cached formatted value text
    virtual HRESULT     AssignValueFromText( BSTR bstrValue, BOOL bMakeDirty ) ;

    virtual HRESULT     InitializeSelectionValues()     { return S_OK ; }
    virtual HRESULT     AddSelectionValue( const PROPVARIANT* pvarVal, BSTR bstrDisplayText = NULL ) ;
    virtual LONG        SelectionValueCount() const ;

    virtual void        SetOrder( int i )    { m_propitem.iOrder = i ; }
    virtual int         Order() const        { return m_propitem.iOrder ; }

protected:
    virtual void        HitTest( LPTVHITTESTINFO pHTI ) ;
    virtual HWND        EditControl()  { return IsWindow( m_hwndEdit ) ? m_hwndEdit : NULL ; }
    virtual HWND        ShowEditControl( UINT nShowCmd, LPCRECT prcEdit, CMetrics* ) ;
    virtual void        RepositionEditControl( LPCRECT prcEdit, CMetrics* ) ;
    virtual HWND        CreateEditControl( LPCRECT prc, CMetrics* ) ;
    virtual void        PositionEditControl( HWND hwndCtl, LPCRECT prcItem, CMetrics* ) ;
    virtual void        InitializeEditControl( HWND hwndCtl, CMetrics* ) ;
    virtual LRESULT     OnEditControlCommand( UINT, UINT, HWND, CMetrics*, BOOL&) ;
    virtual BOOL        OnKillFocus( UINT, CPropertyTreeItem* ) ;
    virtual HRESULT     GetEditedValue( CPropVariant& varNew ) ;
    virtual HRESULT     ConvertEditedValue( LPCTSTR pszVal, CPropVariant& varNew );
    virtual BOOL        OnValueChanging( CPropVariant& varNew ) ;
    virtual void        OnValueChanged( const CPropVariant& varNew ) ;
    virtual BOOL        OnValueRestore() ;
    virtual void        MakeCompositeValue() ;
    virtual HRESULT     AddSelectionValue( CSelectionList&, const PROPVARIANT*, BSTR = NULL ) ;

    virtual LPCTSTR     ValueTextFormat() ;      // returns the formatting string for the property's value.
    virtual ULONG       ValueTextFormatFlags() ; // returns the formatting flags for the property's value.
    virtual LPCTSTR     FormatValueText( ) ;     // formats the value text
    virtual void        SetValueText( LPCTSTR pszText ) ;   // caches the formatted value text
    virtual BOOL        DrawValueText() ;        // draws the cached formatted value text

    virtual HRESULT     AssignValue( LPCTSTR pszValue, BOOL bMakeDirty = FALSE ) ;
    virtual HRESULT     AssignValue( IN const CPropVariant* pvarValue, IN OPTIONAL LPCTSTR pszValue = NULL, IN OPTIONAL BOOL bMakeDirty = NULL ) ;

    PROPERTYITEM        m_propitem ;
    CValueDictionary    m_values ;
    CPropVariant        m_varComposite ;
    LPTSTR              m_pszValue ;
    HWND                m_hwndEdit ;
    BOOL                m_bDirty,
                        m_bCompositeMismatch,
                        m_bCtlInitialized ;
} ;

//-------------------------------------------------------------------------//
class CComboBoxProperty  : public CProperty
//-------------------------------------------------------------------------//
{
public:
    CComboBoxProperty( CPropertyTreeCtl* pCtl, const PROPERTYITEM& propitem )
        : CProperty( pCtl, propitem ) {}

protected:
    virtual int InsertUniqueComboValue( HWND hwndCbo, int idx, CPropVariant* pVar, LPCTSTR pszText ) ;
    virtual int FindComboValue( HWND hwndCbo, const CPropVariant& var, ULONG uFlags ) const ;

    virtual LRESULT  OnEditControlCommand( UINT, UINT, HWND, CMetrics*, BOOL& ) ;
} ;

//-------------------------------------------------------------------------//
class CMruProperty  : public CComboBoxProperty
//-------------------------------------------------------------------------//
{
public:
    CMruProperty( CPropertyTreeCtl* pCtl, const PROPERTYITEM& propitem, int nLimit = 50 ) 
        :  CComboBoxProperty( pCtl, propitem ), m_nLimit( nLimit ) {}

    virtual  void  InitializeEditControl( HWND, CMetrics* ) ;

protected:
    virtual HRESULT InitializeSelectionValues() ;
    virtual HRESULT AddSelectionValue( const PROPVARIANT*, BSTR = 0L ) ;
    virtual LONG    SelectionValueCount() const  { return m_mru.Count() ; }
    virtual HRESULT AssignValue( IN const CPropVariant* pvarValue, IN OPTIONAL LPCTSTR pszValue = NULL, IN OPTIONAL BOOL bMakeDirty = NULL ) ;

    CSelectionList  m_mru ;
    int             m_nLimit ;
} ;

//-------------------------------------------------------------------------//
class CDateProperty : public CProperty
//-------------------------------------------------------------------------//
{
public:
    CDateProperty( CPropertyTreeCtl* pCtl, const PROPERTYITEM& propitem ) 
        : CProperty( pCtl, propitem ) {}

protected:
    virtual HRESULT ConvertEditedValue( LPCTSTR pszVal, CPropVariant& varNew );
    virtual void    InitializeEditControl( HWND hwndCtl, CMetrics* pMetrics ) ;
    virtual ULONG   ValueTextFormatFlags()    { return VAR_DATEVALUEONLY ; }
} ;

//-------------------------------------------------------------------------//
class CTextProperty : public CProperty
//-------------------------------------------------------------------------//
{
public:
    CTextProperty( CPropertyTreeCtl* pCtl, const PROPERTYITEM& propitem, int cchMax = 0xFF )
        : CProperty( pCtl, propitem ), m_cchMax( cchMax ) {}

    virtual BOOL IsMultiline() ;

protected:
    int m_cchMax ;

} ;

//-------------------------------------------------------------------------//
class CEnumProperty : public CComboBoxProperty
//-------------------------------------------------------------------------//
{
public:
    CEnumProperty( CPropertyTreeCtl* pCtl, const PROPERTYITEM& propitem ) :
        CComboBoxProperty( pCtl, propitem ) {}

    virtual void    InitializeEditControl( HWND, CMetrics* ) ;
    virtual BOOL    OnKillFocus( UINT, CPropertyTreeItem* ) ;
    virtual BOOL    OnValueChanging( CPropVariant& varNew ) ;
    virtual BOOL    OnValueRestore() ;
    virtual HRESULT AssignValue( IN const CPropVariant* pvarValue, IN OPTIONAL LPCTSTR pszValue = NULL, IN OPTIONAL BOOL bMakeDirty = NULL ) ;

protected:
    virtual HRESULT AddSelectionValue( const PROPVARIANT*, BSTR = 0L ) ;
    virtual LONG    SelectionValueCount() const  { return m_choices.Count() ; }
    virtual LPCTSTR FormatValueText( ) ;     // formats the value text


    CSelectionList  m_choices ;
} ;

//-------------------------------------------------------------------------//
//  Inline implementation - CPropertyTreeItem
inline CPropertyTreeItem::operator HTREEITEM() const { 
    return m_hTreeItem ; 
}
inline CPropertyTreeItem* CPropertyTreeItem::GetTreeItem( const TV_ITEM& tvi ) { 
    return (CPropertyTreeItem*)tvi.lParam ; 
}
inline CPropertyTreeCtl* CPropertyTreeItem::Control()   {
    return m_pCtl ;
}
inline USHORT CPropertyTreeItem::Type() const  {
    return PIT_NIL ;
}
inline LPCTSTR CPropertyTreeItem::GetName() const   { 
    return m_szText[iName] ; 
}
inline LPCTSTR CPropertyTreeItem::GetDisplayName() const  { 
    return m_szText[iDisplayName] ;
}
inline LPCTSTR CPropertyTreeItem::GetDescription() const  { 
    return m_szText[iDescr] ;
}
inline int CPropertyTreeItem::ItemWidth( BOOL bTextOnly ) const  {
    if( *m_szText[iDisplayName] == 0 ) return 0 ;
    return bTextOnly ? m_cxValue + m_xValue : m_cxValue - m_xLabel ;
}
inline LRESULT CPropertyTreeItem::OnEditControlCommand( UINT, UINT, HWND, CMetrics*, BOOL& bHandled )    {
    bHandled = FALSE ;
    return 0L ;
}
inline LRESULT CPropertyTreeItem::OnEditControlNotify( NMHDR*, CMetrics*, BOOL& bHandled )    {
    bHandled = FALSE ;
    return 0L ;
}
inline HRESULT CPropertyTreeItem::AddSource( const VARIANT*, ULONG, const PROPVARIANT* ) { 
    return E_NOTIMPL ; 
}
inline HRESULT CPropertyTreeItem::RemoveSource( const VARIANT* ) { 
    return E_NOTIMPL ;
}
inline LONG CPropertyTreeItem::SourceCount() const { 
    return 0 ;
}
inline BOOL CPropertyTreeItem::Reconcile( HWND, LONG )  {
    return FALSE ;
}
inline int CPropertyTreeItem::InsertChildren()  { 
    return 0 ;
}
inline void CPropertyTreeItem::Remove() {
    HWND hwndTree = TreeHwnd() ;
    if( hwndTree && TreeView_GetSelection( hwndTree )==m_hTreeItem && 
        IsWindowVisible( hwndTree ) )
        TreeView_SelectItem( hwndTree, NULL ) ;
    m_hTreeItem = NULL ;
}
inline HWND CPropertyTreeItem::ShowEditControl( UINT, LPCRECT, CMetrics* ) { 
    return NULL ; 
}
inline void CPropertyTreeItem::RepositionEditControl( LPCRECT, CMetrics* ) {
}
inline BOOL CPropertyTreeItem::OnKillFocus( UINT, CPropertyTreeItem* ) { 
    return TRUE ;
}
inline LPCTSTR CPropertyTreeItem::ValueText() { 
    return NULL ;
}
inline BOOL CPropertyTreeItem::OnValueChanging( CPropVariant& ) { 
    return TRUE ;
}
inline void CPropertyTreeItem::OnValueChanged( const CPropVariant& ) {
}
inline int* CPropertyTreeItem::ChildSortDir()  { 
    return m_iChildSortDir ;
}
inline BOOL CPropertyTreeItem::ChildSortPending() const { 
    return FALSE ;
}
inline BOOL CPropertyTreeItem::DrawValueText() { 
    return FALSE ;
}

#define ASSIGN_PROPITEM_TEXT(psz, i) \
    if((psz)) lstrcpyn( m_szText[(i)], (psz), CCH_PROPERTY_TEXT ) ; \
    else *m_szText[(i)] = (TCHAR)0

inline void CPropertyTreeItem::SetName( LPCTSTR pszName )   { 
    ASSIGN_PROPITEM_TEXT( pszName, iName ) ;
}
inline void CPropertyTreeItem::SetDisplayName( LPCTSTR pszDisplayName ) { 
    ASSIGN_PROPITEM_TEXT( pszDisplayName, iDisplayName ) ;
}
inline void CPropertyTreeItem::SetDescription( LPCTSTR pszDescr )   { 
    ASSIGN_PROPITEM_TEXT( pszDescr, iDescr ) ;
}
inline BOOL CPropertyTreeItem::IsMultiline() {
    return FALSE ;
}
inline HRESULT CProperty::AddSelectionValue( const PROPVARIANT*, BSTR ) {
    return S_OK ;
}
inline LONG CProperty::SelectionValueCount() const  {
    return 0L ;
}
inline LPCTSTR CProperty::ValueTextFormat()    {
    return NULL ;
}
inline ULONG CProperty::ValueTextFormatFlags() {
    return 0L ;
}
inline LPCTSTR CProperty::ValueText()           { 
    return m_pszValue ? m_pszValue : FormatValueText() ; 
}

#endif __TREEITEMS_H__
