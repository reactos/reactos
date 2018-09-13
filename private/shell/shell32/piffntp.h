/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1992,1993
 *  All Rights Reserved.
 *
 *
 *  PIFFNTP.H
 *  Private PIFMGR include file
 *
 *  History:
 *  Created 22-Mar-1993 2:58pm by Jeff Parsons (from vmdosapp\fontutil.h)
 */

#define PREVIEW_BORDER          1

#define DY_TTBITMAP             12

/*
 *  These parameters control how fast the fdiCache should grow.
 */

#define FDI_TABLE_START         20      /* Number of entries to start */
#define FDI_TABLE_INC           10      /* Increment in number of slots */


typedef struct tagDISPLAYPARAMETERS {   /* dp */
    INT dpHorzSize;
    INT dpVertSize;
    INT dpHorzRes;
    INT dpVertRes;
    INT dpLogPixelsX;
    INT dpLogPixelsY;
    INT dpAspectX;
    INT dpAspectY;
    INT dpBitsPerPixel;
    TCHAR szTTFace[2][LF_FACESIZE];
} DISPLAYPARAMETERS;

//#define BACKGROUND    0x000000FF      /* bright blue  */
//#define BACKGROUNDSEL 0x00FF00FF      /* bright magenta */
//#define BUTTONFACE    0x00C0C0C0      /* bright grey  */
//#define BUTTONSHADOW  0x00808080      /* dark grey    */


#define FNTFLAGSFROMID(id)  ((id - IDC_RASTERFONTS + 1) << FNT_FONTMASKBITS)
#define IDFROMFNTFLAGS(fl)  (IDC_RASTERFONTS - 1 + (((fl) & FNT_FONTMASK) >> FNT_FONTMASKBITS))

#if FNTFLAGSFROMID(IDC_RASTERFONTS) != FNT_RASTERFONTS || \
    IDFROMFNTFLAGS(FNT_RASTERFONTS) != IDC_RASTERFONTS || \
    FNTFLAGSFROMID(IDC_TTFONTS) != FNT_TTFONTS         || \
    IDFROMFNTFLAGS(FNT_TTFONTS) != IDC_TTFONTS         || \
    FNTFLAGSFROMID(IDC_BOTHFONTS) != FNT_BOTHFONTS   || \
    IDFROMFNTFLAGS(FNT_BOTHFONTS) != IDC_BOTHFONTS
#error Dialog control IDs and FNT flags values are not compatible
#endif


/*
 * IsDlgError
 *
 *      To simplify error checking, we assume that all *_ERR values are -1
 *      and all *_ERRSPACE values are -2.
 *
 *      This also assumes a two's complement number system.
 *
 *  Entry:
 *
 *      A return code from a list box or combo box.
 *
 *  Exit:
 *
 *      Nonzero if the return code indicated an error of some sort.
 *      Zero    if the return code indiated no error.
 *
 */

#define B_ERR (-1)

#if LB_ERR != B_ERR || LB_ERRSPACE != -2 || \
    CB_ERR != B_ERR || CB_ERRSPACE != -2
#error Problem with manifest constants.
#endif

#define IsDlgError(dw) ((DWORD)(dw) >= (DWORD)(-2))


/*
 *  Low-level macros
 *
 *  BPFDIFROMREF(lParam)
 *
 *  These three macros pack and unpack list box reference data.
 *
 *  bpfdi     = based pointer into segCache describing the list box entry
 *  fTrueType = nonzero if the font is a TrueType font
 *  lParam    = the reference data
 *
 */

#define BPFDIFROMREF(lParam)         (BPFDI)(lParam)


/*
 *  High-level macros
 *
 *  These macros handle the SendMessages that go to/from list boxes
 *  and combo boxes.
 *
 *  The "lcb" prefix stands for "list or combo box".
 *
 *  Basically, we're providing mnemonic names for what would otherwise
 *  look like a whole slew of confusing SendMessage's.
 *
 */






