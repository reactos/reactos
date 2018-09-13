// Tfont.h : interface of the CTfont class
//           This class takes text entry from the user.
//           It is derived from the CEdit class
/***************************************************************************/

#ifndef __Tfont_H__
#define __Tfont_H__

// TOOLBAAR CONSTANTS -- Bitmap Positions and Toolbar Positions

#define MAX_TBAR_ITEMS        12 // positions 0 through 11

#define BOLD_BMP_POS          0
#define ITALIC_BMP_POS        1
#define UNDERLINE_BMP_POS     2


#define VERTEDIT_BMP_POS      3
#define PEN_BMP_POS           4
#define EDITTEXT_BMP_POS      5
#define KEYBOARD_BMP_POS      6   // still wasting space in image
#define INS_SPACE_BMP_POS     7
#define BACKSPACE_BMP_POS     8
#define NEWLINE_BMP_POS       9

#define SHADOW_BMP_POS        -1  // don't exist currently

#define BOLD_TBAR_POS         0
#define ITALIC_TBAR_POS       1
#define UNDERLINE_TBAR_POS    2


#define VERTEDIT_TBAR_POS     3
#define SPACE_ONE             4
#define PEN_TBAR_TEXT_POS     5

#define INS_SPACE_TBAR_POS    5
#define BACKSPACE_TBAR_POS    6
#define NEWLINE_TBAR_POS      7
#define SPACE_TWO             8
#define EDITTEXT_TBAR_POS     9
#define SPACE_THREE          10
#define PEN_TBAR_PEN_POS     11

#define SHADOW_TBAR_POS       -1  // don't exist currently
#define KEYBOARD_TBAR_POS     -1  // don't exist currently

#define FONT_BMP_TXT_BORDER   2   // # pixels between font bmp (prn/tt) and text

#define UM_DELAYED_TOOLBAR   WM_USER + 900

// definde font types used by m_iFontType in CTfont class
#define TT_FONT             0x0001
#define TT_OPENTYPE_FONT    0x0002
#define PS_OPENTYPE_FONT    0x0004
#define TYPE1_FONT          0x0008
#define DEVICE_FONT         0x0010
#define RASTER_FONT         0x0020

#define NumCPic         5

class CTedit;    // forward reference for change/undo notification
//class CAttrEdit; // forward reference for setfont,...notification

/******************************************************************************/

class CTfontTbar : public CToolBar
    {
    private:

    protected: // create from serialization only

    DECLARE_DYNCREATE(CTfontTbar)

    protected:

    //{{AFX_MSG(CTfontTbar)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
        //  afx_msg void OnInsertObject();  // OLE support
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    public:

    CTfontTbar(void);

    virtual ~CTfontTbar(void);
    BOOL    Create(CWnd* pcParentWnd, BOOL bShowPen = FALSE);
    };

/******************************************************************************/

