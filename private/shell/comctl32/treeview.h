#include "listview.h"   // for some helper routines and border metrics
#define __IOleControl_INTERFACE_DEFINED__       // There is a conflich with the IOleControl's def of CONTROLINFO
#include "shlobj.h"

//
//  Definitions missing from commctrl.h
//
typedef const TVITEMEX *LPCTVITEMEX;

//
//  Private definitions
//
#define MAGIC_MININDENT 5
#define MAGIC_INDENT    3
#define MAGIC_HORZLINE  5

// flags for TV_DrawItem
#define TVDI_NOIMAGE    0x0001  // don't draw image
#define TVDI_NOTREE     0x0002  // don't draw indent, lines, +/-
#define TVDI_TRANSTEXT  0x0004  // draw text transparently in black
#define TVDI_ERASE      0x0008  // erase while drawing
#define TVDI_GRAYTEXT   0x0010  // text is gray (disabled item)
#define TVDI_GRAYCTL    0x0020  // text and background is gray (disabled control)
#define TVDI_FORCEIMAGE 0x0040  // Always draw image
#define TVDI_NOBK       0x0080

// Internal flags for TV_SelectItem
#define TVC_INTERNAL   0x1000

typedef struct _TREE {
    CONTROLINFO ci;

    // Flags
    BITBOOL        fHorz:1;        // horizontal scrollbar present
    BITBOOL        fVert:1;        // vertical scrollbar present
    BITBOOL        fFocus:1;       // currently has focus
    BITBOOL        fNameEditPending:1;  // Is a name edit pending?
    BITBOOL        fRedraw:1;      // should redraw?
    BITBOOL        fScrollWait:1;  // are we waiting for a dblclk to not scroll?
    BITBOOL        fCreatedFont:1; // we created our font
    BITBOOL        fNoDismissEdit:1; // don't dismiss in-place edit control
    BITBOOL        fIndentSet:1;    // is the parent managing the indent size?
    BITBOOL        fTrackSet:1;    // have we set a track event?
    BITBOOL        fPlaceTooltip:1; // should we do the placement of tooltip over the text?
    BITBOOL        fCyItemSet:1;    // the the parent set our item height?
    BITBOOL        fInsertAfter:1; // insert mark should be after htiInsert instead of before
    BITBOOL        fRestoreOldDrop:1; // hOldDrop needs to be restored to hDropTarget

    // Handles
    HTREEITEM   hRoot;          // tree root item
    HTREEITEM   hCaret;         // item with focus caret
    HTREEITEM   hDropTarget;    // item which is the drop target
    HTREEITEM   hOldDrop;       // item which used to be the drop target
    HTREEITEM   htiEdit;        // The item that is being edited.
    HTREEITEM   hHot;           // the currently hottracked item
    HTREEITEM   hToolTip;       // the current item set in tooltips
    HTREEITEM   htiInsert;      // item that is relative to the insert mark
    HTREEITEM   htiSearch;      // item active in most recent incremental search
    HTREEITEM   htiDrag;        // item that's being dragged.
    HDPA        hdpaWatch;      // array of PTVWATCHEDITEMs - items being watched
    HIMAGELIST  hImageList;     // image list
    HIMAGELIST  himlState;      // state image list

    HCURSOR hCurHot; // the cursor when we're over a hot item

    int         iPuntChar;      // number of wm_char's to punt
    int         cxState;
    int         cyState;

    UINT        uDBCSChar;      // DBCS character for incremental search

    HBRUSH      hbrBk;          // background brush
    HFONT       hFont;          // tree font
    HFONT       hFontHot;       // underlined for hot tracking
    HFONT       hFontBold;      // bold tree font
    HFONT       hFontBoldHot;       // underlined for hot tracking
    HBITMAP     hStartBmp;      // initial DC mono bitmap
    HBITMAP     hBmp;           // indent bitmaps in hdcBits
    HDC         hdcBits;        // HDC for drawing indent bitmaps
    HTREEITEM   hItemPainting;  // the guy we are currently painting
    HANDLE      hheap;          // heap for allocs for win32

    POINT       ptCapture;      // Point where the mouse was capture

    COLORREF    clrText;
    COLORREF    clrBk; 
    COLORREF    clrim;          // insert mark color.
    COLORREF    clrLine;        // line color

    // Dimensions
    SHORT       cxImage;        // image width
    SHORT       cyImage;        // image height
    SHORT       cyText;         // text height
    SHORT       cyItem;         // item height
    SHORT       cxBorder;   // horizontal item border
    SHORT       cyBorder;   // vert item border
    SHORT       cxIndent;       // indent width
    SHORT       cxWnd;          // window width
    SHORT       cyWnd;          // window height

    // Scroll Positioners
    WORD        cxMax;          // width of longest item
    WORD        cFullVisible;   // number of items that CAN fully fit in window
    SHORT       xPos;           // horizontal scrolled position
    UINT        cShowing;       // number of showing (non-collapsed) items
    UINT        cItems;         // total number of items
    HTREEITEM   hTop;           // first visible item (i.e., at top of client rect)
    UINT        uMaxScrollTime; // the maximum smooth scroll timing

    // stuff for edit in place
    HWND        hwndEdit;       // Edit window for name editing.
    WNDPROC     pfnEditWndProc; // edit field subclass proc

    //tooltip stuff
    HWND        hwndToolTips;
    LPTSTR      pszTip;         // store current tooltip/infotip string.
#ifdef UNICODE
    LPSTR       pszTipA;        // store current ANSI tooltip/infotip string.
#endif

    //incremental search stuff
    ISEARCHINFO is;

} TREE, NEAR *PTREE;