#define lcbFindStringExact(hwnd, fListBox, lpsz) \
        (DWORD)SendMessage(hwnd, fListBox ? LB_FINDSTRINGEXACT : CB_FINDSTRINGEXACT, \
                          (WPARAM)-1, (LPARAM)(LPTSTR)lpsz)

#define lcbAddString(hwnd, fListBox, lpsz) \
        (DWORD)SendMessage(hwnd, fListBox ? LB_ADDSTRING : CB_ADDSTRING, \
                          0, (LPARAM)(LPTSTR)lpsz)

#define lcbSetItemDataPair(hwnd, fListBox, w, bpfdi, fIsTrueType) \
        if (!IsSpecialBpfdi((BPFDI)bpfdi)) \
            ((BPFDI)bpfdi)->bTT = fIsTrueType; \
        (DWORD)SendMessage(hwnd, fListBox ? LB_SETITEMDATA : CB_SETITEMDATA, \
                    (WPARAM)w, (LPARAM)bpfdi)

#define lcbGetCount(hwnd, fListBox) \
        (DWORD)SendMessage(hwnd, fListBox ? LB_GETCOUNT : CB_GETCOUNT, (WPARAM)0, (LPARAM)0)

#define lcbGetCurSel(hwnd, fListBox) \
        (DWORD)SendMessage(hwnd, fListBox ? LB_GETCURSEL : CB_GETCURSEL, (WPARAM)0, (LPARAM)0)

#define lcbSetCurSel(hwnd, fListBox, w) \
        (DWORD)SendMessage(hwnd, fListBox ? LB_SETCURSEL : CB_SETCURSEL, (WPARAM)w, (LPARAM)0)

#define lcbGetItemDataPair(hwnd, fListBox, w) \
        (DWORD_PTR)SendMessage(hwnd, fListBox ? LB_GETITEMDATA : CB_GETITEMDATA, (WPARAM)w, (LPARAM)0)

#define lcbGetBpfdi(hwnd, fListBox, w) \
        BPFDIFROMREF(lcbGetItemDataPair(hwnd, fListBox, w))

#define lcbInsertString(hwnd, fListBox, lpsz, i) \
        (DWORD)SendMessage(hwnd, fListBox ? LB_INSERTSTRING : CB_INSERTSTRING, \
                           (WPARAM)i, (LPARAM)(LPTSTR)lpsz)

/*
 * the listbox/combox strings are stored as follows. we use the tabs
 * to do TabbedTextOut().  The padding is used to keep the sorting right.
 * TT fonts are distinguished by the hiword of the item data
 *
 *  String:     \t%2d\tx\t%2d
 *               wd    ht
 *
 *  The "Auto" entry is stored as...
 *
 *  String:     \1Auto
 *
 *      The first character is \1 so that Auto sorts at the top of the list.
 *      (The \1 is not actually displayed.)
 *
 */


/*
 * FONTDIMENINFO
 *
 * The distinction between the requested and returned font dimensions is
 * important in the case of TrueType fonts, in which there is no guarantee
 * that what you ask for is what you will get.
 *
 * Note that the correspondence between "Requested" and "Actual" is broken
 * whenever the user changes his display driver, because GDI uses driver
 * parameters to control the font rasterization.
 *
 * The fdiHeightReq and fdiWidthReq fields are both zero if the font is
 * a raster font.
 *
 */

typedef struct tagFONTDIMENINFO {       /* fdi */
    UINT fdiWidthReq;                   /* Font width requested */
    UINT fdiHeightReq;                  /* Font height requested */
    UINT fdiWidthActual;                /* Font width returned */
    UINT fdiHeightActual;               /* Font height returned */
    BOOL bTT;                           /* Font is TT? */
    INT  Index;                         /* Index into listbox */
} FONTDIMENINFO, *LPFONTDIMENINFO, *LPFDI;