class CTfontDlg : public CDialogBar
    {
    private:
    CPic          m_cPictures[NumCPic];
    int           m_Max_cx_FontType_BMP;
    void SetColorsInDC(HDC hdc, BOOL bInverted);
    protected: // create from serialization only

    DECLARE_DYNCREATE(CTfontDlg)

    protected:

    //{{AFX_MSG(CTfontDlg)
    afx_msg void OnRButtonDown ( UINT nFlags, CPoint point );
//
// MFC 4 - had to put the WM_DRAWITEM and WM_MEASUREITEM handlers here instead
// of CTfont. This dialog is the real parent of the owner-draw combobox, don't
// know how the old version worked with these handlers in CTfont.
//
    afx_msg void OnDrawItem    (int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnMeasureItem (int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
    afx_msg DWORD OnGetDefId   ( void );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    public:

    CTfontDlg(void);
    void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct, CString *pcStringText);
    virtual ~CTfontDlg(void);
    BOOL     Create(CWnd* pcParentWnd);
    };

/******************************************************************************/

class CCharSetDesc;
class CFontDesc;

/******************************************************************************/

class CTfont : public CMiniFrmWnd
    {
    private:

    int           m_iControlIDLastChange;
    int           m_iWeight;
    CFont         m_cCurrentFont;
    CString       m_cStrTypeFaceName;
    CString       m_cStrTypeFaceNamePrev;
    int           m_iPointSize;
    int           m_iPointSizePrev;

    CString       m_strFontName;
    int           m_iFontType;
    BYTE          m_nCharSet;
    BYTE          m_nCharSetPrev;

    BOOL          m_bBoldOn;
    BOOL          m_bItalicOn;
    BOOL          m_bUnderlineOn;

    BOOL          m_bVertEditOn;

    BOOL          m_bShadowOn;
    BOOL          m_bPenOn;

    CRect         m_cRectWindow;
    BOOL          m_bDisplayCText;
    BOOL          m_bInUpdate;

    class CTedit* m_pcTedit;
    CTfontDlg     m_cTfontDlg;
    CTfontTbar    *m_pcTfontTbar; // must be dynamic for changing buttons (delete/new)

    BYTE PickCharSet(CCharSetDesc *pCharSetDescList, int iCharSetSelection);

    void ResizeWindow(void);
    void ProcessNewTypeface(void);
    void UpdateEditControlFont(void);
    void FreeMemoryFromCBox(void);
    void OnTypeFaceComboBoxUpdate(void);
    void OnPointSizeComboBoxUpdate(void);

    void SaveToIniFile(void);
    void ReadFromIniFile(void);

    // was used for spin control to save point sizes,...
    //  CMapWordToPtr PointSizeMap;
    //  void EmptyMap(void);

    void RefreshFontList(void);

    int  EnumFontFace( ENUMLOGFONTEX*   lpEnumLogFont,
                       NEWTEXTMETRICEX* lpNewTextMetric,
                       int             iFontType );

    int  EnumFontSizes( LPENUMLOGFONT   lpEnumLogFont,
                        LPNEWTEXTMETRIC lpNewTextMetric,
                        int             iFontType );

    protected: // create from serialization only

    DECLARE_DYNCREATE(CTfont)

    //{{AFX_MSG(CTfont)
    afx_msg void OnSelendokTypeface();
    afx_msg void OnCloseupTypeface();
    afx_msg void OnSelendokPointSize();
    afx_msg void OnCloseupPointSize();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnDestroy();
    afx_msg void OnMove(int x, int y);
    afx_msg void OnClose();
    afx_msg void OnKillfocusPointsize();
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
        afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
        afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
        //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    public:

    CTfont();
    CTfont( CTedit *pcTedit );
    ~CTfont(void);

    CWnd* GetFontSizeControl() { return ((m_cTfontDlg.GetSafeHwnd() == NULL)? NULL:
                                          m_cTfontDlg.GetDlgItem( IDC_POINTSIZE ) ); }

    CWnd* GetFontFaceControl() { return ((m_cTfontDlg.GetSafeHwnd() == NULL)? NULL:
                                          m_cTfontDlg.GetDlgItem( IDC_TYPEFACE ) ); }

    //MY AFX_MSG(CTfont)
    afx_msg void OnBold      ( void );
    afx_msg void OnItalic    ( void );
    afx_msg void OnUnderline ( void );

    afx_msg void OnVertEdit  ( void );
    afx_msg void OnVertEditUpdate  ( CCmdUI* pCmdUI );

    afx_msg void OnShadow    ( void );
    afx_msg void OnPen       ( void );
    afx_msg long OnDelayedPen( WPARAM wParam, LPARAM lParam );
    afx_msg void OnEditText  ( void );
    afx_msg void OnKeyboard  ( void );
    afx_msg void OnInsSpace  ( void );
    afx_msg void OnBackSpace ( void );
    afx_msg void OnNewLine   ( void );
    //MY AFX_MSG

    BOOL IsBoldOn      ( void ) { return m_bBoldOn;      }
    BOOL IsItalicOn    ( void ) { return m_bItalicOn;    }
    BOOL IsUnderlineOn ( void ) { return m_bUnderlineOn; }

    BOOL IsVertEditOn  ( void ) { return m_bVertEditOn;  }

    BOOL IsShadowOn    ( void ) { return m_bShadowOn;    }
    BOOL Create        ( CRect rectEditArea );
    void Undo          ( void );
    void RefreshToolBar( void );
    void GetFontInfo   ( int iFontSelection, BYTE nCharSetSelection);

    static int CALLBACK EnumFontFaceProc(ENUMLOGFONTEX* lpEnumLogFont,
                                         NEWTEXTMETRICEX* lpNewTextMetric,
                                          int iFontType, LPARAM lParam);
    static int CALLBACK EnumFontOneFaceProc(LPENUMLOGFONT lpEnumLogFont,
                                            LPNEWTEXTMETRIC lpNewTextMetric,
                                            int iFontType, LPARAM lParam);

    virtual WORD GetHelpOffset() {return 0;} // for now just return 0

    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

    };

/***************************************************************************/

#endif // __Tfont_H__
