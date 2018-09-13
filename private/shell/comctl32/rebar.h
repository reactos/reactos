typedef struct tagREBARBAND
{
    UINT        fStyle;
    COLORREF    clrFore;
    COLORREF    clrBack;
    LPTSTR      lpText;
    UINT        cxText;         // width of header text
    int         iImage;
    HWND        hwndChild;
    UINT        cxMinChild;     // min width for hwndChild
    UINT        cyMinChild;     // min height for hwndChild
    UINT        cxBmp;
    UINT        cyBmp;
    HBITMAP     hbmBack;
    int         x;              // left edge of band, relative to rebar
    int         y;              // top edge of band, relative to rebar
    int         cx;             // total width of band
    int         cy;             // height of band
    int         cxRequest;      // 'requested' width for band; either requested by host or 
                                // used as temp var during size recalculation
    int         cxMin;          // min width for band
    int         cxIdeal;        // hwndChild's desired width
    UINT        wID;
    UINT        cyMaxChild;     // hwndChild's max height
    UINT        cyIntegral;     // the hell if I know
    UINT        cyChild;        // this differs from cyMinChild only in RBBS_VARIABLEHEIGHT mode
    LPARAM      lParam;

    BITBOOL     fChevron:1;     // band is showing chevron button
    RECT        rcChevron;      // chevron button rect
    UINT        wChevState;     // chevron button state (DFCS_PUSHED, etc.)
} RBB, NEAR *PRBB;

typedef struct tagREBAR
{
    CONTROLINFO ci;
    HPALETTE    hpal;
    BITBOOL     fResizeRecursed:1;
    BITBOOL     fResizePending:1;
    BITBOOL     fResizeNotify:1;
    BITBOOL     fRedraw:1;
    BITBOOL     fRecalcPending:1;
    BITBOOL     fRecalc:1;
    BITBOOL     fParentDrag:1;
    BITBOOL     fRefreshPending:1;
    BITBOOL     fResizing:1;
    BITBOOL     fUserPalette:1;
    BITBOOL     fFontCreated:1;
    BITBOOL     fFullOnDrag:1;
    HDRAGPROXY  hDragProxy;
    HWND        hwndToolTips;
    UINT        cBands;
    int         xBmpOrg;
    int         yBmpOrg;
    HIMAGELIST  himl;
    UINT        cxImage;
    UINT        cyImage;
    HFONT       hFont;
    UINT        cyFont;
    UINT        cy;
    int         iCapture;
    POINT       ptCapture;
    int         xStart;
    PRBB        rbbList;
    COLORREF    clrBk;
    COLORREF    clrText;
    UINT        uResizeNext;    // this marks the next band to resize vertically if needed and allowed (VARIABLEHEIGHT set)
    DWORD       dwStyleEx;
    COLORSCHEME clrsc;
    POINT       ptLastDragPos;
    PRBB        prbbHot;        // band w/ hot chevron
} RB, NEAR *PRB;

void NEAR PASCAL RBPaint(PRB prb, HDC hdc);
void NEAR PASCAL RBDrawBand(PRB prb, PRBB prbb, HDC hdc);
void NEAR PASCAL RBResize(PRB prb, BOOL fForceHeightChange);
BOOL NEAR PASCAL RBSetFont(PRB prb, WPARAM wParam);

BOOL NEAR PASCAL RBGetBandInfo(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi);
BOOL NEAR PASCAL RBSetBandInfo(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi, BOOL fAllowRecalc);
BOOL NEAR PASCAL RBInsertBand(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi);
BOOL NEAR PASCAL RBDeleteBand(PRB prb, UINT uBand);