typedef FONTDIMENINFO *BPFDI;
typedef UINT    CFDI;
typedef UINT    IFDI;

// BILINGUAL stuff
#define CLIP_DFA_OVERRIDE   0x40    /* Disable Font Association */

extern  CFDI    cfdiCache[];            /* # used entries in fdi cache */
extern  CFDI    cfdiCacheActual[];      /* Total # entries in fdi cache */


/*
 * BPFDI_CANCEL must be 0xFFFF because that is what DialogBox returns on
 * failure.
 */
#define BPFDI_CANCEL    (BPFDI)(INT_PTR)(-1)
#define BPFDI_AUTO      (BPFDI)(INT_PTR)(-2)
#define BPFDI_IGNORE    (BPFDI)(INT_PTR)(-3)

#define IsSpecialBpfdi(bpfdi)       ((bpfdi) >= BPFDI_IGNORE)

/* This is the maximum size font we will create. */
#define MAX_FONT_HEIGHT     72          /* 72pt = 1inch */

typedef INT PENALTY;                    /* pnl */

/*
 *  Penalty structures
 *
 *  Do NOT change these structure definitions unless you know what you're
 *  doing, because the relative order of the values is crucial for proper
 *  reading and writing of the INI file in which they are stored.
 */

typedef struct tagPENALTYPAIR {         /* pnlp */
    PENALTY pnlInitial;
    PENALTY pnlScale;
} PENALTYPAIR, *PPENALTYPAIR;


typedef struct tagPENALTYLIST {         /* pnll */
    PENALTYPAIR pnlpOvershoot;
    PENALTYPAIR pnlpShortfall;
} PENALTYLIST, *PPENALTYLIST;


#define MINPENALTY      (-5000)
#define MAXPENALTY        5000
#define SENTINELPENALTY  MAXLONG      /* Must exceed any legitimate penalty */

#define NUMPENALTIES        (SIZEOF(rgpnlPenalties) / SIZEOF(INT))
#define NUMINITIALTTHEIGHTS (SIZEOF(rgwInitialTtHeights) / SIZEOF(WORD))

#define pnllX           ((PPENALTYLIST)rgpnlPenalties)[0]
#define pnllY           ((PPENALTYLIST)rgpnlPenalties)[1]
#define pnlTrueType     (rgpnlPenalties[8])

/*
 *  These values for FindFontMatch's third argument are magical.
 *  WindowInit uses the funky values (with the exception of FFM_PERFECT)
 */
#define FFM_BOTHFONTS   0x00000000
#define FFM_RESTRICTED  0x00000001
#define FFM_RASTERFONTS 0x00000001
#define FFM_TTFONTS     0x80000001

#define FFM_PERFECT     0xFFFFFFFF


/*
 *  Last but not least, per-dialog data (aka roll-your-own DLL instance data)
 */

typedef struct FNTINFO {        /* fi */
    PPROPLINK ppl;              // ppl must ALWAYS be the first field
    BPFDI     bpfdi;
    PROPFNT   fntProposed;      // The properties to use if the user selects OK
    PROPWIN   winOriginal;      // For window preview and auto font selection
    HFONT     hFontPreview;     // Used in font preview window
    BOOL      fMax;             // Should window preview show as maximized?
    POINT     ptCorner;         // Upper-left corner of window
    UINT      uDefaultCp;       // System default code page
} FNTINFO;
typedef FNTINFO *PFNTINFO;      /* pfi */


/*
 * for Font Enumlation
 */
typedef struct FNTENUMINFO {
    HWND      hwndList;
    BOOL      fListBox;
    INT       CodePage;
} FNTENUMINFO;
typedef FNTENUMINFO *LPFNTENUMINFO;

/*
 *  Internal function prototypes
 */

BOOL_PTR CALLBACK DlgFntProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID InitFntDlg(HWND hDlg, PFNTINFO pfi);
VOID ApplyFntDlg(HWND hDlg, PFNTINFO pfi);

