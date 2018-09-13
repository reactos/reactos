//-------------------------------------------------------------------------//
//
//  TreeItems.cpp
//
//-------------------------------------------------------------------------//

#include "pch.h"
#include "TreeItems.h"
#include "PropTree.h"
#include "Ctl.h"
#include "PTutil.h"

//-------------------------------------------------------------------------//
//  tree image list indices.
static enum PROPTREE_IMAGE_INDEX
{
    PTI_NULL,
    PTI_FOLDER,
    PTI_FOLDER_SEL,
    PTI_PROP_READONLY,
    PTI_PROP_READWRITE,
    PTI_MULTIPROP_READONLY,
    PTI_MULTIPROP_READWRITE,
};

//-------------------------------------------------------------------------//
//  CPropertyTreeItem - class implementation
//-------------------------------------------------------------------------//
TCHAR CPropertyTreeItem::m_szCompositeMismatch[] = TEXT("");

//-------------------------------------------------------------------------//
CPropertyTreeItem::CPropertyTreeItem( CPropertyTreeCtl* pCtl )
    :   m_pCtl( pCtl ),
        m_hTreeItem(NULL),
        m_xLabel(0), m_cxLabel(0),
        m_xValue(0), m_cxValue(0)
{
    ASSERT( pCtl );
    memset( m_szText, 0, sizeof(m_szText) );
    memset( m_iChildSortDir, 0, sizeof(m_iChildSortDir) );

    //  Load static string resources
    if( !*m_szCompositeMismatch )
        LoadString( GetResourceInstance(), IDS_COMPOSITE_MISMATCH, 
                    m_szCompositeMismatch, sizeof(m_szCompositeMismatch)/sizeof(TCHAR) );
}

//-------------------------------------------------------------------------//
HINSTANCE CPropertyTreeItem::GetResourceInstance()
{ 
    return _Module.GetResourceInstance();
}

//-------------------------------------------------------------------------//
HINSTANCE CPropertyTreeItem::GetModuleInstance()
{
    return _Module.GetModuleInstance();
}

//-------------------------------------------------------------------------//
const HWND& CPropertyTreeItem::TreeHwnd() const
{ 
    ASSERT( m_pCtl );
    return m_pCtl->TreeHwnd();
}

//-------------------------------------------------------------------------//
HTREEITEM CPropertyTreeItem::Insert( HWND hwndTree, TV_INSERTSTRUCT* pTVI )
{
    _ASSERT( m_hTreeItem==NULL );

    pTVI->item.mask   |= TVIF_PARAM;
    pTVI->item.lParam = (LPARAM)this;
    
    return (m_hTreeItem = TreeView_InsertItem( hwndTree, pTVI ));
}

//-------------------------------------------------------------------------//
CPropertyTreeItem* CPropertyTreeItem::GetNext( PROPTREE_ITEM_TYPE type, BOOL bExpand ) const
{
    CPropertyTreeItem*  pItem = NULL;
    HTREEITEM           hItem;
    TV_ITEM             tviSibling,
                        tviParent;

    memset( &tviSibling, 0, sizeof(tviSibling) );
    memset( &tviParent, 0, sizeof(tviParent) );

    tviSibling.mask      = 
    tviParent.mask       = TVIF_HANDLE|TVIF_PARAM|TVIF_STATE;
    tviSibling.stateMask = 
    tviParent.stateMask  = (DWORD)-1;

    for( hItem = (HTREEITEM)(*this); hItem; )
    {
        //  Walk to next parent
        if( (tviSibling.hItem = TreeView_GetNextSibling( TreeHwnd(), hItem ))==NULL )
        {
            if( (tviParent.hItem = TreeView_GetParent( TreeHwnd(), hItem ))==NULL )
                break;

            if( (tviSibling.hItem = TreeView_GetChild( TreeHwnd(), tviParent.hItem ))==NULL )
                break;
        }

        if( TreeView_GetItem( TreeHwnd(), &tviSibling ) && 
            (pItem = GetTreeItem( tviSibling )) && pItem->Type()==type )
            break;

        pItem = NULL;
        hItem = tviSibling.hItem;
    } 

    if( bExpand && pItem )
    {
        if( (tviParent.hItem = TreeView_GetParent( TreeHwnd(), (HTREEITEM)(*pItem) ))!=NULL &&
            TreeView_GetItem( TreeHwnd(), &tviParent ) &&
            (tviParent.state & TVIS_EXPANDED)==0 )

        TreeView_Expand( TreeHwnd(), tviParent.hItem, TVE_EXPAND );
    }

    return pItem;
}

//-------------------------------------------------------------------------//
CPropertyTreeItem* CPropertyTreeItem::GetPrev( PROPTREE_ITEM_TYPE type, BOOL bExpand ) const
{
    CPropertyTreeItem*  pItem = NULL;
    HTREEITEM           hItem;
    TV_ITEM             tviSibling,
                        tviParent;

    memset( &tviSibling, 0, sizeof(tviSibling) );
    memset( &tviParent, 0, sizeof(tviParent) );

    tviSibling.mask      = 
    tviParent.mask       = TVIF_HANDLE|TVIF_PARAM|TVIF_STATE;
    tviSibling.stateMask = 
    tviParent.stateMask  = (DWORD)-1;

    for( hItem = (HTREEITEM)(*this); hItem; )
    {
        if( (tviSibling.hItem = TreeView_GetPrevSibling( TreeHwnd(), hItem ))==NULL )
        {
            //  Walk to previous parent
            if( (tviParent.hItem = TreeView_GetParent( TreeHwnd(), hItem ))==NULL )
                break;

            //  Walk to last child
            tviSibling.hItem = NULL;
            for( HTREEITEM hChild = TreeView_GetChild( TreeHwnd(), tviParent.hItem );
                 hChild != NULL;
                 hChild = TreeView_GetNextSibling( TreeHwnd(), hChild ) )
            {
                tviSibling.hItem = hChild;
            }
            if( tviSibling.hItem == NULL )
                break;
        }

        if( TreeView_GetItem( TreeHwnd(), &tviSibling ) && 
            (pItem = GetTreeItem( tviSibling )) && pItem->Type()==type )
            break;

        pItem = NULL;
        hItem = tviSibling.hItem;
    } 

    if( bExpand && pItem )
    {
        if( (tviParent.hItem = TreeView_GetParent( TreeHwnd(), (HTREEITEM)(*pItem) ))!=NULL &&
            TreeView_GetItem( TreeHwnd(), &tviParent ) &&
            (tviParent.state & TVIS_EXPANDED)==0 )

        TreeView_Expand( TreeHwnd(), tviParent.hItem, TVE_EXPAND );
    }

    return pItem;
}

//-------------------------------------------------------------------------//
BOOL CPropertyTreeItem::UpdateChildCount( int cChildren )
{
    TV_ITEM tvi;
    tvi.mask = TVIF_CHILDREN|TVIF_HANDLE;
    tvi.hItem = (HTREEITEM)(*this);
    tvi.cChildren = cChildren;
    return TreeView_SetItem( TreeHwnd(), &tvi );
}

