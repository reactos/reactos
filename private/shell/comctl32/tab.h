
typedef struct { // ti
    RECT rc;        // for hit testing and drawing
    int iImage;     // image index
    int xLabel;     // position of the text for drawing (relative to rc)
    int yLabel;     // (relative to rc)
    int cxLabel;    // width of the label.  this is needed if we're drawing in vertical mode
    
    int xImage;     // Position of the icon for drawing (relative to rc)
    int yImage;
    int iRow;           // what row is it in?
    LPTSTR pszText;
    
    DWORD dwState;
    
#if defined(WINDOWS_ME)
    UINT etoRtlReading;
#endif
    
    union {
        LPARAM lParam;
        BYTE   abExtra[1];
    }DUMMYUNIONNAME;
} TABITEM, FAR *LPTABITEM;

typedef struct {
    CONTROLINFO ci;
    
    HWND hwndArrows;    // Hwnd Arrows.
    HDPA hdpa;          // item array structure
    UINT flags;         // TCF_ values (internal state bits)
    int  cbExtra;       // extra bytes allocated for each item
    DWORD dwStyleEx;    // set by TCM_SETEXTENDEDSTYLE
    HFONT hfontLabel;   // font to use for labels
    int iSel;           // index of currently-focused item
    int iNewSel;        // index of next potential selection

    int cxItem;         // width of all tabs
    int cxMinTab;       // width of minimum tab
    int cyTabs;         // height of a row of tabs
    int cxTabs;     // The right hand edge where tabs can be painted.

    int cxyArrows;      // width and height to draw arrows
    int iFirstVisible;  // the index of the first visible item.
                        // wont fit and we need to scroll.
    int iLastVisible;   // Which one was the last one we displayed?

    int cxPad;           // Padding space between edges and text/image
    int cyPad;           // should be a multiple of c?Edge

    int iTabWidth;      // size of each tab in fixed width mode
    int iTabHeight;     // settable size of each tab
    int iLastRow;       // number of the last row.
    int iLastTopRow;    // the number of the last row that's on top (SCROLLOPPOSITE mode)

    int cyText;         // where to put the text vertically
    int cyIcon;         // where to put the icon vertically

    HIMAGELIST himl;    // images,
    HWND hwndToolTips;
#if defined(FE_IME) || !defined(WINNT)
    HIMC hPrevImc;      // previous input context handle
#endif

    HDRAGPROXY hDragProxy;
    DWORD dwDragDelay;  // delay for auto page-change during drag
    int iDragTab;       // last tab dragged over

    int tmHeight;    // text metric height
    BOOL fMinTabSet:1;  // have they set the minimum tab width
    BOOL fTrackSet:1;
    
    int iHot; 
} TC, NEAR *PTC;

#ifndef TCS_MULTISELECT 
#define TCS_MULTISELECT  0x0004
#endif

#define HASIMAGE(ptc, pitem) (ptc->himl && pitem->iImage != -1)

// tab control flag values
#define TCF_FOCUSED     0x0001
#define TCF_MOUSEDOWN   0x0002
#define TCF_DRAWSUNKEN  0x0004
#define TCF_REDRAW      0x0010  /* Value from WM_SETREDRAW message */
#define TCF_BUTTONS     0x0020  /* draw using buttons instead of tabs */

#define TCF_FONTSET     0x0040  /* if this is set, they set the font */
#define TCF_FONTCREATED 0x0080  

#define ID_ARROWS       1

#define TAB_DRAGDELAY   500

// Some helper macros for checking some of the flags...
#define Tab_RedrawEnabled(ptc)          (ptc->flags & TCF_REDRAW)
#define Tab_Count(ptc)                  DPA_GetPtrCount((ptc)->hdpa)
#define Tab_GetItemPtr(ptc, i)          ((LPTABITEM)DPA_GetPtr((ptc)->hdpa, (i)))
#define Tab_FastGetItemPtr(ptc, i)      ((LPTABITEM)DPA_FastGetPtr((ptc)->hdpa, (i)))
#define Tab_IsItemOnBottom(ptc, pitem)  ((BOOL)pitem->iRow > ptc->iLastTopRow)
#define Tab_DrawSunken(ptc)             ((BOOL)(ptc)->flags & TCF_DRAWSUNKEN)

