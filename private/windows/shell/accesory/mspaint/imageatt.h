// imageatt.h : header file
//
#include "imgdlgs.h"

typedef enum
    {
        ePIXELS = 0,
        eINCHES = 1,
        eCM     = 2
    } eUNITS;

/*************************** CImageAttr dialog *****************************/

class CImageAttr : public CDialog
    {
    // Construction
    public:

    CImageAttr(CWnd* pParent = NULL);   // standard constructor

    void SetWidthHeight(UINT nWidthPixels, UINT nHeightPixels);
    CSize GetWidthHeight(void);
    // Dialog Data
    //{{AFX_DATA(CImageAttr)
        enum { IDD = IDD_IMAGE_ATTRIBUTES };
        CString m_cStringWidth;
        CString m_cStringHeight;
        //}}AFX_DATA

    BOOL   m_bMonochrome;

    // Implementation
    protected:
    eUNITS m_eUnitsCurrent;
    BOOL   bEditFieldModified;

    LONG   m_lHeightPixels;
    LONG   m_lWidthPixels;
    LONG   m_lHeight;
    LONG   m_lWidth;

    COLORREF m_crTrans;
    void PaintTransBox ( COLORREF  );

    void FixedFloatPtToString( CString& sString, LONG lFixedFloatPt );
    LONG StringToFixedFloatPt( CString& sString );
    void ConvertWidthHeight( void );
    void PelsToCurrentUnit( void );
    void SetNewUnits( eUNITS NewUnit );

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

        virtual LONG OnHelp(WPARAM wParam, LPARAM lParam);
        virtual LONG OnContextMenu(WPARAM wParam, LPARAM lParam);

    // Generated message map functions
    //{{AFX_MSG(CImageAttr)
        virtual BOOL OnInitDialog();
        virtual void OnOK();
        afx_msg void OnInches();
        afx_msg void OnCentimeters();
        afx_msg void OnPixels();
        afx_msg void OnChangeHeight();
        afx_msg void OnChangeWidth();
        afx_msg void OnDefault();
        afx_msg void OnUseTrans();
        afx_msg void OnSelectColor ();
        afx_msg void OnPaint();
        //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    CString ReformatSizeString (DWORD dwNumber);
    };

/*************************** CZoomViewDlg dialog ***************************/

class CZoomViewDlg : public CDialog
    {
    // Construction
    public:

    CZoomViewDlg(CWnd* pParent = NULL); // standard constructor

    // Dialog Data

    UINT m_nCurrent;

    //{{AFX_DATA(CZoomViewDlg)
        enum { IDD = IDD_VIEW_ZOOM };
        //}}AFX_DATA

    // Implementation
    protected:

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

        virtual LONG OnHelp(WPARAM wParam, LPARAM lParam);
        virtual LONG OnContextMenu(WPARAM wParam, LPARAM lParam);

    // Generated message map functions
    //{{AFX_MSG(CZoomViewDlg)
        virtual BOOL OnInitDialog();
        virtual void OnOK();
        //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    };

/************************* CFlipRotateDlg dialog ***************************/

class CFlipRotateDlg : public CDialog
    {
    // Construction
    public:

    CFlipRotateDlg(CWnd* pParent = NULL);       // standard constructor

    // Dialog Data

    BOOL m_bHorz;
    BOOL m_bAngle;
    UINT m_nAngle;

    //{{AFX_DATA(CFlipRotateDlg)
    enum { IDD = IDD_FLIP_ROTATE };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    // Implementation
    protected:

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

        virtual LONG OnHelp(WPARAM wParam, LPARAM lParam);
        virtual LONG OnContextMenu(WPARAM wParam, LPARAM lParam);

    // Generated message map functions
    //{{AFX_MSG(CFlipRotateDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
        afx_msg void OnByAngle();
        afx_msg void OnNotByAngle();
        //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    };

/********************** CStretchSkewDlg dialog *****************************/

class CStretchSkewDlg : public CDialog
    {
    // Construction
    public:

    CStretchSkewDlg(CWnd* pParent = NULL);      // standard constructor

    // Check that the direction was specified and subtract 100 to make it
    // equivalent to the 0 based system
 //   GetStretchHorz() { return(m_bStretchHorz ? m_iStretchHorz - 100 : 0); }
 //   GetStretchVert() { return(m_bStretchHorz ? 0 : m_iStretchVert - 100); }
    int GetStretchHorz() {return (m_iStretchHorz-100);}
    int GetStretchVert() {return (m_iStretchVert-100);}


    // Check that the direction was specified
  //  GetSkewHorz() { return(m_bSkewHorz ? m_wSkewHorz : 0); }
   // GetSkewVert() { return(m_bSkewHorz ? 0 : m_wSkewVert); }
    int GetSkewHorz() { return(m_wSkewHorz); }
    int GetSkewVert() { return(m_wSkewVert); }

    private:

    // Dialog Data
    //{{AFX_DATA(CStretchSkewDlg)
        enum { IDD = IDD_STRETCH_SKEW };

    int    m_wSkewHorz;
    int    m_wSkewVert;
    int     m_iStretchVert;
    int     m_iStretchHorz;
        //}}AFX_DATA

    // Implementation
    protected:

    BOOL    m_bStretchHorz;
    BOOL    m_bSkewHorz;

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

        virtual LONG OnHelp(WPARAM wParam, LPARAM lParam);
        virtual LONG OnContextMenu(WPARAM wParam, LPARAM lParam);

    // Generated message map functions
    //{{AFX_MSG(CStretchSkewDlg)
    // TODO
    // these are commented out of the message map. delete them?
    virtual void OnOK();
    virtual BOOL OnInitDialog();
        afx_msg void OnSkewHorz();
        afx_msg void OnSkewVert();
        afx_msg void OnStretchHorz();
        afx_msg void OnStretchVert();
        //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
    };

/***************************************************************************/
