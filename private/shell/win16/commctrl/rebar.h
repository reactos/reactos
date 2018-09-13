typedef struct tagREBARBAND
{
    UINT        fStyle;         // 0x00
    COLORREF    clrFore;        // 0x04
    COLORREF    clrBack;        // 0x08
    LPTSTR      lpText;         // 0x0C
    UINT        cxText;         // 0x10
    int         iImage;         // 0x14
    HWND        hwndChild;      // 0x18
    UINT        cxMinChild;     // 0x1C
    UINT        cyMinChild;     // 0x20
    UINT        cxBmp;          // 0x24
    UINT        cyBmp;          // 0x28
    HBITMAP     hbmBack;        // 0x2C
    int         x;              // 0x30
    int         y;              // 0x34
    int         cx;             // 0x38
    int         cy;             // 0x3C
    int         cxRestored;     // 0x40
    int         cxMin;          // 0x44
    int         cxRequest;      // 0x48
    UINT        wID;            // 0x4C
} RBB, NEAR *PRBB;

typedef struct tagREBAR
{
    CONTROLINFO ci;             // 0x00
    HPALETTE    hpal;           // 0x14
    UINT        fStyle;         // 0x18
    HWND        hwndToolTips;   // 0x1C
    UINT        cBands;         // 0x20
    int         xBmpOrg;        // 0x24
    int         yBmpOrg;        // 0x28
    HIMAGELIST  himl;           // 0x2C
    UINT        cxImage;        // 0x30
    UINT        cyImage;        // 0x34
    HFONT       hFont;          // 0x38
    UINT        cyFont;         // 0x3C
    UINT        cy;             // 0x40
    int         iCapture;       // 0x44
    POINT       ptCapture;      // 0x48
    int         xStart;         // 0x50
    PRBB        rbbList;        // 0x54
    UINT        fFullOnDrag:1;  // 0x58   -
} RB, NEAR *PRB;
                                     
void NEAR PASCAL RBPaint(PRB prb, HDC hdc);
void NEAR PASCAL RBDrawBand(PRB prb, PRBB prbb, HDC hdc);
void NEAR PASCAL RBResize(PRB prb, BOOL fForceHeightChange);
BOOL NEAR PASCAL RBSetFont(PRB prb, WPARAM wParam);

BOOL NEAR PASCAL RBGetBandInfo(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi);
BOOL NEAR PASCAL RBSetBandInfo(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi);
BOOL NEAR PASCAL RBInsertBand(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi);
BOOL NEAR PASCAL RBDeleteBand(PRB prb, UINT uBand);

