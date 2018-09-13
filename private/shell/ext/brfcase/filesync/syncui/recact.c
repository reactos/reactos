//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: recact.c
//
//  This file contains the reconciliation-action control class code
//
//
// History:
//  08-12-93 ScottH     Created.
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"         // common headers
#ifdef WINNT
#include <help.h>
#else
#include "..\..\..\win\core\inc\help.h"   // help IDs
#endif

#include "res.h"
#include "recact.h"
#include "dobj.h"

/////////////////////////////////////////////////////  CONTROLLING DEFINES


/////////////////////////////////////////////////////  DEFINES

// Manifest constants
#define SIDE_INSIDE     0
#define SIDE_OUTSIDE    1

// These should be changed if the bitmap sizes change!!
#define CX_ACTIONBMP    26
#define CY_ACTIONBMP    26

#define RECOMPUTE       (-1)

#define X_INCOLUMN      (g_cxIcon*2)

// Image indexes
#define II_RIGHT        0
#define II_LEFT         1
#define II_CONFLICT     2
#define II_SKIP         3
#define II_MERGE        4
#define II_SOMETHING    5
#define II_UPTODATE     6
#define II_DELETE       7

// Menu items
//
#define IDM_ACTIONFIRST     100
#define IDM_TOOUT           100
#define IDM_TOIN            101
#define IDM_SKIP            102
#define IDM_MERGE           103
#define IDM_DELETEOUT       104
#define IDM_DELETEIN        105
#define IDM_DONTDELETE      106
#define IDM_ACTIONLAST      106

#define IDM_WHATSTHIS       107


/////////////////////////////////////////////////////  TYPEDEFS

typedef struct tagRECACT
    {
    HWND        hwnd;

    HWND        hwndLB;
    HWND        hwndTip;
    HDC         hdcOwn;             // Own DC
    HMENU       hmenu;              // Action and help context menu
    HFONT       hfont;
    WNDPROC     lpfnLBProc;         // Default LB proc
    HIMAGELIST  himlAction;         // imagelist for actions
    HIMAGELIST  himlCache;          // control imagelist cache
    HBITMAP     hbmpBullet;
    HDSA        hdsa;

    HBRUSH      hbrBkgnd;
    COLORREF    clrBkgnd;

    LONG        lStyle;             // Window style flags
    UINT        cTipID;             // Tip IDs are handed out 2 per item

    // Metrics
    int         cxItem;             // Generic width of an item
    int         cxMenuCheck;
    int         cyMenuCheck;
    int         cyText;
    int         cxEllipses;

    } RECACT,  * LPRECACT;

#define RecAct_IsNoIcon(this)   IsFlagSet((this)->lStyle, RAS_SINGLEITEM)

// Internal item data struct
//
typedef struct tagRA_PRIV
    {
    UINT uStyle;        // One of RAIS_
    UINT uAction;       // One of RAIA_

    FileInfo * pfi;

    SIDEITEM siInside;
    SIDEITEM siOutside;

    LPARAM  lParam;

    DOBJ    rgdobj[4];      // Array of Draw object info
    int     cx;             // Bounding width and height
    int     cy;

    } RA_PRIV,  * LPRA_PRIV;

#define IDOBJ_FILE      0
#define IDOBJ_ACTION    1
#define IDOBJ_INSIDE    2
#define IDOBJ_OUTSIDE   3

// RecAction menu item definition structure.  Used to define the
//  context menu brought up in this control.
//
typedef struct tagRAMID
    {
    UINT    idm;               // Menu ID (for MENUITEMINFO struct)
    UINT    uAction;           // One of RAIA_* flags
    UINT    ids;               // Resource string ID
    int     iImage;            // Index into himlAction
    RECT    rcExtent;          // Extent rect of string
    } RAMID,  * LPRAMID;   // RecAction Menu Item Definition

// Help menu item definition structure.  Used to define the help
//  items in the context menu.
//
typedef struct tagHMID
    {
    UINT idm;
    UINT ids;
    } HMID;

/////////////////////////////////////////////////////  MACROS

#define RecAct_DefProc      DefWindowProc
#define RecActLB_DefProc    CallWindowProc


// Instance data pointer macros
//
#define RecAct_GetPtr(hwnd)     (LPRECACT)GetWindowLongPtr(hwnd, 0)
#define RecAct_SetPtr(hwnd, lp) (LPRECACT)SetWindowLongPtr(hwnd, 0, (LRESULT)(lp))

#define RecAct_GetCount(this)   ListBox_GetCount((this)->hwndLB)

LPCTSTR PRIVATE SkipDisplayJunkHack(LPSIDEITEM psi);

/////////////////////////////////////////////////////  MODULE DATA

#ifdef SAVE_FOR_RESIZE
static TCHAR const c_szDateDummy[] = TEXT("99/99/99 99:99PM");
#endif

// Map RAIA_* values to image indexes
//
static UINT const c_mpraiaiImage[] =
    { II_RIGHT,
      II_LEFT,
      II_SKIP,
      II_CONFLICT,
      II_MERGE,
      II_SOMETHING,
      II_UPTODATE,
      0,
#ifdef NEW_REC
      II_DELETE,
      II_DELETE,
      II_SKIP
#endif
      };

// Map RAIA_* values to menu command positions
//
static UINT const c_mpraiaidmMenu[] =
    { IDM_TOOUT,
      IDM_TOIN,
      IDM_SKIP,
      IDM_SKIP,
      IDM_MERGE,
      0, 0, 0,
#ifdef NEW_REC
      IDM_DELETEOUT,
      IDM_DELETEIN,
      IDM_DONTDELETE
#endif
      };

// Define the context menu layout
//
static RAMID const c_rgramid[] = {
    { IDM_TOOUT,    RAIA_TOOUT, IDS_MENU_REPLACE,   II_RIGHT,   0 },
    { IDM_TOIN,     RAIA_TOIN,  IDS_MENU_REPLACE,   II_LEFT,    0 },
    { IDM_SKIP,     RAIA_SKIP,  IDS_MENU_SKIP,      II_SKIP,    0 },
    // Merge must be the last item!
    { IDM_MERGE,    RAIA_MERGE, IDS_MENU_MERGE,     II_MERGE,   0 },
    };

static RAMID const c_rgramidCreates[] = {
    { IDM_TOOUT,    RAIA_TOOUT, IDS_MENU_CREATE,    II_RIGHT,   0 },
    { IDM_TOIN,     RAIA_TOIN,  IDS_MENU_CREATE,    II_LEFT,    0 },
    };

#ifdef NEW_REC
static RAMID const c_rgramidDeletes[] = {
    { IDM_DELETEOUT,   RAIA_DELETEOUT, IDS_MENU_DELETE,    II_DELETE,  0 },
    { IDM_DELETEIN,    RAIA_DELETEIN,  IDS_MENU_DELETE,    II_DELETE,  0 },
    { IDM_DONTDELETE,  RAIA_DONTDELETE,IDS_MENU_DONTDELETE,II_SKIP,    0 },
    };
#endif

// Indexes into c_rgramidCreates
//
#define IRAMID_CREATEOUT    0
#define IRAMID_CREATEIN     1

// Indexes into c_rgramidDeletes
//
#define IRAMID_DELETEOUT    0
#define IRAMID_DELETEIN     1
#define IRAMID_DONTDELETE   2

static HMID const c_rghmid[] = {
    { IDM_WHATSTHIS, IDS_MENU_WHATSTHIS },
    };

/////////////////////////////////////////////////////  LOCAL PROCEDURES

LRESULT _export CALLBACK RecActLB_LBProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

/////////////////////////////////////////////////////  PRIVATE FUNCTIONS



#ifdef DEBUG
LPCTSTR PRIVATE DumpRecAction(
    UINT uAction)        // RAIA_
    {
    switch (uAction)
        {
    DEBUG_CASE_STRING( RAIA_TOOUT );
    DEBUG_CASE_STRING( RAIA_TOIN );
    DEBUG_CASE_STRING( RAIA_SKIP );
    DEBUG_CASE_STRING( RAIA_CONFLICT );
    DEBUG_CASE_STRING( RAIA_MERGE );
    DEBUG_CASE_STRING( RAIA_SOMETHING );
    DEBUG_CASE_STRING( RAIA_NOTHING );
    DEBUG_CASE_STRING( RAIA_ORPHAN );
    DEBUG_CASE_STRING( RAIA_DELETEOUT );
    DEBUG_CASE_STRING( RAIA_DELETEIN );
    DEBUG_CASE_STRING( RAIA_DONTDELETE );

    default:        return TEXT("Unknown");
        }
    }


LPCTSTR PRIVATE DumpSideItemState(
    UINT uState)        // SI_
    {
    switch (uState)
        {
    DEBUG_CASE_STRING( SI_UNCHANGED );
    DEBUG_CASE_STRING( SI_CHANGED );
    DEBUG_CASE_STRING( SI_NEW );
    DEBUG_CASE_STRING( SI_NOEXIST );
    DEBUG_CASE_STRING( SI_UNAVAILABLE );
    DEBUG_CASE_STRING( SI_DELETED );

    default:        return TEXT("Unknown");
        }
    }


/*----------------------------------------------------------
Purpose: Dumps a twin pair
Returns: --
Cond:    --
*/
void PUBLIC DumpTwinPair(
    LPRA_ITEM pitem)
    {
    if (pitem)
        {
        TCHAR szBuf[MAXMSGLEN];

        #define szDump   TEXT("Dump TWINPAIR: ")
        #define szBlank  TEXT("               ")

        if (IsFlagClear(g_uDumpFlags, DF_TWINPAIR))
            {
            return;
            }

        wsprintf(szBuf, TEXT("%s.pszName = %s\r\n"), (LPTSTR)szDump, Dbg_SafeStr(pitem->pszName));
        OutputDebugString(szBuf);
        wsprintf(szBuf, TEXT("%s.uStyle = %lx\r\n"), (LPTSTR)szBlank, pitem->uStyle);
        OutputDebugString(szBuf);
        wsprintf(szBuf, TEXT("%s.uAction = %s\r\n"), (LPTSTR)szBlank, DumpRecAction(pitem->uAction));
        OutputDebugString(szBuf);

        #undef szDump
        #define szDump   TEXT("       Inside: ")
        wsprintf(szBuf, TEXT("%s.pszDir = %s\r\n"), (LPTSTR)szDump, Dbg_SafeStr(pitem->siInside.pszDir));
        OutputDebugString(szBuf);
        wsprintf(szBuf, TEXT("%s.uState = %s\r\n"), (LPTSTR)szBlank, DumpSideItemState(pitem->siInside.uState));
        OutputDebugString(szBuf);

        #undef szDump
        #define szDump   TEXT("      Outside: ")
        wsprintf(szBuf, TEXT("%s.pszDir = %s\r\n"), (LPTSTR)szDump, Dbg_SafeStr(pitem->siOutside.pszDir));
        OutputDebugString(szBuf);
        wsprintf(szBuf, TEXT("%s.uState = %s\r\n"), (LPTSTR)szBlank, DumpSideItemState(pitem->siOutside.uState));
        OutputDebugString(szBuf);

        #undef szDump
        #undef szBlank
        }
    }


#endif


/*----------------------------------------------------------
Purpose: Create a monochrome bitmap of the bullet, so we can
         play with the colors later.
Returns: handle to bitmap
Cond:    Caller must delete bitmap
*/
HBITMAP PRIVATE CreateBulletBitmap(
    LPSIZE psize)
    {
    HDC hdcMem;
    HBITMAP hbmp = NULL;

    hdcMem = CreateCompatibleDC(NULL);
    if (hdcMem)
        {
        hbmp = CreateCompatibleBitmap(hdcMem, psize->cx, psize->cy);
        if (hbmp)
            {
            HBITMAP hbmpOld;
            RECT rc;

            // hbmp is monochrome

            hbmpOld = SelectBitmap(hdcMem, hbmp);
            rc.left = 0;
            rc.top = 0;
            rc.right = psize->cx;
            rc.bottom = psize->cy;
            DrawFrameControl(hdcMem, &rc, DFC_MENU, DFCS_MENUBULLET);

            SelectBitmap(hdcMem, hbmpOld);
            }
        DeleteDC(hdcMem);
        }
    return hbmp;
    }


/*----------------------------------------------------------
Purpose: Returns the top and bottom indexes of the visible
         entries in the listbox

Returns: --
Cond:    --
*/
void PRIVATE GetVisibleRange(
    HWND hwndLB,
    int * piTop,
    int * piBottom)
    {
    int i;
    int cel;
    int cyMac;
    RECT rc;

    *piTop = ListBox_GetTopIndex(hwndLB);

    cel = ListBox_GetCount(hwndLB);
    GetClientRect(hwndLB, &rc);
    cyMac = 0;

    for (i = *piTop; i < cel; i++)
        {
        if (cyMac > rc.bottom)
            break;

        cyMac += ListBox_GetItemHeight(hwndLB, i);
        }

    *piBottom = i-1;;
    }


