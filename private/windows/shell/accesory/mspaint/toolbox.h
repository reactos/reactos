// toolbox.h : Declares the class interfaces for the toolbox window class.

#ifndef __TOOLBOX_H__
#define __TOOLBOX_H__

#define TM_TOOLDOWN     (WM_USER+0x0010)
#define TM_TOOLUP       (WM_USER+0x0011)
#define TM_TOOLDBLCLK   (WM_USER+0x0012)
#define TM_QUERYDROP    (WM_USER+0x0013)
#define TM_DROP         (WM_USER+0x0014)
#define TM_ABORTDROP    (WM_USER+0x0015)

#define TF_DISABLED     0x8000
#define TF_GRAYED       TF_DISABLED
#define TF_SELECTED     0x4000
#define TF_DOWN         TF_SELECTED
#define TF_DRAG         0x2000
#define TF_NYI          0x9000     // this represents a NYI tool (note that
                                   // TF_NYI implies TF_DISABLED)

#define TS_DEFAULT      0xC000
#define TS_STICKY       0x4000
#define TS_DRAG         0x2000
#define TS_CMD          0x1000
#define TS_VB           0x0800
#define TS_WELL         0x0400

#define NUM_TOOLS_WIDE  2

class CToolboxWnd;

#ifdef CUSTOMFLOAT
class CImageWell;
#else //!CUSTOMFLOAT
#include "imgwell.h"
#include "imgcolor.h"
#endif

/////////////////////////////////////////////////////////////////////////////

// CTool:
// A CTool is a thin-window button which can be inserted in a CToolboxWnd.
// Note that the tool is "owned" by a separate window, which is notified
// directly when the tool is used (pushed, dragged, unpushed, etc.).  The
// CToolboxWnd sends TM_* messages to the owning window.
//
// The graphics are completely calculated from the single bitmap given to
// the tool upon creation.  The pushed, disabled and unpushed states are
// drawn from the bitmap, which should be a two-color image without any
// chiseling button effects in it.  The graphic is centered in the button.
//
// For buttons defined with the TS_DRAG style, a cursor ID may be specified
// for the can't-drop state.  If not specified, the generic slashed-O
// cursor is used.
//
/******************************************************************************/

class CTool : public CObject
    {
    public: /*****************************************************************/
    CToolboxWnd* m_pOwner;
    WORD         m_wID;
    int          m_nImage; // index into parent's image well

    WORD         m_wState;
    WORD         m_wStyle;

    CTool(CToolboxWnd* pOwner, WORD wID, int nImage,
                      WORD wStyle = 0, WORD wState = 0);
    };

/******************************************************************************/
// CToolboxWnd:
// This is a typical mini-frame window, filled with an array of special
// buttons of the CTool class (above).  Direct access to this CObArray is
// allowed with the GetTools member function.
//
// After directly manipulating the tool array (adding, removing or modifying
// tools), use the Invalidate member function to repaint the window with the
// new state.
//
/******************************************************************************/

#ifdef CUSTOMFLOAT
class CDocking;
#endif

