#include "listview.h"   // for some helper routines and border metrics

#define MAGIC_MININDENT 5
#define MAGIC_INDENT    3

// flags for TV_DrawItem
#define TVDI_NOIMAGE	0x0001	// don't draw image
#define TVDI_NOTREE	0x0002	// don't draw indent, lines, +/-
#define TVDI_TRANSTEXT	0x0004	// draw text transparently in black
#define TVDI_ERASE	0x0008	// erase while drawing
#define TVDI_GRAYTEXT   0x0010  // text is gray (disabled item)
#define TVDI_GRAYCTL    0x0020  // text and background is gray (disabled control)

typedef struct _TREE {
    HWND        hwnd;           // tree window
    HWND	hwndParent;	// parent window to send notifys to
    DWORD	style;		// style bits

    // Flags
    BOOL        fHorz:1;        // horizontal scrollbar present
    BOOL        fVert:1;        // vertical scrollbar present
    BOOL        fFocus:1;       // currently has focus
    BOOL        fNameEditPending:1;  // Is a name edit pending?
    BOOL        fRedraw:1;      // should redraw?
    BOOL        fScrollWait:1;  // are we waiting for a dblclk to not scroll?
    BOOL	fCreatedFont:1;	// we created our font
    BOOL        fNoDismissEdit:1; // don't dismiss in-place edit control
    BOOL        fIndentSet:1;    // is the parent managing the indent size?

    // Handles
    HTREEITEM   hRoot;          // tree root item
    HTREEITEM   hCaret;         // item with focus caret
    HTREEITEM   hDropTarget;    // item which is the drop target
    HTREEITEM   htiEdit;        // The item that is being edited.
    HIMAGELIST  hImageList;     // image list
    HIMAGELIST  himlState;      // state image list

    int         iPuntChar;      // number of wm_char's to punt
    int         cxState;       
    int         cyState;

    HBRUSH      hbrBk;          // background brush
    HFONT       hFont;          // tree font
    HFONT       hFontBold;      // bold tree font 
    HBITMAP     hStartBmp;      // initial DC mono bitmap
    HBITMAP     hBmp;           // indent bitmaps in hdcBits
    HDC         hdcBits;        // HDC for drawing indent bitmaps
    HTREEITEM   hItemPainting;  // the guy we are currently painting
#ifdef WIN32
    HANDLE	hheap;		// heap for allocs for win32
#endif


    // Dimensions
    SHORT       cxImage;        // image width
    SHORT       cyImage;        // image height
    SHORT       cyText;         // text height
    SHORT       cyItem;         // item height
    SHORT       cxIndent;       // indent width
    SHORT       cxWnd;          // window width
    SHORT       cyWnd;          // window height

    // Scroll Positioners
    WORD        cxMax;          // width of longest item
    WORD        cFullVisible;   // number of items that CAN fully fit in window
    SHORT       xPos;           // horizontal scrolled position
    UINT        cShowing;       // number of showing (non-collapsed) items
    UINT        cItems;         // total number of items
    HTREEITEM   hTop;           // first visible item

    // stuff for edit in place
    HWND        hwndEdit;       // Edit window for name editing.
    WNDPROC     pfnEditWndProc; // edit field subclass proc
} TREE, NEAR *PTREE;

#define TV_StateIndex(pitem) ((int)(((DWORD)((pitem)->state) >> 12) & 0xF))

#define KIDS_COMPUTE		0    // use hKids to determine if a node has children
#define KIDS_FORCE_YES		1    // force a node to have kids (ignore hKids)
#define KIDS_FORCE_NO		2    // force a node to not have kids (ignore hKids)
#define KIDS_CALLBACK		3    // callback to see if a node has kids

// BUGBUG: OINK OINK

typedef struct _TREEITEM {
    HTREEITEM hParent;		// allows us to walk back out of the tree
    HTREEITEM hNext;  		// next sibling
    HTREEITEM hKids;  		// first child
    LPSTR     lpstr;    	// item text, can be LPSTR_TEXTCALLBACK
    WORD      state;		// TVIS_ state flags
    WORD      iImage;   	// normal state image at iImage
    WORD      iSelectedImage; 	// selected state image
    WORD      iWidth;		// cached: width of text area (for hit test, drawing)
    WORD      iShownIndex;    	// cached: -1 if not visible, otherwise nth visible item
    unsigned char iLevel;    	// cached: level of item (indent)
    unsigned char fKids;	// KIDS_ values
    LPARAM lParam;          	// item data

#ifdef DEBUG
#define DEBUG_SIG   (('T' << 8) + 'I')
    WORD dbg_sig;
#endif

} TREEITEM;


#define ITEM_VISIBLE(hti) ((hti)->iShownIndex != (WORD)-1)

// get the parent, avoiding the hidden root node
#define VISIBLE_PARENT(hItem) (!(hItem)->iLevel ? NULL : (hItem)->hParent)