/*----------------------------------------------------------
Purpose: Returns the top and bottom indexes of the visible
         entries in the listbox

Returns: --
Cond:    --
*/
int PRIVATE GetHitIndex(
    HWND hwndLB,
    POINT pt)
    {
    int i;
    int iTop;
    int cel;
    int cyMac;
    int cy;
    RECT rc;

    iTop = ListBox_GetTopIndex(hwndLB);

    cel = ListBox_GetCount(hwndLB);
    GetClientRect(hwndLB, &rc);
    cyMac = 0;

    for (i = iTop; i < cel; i++)
        {
        cy = ListBox_GetItemHeight(hwndLB, i);

        if (InRange(pt.y, cyMac, cyMac + cy))
            break;

        cyMac += cy;
        }

    if (i == cel)
        return LB_ERR;

    return i;
    }


/*----------------------------------------------------------
Purpose: Returns the resource ID string given the action
         flag.
Returns: IDS_ value
Cond:    --
*/
UINT PRIVATE GetActionText(
    LPRA_PRIV ppriv)
    {
    UINT ids;

    ASSERT(ppriv);

    switch (ppriv->uAction)
        {
    case RAIA_TOOUT:
        if (SI_NEW == ppriv->siInside.uState ||
            SI_DELETED == ppriv->siOutside.uState)
            {
            ids = IDS_STATE_Creates;
            }
        else
            {
            ids = IDS_STATE_Replaces;
            }
        break;

    case RAIA_TOIN:
        if (SI_NEW == ppriv->siOutside.uState ||
            SI_DELETED == ppriv->siInside.uState)
            {
            ids = IDS_STATE_Creates;
            }
        else
            {
            ids = IDS_STATE_Replaces;
            }
        break;

#ifdef NEW_REC
    case RAIA_DONTDELETE:
        ASSERT(SI_DELETED == ppriv->siInside.uState ||
               SI_DELETED == ppriv->siOutside.uState);

        ids = IDS_STATE_DontDelete;
        break;
#endif

    case RAIA_SKIP:
        // Can occur if the user explicitly wants to skip, or if
        // one side is unavailable.
        ids = IDS_STATE_Skip;
        break;

    case RAIA_CONFLICT:     ids = IDS_STATE_Conflict;       break;
    case RAIA_MERGE:        ids = IDS_STATE_Merge;          break;
    case RAIA_NOTHING:      ids = IDS_STATE_Uptodate;       break;
    case RAIA_SOMETHING:    ids = IDS_STATE_NeedToUpdate;   break;

#ifdef NEW_REC
    case RAIA_DELETEOUT:    ids = IDS_STATE_Delete;         break;
    case RAIA_DELETEIN:     ids = IDS_STATE_Delete;         break;
#endif

    default:                ids = 0;                        break;
        }

    return ids;
    }


/*----------------------------------------------------------
Purpose: Repaint an item in the listbox
Returns: --
Cond:    --
*/
void PRIVATE ListBox_RepaintItemNow(
    HWND hwnd,
    int iItem,
    LPRECT prc,         // Relative to individual entry rect.  May be NULL
    BOOL bEraseBk)
    {
    RECT rc;
    RECT rcItem;

    ListBox_GetItemRect(hwnd, iItem, &rcItem);
    if (prc)
        {
        OffsetRect(prc, rcItem.left, rcItem.top);
        IntersectRect(&rc, &rcItem, prc);
        }
    else
        rc = rcItem;

    InvalidateRect(hwnd, &rc, bEraseBk);
    UpdateWindow(hwnd);
    }


/*----------------------------------------------------------
Purpose: Determine which DOBJ of the item is going to get the caret.

Returns: pointer to DOBJ
Cond:    --
*/
LPDOBJ PRIVATE RecAct_ChooseCaretDobj(
    LPRECACT this,
    LPRA_PRIV ppriv)
    {
    // Focus rect on file icon?
    if (!RecAct_IsNoIcon(this))
        return ppriv->rgdobj;                   // Yes
    else
        return &ppriv->rgdobj[IDOBJ_ACTION];    // No
    }


