//-------------------------------------------------------------------------//
//
//  metrics.h
//
//  Various metric attributes for CPropertyTreeCtl ActiveX control and
//  its subelements.
//
//-------------------------------------------------------------------------//

#ifndef __METRICS_H__
#define __METRICS_H__

//-------------------------------------------------------------------------//
class CMetrics
//-------------------------------------------------------------------------//
{
public:
    CMetrics( CWindow* pHostCtl ) ;
    
    //  Hosting control object
    CWindow*  HostControl()                 { return m_pHostCtl ; } 

    //  Raw metrics access methods.
    int  ItemIndent( int nTier ) const      { return (m_cxItemIndent * nTier) - (nTier - 1) ; }//  Horizontal starting distance from tree view left extreme to item starting point
    int  BestFitDividerPos() const          { return m_cxBestFitDividerPos ; } //  Best fit position of column divider
    int  DividerPos() const                 { return m_cxDividerPos ; }        //  Position of column divider
    int  DividerMargin() const              { return m_cxDividerMargin ; }     //  Distance to reserve to either side of column divider line
    int  EllipsisWidth( ) const             { return m_cxEllipsis ; }          //  Width of ellipsis text in tree view font
    int  MinColWidth() const                { return m_cxMinColWidth ; }       //  Minimum width permitted for a column
    int  ImageCellWidth() const             { return m_cxImageCellWidth ; }    //  Width of tree item image list cell.
    int  ImageMargin() const                { return m_cxImageMargin ; }       //  Distance between tree item image and text box.
    int  TextMargin() const                 { return m_cxTextMargin ; }        //  Distance between item text and its enclosing box.
    int  BorderWidth() const                { return m_nBorderWidth ; }        //  Width of border
    int  HeaderPtSize() const               { return m_ptsizeHeader ; }        //  Header control font point size.
    int  ComboEditBoxHeight() const         { return m_cyComboEditBox ; }
    int  ComboDropListHeight() const        { return m_cyComboDropList; }
    int  EmptyCaptionMargin() const         { return m_nEmptyCaptionMargin ; } //  Margin between empty prompt and tree view control boundaries.

    //  Calculated item metrics methods
    int  LabelTextLeft( int nTier ) const   { return LabelBoxLeft( nTier ) + TextMargin() ; }
    int  LabelTextRight( int cxText, int nTier ) const { return LabelTextLeft( nTier ) + cxText + TextMargin() ; } 
    int  LabelTextRightLimit() const        { return DividerPos() - DividerMargin() - TextMargin() ; }
    int  LabelBoxLeft( int nTier ) const    { return (ItemIndent(nTier)) + ImageCellWidth() + ImageMargin() ; }
    int  LabelBoxRight( int cxText, int nTier ) const  { return LabelTextLeft( nTier ) + cxText + TextMargin() ; }
    int  LabelBoxRightLimit() const         { return DividerPos() - DividerMargin() ; }
    int  CalcDividerPos( int cxText, int nTier ) const { return LabelBoxRight( cxText, nTier ) + DividerMargin() ; }

    int  ValueTextLeft() const              { return DividerPos() + DividerMargin() ; }
    int  ValueTextRight( int cxText ) const { return ValueTextLeft() + cxText + TextMargin() ; }

    //  Attribute assignment.
    int  SetItemIndent( int cx )            { m_cxItemIndent = cx ; return m_cxItemIndent ; }
    int  SetEllipsisWidth( int cx )         { m_cxEllipsis = cx ; return m_cxEllipsis ; }
    int  SetDividerPos( int cx )            { m_cxDividerPos = cx ; return m_cxDividerPos ; }
    int  SetBestFitDividerPos( int cxText, int nTier ) { m_cxBestFitDividerPos =  cxText ?
                                                  LabelBoxRight( cxText, nTier ) + DividerMargin() : 
                                                  m_cxDividerPos /*no op*/;
                                              return m_cxBestFitDividerPos ; }
    int  SetNoOpBestFitDividerPos( )        { m_cxBestFitDividerPos = DividerPos() ; 
                                              return m_cxBestFitDividerPos ; }
    int  SetComboEditBoxHeight( int cy )    { m_cyComboEditBox = cy ; return m_cyComboEditBox ; }

    //  GDI resources, etc.
    HFONT       HeaderFont() const          { return m_hfHdr ; }
    HFONT       FolderItemFont() const      { return m_hfFolderItem ; }
    HFONT       DirtyItemFont() const       { return m_hfDirty ; }
    HBRUSH      EmptyCaptionBrush() ;
    HRGN        ExclusionRgn() const        { return m_hrgnExclude ; }
    HIMAGELIST  TreeImageList() const       { return m_hilTree ; }
    HIMAGELIST  HdrImageList() const        { return m_hilHdr ; }
    
    HFONT       CreateHeaderFont( HFONT hfTree ) ;
    HFONT       CreateFolderItemFont( HFONT hfTree ) ;
    HFONT       CreateDirtyItemFont( HFONT hfTree ) ;
    HRGN        CreateItemExclusionRegion( LPCRECT prc ) ;
    HIMAGELIST  LoadTreeImageList() ;
    HIMAGELIST  LoadHdrImageList() ;
    void        DestroyResources() ;


private:
    int  m_cxItemIndent,        //  Horizontal starting distance from tree view left extreme to item starting point.
         m_cxBestFitDividerPos, //  Best fit position of column divider
         m_cxDividerPos,        //  Position of column divider
         m_cxEllipsis,          //  Width of ellipsis text in tree view font
         m_cxMinColWidth,       //  Minimum width permitted for a column
         m_cxImageCellWidth,    //  Width of tree item image list cell.
         m_cxImageMargin,       //  Distance between tree item image and text box.
         m_cxTextMargin,        //  Distance between item text and its enclosing box.
         m_cxDividerMargin,     //  Distance to reserve to either side of column divider line
         m_cyComboEditBox,      //  Combobox edit control height.
         m_cyComboDropList,     //  Height of combobox drop list.
         m_nBorderWidth,        //  Width of border
         m_ptsizeHeader,        //  Header control font point size.
         m_nEmptyCaptionMargin ;//  Margin between empty prompt and tree view control boundaries.

    HIMAGELIST m_hilTree,       //  Tree view image list
               m_hilHdr ;       //  Header image list
    HRGN       m_hrgnExclude ;  //  Exclusion region which protects vs. default painting 
    HFONT      m_hfHdr,         //  Header font
               m_hfFolderItem,  //  Treeview folder item font.
               m_hfDirty ;      //  Dirty value font
    HBRUSH     m_hbrEmptyCaption ;  // Background brush for empty caption window.

    CWindow*   m_pHostCtl ;
} ;


#endif