#define TV_StateIndex(pitem) ((int)(((DWORD)((pitem)->state) >> 12) & 0xF))

#define KIDS_COMPUTE            0    // use hKids to determine if a node has children
#define KIDS_FORCE_YES          1    // force a node to have kids (ignore hKids)
#define KIDS_FORCE_NO           2    // force a node to not have kids (ignore hKids)
#define KIDS_CALLBACK           3    // callback to see if a node has kids
#define KIDS_INVALID            4    // all values this and above are bogus

#define MAXLABELTEXT            MAX_PATH

// BUGBUG: OINK OINK

//
//  Note that there are multiple senses of "visible" going on.
//
//  TREE.hTop tracks visibility in the sense of "will it be painted?"
//
//  TREEITEM.iShownIndex tracks visibility in the sense of "not collapsed".
//  You can be off the screen but as long as your parent is expanded
//  you get an iShownIndex.
//
//

typedef struct _TREEITEM {
    HTREEITEM hParent;          // allows us to walk back out of the tree
    HTREEITEM hNext;            // next sibling
    HTREEITEM hKids;            // first child
    LPTSTR    lpstr;            // item text, can be LPSTR_TEXTCALLBACK
    LPARAM lParam;              // item data

    WORD      state;            // TVIS_ state flags
    WORD      iImage;           // normal state image at iImage
    WORD      iSelectedImage;   // selected state image
    WORD      iWidth;           // cached: width of text area (for hit test, drawing)
    WORD      iShownIndex;      // cached: -1 if not visible, otherwise nth visible item
                                // invisible = parent is invisible or collapsed
    BYTE      iLevel;           // cached: level of item (indent)
    BYTE      fKids;            // KIDS_ values
    WORD      iIntegral;        // integral height
    WORD      wSignature;       // for parameter validation, put at end of struct

} TREEITEM;

//
//  The signature is intentionally not ASCII characters, so it's
//  harder to run into by mistake.  I choose a value greater than
//  0x8000 so it can't be the high word of a pointer.
//
#define TV_SIG      0xABCD

#define TV_MarkAsDead(hti)      ((hti)->wSignature = 0)

#define ITEM_VISIBLE(hti) ((hti)->iShownIndex != (WORD)-1)

// get the parent, avoiding the hidden root node
#define VISIBLE_PARENT(hItem) (!(hItem)->iLevel ? NULL : (hItem)->hParent)

// REVIEW: make this a function if the optimizer doesn't do well with this
#define FULL_WIDTH(pTree, hItem)  (ITEM_OFFSET(pTree,hItem) + hItem->iWidth)
int FAR PASCAL ITEM_OFFSET(PTREE pTree, HTREEITEM hItem);

#define VTI_NULLOK      1
BOOL ValidateTreeItem(HTREEITEM hItem, UINT flags);

#ifdef DEBUG
#define DBG_ValidateTreeItem(hItem, flags) ValidateTreeItem(hItem, flags)
#else
#define DBG_ValidateTreeItem(hItem, flags)
#endif

