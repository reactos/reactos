#ifndef __IMGDLGS_H__
#define __IMGDLGS_H__

#define GRIDMIN     1           // Minimum grid coordinate value
#define GRIDMAX     1024        // Maximum grid coordinate value

// All App Studio dialog boxes should derive from this class...
//
class C3dDialog : public CDialog
    {
    public:

    C3dDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
    C3dDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL);

    virtual BOOL OnInitDialog();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnRobustOK();

    DECLARE_MESSAGE_MAP()
    };

/***************************************************************************/
// CColorTable dialog

class CColorTable : public CDialog
    {
    // Construction
    public:

    CColorTable(CWnd* pParent = NULL);    // standard constructor

	enum { IDD = IDD_COLORTABLE };

    void SetLeftFlag( BOOL bLeft) { m_bLeft = bLeft; }
    void SetColorIndex( int iColor ) { m_iColor = iColor; }
    int  GetColorIndex() { return m_iColor; }

    // Implementation

    protected:

    BOOL m_bLeft;
    int  m_iColor;

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CColorTable)
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    virtual BOOL OnInitDialog();
    afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
    afx_msg void OnDblclkColorlist();
    virtual void OnOK();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
    };

/***************************************************************************/

class CImgGridDlg : public C3dDialog
    {
    public:

    CImgGridDlg();

    BOOL OnInitDialog();
    void OnOK();
    void OnClickPixelGrid();
    void OnClickTileGrid();

    BOOL m_bPixelGrid;
    BOOL m_bTileGrid;
    int m_nWidth;
    int m_nHeight;

    DECLARE_MESSAGE_MAP()
    };

extern CSize NEAR g_defaultTileGridSize;
extern BOOL  NEAR g_bDefaultTileGrid;

#endif // __IMGDLGS_H__