BOOL LoadGlobalFontData(void);
VOID FreeGlobalFontData(void);
BOOL LoadGlobalFontEditData(void);
VOID FreeGlobalFontEditData(void);
VOID CheckDisplayParameters(void);
HBITMAP LoadBitmaps(INT id);
DWORD GetFlippedSysColor(INT nDispElement);
VOID PreviewInit(HWND hDlg, PFNTINFO pfi);
VOID PreviewUpdate(HWND hwndList, PFNTINFO pfi);
LRESULT WndPreviewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID WndPreviewPaint(HWND hDlg, HWND hwnd);
LRESULT FontPreviewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT  WINAPI CreateFontList(HWND hwndList, BOOL fListBox, LPPROPFNT lpFnt);
VOID WINAPI DrawItemFontList(BOOL fListBox, const LPDRAWITEMSTRUCT lpdis);
INT  WINAPI GetItemFontInfo(HWND hwndFontList, BOOL fListBox, HANDLE hProps, LPPROPFNT lpFnt);
BOOL WINAPI MatchCurrentFont(HWND hwndList, BOOL fListBox, LPPROPFNT lpFnt);
LONG WINAPI MeasureItemFontList(LPMEASUREITEMSTRUCT lpmi);
VOID WINAPI UpdateTTBitmap(void);

BOOL AddRasterFontsToFontListA(HWND hwndList, BOOL fListBox,
                                                  LPCSTR lpszRasterFaceName, INT CodePage);
INT CALLBACK RasterFontEnum(ENUMLOGFONTA *lpelf,
                            NEWTEXTMETRICA *lpntm,
                            INT nFontType, LPARAM lParam);
BPFDI AddToFontListCache(HWND hwndList,
                         BOOL fListBox,
                         UINT uHeightReq,
                         UINT uWidthReq,
                         UINT uHeightActual,
                         UINT uWidthActual,
                         UINT uCodePage);
BOOL  AddTrueTypeFontsToFontListA(HWND hwndList, BOOL fListBox,
                                  LPSTR lpszTTFaceName, INT CodePage);
BPFDI AddOneNewTrueTypeFontToFontListA(HWND hwndList,
                                       BOOL fListBox,
                                       UINT uWidth, UINT uHeight,
                                       LPSTR lpszTTFaceName,
                                       INT CodePage);
DWORD_PTR GetFont(HWND hwndList, BOOL fListBox, PFNTINFO pfi);
void  SetFont(LPPROPFNT lpFnt, BPFDI bpfdi);

#define AspectScale(n1,n2,m) (UINT)(((UINT)n1*(UINT)m)/(UINT)n2)

VOID AspectPoint(LPRECT lprectPreview, LPPOINT lppt);
VOID AspectRect(LPRECT lprectPreview, LPRECT lprc);

HFONT CreateFontFromBpfdi(BPFDI bpfdi, PFNTINFO pfi);

void  FontSelInit(void);

BPFDI GetTrueTypeFontTrueDimensions(UINT dxWidth, UINT dyHeight, INT CodePage);
BPFDI FindFontMatch(UINT dxWidth, UINT dyHeight, LPINT lpfl, INT CodePage);
#ifdef  FONT_INCDEC
BPFDI IncrementFontSize(UINT cyFont, UINT cxFont, INT flFnt, INT CodePage);
#endif  /* FONT_INCDEC */
PENALTY ComputePenaltyFromPair(PPENALTYPAIR ppnlp, UINT dSmaller, UINT dLarger);
PENALTY ComputePenaltyFromList(PPENALTYLIST ppnll, UINT dActual, UINT dDesired);
PENALTY ComputePenalty(UINT cxCells,  UINT cyCells,
                       UINT dxClient, UINT dyClient,
                       UINT dxFont,   UINT dyFont);
BPFDI ChooseBestFont(UINT cxCells, UINT cyCells, UINT dxClient, UINT dyClient, INT fl, INT CodePage);