//
//  TVWATCHEDITEM
//
//  Structure that tracks items being watched.
//
//  See TV_StartWatch for more information, and TV_DoExpandRecurse
//  for an example.
//
//  The hti field is a bit odd.
//
//  if fStale == FALSE, then hti is the item being watched.
//  if fStale == TRUE , then hti is the item *after* the item being watched.
//
//  We keep this strange semantic for fStale==TRUE so that TV_NextWatchItem
//  can successfully step to the item after a deleted item.  (Normally,
//  trying to do anything with a deleted item will fault.)
//

typedef struct TVWATCHEDITEM {
    HTREEITEM   hti;                    // current item
    BOOL        fStale;                 // has the original item been deleted?
} TVWATCHEDITEM, *PTVWATCHEDITEM;

BOOL TV_StartWatch(PTREE pTree, PTVWATCHEDITEM pwi, HTREEITEM htiStart);
BOOL TV_EndWatch(PTREE pTree, PTVWATCHEDITEM pwi);
#define TV_GetWatchItem(pTree, pwi) ((pwi)->hti)
#define TV_RestartWatch(pTree, pwi, htiStart) \
                        ((pwi)->hti = (htiStart), (pwi)->fStale = FALSE)
#define TV_IsWatchStale(pTree, pwi) ((pwi)->fStale)
#define TV_IsWatchValid(pTree, pwi) (!(pwi)->fStale)

//
//  TV_NextWatchItem - Enumerate the item after the watched item.
//                     This works even if the watched item was deleted.
//
#define TV_NextWatchItem(pTree, pwi) \
    ((pwi)->fStale || ((pwi)->hti = (pwi)->hti->hNext)), \
     (pwi)->fStale = FALSE

// in TVSCROLL.C
BOOL      NEAR  TV_ScrollBarsAfterAdd       (PTREE, HTREEITEM);
BOOL      NEAR  TV_ScrollBarsAfterRemove    (PTREE, HTREEITEM);
BOOL      NEAR  TV_ScrollBarsAfterExpand    (PTREE, HTREEITEM);
BOOL      NEAR  TV_ScrollBarsAfterCollapse  (PTREE, HTREEITEM);
void      NEAR  TV_ScrollBarsAfterResize    (PTREE, HTREEITEM, int, UINT);
BOOL      NEAR  TV_ScrollBarsAfterSetWidth  (PTREE, HTREEITEM);
BOOL      NEAR  TV_HorzScroll               (PTREE, UINT, UINT);
BOOL      NEAR  TV_VertScroll               (PTREE, UINT, UINT);
BOOL      NEAR  TV_SetLeft                  (PTREE, int);
#define TV_SetTopItem(pTree, i) TV_SmoothSetTopItem(pTree, i, 0)
BOOL      NEAR  TV_SmoothSetTopItem               (PTREE, UINT, UINT);
BOOL      NEAR  TV_CalcScrollBars           (PTREE);
BOOL      NEAR  TV_ScrollIntoView           (PTREE, HTREEITEM);
BOOL      NEAR  TV_ScrollVertIntoView       (PTREE, HTREEITEM);
HTREEITEM NEAR  TV_GetShownIndexItem        (HTREEITEM, UINT);
UINT      NEAR  TV_ScrollBelow              (PTREE, HTREEITEM, BOOL, BOOL);
BOOL      NEAR  TV_SortChildren(PTREE, HTREEITEM, BOOL);
BOOL      NEAR  TV_SortChildrenCB(PTREE, LPTV_SORTCB, BOOL);
void      NEAR  TV_ComputeItemWidth(PTREE pTree, HTREEITEM hItem, HDC hdc);

// in TVPAINT.C
void       NEAR  TV_GetBackgroundBrush       (PTREE pTree, HDC hdc);
void       NEAR  TV_UpdateTreeWindow         (PTREE, BOOL);
void       NEAR  TV_ChangeColors             (PTREE);
void       NEAR  TV_CreateIndentBmps         (PTREE);
void       NEAR  TV_Paint                    (PTREE, HDC);
HIMAGELIST NEAR  TV_CreateDragImage          (PTREE pTree, HTREEITEM hItem);
BOOL       NEAR  TV_ShouldItemDrawBlue       (PTREE pTree, TVITEMEX *ti, UINT flags);
LRESULT    NEAR  TV_GenerateDragImage        (PTREE ptree, SHDRAGIMAGE* pshdi);

BOOL TV_GetInsertMarkRect(PTREE pTree, LPRECT prc);

// in TVMEM.C

#define TVDI_NORMAL             0x0000  // TV_DeleteItem flags
#define TVDI_NONOTIFY           0x0001
#define TVDI_CHILDRENONLY       0x0002
#define TVDI_NOSELCHANGE        0x0004