class CToolboxWnd : public CControlBar
    {
    DECLARE_DYNAMIC(CToolboxWnd)

    private:    /**************************************************************/

    CBitmap*    m_bmStuck;
    CBitmap*    m_bmPushed;
    CBitmap*    m_bmPopped;
    CTool*      m_tCapture;
    BOOL        m_bInside;
    CRect       m_lasttool;
    HCURSOR     m_oldcursor;

    CObArray*   m_Tools;
    CPoint      m_downpt;            // "click down point" for drag debounce -gh

#ifdef CUSTOMFLOAT
    CDocking*   m_pDocking;
#endif

    CTool* ToolFromPoint(CRect* rect, CPoint* pt);
    void   SizeByButtons(int nButtons = -1, BOOL bRepaint = FALSE);
    BOOL   DrawStockBitmaps();

    protected:  /**************************************************************/
    WORD        m_wWide;
    CPoint     m_btnsize;
    CImageWell m_imageWell;
    CRect      m_rcTools;
    int        m_nOffsetX;

    public:     /**************************************************************/

    static const POINT NEAR ptDefButton;

    CToolboxWnd();
    ~CToolboxWnd();

    virtual BOOL Create(const TCHAR FAR* lpWindowName,
                        DWORD dwStyle, const RECT& rect,
                        const POINT& btnsize = ptDefButton, WORD wWide = 1,
                        CWnd* pParentWnd = NULL, int nImageWellID = 0);
    virtual BOOL OnCommand(UINT wParam, LONG lParam);
    virtual UINT OnCmdHitTest ( CPoint point, CPoint* pCenter );

    int  HitTestToolTip( CPoint point, UINT* pHit );

    void AddTool(CTool* tool);
    void RemoveTool(CTool* tool);
    WORD SetToolState(WORD wID, WORD wState);
    WORD SetToolStyle(WORD wID, WORD wStyle);
    void SelectTool(WORD wid);
    WORD CurrentToolID();
    CTool* GetTool(WORD wID);
    void DrawButtons(CDC& dc, RECT* rcPaint);

    inline int GetToolCount() { return (int)m_Tools->GetSize(); }
    inline CTool* GetToolAt(int nTool) { return (CTool*)m_Tools->GetAt(nTool); }

    void CancelDrag();

    afx_msg void OnSysColorChange();
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT wFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT wFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT wFlags, CPoint point);
    afx_msg void OnMouseMove(UINT wFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT wFlags, CPoint point);
    afx_msg void OnClose();
    afx_msg void OnWinIniChange(LPCTSTR lpSection);
    afx_msg void OnKeyDown(UINT, UINT, UINT);
    afx_msg LONG OnToolDown(UINT wID, LONG lParam);
    afx_msg LONG OnToolUp(UINT wID, LONG lParam);
    afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
//  afx_msg LONG OnSwitch(UINT wID, LONG point);

//  virtual BOOL BeginDragDrop( CTool* pTool, CPoint pt );

    DECLARE_MESSAGE_MAP()
    };

/******************************************************************************/

class CImgToolWnd : public CToolboxWnd
    {
    public:     /**************************************************************/

    CRect        m_rcBrushes;

    virtual BOOL Create(const TCHAR* pWindowName, DWORD dwStyle,
                        const RECT& rect, const POINT& btnSize, WORD wWide,
                        CWnd* pParentWnd, BOOL bDkRegister = TRUE);
    virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);

    BOOL PreTranslateMessage(MSG* pMsg);

    afx_msg void  OnSysColorChange();
    afx_msg BOOL  OnEraseBkgnd(CDC* pDC);
    afx_msg void  OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void  OnLButtonDblClk(UINT nFlags, CPoint pt);
    afx_msg void  OnRButtonDown(UINT nFlags, CPoint pt);
    afx_msg void  OnPaint();
    afx_msg UINT  OnNcHitTest(CPoint point);

    virtual CSize GetSize();
    virtual WORD  GetHelpOffset() { return ID_WND_GRAPHIC; }
    virtual void  OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

    void InvalidateOptions(BOOL bErase = TRUE);
    friend class CImgColorsWnd;

    DECLARE_MESSAGE_MAP();
    };


/******************************************************************************/

#ifdef CUSTOMFLOAT
class CFloatImgToolWnd : public CMiniFrmWnd
    {
    DECLARE_DYNAMIC(CFloatImgToolWnd)

    public:     /**************************************************************/

    virtual ~CFloatImgToolWnd(void);
    virtual BOOL Create(const TCHAR* pWindowName, DWORD dwStyle,
                        const RECT& rect, const POINT& btnSize, WORD wWide,
                        CWnd* pParentWnd, BOOL bDkRegister = TRUE);
    virtual WORD GetHelpOffset() { return ID_WND_GRAPHIC; }
    afx_msg void OnSysColorChange();
    afx_msg void OnClose();

    DECLARE_MESSAGE_MAP()

    };
#endif //CUSTOMFLOAT

/***************************************************************************/

extern CImgToolWnd* NEAR g_pImgToolWnd;

#endif // __TOOLBOX_H__