#define Tab_DrawButtons(ptc)            ((BOOL)(ptc->ci.style & TCS_BUTTONS))
#define Tab_MultiLine(ptc)              ((BOOL)(ptc->ci.style & TCS_MULTILINE))
#define Tab_RaggedRight(ptc)            ((BOOL)(ptc->ci.style & TCS_RAGGEDRIGHT))
#define Tab_FixedWidth(ptc)             ((BOOL)(ptc->ci.style & TCS_FIXEDWIDTH))
#define Tab_Vertical(ptc)               ((BOOL)(ptc->ci.style & TCS_VERTICAL))
#define Tab_Bottom(ptc)                 ((BOOL)(ptc->ci.style & TCS_BOTTOM))
#define Tab_ScrollOpposite(ptc)        ((BOOL)(ptc->ci.style & TCS_SCROLLOPPOSITE))
#define Tab_ForceLabelLeft(ptc)         ((BOOL)(ptc->ci.style & TCS_FORCELABELLEFT))
#define Tab_ForceIconLeft(ptc)          ((BOOL)(ptc->ci.style & TCS_FORCEICONLEFT))
#define Tab_FocusOnButtonDown(ptc)      ((BOOL)(ptc->ci.style & TCS_FOCUSONBUTTONDOWN))
#define Tab_OwnerDraw(ptc)              ((BOOL)(ptc->ci.style & TCS_OWNERDRAWFIXED))
#define Tab_FocusNever(ptc)             ((BOOL)(ptc->ci.style & TCS_FOCUSNEVER))
#define Tab_HotTrack(ptc)             ((BOOL)(ptc->ci.style & TCS_HOTTRACK))
#define Tab_MultiSelect(ptc)            ((BOOL)(ptc->ci.style & TCS_MULTISELECT))
#define Tab_FlatButtons(ptc)            ((BOOL)((ptc)->ci.style & TCS_FLATBUTTONS))

#define Tab_FlatSeparators(ptc)         ((BOOL)((ptc)->dwStyleEx & TCS_EX_FLATSEPARATORS))

#ifdef __cplusplus
extern "C"
{
#endif

LRESULT CALLBACK Tab_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void NEAR PASCAL Tab_InvalidateItem(PTC ptc, int iItem, BOOL bErase);
void NEAR PASCAL CalcPaintMetrics(PTC ptc, HDC hdc);
void NEAR PASCAL Tab_OnHScroll(PTC ptc, HWND hwndCtl, UINT code, int pos);
void NEAR PASCAL Tab_OnAdjustRect(PTC ptc, BOOL fGrow, LPRECT prc);
BOOL NEAR Tab_FreeItem(PTC ptc, TABITEM FAR* pitem);
void NEAR Tab_UpdateArrows(PTC ptc, BOOL fSizeChanged);
int NEAR PASCAL ChangeSel(PTC ptc, int iNewSel,  BOOL bSendNotify, BOOL bUpdateCursorPos);
BOOL NEAR PASCAL RedrawAll(PTC ptc, UINT uFlags);
BOOL FAR PASCAL Tab_Init(HINSTANCE hinst);
void NEAR PASCAL UpdateToolTipRects(PTC ptc);
BOOL NEAR Tab_OnGetItem(PTC ptc, int iItem, TC_ITEM FAR* ptci);
int NEAR Tab_OnHitTest(PTC ptc, int x, int y, UINT FAR *lpuFlags);

#ifdef UNICODE
//
// ANSI <=> UNICODE thunks
//

TC_ITEMW * ThunkItemAtoW (PTC ptc, TC_ITEMA * pItemA);
BOOL ThunkItemWtoA (PTC ptc, TC_ITEMW * pItemW, TC_ITEMA * pItemA);
BOOL FreeItemW (TC_ITEMW *pItemW);
BOOL FreeItemA (TC_ITEMA *pItemA);
#endif

#ifdef __cplusplus
}
#endif