BOOL      NEAR  TV_DeleteItem(PTREE, HTREEITEM, UINT);
HTREEITEM NEAR  TV_InsertItem(PTREE pTree, LPTV_INSERTSTRUCT lpis);
void      NEAR  TV_DestroyTree(PTREE);
LRESULT   NEAR  TV_OnCreate(HWND, LPCREATESTRUCT);


#ifdef UNICODE
HTREEITEM NEAR  TV_InsertItemA(PTREE pTree, LPTV_INSERTSTRUCTA lpis);
#endif


// in TREEVIEW.C
BOOL      NEAR TV_GetItemRect(PTREE, HTREEITEM, LPRECT, BOOL);
BOOL      NEAR TV_Expand(PTREE pTree, WPARAM wCode, TREEITEM FAR * hItem, BOOL fNotify);
HTREEITEM NEAR TV_GetNextItem(PTREE, HTREEITEM, WPARAM);
void      NEAR TV_GetItem(PTREE pTree, HTREEITEM hItem, UINT mask, LPTVITEMEX lpItem);
void      TV_PopBubble(PTREE pTree);

// Flags for TV_SelectItem
#define TVSIF_NOTIFY            0x0001
#define TVSIF_UPDATENOW         0x0002
#define TVSIF_NOSINGLEEXPAND    0x0004

BOOL      NEAR TV_SelectItem(PTREE, WPARAM, HTREEITEM, UINT, UINT);
BOOL      NEAR TV_SendChange(PTREE, HTREEITEM, int, UINT, UINT, UINT, int, int);
HTREEITEM NEAR TV_GetNextVisItem(HTREEITEM);
HTREEITEM NEAR TV_GetPrevItem(HTREEITEM);
HTREEITEM NEAR TV_GetPrevVisItem(HTREEITEM);
void      NEAR TV_CalcShownItems(PTREE, HTREEITEM hItem);
void      NEAR TV_OnSetFont(PTREE, HFONT, BOOL);
BOOL      NEAR TV_SizeWnd(PTREE, UINT, UINT);
void      NEAR TV_InvalidateItem(PTREE, HTREEITEM, UINT uFlags);
VOID NEAR PASCAL TV_CreateBoldFont(PTREE pTree);
BOOL TV_SetInsertMark(PTREE pTree, HTREEITEM hItem, BOOL fAfter);

LRESULT CALLBACK _export TV_EditWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK _export TV_WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL FAR                 TV_Init(HINSTANCE hinst);
void FAR                 TV_Terminate(BOOL fSystemExit);

LRESULT   NEAR  TV_Timer                    (PTREE pTree, UINT uTimerId);
HWND      NEAR  TV_OnEditLabel              (PTREE pTree, HTREEITEM hItem);
void      NEAR  TV_SetEditSize              (PTREE pTree);
BOOL      NEAR  TV_DismissEdit              (PTREE pTree, BOOL fCancel);
void      NEAR  TV_CancelPendingEdit        (PTREE pTree);
int       NEAR  TV_UpdateShownIndexes       (PTREE pTree, HTREEITEM hWalk);


void NEAR TV_UnsubclassToolTips(PTREE pTree);
LRESULT WINAPI TV_SubClassWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void NEAR TV_SubclassToolTips(PTREE pTree);
BOOL TV_UpdateToolTip(PTREE pTree);
BOOL TV_SetToolTipTarget(PTREE pTree, HTREEITEM hItem);
void TV_OnSetBkColor(PTREE pTree, COLORREF clr);
void TV_InitCheckBoxes(PTREE pTree);

#define TVMP_CALCSCROLLBARS (TV_FIRST + 0x1000)

// Fake customdraw.  See comment block in tvscroll.c

typedef struct TVFAKEDRAW {
    NMTVCUSTOMDRAW nmcd;
    PTREE pTree;
    HFONT hfontPrev;
    DWORD dwCustomPrev;
    DWORD dwCustomItem;
} TVFAKEDRAW, *PTVFAKEDRAW;

void TreeView_BeginFakeCustomDraw(PTREE pTree, PTVFAKEDRAW ptvfd);
DWORD TreeView_BeginFakeItemDraw(PTVFAKEDRAW plvfd, HTREEITEM hitem);
void TreeView_EndFakeItemDraw(PTVFAKEDRAW ptvfd);
void TreeView_EndFakeCustomDraw(PTVFAKEDRAW ptvfd);