/*----------------------------------------------------------
Purpose: Returns the tool tip ID for the visible rectangle
         that the given item is currently occupying.

Returns: see above
Cond:    --
*/
UINT PRIVATE RecAct_GetTipIDFromItemID(
    LPRECACT this,
    int itemID)
    {
    int iTop;
    int iBottom;
    int idsa;
    UINT uID;

    GetVisibleRange(this->hwndLB, &iTop, &iBottom);
    ASSERT(iTop <= itemID);
    ASSERT(itemID <= iBottom);

    idsa = itemID - iTop;
    if ( !DSA_GetItem(this->hdsa, idsa, &uID) )
        {
        // This region has not been added yet
        uID = this->cTipID;

        if (-1 != DSA_SetItem(this->hdsa, idsa, &uID))
            {
            TOOLINFO ti;

            ti.cbSize = sizeof(ti);
            ti.uFlags = 0;
            ti.hwnd = this->hwndLB;
            ti.uId = uID;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0;
            SendMessage(this->hwndTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

            ti.uId++;
            SendMessage(this->hwndTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

            this->cTipID += 2;
            }
        }

    return uID;
    }


/*----------------------------------------------------------
Purpose: Finds a listbox item given the tip ID.

Returns: item index
Cond:    --
*/
int PRIVATE RecAct_FindItemFromTipID(
    LPRECACT this,
    UINT uTipID,
    BOOL * pbInside)
    {
    int iTop;
    int iBottom;
    int iVisibleItem = uTipID / 2;
    int iItem;

    ASSERT(0 <= iVisibleItem);

    GetVisibleRange(this->hwndLB, &iTop, &iBottom);
    if (iVisibleItem <= iBottom - iTop)
        {
        iItem = iTop + iVisibleItem;

        if (uTipID % 2)
            *pbInside = FALSE;
        else
            *pbInside = TRUE;
        }
    else
        iItem = LB_ERR;

    return iItem;
    }


/*----------------------------------------------------------
Purpose: Send selection change notification
Returns:
Cond:    --
*/
BOOL PRIVATE RecAct_SendSelChange(
    LPRECACT this,
    int isel)
    {
    NM_RECACT nm;

    nm.iItem = isel;
    nm.mask = 0;

    if (isel != -1)
        {
        LPRA_ITEM pitem;

        ListBox_GetText(this->hwndLB, isel, &pitem);
        if (!pitem)
            return FALSE;

        nm.lParam = pitem->lParam;
        nm.mask |= RAIF_LPARAM;
        }

    return !SendNotify(GetParent(this->hwnd), this->hwnd, RN_SELCHANGED, &nm.hdr);
    }


/*----------------------------------------------------------
Purpose: Send an action change notification
Returns:
Cond:    --
*/
BOOL PRIVATE RecAct_SendItemChange(
    LPRECACT this,
    int iEntry,
    UINT uActionOld)
    {
    NM_RECACT nm;

    nm.iItem = iEntry;
    nm.mask = 0;

    if (iEntry != -1)
        {
        LPRA_PRIV ppriv;

        ListBox_GetText(this->hwndLB, iEntry, &ppriv);
        if (!ppriv)
            return FALSE;

        nm.mask |= RAIF_LPARAM | RAIF_ACTION;
        nm.lParam = ppriv->lParam;
        nm.uAction = ppriv->uAction;
        nm.uActionOld = uActionOld;
        }

    return !SendNotify(GetParent(this->hwnd), this->hwnd, RN_ITEMCHANGED, &nm.hdr);
    }


/*----------------------------------------------------------
Purpose: Create the action context menu
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE RecAct_CreateMenu(
    LPRECACT this)
    {
    HMENU hmenu;

    hmenu = CreatePopupMenu();
    if (hmenu)
        {
        TCHAR sz[MAXSHORTLEN];
        MENUITEMINFO mii;
        int i;

        // Add the help menu items now, since these will be standard
        //
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
        mii.fType = MFT_STRING;
        mii.fState = MFS_ENABLED;

        for (i = 0; i < ARRAYSIZE(c_rghmid); i++)
            {
            mii.wID = c_rghmid[i].idm;
            mii.dwTypeData = SzFromIDS(c_rghmid[i].ids, sz, ARRAYSIZE(sz));
            InsertMenuItem(hmenu, i, TRUE, &mii);
            }

        this->hmenu = hmenu;
        }

    return hmenu != NULL;
    }


/*----------------------------------------------------------
Purpose: Add the action menu items to the context menu
Returns: --
Cond:    --
*/
void PRIVATE AddActionsToContextMenu(
    HMENU hmenu,
    UINT idmCheck,      // menu item to checkmark
    LPRA_PRIV ppriv)
    {
    MENUITEMINFO mii;
    int i;
    int cItems = ARRAYSIZE(c_rgramid);

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
    mii.fType = MFT_OWNERDRAW;
    mii.fState = MFS_ENABLED;

    // Is merge supported?
    if (IsFlagClear(ppriv->uStyle, RAIS_CANMERGE))
        {
        // No
        --cItems;
        }

    for (i = 0; i < cItems; i++)
        {
        mii.wID = c_rgramid[i].idm;
        mii.dwItemData = (DWORD_PTR)&c_rgramid[i];

        InsertMenuItem(hmenu, i, TRUE, &mii);
        }

    // Add the separator
    mii.fMask = MIIM_TYPE;
    mii.fType = MFT_SEPARATOR;
    InsertMenuItem(hmenu, i, TRUE, &mii);

    // Set the initial checkmark.
    CheckMenuRadioItem(hmenu, IDM_ACTIONFIRST, IDM_ACTIONLAST, idmCheck,
        MF_BYCOMMAND | MF_CHECKED);

    // Is the file or its sync copy unavailable?
    if (SI_UNAVAILABLE == ppriv->siInside.uState ||
        SI_UNAVAILABLE == ppriv->siOutside.uState)
        {
        // Yes
        mii.fMask = MIIM_STATE;
        mii.fState = MFS_GRAYED | MFS_DISABLED;
        SetMenuItemInfo(hmenu, IDM_TOIN, FALSE, &mii);
        SetMenuItemInfo(hmenu, IDM_TOOUT, FALSE, &mii);
        SetMenuItemInfo(hmenu, IDM_MERGE, FALSE, &mii);
        }

    // Is the file being created?
    else if (ppriv->siInside.uState == SI_NEW ||
        ppriv->siOutside.uState == SI_NEW)
        {
        // Yes; disable the replace-in-opposite direction
        UINT idmDisable;
        UINT idmChangeVerb;

        if (ppriv->siInside.uState == SI_NEW)
            {
            idmDisable = IDM_TOIN;
            idmChangeVerb = IDM_TOOUT;
            i = IRAMID_CREATEOUT;
            }
        else
            {
            idmDisable = IDM_TOOUT;
            idmChangeVerb = IDM_TOIN;
            i = IRAMID_CREATEIN;
            }

        // Disable one of the directions
        mii.fMask = MIIM_STATE;
        mii.fState = MFS_GRAYED | MFS_DISABLED;
        SetMenuItemInfo(hmenu, idmDisable, FALSE, &mii);

        // Change the verb of the other direction
        mii.fMask = MIIM_DATA;
        mii.dwItemData = (DWORD_PTR)&c_rgramidCreates[i];

        SetMenuItemInfo(hmenu, idmChangeVerb, FALSE, &mii);
        }

#ifdef NEW_REC
    // Is the file being deleted?
    else if (SI_DELETED == ppriv->siInside.uState ||
        SI_DELETED == ppriv->siOutside.uState)
        {
        // Yes;
        UINT idmCreate;
        UINT idmChangeVerb;
        UINT iCreate;

        if (SI_DELETED == ppriv->siInside.uState)
            {
            idmCreate = IDM_TOIN;
            iCreate = IRAMID_CREATEIN;

            idmChangeVerb = IDM_TOOUT;
            i = IRAMID_DELETEOUT;
            }
        else
            {
            ASSERT(SI_DELETED == ppriv->siOutside.uState);

            idmCreate = IDM_TOOUT;
            iCreate = IRAMID_CREATEOUT;

            idmChangeVerb = IDM_TOIN;
            i = IRAMID_DELETEIN;
            }

        // Change one of the directions to be create
        mii.fMask = MIIM_DATA;
        mii.dwItemData = (DWORD_PTR)&c_rgramidCreates[iCreate];
        SetMenuItemInfo(hmenu, idmCreate, FALSE, &mii);

        // Change the verb of the other direction
        mii.fMask = MIIM_DATA | MIIM_ID;
        mii.wID = c_rgramidDeletes[i].idm;
        mii.dwItemData = (DWORD_PTR)&c_rgramidDeletes[i];

        SetMenuItemInfo(hmenu, idmChangeVerb, FALSE, &mii);

        // Change the skip verb to be "Don't Delete"
        mii.fMask = MIIM_DATA | MIIM_ID;
        mii.wID = c_rgramidDeletes[IRAMID_DONTDELETE].idm;
        mii.dwItemData = (DWORD_PTR)&c_rgramidDeletes[IRAMID_DONTDELETE];

        SetMenuItemInfo(hmenu, IDM_SKIP, FALSE, &mii);
        }
#endif
    }


/*----------------------------------------------------------
Purpose: Clear out the context menu
Returns: --
Cond:    --
*/
void PRIVATE ResetContextMenu(
    HMENU hmenu)
    {
    int cnt;

    // If there is more than just the help items, remove them
    //  (but leave the help items)
    //
    cnt = GetMenuItemCount(hmenu);
    if (cnt > ARRAYSIZE(c_rghmid))
        {
        int i;

        cnt -= ARRAYSIZE(c_rghmid);
        for (i = 0; i < cnt; i++)
            {
            DeleteMenu(hmenu, 0, MF_BYPOSITION);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Do the context menu
Returns: --
Cond:    --
*/
void PRIVATE RecAct_DoContextMenu(
    LPRECACT this,
    int x,              // in screen coords
    int y,
    int iEntry,
    BOOL bHelpOnly)     // TRUE: only show the help items
    {
    UINT idCmd;

    if (this->hmenu)
        {
        LPRA_PRIV ppriv;
        RECT rc;
        int idmCheck;
        UINT uActionOld;

        // Only show help-portion of context menu?
        if (bHelpOnly)
            {
            // Yes
            ppriv = NULL;
            }
        else
            {
            // No
            ListBox_GetText(this->hwndLB, iEntry, &ppriv);

            // Determine if this is a help-context menu only.
            //  It is if this is a folder-item or if there is no action
            //  to take.
            //
            ASSERT(ppriv->uAction < ARRAYSIZE(c_mpraiaidmMenu));
            idmCheck = c_mpraiaidmMenu[ppriv->uAction];

            // Build the context menu
            //
            if (IsFlagClear(ppriv->uStyle, RAIS_FOLDER) && idmCheck != 0)
                {
                AddActionsToContextMenu(this->hmenu, idmCheck, ppriv);
                }
            }

        // Show context menu
        //
        SendMessage(this->hwndTip, TTM_ACTIVATE, FALSE, 0L);

        idCmd = TrackPopupMenu(this->hmenu,
                    TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    x, y, 0, this->hwnd, NULL);

        SendMessage(this->hwndTip, TTM_ACTIVATE, TRUE, 0L);

        // Clear menu
        //
        ResetContextMenu(this->hmenu);

        if (ppriv)
            {
            // Save the old action
            uActionOld = ppriv->uAction;
            }

        // Act on whatever the user chose
        switch (idCmd)
            {
        case IDM_TOOUT:
            ppriv->uAction = RAIA_TOOUT;
            break;

        case IDM_TOIN:
            ppriv->uAction = RAIA_TOIN;
            break;

        case IDM_SKIP:
            ppriv->uAction = RAIA_SKIP;
            break;

        case IDM_MERGE:
            ppriv->uAction = RAIA_MERGE;
            break;

#ifdef NEW_REC
        case IDM_DELETEOUT:
            ppriv->uAction = RAIA_DELETEOUT;
            break;

        case IDM_DELETEIN:
            ppriv->uAction = RAIA_DELETEIN;
            break;

        case IDM_DONTDELETE:
            ppriv->uAction = RAIA_DONTDELETE;
            break;
#endif

        case IDM_WHATSTHIS:
            WinHelp(this->hwnd, c_szWinHelpFile, HELP_CONTEXTPOPUP, IDH_BFC_UPDATE_SCREEN);
            return;         // Return now

        default:
            return;         // Return now
            }

        // Repaint action portion of entry
        ppriv->cx = RECOMPUTE;
        rc = ppriv->rgdobj[IDOBJ_ACTION].rcBounding;
        ListBox_RepaintItemNow(this->hwndLB, iEntry, &rc, TRUE);

        // Send a notify message
        ASSERT(NULL != ppriv);      // uActionOld should be valid
        RecAct_SendItemChange(this, iEntry, uActionOld);
        }
    }


/*----------------------------------------------------------
Purpose: Create the windows for this control
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE RecAct_CreateWindows(
    LPRECACT this,
    CREATESTRUCT  * lpcs)
    {
    HWND hwnd = this->hwnd;
    HWND hwndLB;
    RECT rc;
    int cxEdge = GetSystemMetrics(SM_CXEDGE);
    int cyEdge = GetSystemMetrics(SM_CYEDGE);
    TOOLINFO ti;

    // Create listbox
    hwndLB = CreateWindowEx(
                0,
                TEXT("listbox"),
                TEXT(""),
                WS_CHILD | WS_CLIPSIBLINGS | LBS_SORT | LBS_OWNERDRAWVARIABLE |
                WS_VSCROLL | WS_TABSTOP | WS_VISIBLE | LBS_NOINTEGRALHEIGHT |
                LBS_NOTIFY,
                0, 0, lpcs->cx, lpcs->cy,
                hwnd,
                NULL,
                lpcs->hInstance,
                0L);
    if (!hwndLB)
        return FALSE;

    SetWindowFont(hwndLB, this->hfont, FALSE);

    this->hwndLB = hwndLB;

    // Determine layout of window
    GetClientRect(hwnd, &rc);
    InflateRect(&rc, -cxEdge, -cyEdge);
    SetWindowPos(hwndLB, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
        SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);

    GetClientRect(hwndLB, &rc);
    this->cxItem = rc.right - rc.left;

    this->hwndTip = CreateWindow(
                TOOLTIPS_CLASS,
                c_szNULL,
                WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                CW_USEDEFAULT, CW_USEDEFAULT,
                CW_USEDEFAULT, CW_USEDEFAULT,
                hwnd,
                NULL,
                lpcs->hInstance,
                0L);

    // Add a dummy tool so the delay is shorter between other tools
    ti.cbSize = sizeof(ti);
    ti.uFlags = TTF_IDISHWND;
    ti.hwnd = this->hwndLB;
    ti.uId = (UINT_PTR)this->hwndLB;
    ti.lpszText = (LPTSTR)c_szNULL;
    ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0;
    SendMessage(this->hwndTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Set the colors of the control
Returns: --
Cond:    --
*/
void PRIVATE RecAct_SetColors(
    LPRECACT this)
    {
    int cr;

    if (IsFlagClear(this->lStyle, RAS_SINGLEITEM))
        {
        cr = COLOR_WINDOW;
        }
    else
        {
        cr = COLOR_3DFACE;
        }

    this->clrBkgnd = GetSysColor(cr);

    if (this->hbrBkgnd)
        DeleteBrush(this->hbrBkgnd);

    this->hbrBkgnd = CreateSolidBrush(this->clrBkgnd);
    }


/*----------------------------------------------------------
Purpose: Creates an imagelist of the action images

Returns: TRUE on success

Cond:    --
*/
BOOL PRIVATE CreateImageList(
    HIMAGELIST * phiml,
    HDC hdc,
    UINT idb,
    int cxBmp,
    int cyBmp,
    int cImage,
    UINT flags
    )
    {
    BOOL bRet;
    HIMAGELIST himl;

    himl = ImageList_Create(cxBmp, cyBmp, flags, cImage, 1);

    if (himl)
        {
        COLORREF clrMask;
        HBITMAP hbm;

        hbm = LoadBitmap(g_hinst, MAKEINTRESOURCE(idb));
        ASSERT(hbm);

        if (hbm)
            {
            HDC hdcMem = CreateCompatibleDC(hdc);
            if (hdcMem)
                {
                HBITMAP hbmSav = SelectBitmap(hdcMem, hbm);

                clrMask = GetPixel(hdcMem, 0, 0);
                SelectBitmap(hdcMem, hbmSav);

                bRet = (0 == ImageList_AddMasked(himl, hbm, clrMask));

                DeleteDC(hdcMem);
                }
            else
                bRet = FALSE;

            DeleteBitmap(hbm);
            }
        else
            bRet = FALSE;
        }
    else
        bRet = FALSE;

    *phiml = himl;
    return bRet;
    }


/*----------------------------------------------------------
Purpose: WM_CREATE handler
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE RecAct_OnCreate(
    LPRECACT this,
    CREATESTRUCT  * lpcs)
    {
    BOOL bRet = FALSE;
    HWND hwnd = this->hwnd;
    HDC hdc;
    TEXTMETRIC tm;
    RECT rcT;
    LOGFONT lf;
    UINT flags = ILC_MASK;
    this->lStyle = GetWindowLong(hwnd, GWL_STYLE);
    RecAct_SetColors(this);

    // Determine some font things

    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
    this->hfont = CreateFontIndirect(&lf);

    // This window is registered with the CS_OWNDC flag
    this->hdcOwn = GetDC(hwnd);
    ASSERT(this->hdcOwn);

    hdc = this->hdcOwn;

    SelectFont(hdc, this->hfont);
    GetTextMetrics(hdc, &tm);
    this->cyText = tm.tmHeight;

    // Calculate text extent for sideitems (use the listbox font)
    //
    SetRectFromExtent(hdc, &rcT, c_szEllipses);
    this->cxEllipses = rcT.right - rcT.left;

    // Create windows used by control
    if (RecAct_CreateWindows(this, lpcs))
        {
        this->lpfnLBProc = SubclassWindow(this->hwndLB, RecActLB_LBProc);

        this->hdsa = DSA_Create(sizeof(int), 16);
        if (this->hdsa)
            {
            // Get the system imagelist cache
            this->himlCache = ImageList_Create(g_cxIcon, g_cyIcon, TRUE, 8, 8);
            if (this->himlCache)
                {
                if(IS_WINDOW_RTL_MIRRORED(hwnd))
                {
                    flags |= ILC_MIRROR;
                }
                if (CreateImageList(&this->himlAction, hdc, IDB_ACTIONS,
                    CX_ACTIONBMP, CY_ACTIONBMP, 8, flags))
                    {
                    SIZE size;

                    // Get some metrics
                    this->cxMenuCheck = GetSystemMetrics(SM_CXMENUCHECK);
                    this->cyMenuCheck = GetSystemMetrics(SM_CYMENUCHECK);

                    size.cx = this->cxMenuCheck;
                    size.cy = this->cyMenuCheck;
                    this->hbmpBullet = CreateBulletBitmap(&size);
                    if (this->hbmpBullet)
                        {
                        bRet = RecAct_CreateMenu(this);
                        }
                    }
                }
            }
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: WM_DESTROY Handler
Returns: --
Cond:    --
*/
void PRIVATE RecAct_OnDestroy(
    LPRECACT this)
    {
    if (this->himlCache)
        {
        ImageList_Destroy(this->himlCache);
        this->himlCache = NULL;
        }

    if (this->himlAction)
        {
        ImageList_Destroy(this->himlAction);
        this->himlAction = NULL;
        }

    if (this->hbmpBullet)
        {
        DeleteBitmap(this->hbmpBullet);
        this->hbmpBullet = NULL;
        }

    if (this->hmenu)
        {
        DestroyMenu(this->hmenu);
        this->hmenu = NULL;
        }

    if (this->hbrBkgnd)
        DeleteBrush(this->hbrBkgnd);

    if (this->hfont)
        DeleteFont(this->hfont);

    if (this->hdsa)
        DSA_Destroy(this->hdsa);
    }


/*----------------------------------------------------------
Purpose: WM_COMMAND Handler
Returns: --
Cond:    --
*/
VOID PRIVATE RecAct_OnCommand(
    LPRECACT this,
    int id,
    HWND hwndCtl,
    UINT uNotifyCode)
    {
    if (hwndCtl == this->hwndLB)
        {
        switch (uNotifyCode)
            {
        case LBN_SELCHANGE:
            break;
            }
        }
    }


/*----------------------------------------------------------
Purpose: Handles WM_SYSKEYDOWN

Returns: 0 if we processed it

Cond:    --
*/
int PRIVATE RecAct_OnSysKeyDown(
    LPRECACT this,
    UINT vkey,
    LPARAM lKeyData)
    {
    int nRet = -1;

    // Context menu invoked by the keyboard?
    if (VK_F10 == vkey && 0 > GetKeyState(VK_SHIFT))
        {
        // Yes; forward the message
        HWND hwndLB = this->hwndLB;
        int iCaret = ListBox_GetCurSel(hwndLB);

        // Is this in a property page?
        if (RecAct_IsNoIcon(this) && 0 > iCaret)
            {
            // Yes; don't require the item to be selected
            iCaret = 0;
            }

        if (0 <= iCaret)
            {
            LPRA_PRIV ppriv;
            LPDOBJ pdobj;
            POINT pt;
            RECT rc;

            // Determine where to show the context menu
            ListBox_GetText(hwndLB, iCaret, &ppriv);
            pdobj = RecAct_ChooseCaretDobj(this, ppriv);

            ListBox_GetItemRect(hwndLB, iCaret, &rc);
            pt.x = pdobj->x + (g_cxIcon / 2) + rc.left;
            pt.y = pdobj->y + (g_cyIcon / 2) + rc.top;
            ClientToScreen(hwndLB, &pt);

            PostMessage(this->hwnd, WM_CONTEXTMENU, (WPARAM)hwndLB, MAKELPARAM(pt.x, pt.y));
            }
        nRet = 0;
        }

    return nRet;
    }

// ( (4+1) is for ellipses )
#define MAX_HALF    (ARRAYSIZE(pttt->szText)/2 - (4+1))

/*----------------------------------------------------------
Purpose: Handles TTN_NEEDTEXT

Returns: --
Cond:    --
*/
void PRIVATE RecAct_OnNeedTipText(
    LPRECACT this,
    LPTOOLTIPTEXT pttt)
    {
    // Find the visible listbox item associated with this tip ID.
    HWND hwndLB = this->hwndLB;
    LPRA_PRIV ppriv;
    int iItem;
    BOOL bInside;
    SIDEITEM * psi;

    iItem = RecAct_FindItemFromTipID(this, (UINT)pttt->hdr.idFrom, &bInside);

    if (LB_ERR != iItem)
        {
        int cb;

        ListBox_GetText(hwndLB, iItem, &ppriv);
        if (bInside)
            psi = &ppriv->siInside;
        else
            psi = &ppriv->siOutside;

        // Need ellipses?
        cb = CbFromCch(lstrlen(psi->pszDir));
        if (cb >= sizeof(pttt->szText))
            {
            // Yes
            LPTSTR pszLastHalf;
            LPTSTR psz;
            LPTSTR pszStart = psi->pszDir;
            LPTSTR pszEnd = &psi->pszDir[lstrlen(psi->pszDir)];

            for (psz = pszEnd;
                psz != pszStart && (pszEnd - psz) < MAX_HALF;
                psz = CharPrev(pszStart, psz))
                ;

            pszLastHalf = CharNext(psz);
            lstrcpyn(pttt->szText, psi->pszDir, MAX_HALF);
            lstrcat(pttt->szText, c_szEllipses);
            lstrcat(pttt->szText, pszLastHalf);
            }
        else
            lstrcpyn(pttt->szText, psi->pszDir, ARRAYSIZE(pttt->szText));
        }
    else
        *pttt->szText = 0;
    }


/*----------------------------------------------------------
Purpose: WM_NOTIFY handler
Returns: varies
Cond:    --
*/
LRESULT PRIVATE RecAct_OnNotify(
    LPRECACT this,
    int idFrom,
    NMHDR  * lpnmhdr)
    {
    LRESULT lRet = 0;

    switch (lpnmhdr->code)
        {
    case HDN_BEGINTRACK:
        lRet = TRUE;       // prevent tracking
        break;

    default:
        break;
        }

    return lRet;
    }


/*----------------------------------------------------------
Purpose: WM_CONTEXTMENU handler
Returns: --
Cond:    --
*/
void PRIVATE RecAct_OnContextMenu(
    LPRECACT this,
    HWND hwnd,
    int x,
    int y)
    {
    if (hwnd == this->hwndLB)
        {
        POINT pt;
        int iHitEntry;
        BOOL bHelpOnly;

        pt.x = x;
        pt.y = y;
        ScreenToClient(hwnd, &pt);

        iHitEntry = GetHitIndex(hwnd, pt);
        if (LB_ERR != iHitEntry)
            {
            ASSERT(iHitEntry < ListBox_GetCount(hwnd));

            ListBox_SetCurSel(hwnd, iHitEntry);
            ListBox_RepaintItemNow(hwnd, iHitEntry, NULL, FALSE);

            bHelpOnly = FALSE;
            }
        else
            bHelpOnly = TRUE;

        // Bring up the context menu for the listbox
        RecAct_DoContextMenu(this, x, y, iHitEntry, bHelpOnly);
        }
    }


/*----------------------------------------------------------
Purpose: Calculate the rectangle boundary of a sideitem

Returns: calculated rect
Cond:    --
*/
void PRIVATE RecAct_CalcSideItemRect(
    LPRECACT this,
    int nSide,          // SIDE_INSIDE or SIDE_OUTSIDE
    int cxFile,
    int cxAction,
    LPRECT prcOut)
    {
    int x;
    int y = g_cyIconMargin*2;
    int cx = ((this->cxItem - cxFile - cxAction) / 2);

    switch (nSide)
        {
    case SIDE_INSIDE:
        if (RecAct_IsNoIcon(this))
            x = 0;
        else
            x = cxFile;
        break;

    case SIDE_OUTSIDE:
        if (RecAct_IsNoIcon(this))
            x = cx + cxAction;
        else
            x = cxFile + cx + cxAction;
        break;

    default:
        ASSERT(0);
        break;
        }

    x += g_cxMargin;

    prcOut->left   = x + g_cxMargin;
    prcOut->top    = y;
    prcOut->right  = prcOut->left + (cx - 2*g_cxMargin);
    prcOut->bottom = y + (this->cyText * 3);
    }


/*----------------------------------------------------------
Purpose: Draw a reconciliation listbox entry
Returns: --
Cond:    --
*/
void PRIVATE RecAct_RecomputeItemMetrics(
    LPRECACT this,
    LPRA_PRIV ppriv)
    {
    HDC hdc = this->hdcOwn;
    LPDOBJ pdobj = ppriv->rgdobj;
    RECT rcT;
    RECT rcUnion;
    TCHAR szIDS[MAXBUFLEN];
    UINT ids;
    int cyText = this->cyText;
    int dx;
    int cxFile;
    int cxAction;
    POINT pt;

    // Compute the metrics and dimensions of each of the draw objects
    // and store back into the item.

    // File icon and label

    pt.x = 0;
    pt.y = 0;
    ComputeImageRects(FIGetDisplayName(ppriv->pfi), hdc, &pt, &rcT,
        &pdobj->rcLabel, g_cxIcon, g_cyIcon, g_cxIconSpacing, cyText);

    pdobj->uKind = DOK_IMAGE;
    pdobj->lpvObject = FIGetDisplayName(ppriv->pfi);
    pdobj->uFlags = DOF_DIFFER | DOF_CENTER;
    if (RecAct_IsNoIcon(this))
        {
        SetFlag(pdobj->uFlags, DOF_NODRAW);
        cxFile = 0;
        }
    else
        {
        cxFile = rcT.right - rcT.left;
        }
    pdobj->x = pt.x;
    pdobj->y = pt.y;
    pdobj->himl = this->himlCache;
    pdobj->iImage = (UINT)ppriv->pfi->lParam;
    pdobj->rcBounding = rcT;

    rcUnion = pdobj->rcBounding;

    // Action image

    ASSERT(ppriv->uAction <= ARRAYSIZE(c_mpraiaiImage));

    pdobj++;

    ids = GetActionText(ppriv);
    pt.x = 0;       // (we'll adjust this after the call)
    pt.y = 0;
    ComputeImageRects(SzFromIDS(ids, szIDS, ARRAYSIZE(szIDS)), hdc, &pt,
        &rcT, &pdobj->rcLabel, CX_ACTIONBMP, CY_ACTIONBMP,
        g_cxIconSpacing, cyText);

    // (Adjust pt and the two rects to be centered in the remaining space)
    cxAction = rcT.right - rcT.left;
    dx = cxFile + (((this->cxItem - cxFile) / 2) - (cxAction / 2));
    pt.x += dx;
    OffsetRect(&rcT, dx, 0);
    OffsetRect(&pdobj->rcLabel, dx, 0);

    pdobj->uKind = DOK_IMAGE;
    pdobj->lpvObject = (LPVOID)ids;
    pdobj->uFlags = DOF_CENTER | DOF_USEIDS;
    if (!RecAct_IsNoIcon(this))
        SetFlag(pdobj->uFlags, DOF_IGNORESEL);
    pdobj->x = pt.x;
    pdobj->y = pt.y;
    pdobj->himl = this->himlAction;
    pdobj->iImage = c_mpraiaiImage[ppriv->uAction];
    pdobj->rcBounding = rcT;

    UnionRect(&rcUnion, &rcUnion, &pdobj->rcBounding);

    // Sideitem Info (Inside Briefcase)

    RecAct_CalcSideItemRect(this, SIDE_INSIDE, cxFile, cxAction, &rcT);

    pdobj++;
    pdobj->uKind = DOK_SIDEITEM;
    pdobj->lpvObject = &ppriv->siInside;
    pdobj->uFlags = DOF_LEFT;
    pdobj->x = rcT.left;
    pdobj->y = rcT.top;
    pdobj->rcClip = rcT;
    pdobj->rcBounding = rcT;

    // Sideitem Info (Outside Briefcase)

    RecAct_CalcSideItemRect(this, SIDE_OUTSIDE, cxFile, cxAction, &rcT);

    pdobj++;
    pdobj->uKind = DOK_SIDEITEM;
    pdobj->lpvObject = &ppriv->siOutside;
    pdobj->uFlags = DOF_LEFT;
    pdobj->x = rcT.left;
    pdobj->y = rcT.top;
    pdobj->rcClip = rcT;
    pdobj->rcBounding = rcT;

    UnionRect(&rcUnion, &rcUnion, &rcT);

    // Set the bounding rect of this item.
    ppriv->cx = rcUnion.right - rcUnion.left;
    ppriv->cy = max((rcUnion.bottom - rcUnion.top), g_cyIconSpacing);
    }


/*----------------------------------------------------------
Purpose: WM_MEASUREITEM handler
Returns: --
Cond:    --
*/
BOOL PRIVATE RecAct_OnMeasureItem(
    LPRECACT this,
    LPMEASUREITEMSTRUCT lpmis)
    {
    HDC hdc = this->hdcOwn;

    switch (lpmis->CtlType)
        {
    case ODT_LISTBOX: {
        LPRA_PRIV ppriv = (LPRA_PRIV)lpmis->itemData;

        // Recompute item metrics?
        if (RECOMPUTE == ppriv->cx)
            {
            RecAct_RecomputeItemMetrics(this, ppriv);   // Yes
            }

        lpmis->itemHeight = ppriv->cy;
        }
        return TRUE;

    case ODT_MENU:
        {
        int i;
        int cxMac = 0;
        RECT rc;
        TCHAR sz[MAXBUFLEN];

        // Calculate based on font and image dimensions.
        //
        SelectFont(hdc, this->hfont);

        cxMac = 0;
        for (i = 0; i < ARRAYSIZE(c_rgramid); i++)
            {
            SzFromIDS(c_rgramid[i].ids, sz, ARRAYSIZE(sz));
            SetRectFromExtent(hdc, &rc, sz);
            cxMac = max(cxMac,
                        g_cxMargin + CX_ACTIONBMP + g_cxMargin +
                        (rc.right-rc.left) + g_cxMargin);
            }

        lpmis->itemHeight = max(this->cyText, CY_ACTIONBMP);
        lpmis->itemWidth = cxMac;
        }
        return TRUE;
        }
    return FALSE;
    }


/*----------------------------------------------------------
Purpose: Draw a reconciliation listbox entry
Returns: --
Cond:    --
*/
void PRIVATE RecAct_DrawLBItem(
    LPRECACT this,
    const DRAWITEMSTRUCT  * lpcdis)
    {
    LPRA_PRIV ppriv = (LPRA_PRIV)lpcdis->itemData;
    HDC hdc = lpcdis->hDC;
    RECT rc = lpcdis->rcItem;
    POINT ptSav;
    LPDOBJ pdobj;
    UINT cdobjs;

    if (!ppriv)
        {
        // Empty listbox and we're getting the focus
        return;
        }

    SetBkMode(hdc, TRANSPARENT);        // required for Shell_DrawText
    SetViewportOrgEx(hdc, rc.left, rc.top, &ptSav);

    // The Chicago-look mandates that icon and filename are selected,
    // the rest of the entry is normal.

    // Recompute item metrics?
    if (RECOMPUTE == ppriv->cx)
        {
        RecAct_RecomputeItemMetrics(this, ppriv);   // Yes
        }

    // Do we need to redraw everything?
    if (IsFlagSet(lpcdis->itemAction, ODA_DRAWENTIRE))
        {
        // Yes
        TOOLINFO ti;

        cdobjs = ARRAYSIZE(ppriv->rgdobj);
        pdobj = ppriv->rgdobj;

        // Get the tooltip ID given this ith visible entry
        ti.cbSize = sizeof(ti);
        ti.uFlags = 0;
        ti.hwnd = this->hwndLB;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        ti.uId = RecAct_GetTipIDFromItemID(this, lpcdis->itemID);
        ti.rect = ppriv->rgdobj[IDOBJ_INSIDE].rcBounding;
        OffsetRect(&ti.rect, lpcdis->rcItem.left, lpcdis->rcItem.top);
        SendMessage(this->hwndTip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti);

        ti.uId++;
        ti.rect = ppriv->rgdobj[IDOBJ_OUTSIDE].rcBounding;
        OffsetRect(&ti.rect, lpcdis->rcItem.left, lpcdis->rcItem.top);
        SendMessage(this->hwndTip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti);
        }
    else
        {
        // No; should we even draw the file icon or action icon?
        if (lpcdis->itemAction & (ODA_FOCUS | ODA_SELECT))
            {
            cdobjs = 1;     // Yes
            pdobj = RecAct_ChooseCaretDobj(this, ppriv);
            }
        else
            {
            cdobjs = 0;     // No
            pdobj = ppriv->rgdobj;
            }
        }

    Dobj_Draw(hdc, pdobj, cdobjs, lpcdis->itemState, this->cxEllipses, this->cyText,
        this->clrBkgnd);

    // Clean up
    //
    SetViewportOrgEx(hdc, ptSav.x, ptSav.y, NULL);
    }


/*----------------------------------------------------------
Purpose: Draw an action menu item
Returns: --
Cond:    --
*/
void PRIVATE RecAct_DrawMenuItem(
    LPRECACT this,
    const DRAWITEMSTRUCT  * lpcdis)
    {
    LPRAMID pramid = (LPRAMID)lpcdis->itemData;
    HDC hdc = lpcdis->hDC;
    RECT rc = lpcdis->rcItem;
    DOBJ dobj;
    LPDOBJ pdobj;
    POINT ptSav;
    MENUITEMINFO mii;
    int cx;
    int cy;
    UINT uFlags;
    UINT uFlagsChecked;

    ASSERT(pramid);

    if (lpcdis->itemID == -1)
        return;

    SetViewportOrgEx(hdc, rc.left, rc.top, &ptSav);
    OffsetRect(&rc, -rc.left, -rc.top);

    cx = rc.right - rc.left;
    cy = rc.bottom - rc.top;

    // Get the menu state
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE | MIIM_CHECKMARKS;
    GetMenuItemInfo(this->hmenu, lpcdis->itemID, FALSE, &mii);
    uFlagsChecked = IsFlagClear(mii.fState, MFS_CHECKED) ? DOF_NODRAW : 0;

    uFlags = DOF_DIFFER | DOF_MENU | DOF_USEIDS;
    if (IsFlagSet(mii.fState, MFS_GRAYED))
        SetFlag(uFlags, DOF_DISABLED);

    // Build the array of DObjs that we want to draw.

    // Action image

    pdobj = &dobj;

    pdobj->uKind = DOK_IMAGE;
    pdobj->lpvObject = (LPVOID)pramid->ids;
    pdobj->himl = this->himlAction;
    pdobj->iImage = pramid->iImage;
    pdobj->uFlags = uFlags;
    pdobj->x = g_cxMargin;
    pdobj->y = (cy - CY_ACTIONBMP) / 2;
    pdobj->rcLabel.left = 0;
    pdobj->rcLabel.right = cx;
    pdobj->rcLabel.top = 0;
    pdobj->rcLabel.bottom = cy;

    // Draw the entry...
    //
    Dobj_Draw(hdc, &dobj, 1, lpcdis->itemState, 0, this->cyText, this->clrBkgnd);

    // Clean up
    //
    SetViewportOrgEx(hdc, ptSav.x, ptSav.y, NULL);
    }


/*----------------------------------------------------------
Purpose: WM_DRAWITEM handler
Returns: --
Cond:    --
*/
BOOL PRIVATE RecAct_OnDrawItem(
    LPRECACT this,
    const DRAWITEMSTRUCT  * lpcdis)
    {
    switch (lpcdis->CtlType)
        {
    case ODT_LISTBOX:
        RecAct_DrawLBItem(this, lpcdis);
        return TRUE;

    case ODT_MENU:
        RecAct_DrawMenuItem(this, lpcdis);
        return TRUE;
        }
    return FALSE;
    }


/*----------------------------------------------------------
Purpose: WM_COMPAREITEM handler
Returns: -1 (item 1 precedes item 2), 0 (equal), 1 (item 2 precedes item 1)
Cond:    --
*/
int PRIVATE RecAct_OnCompareItem(
    LPRECACT this,
    const COMPAREITEMSTRUCT  * lpcis)
    {
    LPRA_PRIV ppriv1 = (LPRA_PRIV)lpcis->itemData1;
    LPRA_PRIV ppriv2 = (LPRA_PRIV)lpcis->itemData2;

    // We sort based on name of file
    //
    return lstrcmpi(FIGetPath(ppriv1->pfi), FIGetPath(ppriv2->pfi));
    }


/*----------------------------------------------------------
Purpose: WM_DELETEITEM handler
Returns: --
Cond:    --
*/
void RecAct_OnDeleteLBItem(
    LPRECACT this,
    const DELETEITEMSTRUCT  * lpcdis)
    {
    switch (lpcdis->CtlType)
        {
    case ODT_LISTBOX:
        {
        LPRA_PRIV ppriv = (LPRA_PRIV)lpcdis->itemData;

        ASSERT(ppriv);

        if (ppriv)
            {
            FIFree(ppriv->pfi);

            GFree(ppriv->siInside.pszDir);
            GFree(ppriv->siOutside.pszDir);
            GFree(ppriv);
            }
        }
        break;
        }
    }


/*----------------------------------------------------------
Purpose: WM_CTLCOLORLISTBOX handler
Returns: --
Cond:    --
*/
HBRUSH PRIVATE RecAct_OnCtlColorListBox(
    LPRECACT this,
    HDC hdc,
    HWND hwndLB,
    int nType)
    {
    return this->hbrBkgnd;
    }


/*----------------------------------------------------------
Purpose: WM_PAINT handler
Returns: --
Cond:    --
*/
void RecAct_OnPaint(
    LPRECACT this)
    {
    HWND hwnd = this->hwnd;
    PAINTSTRUCT ps;
    RECT rc;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    if (IsFlagSet(this->lStyle, RAS_SINGLEITEM))
        {
        DrawEdge(hdc, &rc, BDR_SUNKENINNER, BF_TOPLEFT);
        DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_BOTTOMRIGHT);
        }
    else
        {
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT);
        }

    EndPaint(hwnd, &ps);
    }


/*----------------------------------------------------------
Purpose: WM_SETFONT handler
Returns: --
Cond:    --
*/
void RecAct_OnSetFont(
    LPRECACT this,
    HFONT hfont,
    BOOL bRedraw)
    {
    this->hfont = hfont;
    FORWARD_WM_SETFONT(this->hwnd, hfont, bRedraw, RecAct_DefProc);
    }


/*----------------------------------------------------------
Purpose: WM_SETFOCUS handler
Returns: --
Cond:    --
*/
void RecAct_OnSetFocus(
    LPRECACT this,
    HWND hwndOldFocus)
    {
    SetFocus(this->hwndLB);
    }


/*----------------------------------------------------------
Purpose: WM_SYSCOLORCHANGE handler
Returns: --
Cond:    --
*/
void RecAct_OnSysColorChange(
    LPRECACT this)
    {
    RecAct_SetColors(this);
    InvalidateRect(this->hwnd, NULL, TRUE);
    }


/*----------------------------------------------------------
Purpose: Insert item
Returns: index
Cond:    --
*/
int PRIVATE RecAct_OnInsertItem(
    LPRECACT this,
    const LPRA_ITEM pitem)
    {
    HWND hwndLB = this->hwndLB;
    LPRA_PRIV pprivNew;
    TCHAR szPath[MAXPATHLEN];
    int iRet = -1;
    int iItem = LB_ERR;

    ASSERT(pitem);
    ASSERT(pitem->siInside.pszDir);
    ASSERT(pitem->siOutside.pszDir);
    ASSERT(pitem->pszName);

    pprivNew = GAlloc(sizeof(*pprivNew));
    if (pprivNew)
        {
        SetWindowRedraw(hwndLB, FALSE);

        // Fill the prerequisite fields first
        //
        pprivNew->uStyle = pitem->uStyle;
        pprivNew->uAction = pitem->uAction;

        // Set the fileinfo stuff and large icon system-cache index.
        //  If we can't get the fileinfo of the inside file, get the outside
        //  file.  If neither can be found, then we fail
        //
        lstrcpy(szPath, SkipDisplayJunkHack(&pitem->siInside));
        if (IsFlagClear(pitem->uStyle, RAIS_FOLDER))
            PathAppend(szPath, pitem->pszName);

        if (FAILED(FICreate(szPath, &pprivNew->pfi, FIF_ICON)))
            {
            // Try the outside file
            //
            lstrcpy(szPath, SkipDisplayJunkHack(&pitem->siOutside));
            if (IsFlagClear(pitem->uStyle, RAIS_FOLDER))
                PathAppend(szPath, pitem->pszName);

            if (FAILED(FICreate(szPath, &pprivNew->pfi, FIF_ICON)))
                {
                // Don't try to touch the file
                if (FAILED(FICreate(szPath, &pprivNew->pfi, FIF_ICON | FIF_DONTTOUCH)))
                    goto Insert_Cleanup;
                }
            }
        ASSERT(pprivNew->pfi);

        pprivNew->pfi->lParam = (LPARAM)ImageList_AddIcon(this->himlCache, pprivNew->pfi->hicon);

        // Fill in the rest of the fields
        //
        lstrcpy(szPath, pitem->siInside.pszDir);
        if (IsFlagSet(pitem->uStyle, RAIS_FOLDER))
            PathRemoveFileSpec(szPath);
        if (!GSetString(&pprivNew->siInside.pszDir, szPath))
            goto Insert_Cleanup;

        pprivNew->siInside.uState = pitem->siInside.uState;
        pprivNew->siInside.fs = pitem->siInside.fs;
        pprivNew->siInside.ichRealPath = pitem->siInside.ichRealPath;

        lstrcpy(szPath, pitem->siOutside.pszDir);
        if (IsFlagSet(pitem->uStyle, RAIS_FOLDER))
            PathRemoveFileSpec(szPath);
        if (!GSetString(&pprivNew->siOutside.pszDir, szPath))
            goto Insert_Cleanup;

        pprivNew->siOutside.uState = pitem->siOutside.uState;
        pprivNew->siOutside.fs = pitem->siOutside.fs;
        pprivNew->siOutside.ichRealPath = pitem->siOutside.ichRealPath;

        pprivNew->lParam = pitem->lParam;

        pprivNew->cx = RECOMPUTE;

        // We know we're doing a redundant sorted add if the element
        //  needs to be inserted at the end of the list, but who cares.
        //
        if (pitem->iItem >= RecAct_GetCount(this))
            iItem = ListBox_AddString(hwndLB, pprivNew);
        else
            iItem = ListBox_InsertString(hwndLB, pitem->iItem, pprivNew);

        if (iItem == LB_ERR)
            goto Insert_Cleanup;

        SetWindowRedraw(hwndLB, TRUE);

        iRet = iItem;
        }
    goto Insert_End;

Insert_Cleanup:
    // Have DeleteString handler clean up field allocations
    //  of pitem.
    //
    if (iItem != LB_ERR)
        ListBox_DeleteString(hwndLB, iItem);
    else
        {
        FIFree(pprivNew->pfi);
        GFree(pprivNew);
        }
    SetWindowRedraw(hwndLB, TRUE);

Insert_End:

    return iRet;
    }


/*----------------------------------------------------------
Purpose: Delete item
Returns: count of items left
Cond:    --
*/
int PRIVATE RecAct_OnDeleteItem(
    LPRECACT this,
    int i)
    {
    HWND hwndLB = this->hwndLB;

    return ListBox_DeleteString(hwndLB, i);
    }


/*----------------------------------------------------------
Purpose: Delete all items
Returns: TRUE
Cond:    --
*/
BOOL PRIVATE RecAct_OnDeleteAllItems(
    LPRECACT this)
    {
    ListBox_ResetContent(this->hwndLB);

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Get item
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE RecAct_OnGetItem(
    LPRECACT this,
    LPRA_ITEM pitem)
    {
    LPRA_PRIV ppriv;
    HWND hwndLB = this->hwndLB;
    UINT uMask;
    int iItem;

    if (!pitem)
        return FALSE;

    iItem = pitem->iItem;
    uMask = pitem->mask;

    ListBox_GetText(hwndLB, iItem, &ppriv);

    if (uMask & RAIF_ACTION)
        pitem->uAction = ppriv->uAction;

    if (uMask & RAIF_NAME)
        pitem->pszName = FIGetPath(ppriv->pfi);

    if (uMask & RAIF_STYLE)
        pitem->uStyle = ppriv->uStyle;

    if (uMask & RAIF_INSIDE)
        pitem->siInside = ppriv->siInside;

    if (uMask & RAIF_OUTSIDE)
        pitem->siOutside = ppriv->siOutside;

    if (uMask & RAIF_LPARAM)
        pitem->lParam = ppriv->lParam;

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Set item
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE RecAct_OnSetItem(
    LPRECACT this,
    LPRA_ITEM pitem)
    {
    LPRA_PRIV ppriv;
    HWND hwndLB = this->hwndLB;
    UINT uMask;
    int iItem;

    if (!pitem)
        return FALSE;

    uMask = pitem->mask;
    iItem = pitem->iItem;

    ListBox_GetText(hwndLB, iItem, &ppriv);

    if (uMask & RAIF_ACTION)
        ppriv->uAction = pitem->uAction;

    if (uMask & RAIF_STYLE)
        ppriv->uStyle = pitem->uStyle;

    if (uMask & RAIF_NAME)
        {
        if (!FISetPath(&ppriv->pfi, pitem->pszName, FIF_ICON))
            return FALSE;

        ppriv->pfi->lParam = (LPARAM)ImageList_AddIcon(this->himlCache, ppriv->pfi->hicon);
        }

    if (uMask & RAIF_INSIDE)
        {
        if (!GSetString(&ppriv->siInside.pszDir, pitem->siInside.pszDir))
            return FALSE;
        ppriv->siInside.uState = pitem->siInside.uState;
        ppriv->siInside.fs = pitem->siInside.fs;
        ppriv->siInside.ichRealPath = pitem->siInside.ichRealPath;
        }

    if (uMask & RAIF_OUTSIDE)
        {
        if (!GSetString(&ppriv->siOutside.pszDir, pitem->siOutside.pszDir))
            return FALSE;
        ppriv->siOutside.uState = pitem->siOutside.uState;
        ppriv->siOutside.fs = pitem->siOutside.fs;
        ppriv->siOutside.ichRealPath = pitem->siOutside.ichRealPath;
        }

    if (uMask & RAIF_LPARAM)
        ppriv->lParam = pitem->lParam;

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Get the current selection
Returns: index
Cond:    --
*/
int PRIVATE RecAct_OnGetCurSel(
    LPRECACT this)
    {
    return ListBox_GetCurSel(this->hwndLB);
    }


/*----------------------------------------------------------
Purpose: Set the current selection
Returns: --
Cond:    --
*/
int PRIVATE RecAct_OnSetCurSel(
    LPRECACT this,
    int i)
    {
    int iRet = ListBox_SetCurSel(this->hwndLB, i);

    if (iRet != LB_ERR)
        RecAct_SendSelChange(this, i);

    return iRet;
    }


/*----------------------------------------------------------
Purpose: Find an item
Returns: TRUE on success
Cond:    --
*/
int PRIVATE RecAct_OnFindItem(
    LPRECACT this,
    int iStart,
    const RA_FINDITEM  * prafi)
    {
    HWND hwndLB = this->hwndLB;
    UINT uMask = prafi->flags;
    LPRA_PRIV ppriv;
    BOOL bPass;
    int i;
    int cItems = ListBox_GetCount(hwndLB);

    for (i = iStart+1; i < cItems; i++)
        {
        bPass = TRUE;       // assume we pass

        ListBox_GetText(hwndLB, i, &ppriv);

        if (uMask & RAFI_NAME &&
            !IsSzEqual(FIGetPath(ppriv->pfi), prafi->psz))
            bPass = FALSE;

        if (uMask & RAFI_ACTION && ppriv->uAction != prafi->uAction)
            bPass = FALSE;

        if (uMask & RAFI_LPARAM && ppriv->lParam != prafi->lParam)
            bPass = FALSE;

        if (bPass)
            break;          // found it
        }

    return i == cItems ? -1 : i;
    }


/////////////////////////////////////////////////////  EXPORTED FUNCTIONS


/*----------------------------------------------------------
Purpose: RecAct window proc
Returns: varies
Cond:    --
*/
LRESULT CALLBACK RecAct_WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
    {
    LPRECACT this = RecAct_GetPtr(hwnd);

    if (this == NULL)
        {
        if (msg == WM_NCCREATE)
            {
            this = GAlloc(sizeof(*this));
            ASSERT(this);
            if (!this)
                return 0L;      // OOM failure

            this->hwnd = hwnd;
            RecAct_SetPtr(hwnd, this);
            }
        else
            {
            return RecAct_DefProc(hwnd, msg, wParam, lParam);
            }
        }

    if (msg == WM_NCDESTROY)
        {
        GFree(this);
        RecAct_SetPtr(hwnd, NULL);
        }

    switch (msg)
        {
        HANDLE_MSG(this, WM_CREATE, RecAct_OnCreate);
        HANDLE_MSG(this, WM_DESTROY, RecAct_OnDestroy);

        HANDLE_MSG(this, WM_SETFONT, RecAct_OnSetFont);
        HANDLE_MSG(this, WM_COMMAND, RecAct_OnCommand);
        HANDLE_MSG(this, WM_NOTIFY, RecAct_OnNotify);
        HANDLE_MSG(this, WM_MEASUREITEM, RecAct_OnMeasureItem);
        HANDLE_MSG(this, WM_DRAWITEM, RecAct_OnDrawItem);
        HANDLE_MSG(this, WM_COMPAREITEM, RecAct_OnCompareItem);
        HANDLE_MSG(this, WM_DELETEITEM, RecAct_OnDeleteLBItem);
        HANDLE_MSG(this, WM_CONTEXTMENU, RecAct_OnContextMenu);
        HANDLE_MSG(this, WM_SETFOCUS, RecAct_OnSetFocus);
        HANDLE_MSG(this, WM_CTLCOLORLISTBOX, RecAct_OnCtlColorListBox);
        HANDLE_MSG(this, WM_PAINT, RecAct_OnPaint);
        HANDLE_MSG(this, WM_SYSCOLORCHANGE, RecAct_OnSysColorChange);


        case WM_HELP:
                WinHelp(this->hwnd, c_szWinHelpFile, HELP_CONTEXTPOPUP, IDH_BFC_UPDATE_SCREEN);
                return 0;

        case RAM_GETITEMCOUNT:
                return (LRESULT)RecAct_GetCount(this);

        case RAM_GETITEM:
                return (LRESULT)RecAct_OnGetItem(this, (LPRA_ITEM)lParam);

        case RAM_SETITEM:
                return (LRESULT)RecAct_OnSetItem(this, (const LPRA_ITEM)lParam);

        case RAM_INSERTITEM:
                return (LRESULT)RecAct_OnInsertItem(this, (const LPRA_ITEM)lParam);

        case RAM_DELETEITEM:
                return (LRESULT)RecAct_OnDeleteItem(this, (int)wParam);

        case RAM_DELETEALLITEMS:
                return (LRESULT)RecAct_OnDeleteAllItems(this);

        case RAM_GETCURSEL:
                return (LRESULT)RecAct_OnGetCurSel(this);

        case RAM_SETCURSEL:
                return (LRESULT)RecAct_OnSetCurSel(this, (int)wParam);

        case RAM_FINDITEM:
                return (LRESULT)RecAct_OnFindItem(this, (int)wParam, (const RA_FINDITEM  *)lParam);

        case RAM_REFRESH:
                RedrawWindow(this->hwndLB, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);

        default:
                return RecAct_DefProc(hwnd, msg, wParam, lParam);
                }
        }


/////////////////////////////////////////////////////  PUBLIC FUNCTIONS


/*----------------------------------------------------------
Purpose: Initialize the reconciliation-action window class
Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC RecAct_Init(
    HINSTANCE hinst)
    {
    WNDCLASSEX wc;

    wc.cbSize       = sizeof(WNDCLASSEX);
    wc.style        = CS_DBLCLKS | CS_OWNDC;
    wc.lpfnWndProc  = RecAct_WndProc;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = sizeof(LPRECACT);
    wc.hInstance    = hinst;
    wc.hIcon        = NULL;
    wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground= NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName= WC_RECACT;
    wc.hIconSm      = NULL;

    return RegisterClassEx(&wc) != 0;
    }


/*----------------------------------------------------------
Purpose: Clean up RecAct window class
Returns: --
Cond:    --
*/
void PUBLIC RecAct_Term(
    HINSTANCE hinst)
    {
    UnregisterClass(WC_RECACT, hinst);
    }


/*----------------------------------------------------------
Purpose: Special sub-class listbox proc
Returns: varies
Cond:    --
*/
LRESULT _export CALLBACK RecActLB_LBProc(
    HWND hwnd,          // window handle
    UINT uMsg,           // window message
    WPARAM wparam,      // varies
    LPARAM lparam)      // varies
    {
    LRESULT lRet;
    LPRECACT lpra = NULL;

    // Get the instance data for the control
    lpra = RecAct_GetPtr(GetParent(hwnd));
    ASSERT(lpra);

    switch (uMsg)
        {
    case WM_NOTIFY: {
        NMHDR * pnmhdr = (NMHDR *)lparam;

        if (TTN_NEEDTEXT == pnmhdr->code)
            {
            RecAct_OnNeedTipText(lpra, (LPTOOLTIPTEXT)pnmhdr);
            }
        }
        break;

    case WM_SYSKEYDOWN: {
        lRet = RecAct_OnSysKeyDown(lpra, (UINT)wparam, lparam);

        if (0 != lRet)
            lRet = RecActLB_DefProc(lpra->lpfnLBProc, hwnd, uMsg, wparam, lparam);
        }
        break;

    case WM_MOUSEMOVE: {
        MSG msg;

        ASSERT(hwnd == lpra->hwndLB);

        msg.lParam = lparam;
        msg.wParam = wparam;
        msg.message = uMsg;
        msg.hwnd = hwnd;
        SendMessage(lpra->hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);

        lRet = RecActLB_DefProc(lpra->lpfnLBProc, hwnd, uMsg, wparam, lparam);
        }
        break;

    default:
        lRet = RecActLB_DefProc(lpra->lpfnLBProc, hwnd, uMsg, wparam, lparam);
        break;
        }

    return lRet;
    }


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Converts a recnode state to a sideitem state

Returns: see above
Cond:    --
*/
UINT PRIVATE SiFromRns(
    RECNODESTATE rnstate)
    {
    switch (rnstate)
        {
#ifdef NEW_REC
    case RNS_NEVER_RECONCILED:      return SI_CHANGED;
#endif

    case RNS_UNAVAILABLE:           return SI_UNAVAILABLE;
    case RNS_DOES_NOT_EXIST:        return SI_NOEXIST;
    case RNS_DELETED:               return SI_DELETED;
    case RNS_NOT_RECONCILED:        return SI_UNCHANGED;
    case RNS_UP_TO_DATE:            return SI_UNCHANGED;
    case RNS_CHANGED:               return SI_CHANGED;

    default:
        ASSERT(0);
        return SI_UNCHANGED;
        }
    }


/*----------------------------------------------------------
Purpose: Hack to skip potential volume name.

Returns: pointer to beginning of pathname in sideitem
Cond:    --
*/
LPCTSTR PRIVATE SkipDisplayJunkHack(
    LPSIDEITEM psi)
    {
    UINT ich;

    ASSERT(psi);
    ASSERT(psi->pszDir);
    ASSERT(TEXT('(') == *psi->pszDir && 0 < psi->ichRealPath ||
           0 == psi->ichRealPath);
    ASSERT(psi->ichRealPath <= (UINT)lstrlen(psi->pszDir));

    // Paranoid checking here.  This function is being added close
    // to RTM, so as an added safety net, we're adding this min()
    // check.  For Nashville, after we're sure that there is no
    // problem with ichRealPath, we can remove the min() function.
    ich = min(psi->ichRealPath, (UINT)lstrlen(psi->pszDir));
    return &psi->pszDir[ich];
    }


/*----------------------------------------------------------
Purpose: Returns a path that uses the share name of the hvid,
         or the machine name if that is not available.

Returns: Pointer to buffer
Cond:    --
*/
LPTSTR PRIVATE GetAlternativePath(
    LPTSTR pszBuf,           // Must be MAX_PATH in length
    LPCTSTR pszPath,
    HVOLUMEID hvid,
    LPUINT pichRealPath)
    {
    TWINRESULT tr;
    VOLUMEDESC vd;

    ASSERT(pichRealPath);

    *pichRealPath = 0;

    vd.ulSize = sizeof(vd);
    tr = Sync_GetVolumeDescription(hvid, &vd);
    if (TR_SUCCESS == tr)
        {
        // Is a share name available?
        if (IsFlagSet(vd.dwFlags, VD_FL_NET_RESOURCE_VALID))
            {
            // Yes; use that
            lstrcpy(pszBuf, vd.rgchNetResource);
            PathAppend(pszBuf, PathFindEndOfRoot(pszPath));
            PathMakePresentable(pszBuf);
            }
        else if (IsFlagSet(vd.dwFlags, VD_FL_VOLUME_LABEL_VALID))
            {
            // No; use volume label
            LPTSTR pszMsg;

            PathMakePresentable(vd.rgchVolumeLabel);
            if (ConstructMessage(&pszMsg, g_hinst, MAKEINTRESOURCE(IDS_ALTNAME),
                vd.rgchVolumeLabel, pszPath))
                {
                lstrcpy(pszBuf, pszMsg);
                GFree(pszMsg);
                }
            else
                lstrcpy(pszBuf, pszPath);

            *pichRealPath = 3 + lstrlen(vd.rgchVolumeLabel);
            PathMakePresentable(&pszBuf[*pichRealPath]);
            }
        else
            {
            lstrcpy(pszBuf, pszPath);
            PathMakePresentable(pszBuf);
            }
        }
    else
        {
        lstrcpy(pszBuf, pszPath);
        PathMakePresentable(pszBuf);
        }

    return pszBuf;
    }


/*----------------------------------------------------------
Purpose: Constructs a path that would be appropriate for
         the sideitem structure.  The path is placed in the
         provided buffer.

         Typically the path will simply be the folder path in
         the recnode.  In cases when the recnode is unavailable,
         this function prepends the machine name (or share name)
         to the path.

Returns: --
Cond:    --
*/
void PRIVATE PathForSideItem(
    LPTSTR pszBuf,           // Must be MAX_PATH in length
    HVOLUMEID hvid,
    LPCTSTR pszFolder,
    RECNODESTATE rns,
    LPUINT pichRealPath)
    {
    ASSERT(pszBuf);
    ASSERT(pszFolder);
    ASSERT(pichRealPath);

    if (RNS_UNAVAILABLE == rns)
        GetAlternativePath(pszBuf, pszFolder, hvid, pichRealPath);
    else
        {
        lstrcpy(pszBuf, pszFolder);
        PathMakePresentable(pszBuf);
        *pichRealPath = 0;
        }
    MyPathRemoveBackslash(pszBuf);
    }


/*----------------------------------------------------------
Purpose: Determines the recact action based on the combination
         of the inside and outside recnode actions

Returns: FALSE if this pair seems like an unlikely match.

         (This can occur if there are two recnodes inside the
         briefcase and we choose the wrong one such that the
         pair consists of two destinations but no source.)

Cond:    --
*/
BOOL PRIVATE DeriveFileAction(
    RA_ITEM * pitem,
    RECNODEACTION rnaInside,
    RECNODEACTION rnaOutside)
    {
    BOOL bRet = TRUE;

    if (RNA_COPY_FROM_ME == rnaInside &&
        RNA_COPY_TO_ME == rnaOutside)
        {
        pitem->uAction = RAIA_TOOUT;
        }
    else if (RNA_COPY_TO_ME == rnaInside &&
        RNA_COPY_FROM_ME == rnaOutside)
        {
        pitem->uAction = RAIA_TOIN;
        }

#ifdef NEW_REC
    else if (RNA_DELETE_ME == rnaInside)
        {
        pitem->uAction = RAIA_DELETEIN;
        }
    else if (RNA_DELETE_ME == rnaOutside)
        {
        pitem->uAction = RAIA_DELETEOUT;
        }
#endif

    else if (RNA_MERGE_ME == rnaInside &&
        RNA_MERGE_ME == rnaOutside)
        {
        pitem->uAction = RAIA_MERGE;
        }
    else if (RNA_COPY_TO_ME == rnaInside &&
        RNA_MERGE_ME == rnaOutside)
        {
        // (This is the merge-first-then-copy to third
        // file case.  We sorta punt because we're not
        // showing the implicit merge.)
        pitem->uAction = RAIA_TOIN;
        }
    else if (RNA_MERGE_ME == rnaInside &&
        RNA_COPY_TO_ME == rnaOutside)
        {
        // (This is the merge-first-then-copy to third
        // file case.  We sorta punt because we're not
        // showing the implicit merge.)
        pitem->uAction = RAIA_TOOUT;
        }
    else if (RNA_NOTHING == rnaInside)
        {
        // Is one side unavailable?
        if (SI_UNAVAILABLE == pitem->siInside.uState ||
            SI_UNAVAILABLE == pitem->siOutside.uState)
            {
            // Yes; force a skip
            pitem->uAction = RAIA_SKIP;
            }
        else if (SI_DELETED == pitem->siOutside.uState)
            {
            // No; the outside was deleted and the user had previously
            // said don't delete, so it is an orphan now.
            pitem->uAction = RAIA_ORPHAN;
            }
        else
            {
            // No; it is up-to-date or both sides don't exist
            pitem->uAction = RAIA_NOTHING;
            }
        }
    else
        {
        pitem->uAction = RAIA_TOIN;

        bRet = FALSE;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Determines the action and possibly a better inside
         path if there are multiple nodes to pick from.

Returns: better (or same) inside path
Cond:    --
*/
PCHOOSESIDE PRIVATE DeriveFileActionAndSide(
    RA_ITEM * pitem,
    HDSA hdsa,
    PCHOOSESIDE pchsideInside,
    PCHOOSESIDE pchsideOutside,     // May be NULL
    BOOL bKeepFirstChoice)
    {
    ASSERT(pchsideInside);

    if (pchsideOutside)
        {
        PRECNODE prnInside = pchsideInside->prn;
        PRECNODE prnOutside = pchsideOutside->prn;
        PRECITEM pri = prnInside->priParent;

#ifndef NEW_REC
        // Was the original deleted?
        if (RNS_DELETED == prnOutside->rnstate)
            {
            // Yes; make this an orphan
            TRACE_MSG(TF_GENERAL, TEXT("Found outside path to be deleted"));

            pitem->uAction = RAIA_ORPHAN;
            }
        else
#endif
            {
            // No
            BOOL bDoAgain;
            PCHOOSESIDE pchside = pchsideInside;

            // Determine the action based on the currently
            // chosen inside and outside pair.  If DeriveFileAction
            // determines that the current inside selection is
            // unlikely, we get the next best choice and try
            // again.

            do
                {
                BOOL bGetNextBest = !DeriveFileAction(pitem,
                                            pchside->prn->rnaction,
                                            prnOutside->rnaction);

                bDoAgain = FALSE;
                if (!bKeepFirstChoice)
                    {
                    if (bGetNextBest &&
                        2 < pri->ulcNodes)
                        {
                        TRACE_MSG(TF_GENERAL, TEXT("Getting next best node"));

                        if (!ChooseSide_GetNextBest(hdsa, &pchside))
                            break;

                        bDoAgain = TRUE;
                        }
                    else if (!bGetNextBest)
                        pchsideInside = pchside;
                    else
                        ASSERT(0);
                    }

                } while (bDoAgain);

            // Is this a broken merge?
            if (RIA_BROKEN_MERGE == pri->riaction)
                {
                // Yes; override and say it is a conflict
                pitem->uAction = RAIA_CONFLICT;
                }
            }
        }
    else
        {
        TRACE_MSG(TF_GENERAL, TEXT("Outside path doesn't exist in recitem"));

        pitem->uAction = RAIA_ORPHAN;
        }
    return pchsideInside;
    }


/*----------------------------------------------------------
Purpose: Updates *prns and *prna based on given pchside, or
         leaves them alone.

Returns: --
Cond:    --
*/
void PRIVATE DeriveFolderStateAndAction(
    PCHOOSESIDE pchside,
    RECNODESTATE * prns,
    UINT * puAction)
    {
    PRECNODE prn;

    ASSERT(pchside);
    ASSERT(prns);
    ASSERT(puAction);
    ASSERT(RAIA_SOMETHING == *puAction || RAIA_NOTHING == *puAction ||
           RAIA_SKIP == *puAction);

    prn = pchside->prn;
    ASSERT(prn);

    switch (prn->rnstate)
        {
    case RNS_UNAVAILABLE:
        *prns = RNS_UNAVAILABLE;
        *puAction = RAIA_SKIP;      // (Always takes precedence)
        break;

#ifdef NEW_REC
    case RNS_NEVER_RECONCILED:
#endif
    case RNS_CHANGED:
        *prns = RNS_CHANGED;
        if (RAIA_NOTHING == *puAction)
            *puAction = RAIA_SOMETHING;
        break;

    case RNS_DELETED:
#ifdef NEW_REC
        if (RNA_DELETE_ME == prn->rnaction)
            {
            *prns = RNS_CHANGED;
            if (RAIA_NOTHING == *puAction)
                *puAction = RAIA_SOMETHING;
            }
#else
        // Leave the state as it is
#endif
        break;

    case RNS_DOES_NOT_EXIST:
    case RNS_UP_TO_DATE:
    case RNS_NOT_RECONCILED:
        switch (prn->rnaction)
            {
        case RNA_COPY_TO_ME:
#ifdef NEW_REC
            if (RAIA_NOTHING == *puAction)
                *puAction = RAIA_SOMETHING;

#else
            // Poor man's tombstoning.  Don't say the folder
            // needs updating if files have been deleted or
            // the whole folder has been deleted.
            //
            if (!PathExists(prn->pcszFolder))
                {
                // Folder is gone.  Say this is an orphan now.
                *prns = RNS_DELETED;
                }
            else if (RAIA_NOTHING == *puAction)
                {
                *puAction = RAIA_SOMETHING;
                }
#endif
            break;

#ifdef NEW_REC
        case RNA_DELETE_ME:
#endif
        case RNA_MERGE_ME:
            if (RAIA_NOTHING == *puAction)
                *puAction = RAIA_SOMETHING;
            break;
            }
        break;

    default:
        ASSERT(0);
        break;
        }
    }


/*----------------------------------------------------------
Purpose: Determine the recnode state of a folder that has
         no intersecting recnodes.

Returns: recnode state
Cond:    --
*/
RECNODESTATE PRIVATE DeriveFolderState(
    PCHOOSESIDE pchside)
    {
    FOLDERTWINSTATUS uStatus;
    RECNODESTATE rns;

    Sync_GetFolderTwinStatus((HFOLDERTWIN)pchside->htwin, NULL, 0, &uStatus);
    if (FTS_UNAVAILABLE == uStatus)
        rns = RNS_UNAVAILABLE;
    else
        rns = RNS_UP_TO_DATE;
    return rns;
    }


/*----------------------------------------------------------
Purpose: Initialize a paired-twin structure assuming pszPath
         is a file.

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE RAI_InitAsRecItem(
    LPRA_ITEM pitem,
    LPCTSTR pszBrfPath,
    LPCTSTR pszPath,              // May be NULL
    PRECITEM pri,
    BOOL bKeepFirstChoice)
    {
    HRESULT hres;
    HDSA hdsa;

    ASSERT(pitem);
    ASSERT(pszBrfPath);
    ASSERT(pri);

    hres = ChooseSide_CreateAsFile(&hdsa, pri);
    if (SUCCEEDED(hres))
        {
        TCHAR sz[MAX_PATH];
        PCHOOSESIDE pchside;
        PCHOOSESIDE pchsideOutside;
        UINT ichRealPath;

        DEBUG_CODE( Sync_DumpRecItem(TR_SUCCESS, pri, TEXT("RAI_InitAsFile")); )

        pitem->mask = RAIF_ALL & ~RAIF_LPARAM;
        if (!GSetString(&pitem->pszName, pri->pcszName))
            goto Error;
        PathMakePresentable(pitem->pszName);

        // Default style
        if (RIA_MERGE == pri->riaction)
            pitem->uStyle = RAIS_CANMERGE;
        else
            pitem->uStyle = 0;

        // Is there an outside file?
        if (ChooseSide_GetBest(hdsa, pszBrfPath, NULL, &pchside))
            {
            // Yes
            RECNODESTATE rns = pchside->prn->rnstate;

            DEBUG_CODE( ChooseSide_DumpList(hdsa); )

            pitem->siOutside.uState = SiFromRns(rns);
            PathForSideItem(sz, pchside->hvid, pchside->pszFolder, rns, &ichRealPath);
            if (!GSetString(&pitem->siOutside.pszDir, sz))
                goto Error;
            pitem->siOutside.fs = pchside->prn->fsCurrent;
            pitem->siOutside.ichRealPath = ichRealPath;
            }
        else
            {
            // No; this is an orphan
            DEBUG_CODE( ChooseSide_DumpList(hdsa); )

            if (!GSetString(&pitem->siOutside.pszDir, c_szNULL))
                goto Error;
            pitem->siOutside.uState = SI_NOEXIST;
            pitem->siOutside.ichRealPath = 0;
            }
        pchsideOutside = pchside;

        // Make sure we have some fully qualified folder on which
        // to base our decision for an inside path
        if (pszPath)
            {
            lstrcpy(sz, pszPath);
            PathRemoveFileSpec(sz);
            }
        else
            lstrcpy(sz, pszBrfPath);    // (best we can do...)

        // Get the inside folder
        if (ChooseSide_GetBest(hdsa, pszBrfPath, sz, &pchside))
            {
            RECNODESTATE rns;

            DEBUG_CODE( ChooseSide_DumpList(hdsa); )

            pchside = DeriveFileActionAndSide(pitem, hdsa, pchside, pchsideOutside, bKeepFirstChoice);

            // Determine status of inside file
            rns = pchside->prn->rnstate;

            pitem->siInside.uState = SiFromRns(rns);
            PathForSideItem(sz, pchside->hvid, pchside->pszFolder, rns, &ichRealPath);
            GSetString(&pitem->siInside.pszDir, sz);
            pitem->siInside.fs = pchside->prn->fsCurrent;
            pitem->siInside.ichRealPath = ichRealPath;

            // Is there a node for the outside?
            if (pchsideOutside)
                {
                // Yes; special case.  If a single side does not exist
                // then say the existing side is new.
                if (SI_NOEXIST == pitem->siInside.uState &&
                    SI_NOEXIST == pitem->siOutside.uState)
                    ;       // Do nothing special
                else if (SI_NOEXIST == pitem->siInside.uState)
                    {
                    ASSERT(SI_NOEXIST != pitem->siOutside.uState);
                    pitem->siOutside.uState = SI_NEW;
                    }
                else if (SI_NOEXIST == pitem->siOutside.uState)
                    {
                    ASSERT(SI_NOEXIST != pitem->siInside.uState);
                    pitem->siInside.uState = SI_NEW;
                    }
                }

            // Save away twin handle.  Use the inside htwin because
            // we want to always delete from inside the briefcase
            // (it's all in your perspective...)
            pitem->htwin = (HTWIN)pchside->prn->hObjectTwin;
            }
        else
            {
            // It is relatively bad to be here

            DEBUG_CODE( ChooseSide_DumpList(hdsa); )
            ASSERT(0);

            hres = E_FAIL;
            }

        DEBUG_CODE( DumpTwinPair(pitem); )

        ChooseSide_Free(hdsa);
        hdsa = NULL;
        }
    else
        {
        hdsa = NULL;

Error:
        hres = E_OUTOFMEMORY;
        }

    if (FAILED(hres))
        {
        ChooseSide_Free(hdsa);
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Choose a recitem whose name matches the given name.

Returns: A pointer to the recitem in the given reclist
         NULL if filespec is not found

Cond:    --
*/
PRECITEM PRIVATE ChooseRecItem(
    PRECLIST prl,
    LPCTSTR pszName)
    {
    PRECITEM pri;

    for (pri = prl->priFirst; pri; pri = pri->priNext)
        {
        if (IsSzEqual(pri->pcszName, pszName))
            return pri;
        }
    return NULL;
    }


/*----------------------------------------------------------
Purpose: Initialize a paired-twin structure assuming pszPath
         is a file.

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE RAI_InitAsFile(
    LPRA_ITEM pitem,
    LPCTSTR pszBrfPath,
    LPCTSTR pszPath,
    PRECLIST prl)
    {
    HRESULT hres;
    PRECITEM pri;
    LPCTSTR pszFile;

    ASSERT(pitem);
    ASSERT(pszBrfPath);
    ASSERT(pszPath);
    ASSERT(prl);

    pszFile = PathFindFileName(pszPath);
    pri = ChooseRecItem(prl, pszFile);
    ASSERT(pri);

    if (pri)
        {
        hres = RAI_InitAsRecItem(pitem, pszBrfPath, pszPath, pri, TRUE);
        }
    else
        {
        hres = E_OUTOFMEMORY;
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Initialize a paired-twin structure assuming pszPath
         is a file.

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE RAI_InitAsFolder(
    LPRA_ITEM pitem,
    LPCTSTR pszBrfPath,
    LPCTSTR pszPath,              // Should be inside the briefcase
    PRECLIST prl,
    PFOLDERTWINLIST pftl)
    {
    HRESULT hres;
    HDSA hdsa;

    ASSERT(pitem);
    ASSERT(pszBrfPath);
    ASSERT(pszPath);
    ASSERT(prl);
    ASSERT(pftl);
    ASSERT(0 < pftl->ulcItems);

    pitem->mask = RAIF_ALL & ~RAIF_LPARAM;

    DEBUG_CODE( Sync_DumpRecList(TR_SUCCESS, prl, TEXT("RAI_InitAsFolder")); )
    DEBUG_CODE( Sync_DumpFolderTwinList(pftl, NULL); )

    // We only need to flag RAIS_FOLDER for the folder case.
    // (Context menu isn't available for folders, so RAIS_CANMERGE is
    //  unnecessary.)
    //
    pitem->uStyle = RAIS_FOLDER;

    hres = ChooseSide_CreateEmpty(&hdsa);
    if (SUCCEEDED(hres))
        {
        PRECITEM pri;
        RECNODESTATE rnsInside;
        RECNODESTATE rnsOutside;
        PCHOOSESIDE pchside;

        // Set starting defaults
        pitem->uAction = RAIA_NOTHING;
        rnsInside = RNS_UP_TO_DATE;
        rnsOutside = RNS_UP_TO_DATE;

        // Iterate thru reclist, choosing recnode pairs and dynamically
        // updating rnsInside, rnsOutside and pitem->uAction.
        for (pri = prl->priFirst; pri; pri = pri->priNext)
            {
            ChooseSide_InitAsFile(hdsa, pri);

            // Get the inside item
            if (ChooseSide_GetBest(hdsa, pszBrfPath, pszPath, &pchside))
                {
                DeriveFolderStateAndAction(pchside, &rnsInside, &pitem->uAction);
                }
            else
                ASSERT(0);

            // Get the outside item
            if (ChooseSide_GetBest(hdsa, pszBrfPath, NULL, &pchside))
                {
                DeriveFolderStateAndAction(pchside, &rnsOutside, &pitem->uAction);
                }
            else
                ASSERT(0);
            }
        ChooseSide_Free(hdsa);

        // Finish up
        hres = ChooseSide_CreateAsFolder(&hdsa, pftl);
        if (SUCCEEDED(hres))
            {
            TCHAR sz[MAX_PATH];
            UINT ichRealPath;

            // Name
            if (!GSetString(&pitem->pszName, PathFindFileName(pszPath)))
                goto Error;
            PathMakePresentable(pitem->pszName);

            // Get the inside folder
            if (ChooseSide_GetBest(hdsa, pszBrfPath, pszPath, &pchside))
                {
                DEBUG_CODE( ChooseSide_DumpList(hdsa); )

                // Are there any intersecting files in this folder twin?
                if (0 == prl->ulcItems)
                    rnsInside = DeriveFolderState(pchside);     // No

                pitem->siInside.uState = SiFromRns(rnsInside);
                PathForSideItem(sz, pchside->hvid, pchside->pszFolder, rnsInside, &ichRealPath);
                if (!GSetString(&pitem->siInside.pszDir, sz))
                    goto Error;
                // (Hack to avoid printing bogus time/date)
                pitem->siInside.fs.fscond = FS_COND_UNAVAILABLE;
                pitem->siInside.ichRealPath = ichRealPath;
                }
            else
                {
                DEBUG_CODE( ChooseSide_DumpList(hdsa); )
                ASSERT(0);
                }

            // Get the outside folder
            if (ChooseSide_GetBest(hdsa, pszBrfPath, NULL, &pchside))
                {
                DEBUG_CODE( ChooseSide_DumpList(hdsa); )

                // Are there any intersecting files in this folder twin?
                if (0 == prl->ulcItems)
                    rnsOutside = DeriveFolderState(pchside);     // No

                pitem->siOutside.uState = SiFromRns(rnsOutside);
                PathForSideItem(sz, pchside->hvid, pchside->pszFolder, rnsOutside, &ichRealPath);
                if (!GSetString(&pitem->siOutside.pszDir, sz))
                    goto Error;
                // (Hack to avoid printing bogus time/date)
                pitem->siOutside.fs.fscond = FS_COND_UNAVAILABLE;
                pitem->siOutside.ichRealPath = ichRealPath;

                // Save away twin handle.  Use the outside handle
                // for folders.
                pitem->htwin = pchside->htwin;
                }
            else
                {
                DEBUG_CODE( ChooseSide_DumpList(hdsa); )
                ASSERT(0);
                }

            DEBUG_CODE( DumpTwinPair(pitem); )

            ChooseSide_Free(hdsa);
            }
        }

    if (FAILED(hres))
        {
Error:
        if (SUCCEEDED(hres))
            hres = E_OUTOFMEMORY;

        ChooseSide_Free(hdsa);
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Create a paired-twin structure given a path name.

Returns: standard result
Cond:    --
*/
HRESULT PUBLIC RAI_Create(
    LPRA_ITEM * ppitem,
    LPCTSTR pszBrfPath,
    LPCTSTR pszPath,              // Should be inside the briefcase
    PRECLIST prl,
    PFOLDERTWINLIST pftl)       // NULL if pszPath is a file
    {
    HRESULT hres;
    LPRA_ITEM pitem;

    ASSERT(ppitem);
    ASSERT(pszPath);
    ASSERT(pszBrfPath);
    ASSERT(prl);

    DBG_ENTER_SZ(TEXT("RAI_Create"), pszPath);

    if (PathExists(pszPath))
        {
        pitem = GAlloc(sizeof(*pitem));
        if (pitem)
            {
            if (PathIsDirectory(pszPath))
                hres = RAI_InitAsFolder(pitem, pszBrfPath, pszPath, prl, pftl);
            else
                hres = RAI_InitAsFile(pitem, pszBrfPath, pszPath, prl);

            if (FAILED(hres))
                {
                // Cleanup
                RAI_Free(pitem);
                pitem = NULL;
                }
            }
        else
            hres = E_OUTOFMEMORY;
        }
    else
        {
        pitem = NULL;
        hres = E_FAIL;
        }

    *ppitem = pitem;

    DBG_EXIT_HRES(TEXT("RAI_Create"), hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: Create a paired-twin structure given a recitem.

Returns: standard result
Cond:    --
*/
HRESULT PUBLIC RAI_CreateFromRecItem(
    LPRA_ITEM * ppitem,
    LPCTSTR pszBrfPath,
    PRECITEM pri)
    {
    HRESULT hres;
    LPRA_ITEM pitem;

    ASSERT(ppitem);
    ASSERT(pszBrfPath);
    ASSERT(pri);

    DBG_ENTER(TEXT("RAI_CreateFromRecItem"));

    pitem = GAlloc(sizeof(*pitem));
    if (pitem)
        {
        hres = RAI_InitAsRecItem(pitem, pszBrfPath, NULL, pri, FALSE);

        if (FAILED(hres))
            {
            // Cleanup
            RAI_Free(pitem);
            pitem = NULL;
            }
        }
    else
        hres = E_OUTOFMEMORY;

    *ppitem = pitem;

    DBG_EXIT_HRES(TEXT("RAI_CreateFromRecItem"), hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: Free a paired item structure

Returns: standard result
Cond:    --
*/
HRESULT PUBLIC RAI_Free(
    LPRA_ITEM pitem)
    {
    HRESULT hres;

    if (pitem)
        {
        GFree(pitem->pszName);
        GFree(pitem->siInside.pszDir);
        GFree(pitem->siOutside.pszDir);
        GFree(pitem);
        hres = NOERROR;
        }
    else
        hres = E_FAIL;

    return hres;
    }