//-------------------------------------------------------------------------//
LRESULT CPropertyTreeItem::OnTreeItemCallback( TV_DISPINFO* pDI, BOOL& bHandled )
{
    if( pDI->item.mask & TVIF_TEXT )
    {
        // If we give the TreeView some text, he takes control of the
        // horizontal scroll bars, which we want to do ourselves.  So
        // we'll give him nothing!
        pDI->item.pszText    = TEXT("");
        pDI->item.cchTextMax = 0;

        // Note: derived class(es) will implement other attributes 
        // only if we leave bHandled FALSE.
        // So, we've completely handled the notification iif TVIF_TEXT
        // is the only attribute requested. 
        bHandled = (BOOL)(pDI->item.mask==TVIF_TEXT); 
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  String helper function; creates a single line representation of the indicated 
//  multi-line value. The return value, if non-NULL, is a buffer allocated with 
//  operator new(), and must be freed by the caller with operator delete().
LPTSTR CPropertyTreeItem::MultilineToSingleline( LPCTSTR pszValue )
{
    LPTSTR pszMultiline = NULL;
    int    cch = pszValue ? lstrlen(pszValue) : 0;

    if( (pszMultiline = new TCHAR[cch + 5])!=NULL )
    {
        lstrcpy( pszMultiline, pszValue );
        MakeSingleLine( pszMultiline, cch, cch + 5 );
    }
    return pszMultiline;
}

//-------------------------------------------------------------------------//
LRESULT CPropertyTreeItem::Draw( LPNMTVCUSTOMDRAW pDraw, CMetrics* pMetrics )
{
    LRESULT lRet = CDRF_DODEFAULT;
    
    //  Respond to tree view item prepaint.
    HDC         hdc          = pDraw->nmcd.hdc;
    RECT        rcLabel      = pDraw->nmcd.rc, // rect of label text text
                rcLabelBox   = pDraw->nmcd.rc, // rect of label box (i.e. for focus rect).
                rcLabelBkgnd = pDraw->nmcd.rc, // rect of label background.
                rcValue      = pDraw->nmcd.rc, // rect of value text
                rcValueBkgnd = pDraw->nmcd.rc; // rect of value background.
    LPCTSTR     pszLabel     = NULL ,          // label text
                pszValue     = NULL;          // value text
    LPTSTR      pszMultiline = NULL;
    int         cchLabel     = 0,              // length of label text
                cchValue     = 0;             // length of value text
    HWND        hwndFocus    = GetFocus();    
    BOOL        bWndFocus    = hwndFocus == TreeHwnd() || IsChild( TreeHwnd(), hwndFocus ),
                bSelected    = (pDraw->nmcd.uItemState & CDIS_SELECTED)!=0,
                bFocus       = (pDraw->nmcd.uItemState & CDIS_FOCUS)!=0,
                bValMismatch = IsCompositeMismatch(),
                bDrawValue   = DrawValueText(); // should we draw value text?
    HFONT       hf           = NULL,           // text font (NULL = use default)
                hfOld;                       
    SIZE        sizeText;                    // scratch var for GetTextExtentPoint().
    
    switch( Type() )
    {
        case PIT_PROPERTY:
        {
            //  Ultimate boundary for label drawing is the column divider.
            rcLabel.right = pMetrics->DividerPos();

            //  Establish 'dirty' property text attributes
            BOOL bDirty;
            if( (bDirty = ((CProperty*)this)->IsDirty()) )
            {
                // Draw dirty text in bold
                hf = pMetrics->DirtyItemFont(); 
            }
            break;
        }

        case PIT_FOLDER:
            // Draw folder text in bold underline.
            hf = pMetrics->FolderItemFont(); 
            break;

        default:
            return CDRF_DODEFAULT; // bail
    }

    //  Assign label text
    pszLabel = GetDisplayName();
    
    //  Assign value text
    if( bDrawValue )
    {
        pszValue = bValMismatch ? (LPTSTR)m_szCompositeMismatch : 
                                  (LPTSTR)ValueText(); 
        if( NULL == pszValue || *pszValue == (TCHAR)0 )
            bDrawValue = FALSE;
        else if( IsMultiline() )
        {
            pszMultiline = MultilineToSingleline( pszValue );
            if( (pszValue = (LPTSTR)pszMultiline) == NULL || *pszValue == (TCHAR)0 )
                bDrawValue = FALSE;
        }
    }

    //  Select the alternative font, if appropriate
    if( hf ) hfOld = (HFONT)SelectObject( hdc, hf );

    //  Calculate the label text rectangle
    cchLabel = lstrlen( pszLabel );
    GetTextExtentPoint32( hdc, pszLabel, cchLabel, &sizeText );
    rcLabel.left = pMetrics->LabelTextLeft( TierNumber() );
    rcLabel.right = min( pMetrics->LabelTextRightLimit(), 
                         pMetrics->LabelTextRight( sizeText.cx, TierNumber() ) );

    //  Calculate the label bkgnd and box rectangle
    rcLabelBox.left     = 
    rcLabelBkgnd.left   = pMetrics->LabelBoxLeft( TierNumber() );
    rcLabelBox.right    = min( pMetrics->LabelBoxRightLimit(), 
                               pMetrics->LabelBoxRight( sizeText.cx, TierNumber() ) );
    rcLabelBkgnd.right  = pMetrics->LabelTextRightLimit();

    //  Update horizontal item label metrics
    m_xLabel  = rcLabelBox.left;
    m_cxLabel = RECTWIDTH( &rcLabelBox );

    if( bDrawValue )
    {
        //  Calculate the value text rectangle
        cchValue = lstrlen( pszValue );
        GetTextExtentPoint32( hdc, pszValue, cchValue, &sizeText );
        rcValue.left      = 
        rcValueBkgnd.left = pMetrics->ValueTextLeft();
        rcValue.right     = pMetrics->ValueTextRight( sizeText.cx );

        //  Update cached horizontal item value metrics
        m_xValue  = rcValue.left - pMetrics->TextMargin();
        m_cxValue = RECTWIDTH( &rcValue ) + pMetrics->TextMargin() * 2;

        //if( cchValue )
        //    rcValue.right += ( (RECTWIDTH(&rcValue) / cchValue) * 2 );
    }
    else
    {
        //  Update cached horizontal item value metrics
        m_xValue = m_cxValue = 0;
    }

    //  Paint the label box and focus rectangle
    //  We want to draw the text in normal highlight even
    //  if the tree has lost focus to its child edit control.
    SetBkColor( hdc, GetSysColor( COLOR_WINDOW ) );
    ExtTextOut( hdc, rcLabelBkgnd.left, rcLabelBkgnd.top, ETO_OPAQUE,
                &rcLabelBkgnd, NULL, 0, NULL );
    if( bDrawValue )
        ExtTextOut( hdc, rcValueBkgnd.left, rcValueBkgnd.top, ETO_OPAQUE,
                    &rcValueBkgnd, NULL, 0, NULL );

    if( bWndFocus && bSelected )
    {
        SetBkColor  ( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
        SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
    }
    else
    {
        SetBkColor  ( hdc, GetSysColor( COLOR_WINDOW ) );
        SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );
    }

    ExtTextOut( hdc, rcLabelBox.left, rcLabelBox.top, ETO_OPAQUE,
                &rcLabelBox, NULL, 0, NULL );
    if( bSelected )
        DrawFocusRect( hdc, &rcLabelBox );

    //  Set the background paint mode to transparent
    int nOldMode = SetBkMode( hdc, TRANSPARENT );
    
    //  Paint the label text
    DrawText( hdc, pszLabel, cchLabel, &rcLabel, 
              DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX );

    if( bDrawValue )
    {
        //  Paint the value text
        COLORREF  rgbVal = GetSysColor( bValMismatch ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT ),
                  rgbTx = SetTextColor( hdc, rgbVal );
        DrawText( hdc, pszValue, cchValue, &rcValue, 
                  DT_LEFT|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX|DT_SINGLELINE );

        SetTextColor( hdc, rgbTx );
    }

    //  Cleanup...
    SetBkMode( hdc, nOldMode );            // Reset the background paint mode
    if( pszMultiline ) delete [] pszMultiline;
    if( hf ) SelectObject( hdc, hfOld );   // Restore font

    //      Set up an exclusion region to mask what we've done from
    //      tree view's default drawing
    rcLabel.right = pDraw->nmcd.rc.right;
    ExtSelectClipRgn( hdc, 
                      pMetrics->CreateItemExclusionRegion( &rcLabel ), 
                      RGN_DIFF );


    return CDRF_DODEFAULT; // let the tree view wndproc to the rest
}

//-------------------------------------------------------------------------//
BOOL CPropertyTreeItem::GetLabelRect( OUT LPRECT prc, BOOL fTextBox )
{
    HTREEITEM hItem;
    if( prc && (hItem = (HTREEITEM)(*this)) != NULL )
    {
        if( TreeView_GetItemRect( TreeHwnd(), hItem, prc, !fTextBox ) )
        {
            prc->left = m_xLabel;
            prc->right = m_xLabel + m_cxLabel;
            return TRUE;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CPropertyTreeItem::GetValueRect( OUT LPRECT prc, BOOL fTextBox )
{
    HTREEITEM hItem;
    if( prc && (hItem = (HTREEITEM)(*this)) != NULL )
    {
        if( TreeView_GetItemRect( TreeHwnd(), hItem, prc, !fTextBox ) )
        {
            prc->left = m_xValue;
            prc->right = m_xValue + m_cxValue;
            return TRUE;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
void CPropertyTreeItem::HitTest( LPTVHITTESTINFO pHTI )
{
    ASSERT( pHTI->hItem == (HTREEITEM)*this );
    
    //  These flags will always be inaccurate, so remove them and recalc
    pHTI->flags &= ~(TVHT_ONITEMLABEL|TVHT_ONITEMRIGHT);

    if( (pHTI->pt.x >= m_xLabel && pHTI->pt.x <= m_xLabel + m_cxLabel) ||
        (pHTI->pt.x >= m_xValue && pHTI->pt.x <= m_xValue + m_cxValue) )
    {
        pHTI->flags |= TVHT_ONITEMLABEL;
    }
    else if( pHTI->pt.x > m_xLabel + m_cxLabel &&
             pHTI->pt.x > m_xValue + m_cxValue )
    {
        pHTI->flags |= TVHT_ONITEMRIGHT;
    }
}

//-------------------------------------------------------------------------//
void CPropertyTreeItem::DisplayQtipText( HWND hwndQtip )
{
    LPCTSTR pszQtip = GetDescription();

    if( pszQtip && *pszQtip && 0==(GetAccess() & PTIA_WRITE) )
    {
        int    cchQtip = lstrlen( pszQtip ),
               cchBuf  = cchQtip + 64;
        LPTSTR pszBuf;

        if( NULL != (pszBuf = new TCHAR[cchBuf]) )
        {
            lstrcpy( pszBuf, pszQtip );
            LoadString( _Module.GetResourceInstance(), IDS_READONLY, &pszBuf[cchQtip],
                        cchBuf - cchQtip );
            SetWindowText( hwndQtip, pszBuf );
            delete [] pszBuf;
        }
    }
    else
        SetWindowText( hwndQtip, GetDescription() );
}

//-------------------------------------------------------------------------//
//  CPropertyFolder : public CPropertyTreeItem - class implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
CPropertyFolder::CPropertyFolder( CPropertyTreeCtl* pCtl, const PROPFOLDERITEM& folderitem )
    :   CPropertyTreeItem( pCtl ), 
        m_fSortPending(FALSE)
{
    ASSERT( IsValidPropFolderItem( &folderitem ) );
    InitPropFolderItem( &m_folderitem );
    CopyPropFolderItem( &m_folderitem, &folderitem );
}

//-------------------------------------------------------------------------//
CPropertyFolder::~CPropertyFolder()
{
    ClearPropFolderItem( &m_folderitem );
    m_srcs.Clear();
}

//-------------------------------------------------------------------------//
BOOL CPropertyFolder::Initialize()
{
    USES_CONVERSION;
    SetName( W2T( m_folderitem.bstrName ) );
    SetDisplayName( W2T( m_folderitem.bstrDisplayName ) );
    SetDescription( W2T( m_folderitem.bstrQtip ) );
    return TRUE;
}

//-------------------------------------------------------------------------//
HRESULT CPropertyFolder::AddSource( const VARIANT* pvarSrc, ULONG dwAccess, const PROPVARIANT* pvarVal )
{
    //  Folders don't care about values (note: pvarVal
    //  will always be NULL), but they do care about source for 
    //  multi-source reconciliation
    UNREFERENCED_PARAMETER( pvarVal );
    UNREFERENCED_PARAMETER( dwAccess );

    if( !pvarSrc )
        return E_POINTER;

    CComVariant* pvar  = NULL;

    if( (pvar = new CComVariant( *pvarSrc ))==NULL )
        return E_OUTOFMEMORY;
    
    m_srcs[pvar] = 0L;
    return S_OK;
}

//-------------------------------------------------------------------------//
HRESULT CPropertyFolder::RemoveSource( const VARIANT* pvarSrc )
{
    //  Folders don't really care about sources, 
    //  but they do care about a raw count for folder 
    //  reconciliation under multi-source conditions.
    CComVariant src( *pvarSrc );

    if( !pvarSrc ) return E_POINTER;
    if( !m_srcs.DeleteKey( &src ) )
        return E_FAIL;
    return S_OK;
}

//-------------------------------------------------------------------------//
LONG CPropertyFolder::SourceCount() const 
{ 
    return m_srcs.Count();
}

//-------------------------------------------------------------------------//
//  Shows or hides the folder tree item as appropriate under 
//  multi-source conditions.  Returns TRUE if the tree item was inserted, 
//  otherwise FALSE.
BOOL CPropertyFolder::Reconcile( HWND hwndTree, LONG cMasterSourceCount )
{
    BOOL bRet = FALSE;
    LONG cSrcs = m_srcs.Count();
    
    if( hwndTree )
    {
        HTREEITEM hThis = (HTREEITEM)(*this);
        if( cSrcs==0 || cSrcs < cMasterSourceCount )
        {
            if( NULL != hThis )
                TreeView_DeleteItem( hwndTree, hThis );
        }
        else if( cSrcs == cMasterSourceCount )
        {
            if( NULL == hThis )
                bRet = Insert( hwndTree, TVI_ROOT, TVI_LAST ) != NULL;
        }
#ifdef _DEBUG
        else
        {
            // m_cSrcs > cMasterSourceCount should never happen 
            // unless the master folder dictionary is leaking or something.
            ASSERT( FALSE ); 
        }
#endif _DEBUG

    }
    return bRet;
}

//-------------------------------------------------------------------------//
HTREEITEM CPropertyFolder::Insert( HWND hwndTree, HTREEITEM hParent, HTREEITEM hInsertAfter )
{
    TV_INSERTSTRUCT tvi;
    memset( &tvi, 0, sizeof(tvi) );
    tvi.hParent             = hParent;
    tvi.hInsertAfter        = hInsertAfter;
    tvi.item.pszText        = LPSTR_TEXTCALLBACK;
    tvi.item.mask           = TVIF_TEXT|TVIF_CHILDREN|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_STATE;
    tvi.item.stateMask      = 
    tvi.item.state          = TVIS_EXPANDED;   // folders are inserted in the expanded state.
    tvi.item.cChildren      = 1;               // we'll update this following reconciliation of child properties.
    tvi.item.iImage         = 
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    return CPropertyTreeItem::Insert( hwndTree, &tvi );
}

//-------------------------------------------------------------------------//
int CPropertyFolder::InsertChildren()
{

    int cnt = 0;
#if 0    

    //  Update child item count
    TV_ITEM       tvi;
    tvi.mask      = TVIF_CHILDREN|TVIF_HANDLE;
    tvi.hItem     = *this;
    tvi.cChildren = cnt;
    TreeView_SetItem( TreeHwnd(), &tvi );
#endif

    return cnt;        
}

//-------------------------------------------------------------------------//
LRESULT CPropertyFolder::OnTreeItemCallback( TV_DISPINFO* pDI, BOOL& bHandled )
{
    LRESULT lRet = CPropertyTreeItem::OnTreeItemCallback( pDI, bHandled );
    
    if( bHandled )
        return lRet;

    if( pDI->item.mask & TVIF_IMAGE || pDI->item.mask & TVIF_SELECTEDIMAGE )
    {
        pDI->item.iImage = 
        pDI->item.iSelectedImage = (pDI->item.state & TVIS_EXPANDED) ?
                                   PTI_FOLDER_SEL : PTI_FOLDER;
            
        bHandled = TRUE;
    }

    return 0L;
}

//-------------------------------------------------------------------------//
HTREEITEM  CPropertyFolder::GetSortTarget( int iCol )
{
    HTREEITEM hItemThis = (HTREEITEM)*this;
   
    if( 0 != iCol ) 
    {
        // if we're sorting a folder on anything but the label column, 
        // delegate the sort to children. (NT raid 307685).
        HTREEITEM hItemChild = TreeView_GetNextItem( TreeHwnd(), hItemThis, TVGN_CHILD ); 
        if( hItemChild != NULL )
            return hItemChild;
    }
    return hItemThis;
}

//-------------------------------------------------------------------------//
//  CProperty : public CPropertyTreeItem - class implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
CProperty::CProperty( CPropertyTreeCtl* pCtl, const PROPERTYITEM& propitem )
    :   CPropertyTreeItem( pCtl ),
        m_hwndEdit( NULL ),
        m_pszValue( NULL ),
        m_bDirty( FALSE ),
        m_bCtlInitialized( FALSE ),
        m_bCompositeMismatch(FALSE)
{
    ASSERT( IsValidPropertyItem( &propitem ) );
    InitPropertyItem( &m_propitem );
    CopyPropertyItem( &m_propitem, &propitem );
    m_varComposite.vt = m_propitem.puid.vt;
}

//-------------------------------------------------------------------------//
CProperty::~CProperty()
{
    ClearPropertyItem( &m_propitem );
    SetValueText( NULL );
    m_values.Clear();
}

//-------------------------------------------------------------------------//
void CProperty::SetValueText( LPCTSTR pszText )
{
    if( m_pszValue )
    {
        delete [] m_pszValue;
        m_pszValue = NULL;
    }

    if( pszText && *pszText )
    {
        int cch = lstrlen( pszText );
        if( (m_pszValue = new TCHAR[cch+1])==NULL )
            return;
        lstrcpy( m_pszValue, pszText );
    }
}

//-------------------------------------------------------------------------//
LPCTSTR CProperty::FormatValueText( )
{
    BSTR    bstrValue = NULL;
    
    SetValueText( NULL );  // empty cache

    if( SUCCEEDED( m_varComposite.GetDisplayText( 
            bstrValue, ValueTextFormat(), ValueTextFormatFlags() ) ) )
    {
        //  Update cache
        USES_CONVERSION;
        SetValueText( W2T( bstrValue ) );
        SysFreeString( bstrValue );
    }
    return m_pszValue;
}

//-------------------------------------------------------------------------//
//  Determines whether value text will be drawn to the tree control DC.
BOOL CProperty::DrawValueText()
{
    return m_hwndEdit == NULL; // text will be otherwise drawn 
                                // under the edit control
}

//-------------------------------------------------------------------------//
BOOL CProperty::Initialize()
{
    USES_CONVERSION;
    SetName( W2T( m_propitem.bstrName ) );
    SetDisplayName( W2T( m_propitem.bstrDisplayName ) );
    SetDescription( W2T( m_propitem.bstrQtip ) );
    m_varComposite.vt = m_propitem.puid.vt;

    return TRUE;    
}

//-------------------------------------------------------------------------//
HRESULT CProperty::Apply()
{
    HRESULT             hr, hrComposite;
    HANDLE              hEnum;
    BOOLEAN             bEnum = TRUE;
    LPCOMVARIANT        pvarSrc;
    LPCOMPROPVARIANT    pvarVal;
    int                 cValuesApplied = 0;

    hr = hrComposite = S_OK;
    
    ASSERT( IsDirty() );
    ASSERT( Control() );

    //  Enumerate each source, persisting new value.
    for( hEnum = m_values.EnumFirst( pvarSrc, pvarVal );
         hEnum && bEnum;
         bEnum = m_values.EnumNext( hEnum, pvarSrc, pvarVal ) )
    {
        PTSERVER server;
        
        if( FAILED( (hr = Control()->GetServerForSource( pvarSrc, server )) ) )
            hrComposite = hr;
        else
        {
            //  Make a copy of PROPERTYITEM representation
            PROPERTYITEM  propitem;
            InitPropertyItem( &propitem );
            CopyPropertyItem( &propitem, &m_propitem );
            propitem.lParam = server.lParamSrc; // make sure that the server is 
                                                 // handed the proper source param.

            //  Copy the proposed new value
            PropVariantCopy( &propitem.val, &m_varComposite );

            //  Ask server to persist the value
            if( FAILED( (hr = server.pServer->PersistAdvanced( &propitem )) ) )
                hrComposite = hr;
            else
                cValuesApplied++;
        
            //  Free resources
            ClearPropertyItem( &propitem );
        }
    }
    m_values.EndEnum( hEnum );

    ASSERT( cValuesApplied <= SourceCount() );
    SetDirty( cValuesApplied != SourceCount() );

    //  Redraw the item to update dirty/clean representation.
    RECT rcItem;
    if( GetLabelRect( &rcItem ) )
        InvalidateRect( TreeHwnd(), &rcItem, FALSE );
    if( GetValueRect( &rcItem ) )
        InvalidateRect( TreeHwnd(), &rcItem, FALSE );

#ifdef _DEBUG
    if( cValuesApplied != SourceCount() )
        ASSERT( FAILED( hrComposite ) );
#endif

    return hrComposite;
}

//-------------------------------------------------------------------------//
void CProperty::SetDirty( BOOL bDirty )
{
    BOOL bNotify = m_bDirty != bDirty;
    
    m_bDirty = bDirty; 
    if( Control() && bNotify )
    {
        BSTR     bstrFmtID = NULL;
        LPOLESTR pwszFmtID = NULL;
        LONG     cDirty, cDirtyVis;
        HRESULT  hr = Control()->GetDirtyCount( cDirty, cDirtyVis );

        //  Fire PropertyDirty event
        if( SUCCEEDED( StringFromCLSID( m_propitem.puid.fmtid, &pwszFmtID ) ) )
        {
            bstrFmtID = SysAllocString( pwszFmtID );
            CoTaskMemFree( pwszFmtID );

            Control()->Fire_PropertyDirty( 
                        bstrFmtID,
                        m_propitem.puid.propid,
                        m_propitem.puid.vt, 
                        (VARIANT_BOOL)bDirty );
        }
        
        //  Fire DirtyCountChanged event
        if( SUCCEEDED( hr ) )
            Control()->Fire_DirtyCountChanged( cDirty, cDirtyVis );
    }
}

//-------------------------------------------------------------------------//
HRESULT CProperty::AddSource( const VARIANT* pvarSrc, ULONG dwAccess, const PROPVARIANT* pvalSrc )
{
    if( !( pvarSrc && pvalSrc ) )
        return E_POINTER;

    CComVariant* pvar  = NULL;
    CPropVariant* pval = NULL;

    if( (pvar = new CComVariant( *pvarSrc ))==NULL )
        return E_OUTOFMEMORY;
    
    if( (pval = new CPropVariant( *pvalSrc ))==NULL )   {
        delete pvar;
        return E_OUTOFMEMORY;
    }

    //  Merge access rights, favoring most restrictive.
    if( dwAccess != m_propitem.dwAccess )
    {
        if( 0 == (dwAccess & PTIA_VISIBLE) )
            m_propitem.dwAccess &= ~PTIA_VISIBLE;
        if( 0 == (dwAccess & PTIA_WRITE) )
            m_propitem.dwAccess &= ~PTIA_WRITE;
    }

    m_values[pvar] = pval;
    MakeCompositeValue();
    return S_OK;
}

//-------------------------------------------------------------------------//
HRESULT CProperty::RemoveSource( const VARIANT* pvarSrc )
{
    if( !pvarSrc ) return E_POINTER;
    CComVariant varSrc( *pvarSrc );
    if( !m_values.DeleteKey( &varSrc ) )
        return E_FAIL;

    MakeCompositeValue();
    return S_OK;
}

//-------------------------------------------------------------------------//
LONG CProperty::SourceCount() const
{
    return m_values.Count();
}

//-------------------------------------------------------------------------//
//  Shows or hides the property tree item as appropriate under 
//  multi-source conditions.  Returns TRUE if the tree item was inserted, 
//  otherwise FALSE.
BOOL CProperty::Reconcile( HWND hwndTree, LONG cMasterSourceCount )
{
    BOOL bRet = FALSE;

    if( hwndTree )
    {
        CPropertyFolder* pParent = NULL;
        HTREEITEM        hThis   = (HTREEITEM)(*this),
                         hParent = NULL;
        LONG             cSrcs   = SourceCount();

        if( Control()->FolderDictionary().Lookup( m_propitem.pfid, pParent ) &&
            pParent!=NULL )
            hParent = (HTREEITEM)*pParent;

        if( cSrcs==0 || cSrcs < cMasterSourceCount || 
            0 == (m_propitem.dwAccess & PTIA_VISIBLE) )
        {
            if( hThis!=NULL )
            {
                if( TreeView_GetSelection( hwndTree )==hThis )
                    TreeView_SelectItem( hwndTree, NULL );
                TreeView_DeleteItem( hwndTree, hThis );
                hThis = NULL;
            }
        }
        else if( cSrcs == cMasterSourceCount )
        {
            if( hThis==NULL && hParent != NULL )
                if( (bRet = Insert( hwndTree, hParent, TVI_LAST )!=NULL) )
                    pParent->PendChildSort();
            
        }
#ifdef _DEBUG
        else
        {
            // m_cSrcs > cMasterSourceCount should never happen 
            // unless the master folder dictionary is leaking or something.
            ASSERT( FALSE ); 
        }
#endif _DEBUG
        
        if( IsWindow( m_hwndEdit ) && 
            (NULL == hThis || 0 == (m_propitem.dwAccess & PTIA_WRITE)) )
            ShowEditControl( SW_HIDE, NULL, NULL );
    }
    return bRet;
}

//-------------------------------------------------------------------------//
void CProperty::MakeCompositeValue()
{
    HANDLE  hEnum;
    BOOLEAN bEnum, bInit;    
    CComVariant*    pSrc;
    CPropVariant*   pVal, var;
    
    bInit = TRUE;
    m_bCompositeMismatch = FALSE;

    for( hEnum = m_values.EnumFirst( pSrc, pVal ), bEnum =TRUE;
         hEnum && bEnum;
         bEnum = m_values.EnumNext( hEnum, pSrc, pVal ) )
    {
        //  Assign first value to composite
        if( bInit ) {
            var = *pVal;
            bInit = FALSE;
        }
        else
        {
            //  If any other value differs from the first, 
            //  blank the composite and bail.
            if( var.Compare( *pVal, STRICT_COMPARE ) != 0 )
            {
                m_bCompositeMismatch = TRUE;
                var.Clear();
                break;
            }
        }
    }
    m_values.EndEnum( hEnum );
    
    var.vt = m_propitem.puid.vt;
    if( var.Compare( m_varComposite, STRICT_COMPARE ) != 0 )
    {
        USES_CONVERSION;

        m_varComposite = var;
        if( IsWindow( m_hwndEdit ) )
            SetWindowText( m_hwndEdit, FormatValueText() );
    }
}

//-------------------------------------------------------------------------//
HRESULT CProperty::AddSelectionValue( 
    CSelectionList& list, 
    const PROPVARIANT* pvarVal, 
    BSTR bstrDisplay )
{
    CDisplayPropVariant* pVar;
    HRESULT              hr = S_OK;

    if( !pvarVal ) 
        return E_POINTER;

    if( (pVar = new CDisplayPropVariant( *pvarVal ))==NULL )
        return E_OUTOFMEMORY;

    if( list.Lookup( pVar, NULL ) )
        delete pVar;
    else
    {
        if( bstrDisplay )
            pVar->SetDisplayText( bstrDisplay );
        else
            pVar->CreateDisplayText();
        
        if( !list.InsertHead( pVar ) )
            hr = E_UNEXPECTED;
    }
    
    return hr;
}

//-------------------------------------------------------------------------//
HTREEITEM CProperty::Insert( HWND hwndTree, HTREEITEM hParent, HTREEITEM hInsertAfter )
{
    TV_INSERTSTRUCT tvi;
    memset( &tvi, 0, sizeof(tvi) );
    tvi.hParent             = hParent;
    tvi.hInsertAfter        = hInsertAfter;
    tvi.item.mask           = TVIF_TEXT|TVIF_CHILDREN|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
    tvi.item.pszText        = LPSTR_TEXTCALLBACK;
    tvi.item.iImage         =
    tvi.item.iSelectedImage = I_IMAGECALLBACK;
    tvi.item.cChildren      = 0;

    return CPropertyTreeItem::Insert( hwndTree, &tvi );
}

//-------------------------------------------------------------------------//
void CProperty::Remove()
{
    if( m_hwndEdit )
        ShowWindow( m_hwndEdit, SW_HIDE );
    
    CPropertyTreeItem::Remove();
}

//-------------------------------------------------------------------------//
LRESULT CProperty::OnTreeItemCallback( TV_DISPINFO* pDI, BOOL& bHandled )
{
    LRESULT lRet = CPropertyTreeItem::OnTreeItemCallback( pDI, bHandled );
    
    if( bHandled )
        return lRet;

    if( pDI->item.mask & TVIF_IMAGE || pDI->item.mask & TVIF_SELECTEDIMAGE )
    {
        int cSources = SourceCount();

        if( (m_propitem.dwAccess & PTIA_WRITE)==0 ) {
            pDI->item.iImage = 
            pDI->item.iSelectedImage = 
                cSources > 1 ? PTI_MULTIPROP_READONLY : PTI_PROP_READONLY;
        }
        else    {
            pDI->item.iImage = 
            pDI->item.iSelectedImage = 
                cSources > 1 ? PTI_MULTIPROP_READWRITE : PTI_PROP_READWRITE;
        }
        bHandled = TRUE;
    }

    return 0L;
}

//-------------------------------------------------------------------------//
void CProperty::HitTest( LPTVHITTESTINFO pHTI )
{
    //  and call base class.
    CPropertyTreeItem::HitTest( pHTI );

    if( pHTI->flags & TVHT_ONITEMRIGHT )
    {
        pHTI->flags &= ~TVHT_ONITEMRIGHT;
        pHTI->flags |= TVHT_ONITEMLABEL;
    }
}

//-------------------------------------------------------------------------//
HWND CProperty::ShowEditControl( UINT nShowCmd, LPCRECT prcCtl, CMetrics* pMetrics )
{
    if( m_propitem.ctlID == 0 )
        return NULL;

    HWND hwndCtl = NULL, 
         hwndTree = TreeHwnd();

    if( 0 == (m_propitem.dwAccess & PTIA_WRITE) )
        nShowCmd = SW_HIDE;

    //  Create the control if necessary
    if( (hwndCtl = GetDlgItem( TreeHwnd(), MAKE_INPLACE_CTLID( m_propitem.ctlID ) ))==NULL )
        if( nShowCmd != SW_HIDE )
            hwndCtl = CreateEditControl( prcCtl, pMetrics );

    //  Show it as requested.
    if( IsWindow( hwndCtl ) )
    {
        if( nShowCmd != SW_HIDE /*&& (m_propitem.dwFlags & MPF_WRITE)*/ )
        {
            m_hwndEdit = hwndCtl; // cache the window handle

            //  Allow fine tuning of edit control
            PositionEditControl( hwndCtl, prcCtl, pMetrics );
            InitializeEditControl( hwndCtl, pMetrics );
            m_bCtlInitialized = TRUE;

            ShowWindow( hwndCtl, nShowCmd );
            UpdateWindow( hwndCtl );
        }

        if( nShowCmd == SW_HIDE )
        {
            ShowWindow( hwndCtl, nShowCmd );
            m_hwndEdit = NULL;     // uncache the window handle
            m_bCtlInitialized = FALSE;
        }
    }
    return hwndCtl;
}

//-------------------------------------------------------------------------//
typedef struct tagINPLACECTL_CREATEPARAMS
{
    UINT        nPTCtlID;
    LPCTSTR     pszClass;
    BOOL        fSubclassed;
    DWORD       dwStyle;
    DWORD       dwStyleEx;
    ULONG       Reserved;
} INPLACECTL_CREATEPARAMS, *PINPLACECTL_CREATEPARAMS;

//-------------------------------------------------------------------------//
static const INPLACECTL_CREATEPARAMS InPlaceCtlCreateParams[] = 
{
    { PTCTLID_SINGLELINE_EDIT,
      szEDITCLASS, TRUE,
      ES_AUTOHSCROLL|ES_LEFT, 0L },

    { PTCTLID_DROPDOWN_COMBO, 
      szCOMBOBOXCLASS, TRUE,
      CBS_DROPDOWN|CBS_AUTOHSCROLL, 0L },

    { PTCTLID_DROPLIST_COMBO,
      szCOMBOBOXCLASS, TRUE,
      CBS_DROPDOWNLIST|CBS_AUTOHSCROLL, 0L },

    { PTCTLID_MULTILINE_EDIT,
      szDROPWINDOWCLASS, FALSE,
      0L, WS_EX_CLIENTEDGE },

	{ PTCTLID_CALENDARTIME,
      szDROPWINDOWCLASS, FALSE,
      0L, WS_EX_CLIENTEDGE },

	{ PTCTLID_CALENDAR,
      szDROPWINDOWCLASS, FALSE,
      0L, WS_EX_CLIENTEDGE },

	{ PTCTLID_TIME,
      szDROPWINDOWCLASS, FALSE,
      0L, WS_EX_CLIENTEDGE },
};
#define INPLACECTL_CREATEPARAMS_COUNT \
    sizeof(InPlaceCtlCreateParams)/sizeof(INPLACECTL_CREATEPARAMS)

//-------------------------------------------------------------------------//
HWND CProperty::CreateEditControl( LPCRECT prc, CMetrics* pMetrics )
{
    ASSERT( m_propitem.ctlID != 0 );

    CInPlaceBase* pCtl     = NULL;    
    HWND          hwndCtl  = NULL,
                  hwndTree = TreeHwnd();

    //  Retrieve the tree view's font; we'll need it
    HFONT hfTree = (HFONT)SendMessage( hwndTree, WM_GETFONT, 0L, 0L );

    for( int i = 0; i < INPLACECTL_CREATEPARAMS_COUNT; i++ )
    {
        //  If its the control that we're assigned to, create it.
        if( InPlaceCtlCreateParams[i].nPTCtlID == m_propitem.ctlID )
        {
            //  Is this a subclassed implementation?
            if( InPlaceCtlCreateParams[i].fSubclassed )
            {
                //  Create the subclass target window
                if( (hwndCtl = CreateWindowEx( 
                                    InPlaceCtlCreateParams[i].dwStyleEx,
                                    InPlaceCtlCreateParams[i].pszClass, NULL,
                                    InPlaceCtlCreateParams[i].dwStyle | WS_CHILD,
                                    prc->left, prc->top, 
                                    RECTWIDTH( prc ), RECTHEIGHT( prc ), 
                                    hwndTree, (HMENU)MAKE_INPLACE_CTLID( m_propitem.ctlID ), 
                                    GetModuleInstance(), NULL ))!=NULL )
                {
                    SendMessage( hwndCtl, WM_SETFONT, (WPARAM)hfTree, 0L );
                    
                    //  Allocate the appropriate in-place edit window object
                    switch( m_propitem.ctlID )
                    {
                        //  Standard in-place edit controls:
                        case PTCTLID_SINGLELINE_EDIT:
	                    case PTCTLID_DROPDOWN_COMBO:
                            pCtl = new CInPlaceBase( hwndTree );
                            break;

	                    case PTCTLID_DROPLIST_COMBO:
                            pCtl = new CInPlaceDropList( hwndTree );
                            break;

                        default:
                            ASSERT( FALSE );
                    }
            
                    //  Subclass the target's edit control:
                    if( !pCtl->SubclassWindow( hwndCtl ) ) {
                        ASSERT( FALSE );
                        delete pCtl;
                    }
                    break;
                }
#ifdef TRACE
                else
                {
                    TRACE( TEXT("CProperty::CreateEditControl(1) FAILED CreateWindow( %ld ).\n"), GetLastError() );
                    TRACE( TEXT("styleEx: \t%08lXh\n"), InPlaceCtlCreateParams[i].dwStyleEx );
                    TRACE( TEXT("class: \t%s\n"), InPlaceCtlCreateParams[i].pszClass );
                    TRACE( TEXT("style: \t%08lXh\n"), InPlaceCtlCreateParams[i].dwStyle | WS_CHILD );
                    TRACE( TEXT("TreeHwnd(): \t%08lXh\n"), hwndTree );
                    TRACE( TEXT("DlgItemID: \t%d\n"), MAKE_INPLACE_CTLID( m_propitem.ctlID ) );
                    TRACE( TEXT("Instance: \t%08lXh\n"), GetModuleInstance() );
                    TRACE( TEXT("x: %d, y: %d, cx: %d, cy: %d\n\n"), prc->left, prc->top, RECTWIDTH( prc ), RECTHEIGHT( prc ) );
                }
#endif TRACE
            }
            else //  not a subclassed implementation.  
            {
                //  Allocate the appropriate in-place edit window object
                switch( m_propitem.ctlID )
                {
                    //  Specialized drop down controls: edit
                    case PTCTLID_MULTILINE_EDIT:
                        pCtl = new CInPlaceDropEdit( hwndTree );
                        break;

	                //  In-place drop-down calendar/time picker
                    case PTCTLID_CALENDARTIME:
	                case PTCTLID_CALENDAR:
	                case PTCTLID_TIME:
                        pCtl = new CInPlaceDropCalendar( hwndTree );
                        break;

                    default: 
                        ASSERT( FALSE ); // unknown PTCTLID_
                }

                //  Create the in-place edit window
                if( pCtl )
                {
                    RECT rc = *prc;
                    if( !pCtl->Create(
                                hwndTree, rc, NULL, 
                                InPlaceCtlCreateParams[i].dwStyle | WS_CHILD,
                                InPlaceCtlCreateParams[i].dwStyleEx,
                                MAKE_INPLACE_CTLID( m_propitem.ctlID ) ) )
                    {
                        ASSERT( FALSE );
                        delete pCtl;
                        break;
                    }                            
                    hwndCtl = pCtl->m_hWnd;
                    SendMessage( hwndCtl, WM_SETFONT, (WPARAM)hfTree, 0L );
                    break;
                }
            } // InPlaceCtlCreateParams[i].fSubclassed
        } // InPlaceCtlCreateParams[i].nPTCtlID == m_propitem.ctlID
    } // for
    return hwndCtl;
}

//-------------------------------------------------------------------------//
//  This member is invoked when something has happened in the tree control
//  that should cause the position of the property's edit control to be updated.
void CProperty::RepositionEditControl( LPCRECT prcCtl, CMetrics* pMetrics )
{
    HWND hwndCtl;

    if( (hwndCtl = GetDlgItem( TreeHwnd(), MAKE_INPLACE_CTLID( m_propitem.ctlID ) ))!=NULL )
    {
        PositionEditControl( hwndCtl, prcCtl, pMetrics );
        UpdateWindow( hwndCtl );
    }
}

//-------------------------------------------------------------------------//
void CProperty::PositionEditControl( HWND hwndCtl, LPCRECT prcCtl, CMetrics* pMetrics )
{
    switch( m_propitem.ctlID )
    {
        case PTCTLID_DROPDOWN_COMBO:
        case PTCTLID_DROPLIST_COMBO:
        {
            int cyComboEditBox = pMetrics->ComboEditBoxHeight(),
                cyCenterOffset = 0;   // amount of v-offset to center the control over
                                        // the item rect.

            //  Calculate the combo edit box height
            if( cyComboEditBox <= 0 )
            {
                RECT rcClient;
                GetClientRect( hwndCtl, &rcClient );
                cyComboEditBox =
                    pMetrics->SetComboEditBoxHeight( RECTHEIGHT( &rcClient ) );
            }

            cyCenterOffset = (RECTHEIGHT( prcCtl ) - cyComboEditBox)/2;

            SetWindowPos( hwndCtl, NULL, 
                          prcCtl->left, prcCtl->top + cyCenterOffset,
                          RECTWIDTH( prcCtl ), RECTHEIGHT( prcCtl ),
                          SWP_NOZORDER|SWP_NOACTIVATE );
            break;
        }

        default:
            SetWindowPos( hwndCtl, NULL, prcCtl->left, prcCtl->top,
                          RECTWIDTH( prcCtl ), RECTHEIGHT( prcCtl ),
                          SWP_NOZORDER|SWP_NOACTIVATE );
    }
}

//-------------------------------------------------------------------------//
LRESULT CProperty::OnEditControlCommand( 
    UINT nID, UINT nCode, HWND hwndEdit, CMetrics* pMetrics, BOOL& bHandled )
{
    LRESULT lRet = CPropertyTreeItem::OnEditControlCommand( nID, nCode, hwndEdit, pMetrics, bHandled );
    if( bHandled ) return lRet;

    if( !( nID == (UINT)MAKE_INPLACE_CTLID( m_propitem.ctlID ) && 
           hwndEdit == m_hwndEdit ) )
        return lRet;

    return lRet;
}

//-------------------------------------------------------------------------//
void CProperty::InitializeEditControl( HWND hwndCtl, CMetrics* pMetrics )
{
    SendMessage( hwndCtl, CB_RESETCONTENT, 0, 0 );
    SetWindowText( hwndCtl, ValueText() );
}

//-------------------------------------------------------------------------//
HRESULT CProperty::GetEditedValue( CPropVariant& var )
{
    HRESULT hr = E_FAIL;
    if( m_hwndEdit != NULL )
    {
        int     cchValue = GetWindowTextLength( m_hwndEdit )+1;
        LPTSTR  pszValue = NULL;

        if( (pszValue = new TCHAR[cchValue])==NULL )
            return E_OUTOFMEMORY;

        GetWindowText( m_hwndEdit, pszValue, cchValue );
        hr = ConvertEditedValue( pszValue, var );
        
        delete [] pszValue;
    }
    return hr;
}

//-------------------------------------------------------------------------//
HRESULT CProperty::ConvertEditedValue( LPCTSTR pszVal, CPropVariant& varNew )
{
    varNew.SetType( m_propitem.puid.vt );
    return varNew.AssignFromDisplayText( pszVal, NULL );
}

//-------------------------------------------------------------------------//
//  When focus is lost, we must evaluate whether a change was made to the
//  value of the property, and if so, store this as our new composite value
//  and set our dirty bit.
BOOL CProperty::OnKillFocus( UINT uAction, CPropertyTreeItem* pItemNew )
{
    CPropVariant varNew;
    BOOL         bRet = CPropertyTreeItem::OnKillFocus( uAction, pItemNew );
    
    //  If we can acquire a new value from the edit control
    if( SUCCEEDED( GetEditedValue( varNew ) ) )
    {
        //  And the value is different from the current value
        if( m_varComposite.Compare( varNew, STRICT_COMPARE )!=0 )
        {
            //  Validate and act on new value
            if( (bRet = OnValueChanging( varNew )) )
                OnValueChanged( varNew );
        }
    }
    return bRet;
}

//-------------------------------------------------------------------------//
//  Overrideable to perform data validation.  If the new value is acceptable,
//  this base class implementation should be called in order to 
//  assign the new value to the property.
BOOL CProperty::OnValueChanging( CPropVariant& varNew )
{
    BSTR    bstrValue;
    USES_CONVERSION;

    HRESULT hrAlloc = varNew.GetDisplayText( 
                            bstrValue, ValueTextFormat(), ValueTextFormatFlags() ),
            hrRet   = AssignValue( &varNew, W2T( bstrValue ), TRUE );
    
    if( SUCCEEDED( hrAlloc ) )
        SysFreeString( bstrValue );

    return SUCCEEDED( hrRet );
}

//-------------------------------------------------------------------------//
//  Invoked when property value is implicitly assigned
HRESULT CProperty::AssignValueFromText( BSTR bstrValue, BOOL bMakeDirty )
{
    CPropVariant varNew;
    varNew.vt = m_varComposite.vt;

    HRESULT hr;
    if( SUCCEEDED( (hr = varNew.AssignFromDisplayText( bstrValue )) ) )
    {
        if( OnValueChanging( varNew ) )
        {
            OnValueChanged( varNew );

            //  Revoke dirty flag if so desired.
            if( IsDirty() && !bMakeDirty )
                m_bDirty = FALSE;
        }
        else
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Assigns a property value given only a display string
HRESULT CProperty::AssignValue( LPCTSTR pszValue, BOOL bMakeDirty ) 
{
    HRESULT hr = E_FAIL;

    CPropVariant varValue;
    varValue.SetType( m_varComposite.Type() );

    if( FAILED( (hr = varValue.AssignFromDisplayText( pszValue )) ) )
        return hr;

    return AssignValue( &varValue, pszValue, bMakeDirty );
}

//-------------------------------------------------------------------------//
//  Performs work of assigning a property value given a CPropVariant instance
//  containing the new value.
HRESULT CProperty::AssignValue( IN const CPropVariant* pvarValue, IN OPTIONAL LPCTSTR pszValue, IN OPTIONAL BOOL bMakeDirty ) 
{
    HRESULT hr = E_FAIL;

    ASSERT( pvarValue );

    //  Make sure the value is the right data type.
    if( m_varComposite.Type() != pvarValue->Type() )
        return OLE_E_CANTCONVERT;

    //  Assign the value.
    m_varComposite = *pvarValue;
    m_bCompositeMismatch = FALSE;
    hr = S_OK; // past the hump

    //  Cache the display text
    if( pszValue )
        SetValueText( pszValue );
    else
    {
        BSTR bstrValue = NULL;
        USES_CONVERSION;

        if( SUCCEEDED( (hr = m_varComposite.GetDisplayText( 
                bstrValue, ValueTextFormat(), ValueTextFormatFlags() )) ) )
        {
            SetValueText( W2T( bstrValue ) );
            SysFreeString( bstrValue );
        }
        else
            SetValueText( NULL );
    }
    SetDirty( bMakeDirty );

    return hr;
}

//-------------------------------------------------------------------------//
void CProperty::OnValueChanged( const CPropVariant& varNew )
{
}

//-------------------------------------------------------------------------//
//  Restores the property value selected in the control to the item's composite value
//  after user has canceled changes.  The default behavior is to restore the
//  edit control's display text, which will result in an evalation of 'unchanged'
//  upon losing focus.
BOOL CProperty::OnValueRestore()
{
    //  User pressed 'ESC' during editing.  Simply restore the edit
    //  control's text to current value.
    BSTR bstrValue;

    if( m_hwndEdit && SUCCEEDED( m_varComposite.GetDisplayText( 
            bstrValue, ValueTextFormat(), ValueTextFormatFlags() ) ) )
    {
        USES_CONVERSION;
        SetWindowText( m_hwndEdit, W2T( bstrValue ) );
        InvalidateRect( m_hwndEdit, NULL, FALSE );
        SysFreeString( bstrValue );
    }
        
    return TRUE;   // handled.
}

//-------------------------------------------------------------------------//
//  CComboBoxProperty : public CProperty - class implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
LRESULT CComboBoxProperty::OnEditControlCommand( 
    UINT nID, UINT nCode, HWND hwndCtl, CMetrics* pMetrics, BOOL& bHandled )
{
    LRESULT lRet = CProperty::OnEditControlCommand( nID, nCode, hwndCtl, pMetrics, bHandled );
    if( bHandled ) return lRet;

    switch( nCode )
    {
        case CBN_DROPDOWN:
        case CBN_CLOSEUP:
        {
            RECT rcCtl;

            //  If we have items in the combo, resize the drop listbox.
            if( ::SendMessage( hwndCtl, CB_GETCOUNT, 0, 0L ) > 0 )
            {
                GetWindowRect( hwndCtl, &rcCtl );
                MapWindowPoints( HWND_DESKTOP, TreeHwnd(), (LPPOINT)&rcCtl, 2 );

                //  If we're dropping, inflate drop height; if we're closing up,
                //  it's height should be zero.
                if( nCode==CBN_DROPDOWN ) 
                    rcCtl.bottom += pMetrics->ComboDropListHeight();

                SetWindowPos( hwndCtl, NULL, 0, 0,
                              RECTWIDTH( &rcCtl ),
                              RECTHEIGHT( &rcCtl ),
                              SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
            }
            break;
        }

        case CBN_SELENDCANCEL:
            if( SendMessage( hwndCtl, CB_GETDROPPEDSTATE, 0, 0L ) )
            {
                TRACE(TEXT("Combo selection canceled\n"));
                OnValueRestore();
            }
            break;
        
        case CBN_SELENDOK:
        {
            
            CPropVariant  *pCurSel;
            INT_PTR       idx;

            if( (idx = SendMessage( hwndCtl, CB_GETCURSEL, 0, 0L ))!=CB_ERR )
            {
                if( (pCurSel = (CPropVariant*)SendMessage( hwndCtl, CB_GETITEMDATA, idx, 0L ))!= 
                               (CPropVariant*)CB_ERR )
                {
                    // Create a stack instance, because the selected one may be deleted.
                    CPropVariant varNew( *pCurSel );  
                    
                    if( m_varComposite.Compare( varNew, STRICT_COMPARE )!=0 )
                    {
                        if( OnValueChanging( varNew ) )
                            OnValueChanged( varNew );
                    }
                }
            }
            break;
        }
    }
    return lRet;
}

//-------------------------------------------------------------------------//
int CComboBoxProperty::InsertUniqueComboValue( HWND hwndCbo, int idx, CPropVariant* pVar, LPCTSTR pszText )
{
    int i;
    if( pVar && (i = FindComboValue( hwndCbo, *pVar, LAX_COMPARE ))!=CB_ERR )
        return i;

    if( (pszText != NULL ) )
    {
        //  Found a text match.
        if( (i = ComboBox_FindStringExact( hwndCbo, 0, pszText )!=CB_ERR) )
            return i;
        
        if( (idx = ComboBox_InsertString( hwndCbo, idx, pszText ))!=CB_ERR )
        {
            ComboBox_SetItemData( hwndCbo, idx, pVar );
            return idx;
        }
    }
    
    return CB_ERR;
}

//-------------------------------------------------------------------------//
int CComboBoxProperty::FindComboValue( HWND hwndCbo, const CPropVariant& var, ULONG uFlags ) const
{
    int i, cnt = ComboBox_GetCount( hwndCbo );
    
    for( i=0; i<cnt; i++ )
    {
        CPropVariant* pvarOther;
        if( (pvarOther = (CPropVariant*)ComboBox_GetItemData( hwndCbo, i ))!=NULL )
        {
            if( var.Compare( *pvarOther, uFlags )==0 )
            {
                return i;
            }
        }
    }
    return CB_ERR;
}

//-------------------------------------------------------------------------//
//  CMruProperty : public CComboBoxProperty - class implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
HRESULT CMruProperty::InitializeSelectionValues()
{
    //  Take this opportunity to add a blank value to the MRU list.
    if( 0 == m_mru.Count() )
    {
        PROPVARIANT varBlank;
        PropVariantInit( &varBlank );
        varBlank.vt = m_propitem.puid.vt;
        
        return CProperty::AddSelectionValue( m_mru, &varBlank, NULL );
    }
    return S_OK;
}

//-------------------------------------------------------------------------//
HRESULT CMruProperty::AddSelectionValue( const PROPVARIANT* pvarVal, BSTR bstrDisplay )
{
    return CProperty::AddSelectionValue( m_mru, pvarVal, bstrDisplay );
}

//-------------------------------------------------------------------------//
void CMruProperty::InitializeEditControl( HWND hwndCtl, CMetrics* pMetrics )
{
    CDisplayPropVariant*  pvarMru = NULL;
    LPTSTR                pszText = NULL;
    USES_CONVERSION;
    HANDLE                hEnum;
    BOOL                  bEnum;

    CComboBoxProperty::InitializeEditControl( hwndCtl, pMetrics );

    //  Insert current value into combo box
    if( (pszText = (LPTSTR)ValueText()) && *pszText )
        InsertUniqueComboValue( hwndCtl, 0, &m_varComposite, pszText );

    //  Add MRU values to combo box
    for( hEnum = m_mru.EnumHead( pvarMru ), bEnum = TRUE;
         hEnum && bEnum;
         bEnum = m_mru.EnumNext( hEnum, pvarMru ) )
    {
        BSTR bstrDisplay = pvarMru->DisplayText();
        InsertUniqueComboValue( hwndCtl, -1, pvarMru, 
                                bstrDisplay ? W2T( bstrDisplay ) : TEXT("") );
    }
    m_mru.EndEnum( hEnum );
}

//-------------------------------------------------------------------------//
//  Pushes a new value onto the MRU list, and updates the combo box if it 
//  the property has the focus.
HRESULT CMruProperty::AssignValue( IN const CPropVariant* pvarValue, IN OPTIONAL LPCTSTR pszValue, IN OPTIONAL BOOL bMakeDirty ) 
{
    HRESULT hr;

    //  First call the base class, who will do all the basic work in updating our
    //  data members, etc.
    if( SUCCEEDED( (hr = CComboBoxProperty::AssignValue( pvarValue, pszValue, bMakeDirty )) ) )
    {
        CDisplayPropVariant *pvarMruAlloc, 
                            *pvarMru = NULL;
        USES_CONVERSION;
        
        //  Prepend the new item to the MRU list.
        if( (pvarMruAlloc = new CDisplayPropVariant( *pvarValue ))==NULL )
            return E_OUTOFMEMORY;
        pvarMru = NULL;

        //  Determine if the item's value is already in the property's MRU list.
        if( m_mru.Lookup( pvarMruAlloc, &pvarMru ) )
        {
            delete pvarMruAlloc;    // don't need it, already have it.
            
            //  Find the value in the combo box and select it.
            if( m_hwndEdit )
            {
                int idx, cnt = CB_ERR;
                for( idx = 0, cnt = (int)SendMessage( m_hwndEdit, CB_GETCOUNT, 0, 0L );
                     idx < cnt; idx++ )
                {
                    PDISPLAYPROPVARIANT pvarMruItem = 
                        (PDISPLAYPROPVARIANT)SendMessage( m_hwndEdit, CB_GETITEMDATA, idx, 0L );
                    
                    if( pvarMru == pvarMruItem )
                    {
                        SendMessage( m_hwndEdit, CB_SETCURSEL, idx, 0L );
                        break;
                    }
                }
            }
        }
        else
        {
            //  Assign display text to item.
            pvarMruAlloc->SetDisplayTextT( ValueText() );

            //  Add to our list of selection values.
            m_mru.InsertHead( pvarMruAlloc );

            //  Prepend the new value to the combo box and select it.
            if( m_hwndEdit )
            {
                INT_PTR idx;
                if( (idx = SendMessage( m_hwndEdit, CB_INSERTSTRING, 0, (LPARAM)ValueText() ))!= CB_ERR &&
                    SendMessage( m_hwndEdit, CB_SETITEMDATA, idx, (LPARAM)pvarMruAlloc ) != CB_ERR )
                {
                    INT_PTR iCurSel;
                    if( (iCurSel = SendMessage( m_hwndEdit, CB_GETCURSEL, 0, 0L ))!= idx )
                        SendMessage( m_hwndEdit, CB_SETCURSEL, idx, 0L );
                }
            }
        }
    }

    return hr;
}

//-------------------------------------------------------------------------//
HRESULT CEnumProperty::AddSelectionValue( const PROPVARIANT* pvarVal, BSTR bstrDisplay )
{
    return CProperty::AddSelectionValue( m_choices, pvarVal, bstrDisplay );
}

//-------------------------------------------------------------------------//
LPCTSTR CEnumProperty::FormatValueText( )
{
    CDisplayPropVariant*  pvarEnum = NULL;
    HANDLE                hEnum;
    BOOL                  bEnum;

    for( hEnum = m_choices.EnumHead( pvarEnum ), bEnum = TRUE;
         hEnum && bEnum;
         bEnum = m_choices.EnumNext( hEnum, pvarEnum ) )
    {
        if( 0 == m_varComposite.Compare( *pvarEnum, STRICT_COMPARE ) )
        {
            USES_CONVERSION;
            SetValueText( W2T( pvarEnum->DisplayText() ) );
            return ValueText();
        }
    }
    return CComboBoxProperty::FormatValueText();
}

//-------------------------------------------------------------------------//
void CEnumProperty::InitializeEditControl( HWND hwndCtl, CMetrics* pMetrics )
{
    CDisplayPropVariant*  pvarEnum = NULL;
    HANDLE                hEnum;
    BOOL                  bEnum;

    CComboBoxProperty::InitializeEditControl( hwndCtl, pMetrics );

    //  Add MRU values to combo box
    for( hEnum = m_choices.EnumHead( pvarEnum ), bEnum = TRUE;
         hEnum && bEnum;
         bEnum = m_choices.EnumNext( hEnum, pvarEnum ) )
    {
        BSTR bstrDisplay;
        INT_PTR idx;
        USES_CONVERSION;
        
        if( (bstrDisplay = pvarEnum->DisplayText())!=NULL )
        {
            idx = SendMessage( hwndCtl, CB_INSERTSTRING, -1, (LPARAM)W2T( bstrDisplay ) );

            if( idx != CB_ERR )
            {
                SendMessage( hwndCtl, CB_SETITEMDATA, idx, (LPARAM)pvarEnum );

                //  If the choice is equivalent to the property's current value, select it!
                if( 0 == pvarEnum->Compare( m_varComposite, STRICT_COMPARE ) )
                    SendMessage( hwndCtl, CB_SETCURSEL, idx, 0L );
            }
        }
    }
    m_choices.EndEnum( hEnum );
}

//-------------------------------------------------------------------------//
//  Restores the property value selected in the control to the item's composite value
//  after user has canceled changes.
//  This task differs for an enumerated property from
//  other CProperty derivatives, which simply restore their edit control's display text;
//  a CBS_DROPDOWNLIST combo requires that the selected item be restored explicitly.
BOOL CEnumProperty::OnValueRestore()
{
    INT_PTR i, max;

    for( i=0, max = SendMessage( m_hwndEdit, CB_GETCOUNT, 0, 0 ); i<max; i++ )
    {
        CDisplayPropVariant* pVar;
        if( (pVar = (CDisplayPropVariant*)::SendMessage( m_hwndEdit, CB_GETITEMDATA, i, 0 )) != 
                    (CDisplayPropVariant*)CB_ERR && pVar != NULL )
        {
            if( m_varComposite.Compare( *pVar, STRICT_COMPARE )==0 )
                return SendMessage( m_hwndEdit, CB_SETCURSEL, i, 0L ) == i;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CEnumProperty::OnKillFocus( UINT uAction, CPropertyTreeItem* pItemNew )
{
    INT_PTR idx;
    BOOL    bRet = TRUE;

    if( (idx = SendMessage( m_hwndEdit, CB_GETCURSEL, 0, 0L ))!=CB_ERR )
    {
        CPropVariant* pVar;
        pVar = (CPropVariant*)SendMessage( m_hwndEdit, CB_GETITEMDATA, idx, 0L );
        
        if( pVar != (CPropVariant*)CB_ERR && pVar != NULL && 
            m_varComposite.Compare( *pVar, STRICT_COMPARE )!=0 )
        {
            CPropVariant varNew( *pVar );
            if( (bRet = OnValueChanging( varNew )) )
                OnValueChanged( varNew );
        }
    }
    return bRet;
}

//-------------------------------------------------------------------------//
BOOL CEnumProperty::OnValueChanging( CPropVariant& varNew )
{
    return CComboBoxProperty::OnValueChanging( varNew );
}

//-------------------------------------------------------------------------//
HRESULT CEnumProperty::AssignValue( IN const CPropVariant* pvarValue, IN OPTIONAL LPCTSTR pszValue, IN OPTIONAL BOOL bMakeDirty )
{
    HRESULT hr;
    if( SUCCEEDED( (hr = CComboBoxProperty::AssignValue( pvarValue, pszValue, bMakeDirty )) ) )
        FormatValueText();
    return hr;
}

//-------------------------------------------------------------------------//
void CDateProperty::InitializeEditControl( HWND hwndCtl, CMetrics* pMetrics )
{
    SYSTEMTIME st;
    BOOL       bConverted = FALSE;

    memset( &st, 0, sizeof(st) );

    switch( m_varComposite.vt )
    {
        case VT_FILETIME:
            bConverted = ( m_varComposite.filetime.dwLowDateTime == 0L && 
                           m_varComposite.filetime.dwHighDateTime == 0L ) ?
                         TRUE : FileTimeToSystemTime( &m_varComposite.filetime, &st );
            break;

        case VT_DATE:
            bConverted =  m_varComposite.date ? 
                          VariantTimeToSystemTime( m_varComposite.date, &st ) : TRUE;
            break;
    }
    if( bConverted )
    {
        SendMessage( hwndCtl, WMU_SETITEMDATA, 0L, (LPARAM)&st );
        SetWindowText( hwndCtl, ValueText() );
    }
}

//-------------------------------------------------------------------------//
HRESULT CDateProperty::ConvertEditedValue( LPCTSTR pszVal, CPropVariant& varNew )
{
    HRESULT hr = CProperty::ConvertEditedValue( pszVal, varNew );
    if( pszVal && *pszVal && SUCCEEDED( hr ) )
        PropVariantMakeTimeless( &varNew );

    return hr;
}

//-------------------------------------------------------------------------//
BOOL CTextProperty::IsMultiline()
{
    LPCTSTR pszValue = ValueText();
    for( int i = 0, cch = pszValue ? lstrlen( pszValue ) : 0;
         i < cch; i++ )
    {
        if( pszValue[i] == TEXT('\r') || pszValue[i] == TEXT('\n') )
            return TRUE;
    }
    return FALSE;
}