// REVIEW: make this a function if the optimizer doesn't do well with this
#define FULL_WIDTH(pTree, hItem)  (ITEM_OFFSET(pTree,hItem) + hItem->iWidth)
int FAR PASCAL ITEM_OFFSET(PTREE pTree, HTREEITEM hItem);


#ifdef DEBUG
void NEAR   ValidateTreeItem(HTREEITEM hItem, BOOL bNullOk);
#else
#define ValidateTreeItem(hItem, bNullOk)
#endif




// in TVSCROLL.C
BOOL      NEAR  TV_ScrollBarsAfterAdd       (PTREE, HTREEITEM);
BOOL      NEAR  TV_ScrollBarsAfterRemove    (PTREE, HTREEITEM);
BOOL      NEAR  TV_ScrollBarsAfterExpand    (PTREE, HTREEITEM);
BOOL      NEAR  TV_ScrollBarsAfterCollapse  (PTREE, HTREEITEM);
BOOL      NEAR  TV_ScrollBarsAfterSetWidth  (PTREE, HTREEITEM);
BOOL      NEAR  TV_HorzScroll               (PTREE, UINT, UINT);
BOOL      NEAR  TV_VertScroll               (PTREE, UINT, UINT);
BOOL      NEAR  TV_SetLeft                  (PTREE, int);
BOOL      NEAR  TV_SetTopItem               (PTREE, UINT);
BOOL      NEAR  TV_CalcScrollBars           (PTREE);
BOOL      NEAR  TV_ScrollIntoView           (PTREE, HTREEITEM);
BOOL      NEAR  TV_ScrollVertIntoView       (PTREE, HTREEITEM);
HTREEITEM NEAR  TV_GetShownIndexItem        (HTREEITEM, UINT);
UINT      NEAR  TV_ScrollBelow              (PTREE, HTREEITEM, BOOL, BOOL);
BOOL      NEAR  TV_SortChildren(PTREE, HTREEITEM, BOOL);
BOOL      NEAR  TV_SortChildrenCB(PTREE, LPTV_SORTCB, BOOL);
void	  NEAR  TV_ComputeItemWidth(PTREE pTree, HTREEITEM hItem, HDC hdc);

// in TVPAINT.C
void      NEAR TV_GetBackgroundBrush        (PTREE pTree, HDC hdc);
void      NEAR  TV_UpdateTreeWindow         (PTREE, BOOL);
void      NEAR  TV_ChangeColors             (PTREE);
void      NEAR  TV_CreateIndentBmps         (PTREE);
void      NEAR  TV_Paint                    (PTREE, HDC);
HIMAGELIST NEAR TV_CreateDragImage	    (PTREE pTree, HTREEITEM hItem);

// in TVMEM.C

#define TVDI_NORMAL		0x0000	// TV_DeleteItem flags
#define TVDI_NONOTIFY		0x0001
#define TVDI_CHILDRENONLY	0x0002
#define TVDI_NOSELCHANGE	0x0004

BOOL      NEAR  TV_DeleteItem(PTREE, HTREEITEM, UINT);
HTREEITEM NEAR  TV_InsertItem(PTREE pTree, LPTV_INSERTSTRUCT lpis);
void      NEAR  TV_DestroyTree(PTREE);
LRESULT   NEAR  TV_OnCreate(HWND, LPCREATESTRUCT);

// in TREEVIEW.C
BOOL      NEAR TV_GetItemRect(PTREE, HTREEITEM, LPRECT, BOOL);
BOOL 	  NEAR TV_Expand(PTREE pTree, UINT wCode, TREEITEM FAR * hItem, BOOL fNotify);
HTREEITEM NEAR TV_GetNextItem(PTREE, HTREEITEM, UINT);
void 	  NEAR TV_GetItem(PTREE pTree, HTREEITEM hItem, UINT mask, LPTV_ITEM lpItem);

BOOL      NEAR TV_SelectItem(PTREE, UINT, HTREEITEM, BOOL, BOOL, UINT);
BOOL      NEAR TV_SendChange(PTREE, HTREEITEM, int, UINT, UINT, UINT, int, int);
HTREEITEM NEAR TV_GetNextVisItem(HTREEITEM);
HTREEITEM NEAR TV_GetPrevItem(HTREEITEM);
HTREEITEM NEAR TV_GetPrevVisItem(HTREEITEM);
void      NEAR TV_CalcShownItems(PTREE, HTREEITEM hItem);
void      NEAR TV_OnSetFont(PTREE, HFONT, BOOL);
BOOL      NEAR TV_SizeWnd(PTREE, UINT, UINT);
void      NEAR TV_InvalidateItem(PTREE, HTREEITEM, UINT uFlags);
VOID NEAR PASCAL TV_CreateBoldFont(PTREE pTree)	;

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


