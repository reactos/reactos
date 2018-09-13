/**********************************************************************/
/**                  Microsoft Windows                               **/
/**            Copyright(c) Microsoft Corp., 1991, 1992              **/
/**********************************************************************/
/*
 *      MODULE NAME:            BTNLIST.C
 *
 *      AUTHOR:                 John Rivard
 *                              Microsoft Corp.
 *                              (johnri@microsoft.com)
 *
 *      SHORT DESCRIPTION:      Button ListBox Control
 *
 *
 *      FUNCTIONS:              InitButtonListBoxClass
 *                              UnInitButtonListBoxClass
 *
 *                              ButtonListBoxProc
 *                              BL_OnCreate
 *                              BL_OnDestroy
 *                              BL_OnSetFocus
 *                              BL_OnKillFocus
 *                              BL_OnDrawItem
 *                              BL_OnMeasureItem
 *                              BL_OnCompareItem
 *                              BL_OnCharToItem
 *                              BL_OnDeleteItem
 *                              BL_OnGetDlgCode
 *                              BL_OnCtlColor
 *                              BL_OnCommand
 *
 *                              SubListBoxProc
 *                              Sub_OnLButtonDown
 *                              Sub_OnLButtonUp
 *                              Sub_OnMouseMove
 *                              Sub_OnKey
 *
 *                              CreateListButton
 *                              DeleteListButton
 *                              CreateButtonBitmap
 *
 *      FILE HISTORY:
 *
 *      johnri  03-09-92        Create.
 *      johnri  04-29-92        Port from standalone DLL to COMMCTRL.DLL
 *
\**********************************************************************/


#include "ctlspriv.h"   /* commctrl private definitions */

/******* Definitions and typedefs ******************************************/

/* Use the first definition if you want to clean up unreferenced params
 */
#if 0
#define Reference(x)
#else
#define Reference(x) (x) = (x)
#endif

// Standard list box control for button listbox
#define LISTBOX         "ListBox"
#define ID_LISTBOX      1

// Button listbox info; data for the entire control
#define GWL_BLINFO      0
typedef struct tagBLINFO
{
    BOOL        fNoScroll;
    int         cxButton;
    int         cyButton;
    int         nTrackButton;
    int         cButtonMax;
} BLINFO, NEAR *PBLINFO;

// List button data; data for each button
typedef struct tagLBD
{
    DWORD       dwItemData; // user item data for
                            // LB_SETITEMDATA and LB_GETITEMDATA
    BOOL        fButtonDown;// TRUE if button pressed
    UINT        chUpper;    // button key uppercase
    UINT        chLower;    // button key lowercase
    HBITMAP     hbmpUp;     // bitmap for up button
    HBITMAP     hbmpDown;   // bitmap for down button
    RECT        rcText;     // text rectangle for up button
    char        szText[1];  // button text
} LISTBUTTONDATA, NEAR *PLISTBUTTONDATA;


/******* Internal Function Declarations ************************************/

// Control and Subclass Window Procedures
LRESULT CALLBACK ButtonListBoxProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SubListBoxProc(HWND, UINT, WPARAM, LPARAM);

// ButtonListBoxProc Message Handlers
BOOL NEAR PASCAL BL_OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct);
void NEAR PASCAL BL_OnDestroy(HWND hwnd);
BOOL NEAR PASCAL BL_OnSetFocus(HWND hwnd, HWND hwndOldFocus);
void NEAR PASCAL BL_OnKillFocus(HWND hwnd, HWND hwndOldFocus);
void NEAR PASCAL BL_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT FAR* lpDrawItem);
void NEAR PASCAL BL_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT FAR* lpMeasureItem);
int  NEAR PASCAL BL_OnCompareItem(HWND hwnd, const COMPAREITEMSTRUCT FAR* lpCompareItem);
int  NEAR PASCAL BL_OnCharToItem(HWND hwnd, UINT ch, HWND hwndListbox, int iCaret);
void NEAR PASCAL BL_OnDeleteItem(HWND hwnd, const DELETEITEMSTRUCT FAR* lpDeleteItem);
UINT NEAR PASCAL BL_OnGetDlgCode(HWND hwnd, MSG FAR* lpmsg);
HBRUSH NEAR PASCAL BL_OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type);
void NEAR PASCAL BL_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LRESULT NEAR PASCAL BL_OnButtonListBox(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// SubListBoxProc Message Handlers
void NEAR PASCAL Sub_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
LONG NEAR PASCAL Sub_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
void NEAR PASCAL Sub_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
void NEAR PASCAL Sub_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);

// Miscellaneous functions
PLISTBUTTONDATA NEAR PASCAL CreateListButton(HWND hLB, CREATELISTBUTTON FAR* lpCLB);
VOID NEAR PASCAL DeleteListButton(PLISTBUTTONDATA pLBD);
HBITMAP NEAR PASCAL CreateButtonBitmap(HWND hLB, int nWidth, int nHeight, BOOL fButtonDown,
                           HBITMAP hUserBitmap, LPCSTR lpszUserText,
                           LPRECT rcText);

// Debugging
#ifdef DEBUG
static void cdecl DebugPrintf(LPCSTR lpsz, ...);
#define DEBUGPRINTF(arglist) DebugPrintf arglist
#else
#define DEBUGPRINTF(arglist)
#endif


/******* Global Data *******************************************************/

//
// Global - REVIEW_32
//
static BOOL      fInitResult            = FALSE;    // result of initialization
static WNDPROC   lpDefListBoxProc       = NULL;     // Default ListBox proc
static HBRUSH    hBrushBackground       = NULL;     // Control background brush

 /**********************************************************************\
 *
 *  NAME:       InitButtonListBoxClass
 *
 *  SYNOPSIS:   Init the control class and module.
 *
 *  ENTRY:      hInstance           HINSTANCE   DLL instance handle
 *
 *  EXIT:       return              BOOL        Result of initialization
 *
 *  NOTES:      If called more than once it only returns the result
 *              the first initialization.
 *
\**********************************************************************/

#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL InitButtonListBoxClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    // Button List Control Class
    if (!GetClassInfo(hInstance, s_szBUTTONLISTBOX, &wc))
    {
#ifndef WIN32
        extern LRESULT CALLBACK _ButtonListBoxProc(HWND, UINT, WPARAM, LPARAM);
        wc.lpfnWndProc   = _ButtonListBoxProc;
#else
        wc.lpfnWndProc   = (WNDPROC)ButtonListBoxProc;
#endif
        fInitResult = FALSE;

        wc.style         = CS_DBLCLKS|CS_PARENTDC|CS_GLOBALCLASS;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = sizeof(PBLINFO);
        wc.hInstance     = hInstance;
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
        wc.lpszMenuName  = (LPCSTR)NULL;
        wc.lpszClassName = s_szBUTTONLISTBOX;

        if (!RegisterClass(&wc))
            return FALSE;

        hBrushBackground = GetStockObject(GRAY_BRUSH);
        if (!hBrushBackground)
            return FALSE;
    }

    return (fInitResult = TRUE);

}

#pragma code_seg()

/**********************************************************************\
 *
 *  NAME:       ButtonListBoxProc
 *
 *  SYNOPSIS:   Window proc for class buttonlistbox.
 *
 *  ENTRY:      hwnd                HWND    Window handle
 *              uMsg                UINT    Window message
 *              wParam              WPARAM  message dependent param
 *              lParam              LPARAM  message dependent param
 *
 *  EXIT:       return              LRESULT message dependent
 *
 *  NOTES:      This window proc handles message to the Button ListBox
 *              control. Since the button listbox is implemented by
 *              using a child listbox control, all listbox messages are
 *              forwarded to the child listbox.
 *
 *              Other messages are handled to provide the specific
 *              functionality of the button listbox control and to process
 *              the owner-draw messages from the child listbox.
 *
\**********************************************************************/

LRESULT CALLBACK ButtonListBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {

    // Draw a button
    HANDLE_MSG(hwnd,WM_DRAWITEM,    BL_OnDrawItem);

    // Measure a button
    HANDLE_MSG(hwnd,WM_MEASUREITEM, BL_OnMeasureItem);

    // Compare two buttons
    HANDLE_MSG(hwnd,WM_COMPAREITEM, BL_OnCompareItem);

    // Keyboard jump
    HANDLE_MSG(hwnd,WM_CHARTOITEM,  BL_OnCharToItem);

    // Delete a button
    HANDLE_MSG(hwnd,WM_DELETEITEM,  BL_OnDeleteItem);

    // Set focus to child listbox
    HANDLE_MSG(hwnd,WM_SETFOCUS,    BL_OnSetFocus);

    // Init buttonlistbox
    HANDLE_MSG(hwnd,WM_CREATE,      BL_OnCreate);

    // Cleanup buttonlistbox
    HANDLE_MSG(hwnd,WM_DESTROY,     BL_OnDestroy);

    // Tell dlg mgr about us
    HANDLE_MSG(hwnd,WM_GETDLGCODE,  BL_OnGetDlgCode);

    // Set control bkgnd color
#ifdef WIN32
    HANDLE_MSG(hwnd,WM_CTLCOLORLISTBOX, BL_OnCtlColor);
#else // WIN32
    HANDLE_MSG(hwnd,WM_CTLCOLOR,    BL_OnCtlColor);
#endif

    // Forward commands from the child listbox
    HANDLE_MSG(hwnd,WM_COMMAND,     BL_OnCommand);

    default:

    // Forward button listbox messages to button listbox msg handler
    if (uMsg >= WM_USER)
        return BL_OnButtonListBox(hwnd,uMsg,wParam,lParam);

    // Pass all other messages to the default window proc
    else
        return DefWindowProc(hwnd,uMsg,wParam,lParam);

    }

}

PLISTBUTTONDATA NEAR PASCAL GetListButtonData(HWND hLB, int iItem)
{
    DWORD dw;

    dw = ListBox_GetItemData(hLB, iItem);
    if (dw == LB_ERR)
       return NULL;
    else
        return (PLISTBUTTONDATA)(UINT)dw;
}

/**********************************************************************\
 *
 *  NAME:       BL_OnButtonListBox
 *
 *  SYNOPSIS:   Handle list box messages for button listbox
 *
 *  ENTRY:      hLB             HWND    window handle of child listbox
 *              uMsg            UINT    listbox message
 *              wParam          WPARAM  message dependent
 *              lParam          LPARAM  message dependent
 *
 *  EXIT:       return          LRESULT message dependent
 *
\**********************************************************************/
LRESULT NEAR PASCAL BL_OnButtonListBox(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PLISTBUTTONDATA pLBD;
    PBLINFO pbli;
    int                 cItems;
    HWND                hLB;

    hLB = GetDlgItem(hwnd,ID_LISTBOX);
    pbli = (PBLINFO)GetWindowInt(hwnd, 0);

    // Handle button list box messages

    switch (uMsg)
    {
    case BL_ADDBUTTON:
        if (!pbli)
            return BL_ERR;
        cItems = ListBox_GetCount(hLB);
        DEBUGPRINTF(("AddButton: Max %d, Count %d",pbli->cButtonMax,cItems));
        if (cItems >= pbli->cButtonMax)
        {
            DEBUGPRINTF(("AddButton: BL_ERR"));
            return BL_ERR;
        }
        pLBD = CreateListButton(hLB, (CREATELISTBUTTON FAR*)lParam);
        if (!pLBD)
            return BL_ERRSPACE;
        else
            return ListBox_AddItemData(hLB, (UINT)pLBD);

    case BL_DELETEBUTTON:
        return ListBox_DeleteString(hLB,wParam);

    case BL_GETCARETINDEX:
        return ListBox_GetCaretIndex(hLB);

    case BL_GETCOUNT:
        return ListBox_GetCount(hLB);

    case BL_GETCURSEL:
        return ListBox_GetCurSel(hLB);

    case BL_GETITEMDATA:
        pLBD = GetListButtonData(hLB, wParam);
        if (pLBD)
            return (LRESULT)pLBD->dwItemData;
        else
            return (LRESULT)BL_ERR;

    case BL_GETITEMRECT:
        return ListBox_GetItemRect(hLB,wParam,lParam);

    case BL_GETTEXT:
        pLBD = GetListButtonData(hLB, wParam);
        if (pLBD) {
            lstrcpy((LPSTR)lParam, pLBD->szText);
            return (LRESULT)lstrlen(pLBD->szText);
	} else
	    return BL_ERR;

    case BL_GETTEXTLEN:
        pLBD = GetListButtonData(hLB, wParam);
        if (pLBD)
            return (LRESULT)lstrlen(pLBD->szText);
        else
            return (LRESULT)BL_ERR;

    case BL_GETTOPINDEX:
        return ListBox_GetTopIndex(hLB);

    case BL_INSERTBUTTON:
        if (!pbli)
            return BL_ERR;
        cItems = ListBox_GetCount(hLB);
        if (cItems >= pbli->cButtonMax)
            return BL_ERR;
        pLBD = CreateListButton(hLB, (CREATELISTBUTTON FAR*)lParam);
        if (!pLBD)
            return BL_ERRSPACE;
        else
            return ListBox_InsertItemData(hLB, wParam, (UINT)pLBD);

    case BL_RESETCONTENT:
        return ListBox_ResetContent(hLB);

    case BL_SETCARETINDEX:
        return ListBox_SetCaretIndex(hLB,wParam);

    case BL_SETCURSEL:
        return ListBox_SetCurSel(hLB,wParam);

    case BL_SETITEMDATA:
        pLBD = GetListButtonData(hLB, wParam);
        if (pLBD)
        {
            pLBD->dwItemData = (DWORD)lParam;
            return (LRESULT)BL_OKAY;
        }
        else
            return (LRESULT)BL_ERR;

    case BL_SETTOPINDEX:
        return ListBox_SetTopIndex(hLB,wParam);


    default:
        DEBUGPRINTF(("BL_OnButtonListBox: unknown message %d",uMsg));
        return (LRESULT)BL_ERR;
    }

}

/**********************************************************************\
 *
 *  NAME:       BL_OnCreate
 *
 *  SYNOPSIS:   Handle WM_CREATE for button listbox
 *
 *  ENTRY:      hwnd                HWND    window handle
 *              lpCS                CREATESTRUCT FAR* window create data
 *
 *  EXIT:       return              BOOL    TRUE if success, else false
 *
 *  NOTES:      When a button listbox is created it must position itself
 *              along one of the edges of the parent dialog as specified
 *              by the style bits and create the child listbox.
 *
 *              The dimensions of the buttons within the child listbox
 *              are determined by the cx and cy parameters in the
 *              CREATESTRUCT. (For other controls, these would indicate
 *              the width and height of the entire control, but that is
 *              determined by the parent dialog window size.) The cx and
 *              cy values come from the CONTROL statement in the dialog
 *              template.
 *
 *              This function subclasses the child listbox with the
 *              SubListBoxProc.
 *
\**********************************************************************/

BOOL NEAR PASCAL BL_OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCS)
{
    typedef enum tagORIENTATION { VERTICAL, HORIZONTAL } ORIENTATION;
    ORIENTATION         orientation;
    BOOL                fNoScroll;
    PBLINFO pbli;
    DWORD               dwStyle;
    DWORD               dwListBoxStyle;
    HWND                hListBox;
    BYTE                buttonsPerListbox;

    // Setup the users styles and get the button dimensions from the style
    dwStyle = lpCS->style;
    orientation = (dwStyle & BLS_VERTICAL) ? VERTICAL : HORIZONTAL;
    fNoScroll = (dwStyle & BLS_NOSCROLL) != 0L;
    if ((buttonsPerListbox = (BYTE)(dwStyle & BLS_NUMBUTTONS)) == 0)
        buttonsPerListbox = 1;

    // force a border around the control by default and get rid of
    // the non-standard window styles.
    dwStyle |= WS_BORDER;
    SetWindowLong(hwnd, GWL_STYLE, dwStyle);

    // Allocate a global structure for holding data for the entire
    // button listbox control and set the pointer in the window
    pbli = (PBLINFO)LocalAlloc(LPTR, sizeof(BLINFO));
    if (!pbli)
    {
        DEBUGPRINTF(("BL_OnCreate: could not allocate pbli"));
        return FALSE;
    }
    SetWindowInt(hwnd, 0, (int)pbli);

    // Init values for pbli
    pbli->fNoScroll = fNoScroll;
    pbli->nTrackButton = -1;    // no button currently down
    pbli->cButtonMax = 3000;    // very large integer
    pbli->cxButton = lpCS->cx;
    pbli->cyButton = lpCS->cy;


    /* Adust the width and height of the control to fit the
     * requested number of buttons
     */
    if (orientation == HORIZONTAL)
    {
        lpCS->cx = buttonsPerListbox * pbli->cxButton
                    + 2*g_cxBorder
                    + ((pbli->fNoScroll)? 0 : MulDiv(pbli->cxButton,2,3));

        lpCS->cy = pbli->cyButton;
        lpCS->cy += (pbli->fNoScroll)?
                     2*g_cyBorder :
                     (g_cyHScroll
                     + g_cyBorder);

        /* if no scrollbar, calculate the max number of buttons that fit */
        if (pbli->fNoScroll)
            pbli->cButtonMax = MulDiv(lpCS->cx,1,pbli->cxButton);
    }
    else
    {
        lpCS->cy = buttonsPerListbox * pbli->cyButton
                    + 2*g_cyBorder
                    + ((pbli->fNoScroll)? 0: MulDiv(pbli->cyButton,2,3));

        lpCS->cx = pbli->cxButton;
        lpCS->cx += (pbli->fNoScroll)?
                      2*g_cxBorder :
                      (g_cxVScroll
                      + g_cxBorder);

        /* if no scrollbar, calculate the max number of buttons that fit */
        if (pbli->fNoScroll)
            pbli->cButtonMax = MulDiv(lpCS->cy,1,pbli->cyButton);
    }

    /* Now change the control size to fit the calculated buttons */
    SetWindowPos(hwnd,NULL,lpCS->x,lpCS->y,lpCS->cx,lpCS->cy,
                   SWP_NOZORDER | SWP_NOACTIVATE);

    // Set the standard style bits for all button child listboxes
    // Set style for vertical/horizontal
    // Set style for no scrollbars
    dwListBoxStyle =  WS_CHILD
                    | WS_VISIBLE
                    | WS_CLIPSIBLINGS
                    | WS_BORDER
                    | LBS_NOTIFY
                    | LBS_SORT
                    | LBS_NOINTEGRALHEIGHT
                    | LBS_OWNERDRAWFIXED
                    | LBS_WANTKEYBOARDINPUT
                    | LBS_DISABLENOSCROLL;

    if (orientation == HORIZONTAL)
        dwListBoxStyle |= (WS_HSCROLL | LBS_MULTICOLUMN);
    else
        dwListBoxStyle |= WS_VSCROLL;

    if (fNoScroll)
        dwListBoxStyle &= ~(WS_HSCROLL | WS_VSCROLL);


    // Create the child list box
    hListBox =
        CreateWindowEx(
            WS_EX_NOPARENTNOTIFY,
            "ListBox",                  // class
            "",                         // window name
            dwListBoxStyle,             // style
            0,                          // left
            0,                          // top
            lpCS->cx - 2*g_cxBorder, // width
            lpCS->cy - 2*g_cyBorder, // height
            hwnd,                       // parent
            (HMENU)ID_LISTBOX,          // control id of the child listbox
            HINST_THISDLL,                    // instance
            NULL                        // no createparams
            );

    if (!hListBox)
    {
        DEBUGPRINTF(("BL_OnCreate: could not create child listbox"));
        LocalFree((HLOCAL)pbli);
        return FALSE;
    }

    // Sub-class the list box
    // Note that window procedures in protect mode only DLL's may be called
    // directly.
    if (!lpDefListBoxProc)
        lpDefListBoxProc = (WNDPROC)GetWindowLong(hListBox, GWL_WNDPROC);
    SetWindowLong(hListBox, GWL_WNDPROC, (LONG)SubListBoxProc);

    return TRUE;

}

/**********************************************************************\
 *
 *  NAME:       BL_OnDestroy
 *
 *  SYNOPSIS:   Handle WM_DESTROY for button listbox
 *
 *  ENTRY:      hwnd                HWND    Window handle
 *
 *  EXIT:       void
 *
 *  NOTES:      Clean up memory allocated in BL_OnCreate.
 *
\**********************************************************************/

void NEAR PASCAL BL_OnDestroy(HWND hwnd)
{
    PBLINFO pbli;

    // Free up the button list info data
    pbli = (PBLINFO)GetWindowInt(hwnd, 0);
    if (pbli != NULL)
        LocalFree((HLOCAL)pbli);

}


/**********************************************************************\
 *
 *  NAME:       BL_OnSetFocus
 *
 *  SYNOPSIS:   Handle WM_SETFOCUS for button listbox
 *
 *  ENTRY:      hwnd                HWND    Window getting focus
 *              hwndOldFocus        HWND    Window losing focus
 *
 *  EXIT:       void
 *
 *  NOTES:      Pass focus to child listbox.
 *
\**********************************************************************/

BOOL NEAR PASCAL BL_OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
    HWND hLB;

    Reference(hwndOldFocus);

    hLB = GetDlgItem(hwnd,ID_LISTBOX);
    SetFocus(hLB);

    // Be sure there is alwyas a current selection
    // or else the spacebar will not select a button.

    if (ListBox_GetCurSel(hLB) == LB_ERR )
        ListBox_SetCurSel(hLB,ListBox_GetCaretIndex(hLB));

    return 0;
}


/**********************************************************************\
 *
 *  NAME:       BL_OnDrawItem
 *
 *  SYNOPSIS:   Handle WM_DRAWITEM for button listbox
 *
 *  ENTRY:      hwnd                HWND    Window handle
 *              lpDrawItem          DRAWITEMSTRUCT FAR*
 *
 *  EXIT:       void
 *
 *  NOTES:      BitBlt the up or down button and draw the focus rect.
 *
\**********************************************************************/

void NEAR PASCAL BL_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT FAR* lpDrawItem)
{

    PLISTBUTTONDATA pLBD;
    HDC                 hMemoryDC;
    HBITMAP             hOldBitmap;
    HBITMAP             hBitmap;
    PBLINFO pbli;

    pLBD = (PLISTBUTTONDATA)(UINT)(lpDrawItem->itemData);
    if (!pLBD)
        return;
    pbli = (PBLINFO)GetWindowInt(hwnd, 0);
    if (!pbli)
        return;


    /****************************/
    /* Draw the standard button */
    /****************************/

    if ((lpDrawItem->itemAction & ODA_DRAWENTIRE)
         || (lpDrawItem->itemAction & ODA_SELECT))
    {
        hBitmap = (pLBD->fButtonDown) ?
                    pLBD->hbmpDown :
                    pLBD->hbmpUp;

        hMemoryDC = CreateCompatibleDC(lpDrawItem->hDC);
        hOldBitmap = SelectObject(hMemoryDC,hBitmap);
        if (hOldBitmap)
        {
            BitBlt(lpDrawItem->hDC,
                   lpDrawItem->rcItem.left,
                   lpDrawItem->rcItem.top,
                   lpDrawItem->rcItem.right - lpDrawItem->rcItem.left,
                   lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                   hMemoryDC, 0,0,SRCCOPY);
            SelectObject(hMemoryDC, hOldBitmap);
        }
        DeleteDC(hMemoryDC);
    }

    /******************************/
    /* Draw the focus rect        */
    /******************************/
    if (lpDrawItem->itemAction & ODA_FOCUS)
    {
        RECT rcFocus;

        CopyRect(&rcFocus,&pLBD->rcText);
        OffsetRect(&rcFocus,lpDrawItem->rcItem.left,lpDrawItem->rcItem.top);
        InflateRect(&rcFocus,1,1);
        if (pLBD->fButtonDown)
            OffsetRect(&rcFocus,2,2);

        DrawFocusRect(lpDrawItem->hDC,&rcFocus);
    }


}


/**********************************************************************\
 *
 *  NAME:       BL_OnMeasureItem
 *
 *  SYNOPSIS:   Handle WM_MEASUREITEM for button listbox
 *
 *  ENTRY:      hwnd                HWND    window handle
 *              lpMeasureItem       MEASUREITEM FAR*
 *
 *  EXIT:       void
 *
 *  NOTES:      Return the item width and height for the button listbox.
 *              The item width and height are not equal to the button
 *              width and height since we want the button borders to
 *              overlap by 1 pixel.
 *
\**********************************************************************/

void NEAR PASCAL BL_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT FAR* lpMeasureItem)
{
    PBLINFO pbli;

    pbli = (PBLINFO)GetWindowInt(hwnd, 0);
    if (!pbli)
        return;

    lpMeasureItem->itemWidth = pbli->cxButton;
    lpMeasureItem->itemHeight = pbli->cyButton;

}


/**********************************************************************\
 *
 *  NAME:       BL_OnCompareItem
 *
 *  SYNOPSIS:   Handle WM_COMPAREITEM for button listbox
 *
 *  ENTRY:      hwnd                HWND    window handle
 *              lpCompareItem       COMPAREITEMSTRUCT FAR*
 *
 *  EXIT:       int                 -1 if item1 < item2
 *                                   0 if item1 == item 2
 *                                   1 if item2 > item2
 *
 *  NOTES:      The comparison is based on the button text.
 *
\**********************************************************************/

int  NEAR PASCAL BL_OnCompareItem(HWND hwnd, const COMPAREITEMSTRUCT FAR* lpCompareItem)
{
    PLISTBUTTONDATA pLBD1, pLBD2;

    Reference(hwnd);

    pLBD1 = (PLISTBUTTONDATA)(UINT)lpCompareItem->itemData1;
    pLBD2 = (PLISTBUTTONDATA)(UINT)lpCompareItem->itemData2;

    return lstrcmpi(pLBD1->szText, pLBD2->szText);
}


/**********************************************************************\
 *
 *  NAME:       BL_OnCharToItem
 *
 *  SYNOPSIS:   Handle WM_CHARTOITEM for button listbox
 *
 *  ENTRY:      hwnd                HWND    window handle
 *              ch                  UINT    character input
 *              hwndListBox         HWND    listbox control handle
 *              iCaret              int     current caret position
 *
 *  EXIT:       return              int     -2 if button selected
 *                                          -1 if not
 *
 *  NOTES:      Find next button whose text begins with ch starting
 *              with the current button.
 *
\**********************************************************************/

int NEAR PASCAL BL_OnCharToItem(HWND hwnd, UINT ch, HWND hwndListbox, int iCaret)
{
    PLISTBUTTONDATA pLBD;
    int             cbItems;
    int             nItem;

    Reference(hwnd);

    cbItems = ListBox_GetCount(hwndListbox);
    for (nItem = iCaret+1; nItem < cbItems; nItem++)
    {
        pLBD = GetListButtonData(hwndListbox, nItem);
        if (!pLBD)
            return -1;
        if ((pLBD->chUpper == ch) || (pLBD->chLower == ch))
        {
            ListBox_SetCurSel(hwndListbox, nItem);
            return -2;
        }
    }

    for (nItem = 0; nItem < iCaret; nItem++)
    {
        pLBD = GetListButtonData(hwndListbox, nItem);
        if (!pLBD)
            return -1;
        if ((pLBD->chUpper == ch) || (pLBD->chLower == ch))
        {
            ListBox_SetCurSel(hwndListbox,nItem);
            return -2;
        }
    }

    return -1;
}


/**********************************************************************\
 *
 *  NAME:       BL_OnDeleteItem
 *
 *  SYNOPSIS:   Handle WM_DELETEITEM for button listbox
 *
 *  ENTRY:      hwnd                HWND    window handle
 *              lpDeleteItem        DELETEITEMSTRUCT FAR*
 *
 *  EXIT:       void
 *
 *  NOTES:      Clean up the stuff that we created in CreateListButton and
 *              forward the message to the parent dialog.
 *
\**********************************************************************/

void NEAR PASCAL BL_OnDeleteItem(HWND hwnd, const DELETEITEMSTRUCT FAR* lpDeleteItem)
{
    PLISTBUTTONDATA pLBD;
    DELETEITEMSTRUCT di;

    pLBD = (PLISTBUTTONDATA)(UINT)lpDeleteItem->itemData;
    if (!pLBD)
        return;

    di.CtlType = lpDeleteItem->CtlType;
    di.CtlID = GetDlgCtrlID(hwnd);
    di.itemID = lpDeleteItem->itemID;
    di.hwndItem = hwnd;
    di.itemData = pLBD->dwItemData;

    SendMessage(GetParent(hwnd),WM_DELETEITEM,(WPARAM)di.CtlID,(LPARAM)(LPSTR)&di);

    DeleteListButton(pLBD);
}


/**********************************************************************\
 *
 *  NAME:       BL_OnGetDlgCode
 *
 *  SYNOPSIS:   Handle WM_GETDLGCODE for button listbox
 *
 *  ENTRY:      hwnd                HWND    window handle
 *              lpmsg               MSG FAR*
 *
 *  EXIT:       return              UINT    dialog code
 *
 *  NOTES:      Get the code from the child listbox and also set
 *              the button bit.
 *
\**********************************************************************/

UINT NEAR PASCAL BL_OnGetDlgCode(HWND hwnd, MSG FAR* lpmsg)
{
    UINT uCode;

    uCode = FORWARD_WM_GETDLGCODE(GetDlgItem(hwnd,ID_LISTBOX),lpmsg,SendMessage);
    uCode |= DLGC_BUTTON;
    return uCode;
}

/**********************************************************************\
 *
 *  NAME:       BL_OnCtlColor
 *
 *  SYNOPSIS:   Handle WM_CTLCOLOR for button listbox
 *
 *  ENTRY:      hwnd                HWND    window handle
 *              hcd                 HDC     dc of window
 *              hwndChild           HWND    control handle
 *              type                int     control type
 *
 *  EXIT:       return              HBRUSH  Button listbox bg color
 *
\**********************************************************************/


HBRUSH NEAR PASCAL BL_OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
    Reference(hwnd);
    Reference(hdc);
    Reference(hwndChild);
    Reference(type);

    return hBrushBackground;
}


/**********************************************************************\
 *
 *  NAME:       BL_OnCommand
 *
 *  SYNOPSIS:   Handle WM_COMMAND for button listbox
 *
 *  ENTRY:      hwnd                HWND    window handle
 *              hwndCtl             HWND    control handle
 *              codeNotiry          UINT    notify code from control
 *
 *  EXIT:       void
 *
 *  NOTES:      Pass the message to the parent dialog as though the
 *              button listbox generated it instead of the child listbox.
 *
\**********************************************************************/

void NEAR PASCAL BL_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    id = GetDlgCtrlID(hwnd);
    hwndCtl = hwnd;
    hwnd = GetParent(hwnd);
    UpdateWindow(hwndCtl);
    FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, SendMessage);
}


/**********************************************************************\
 *
 *  NAME:       CreateButtonBitmap
 *
 *  SYNOPSIS:   Create a new bitmap for the up or down button.
 *
 *  ENTRY:      nWidth              int     width of button
 *              nHeight             int     height of button
 *              fButtonDown         BOOL    TRUE if button pressed
 *              hUserBitmap         HBITMAP user bitmap to draw on button
 *              lpszUserText        LPCSTR  user text to draw on button
 *              rcText              LPRECT  rect of text in button
 *
 *  EXIT:       return              HBITMAP new bitmap for button
 *              rcText              LPRECT  rect of button text
 *
\**********************************************************************/


HBITMAP NEAR PASCAL CreateButtonBitmap(HWND hLB, int nWidth, int nHeight, BOOL fButtonDown,
                           HBITMAP hUserBitmap, LPCSTR lpszUserText,
                           LPRECT rcText)
{
    HDC     hdc;
    HDC     hMemoryDC;
    HBRUSH  hOldBrush;
    HBRUSH  hBlackBrush;
    HBITMAP hBitmap;
    HBITMAP hOldBitmap;
    BITMAP  bmButton;

    HDC     hUserDC;
    HBITMAP hOldUserBitmap;
    BITMAP  bm;
    int     nWidthT, nHeightT;
    int     textWidth, textHeight;
    int     xDest, yDest;
    int     cxDest, cyDest;
    char    szText[40];
    int     cbText;

    HFONT   hFontText;
    HFONT   hOldFont;

    DWORD   dwLBStyle;
    BOOL    fRightBorder;
    BOOL    fBottomBorder;
    LOGFONT lf;
#ifndef WIN32
    LOGFONT_32 lf32;
#endif

    #define BORDER_WIDTH        1
    #define HILIGHT_WIDTH       2
    #define FACE_BORDER_WIDTH   (BORDER_WIDTH+HILIGHT_WIDTH+1)

    #define PICT_BORDER_WIDTH   (FACE_BORDER_WIDTH+0)
    #define TEXT_BORDER_WIDTH   (FACE_BORDER_WIDTH+0)
    #define TEXTLINES           1

    // Get the listbox style
    dwLBStyle = (DWORD)GetWindowLong(hLB, GWL_STYLE);
    fRightBorder = ((dwLBStyle & LBS_MULTICOLUMN) != 0L);
    fBottomBorder = !fRightBorder;

    // Create drawing DC and bitmap based upon the desktop window
    // BUGBUG, not error checks!
    hdc = GetDC(NULL);
    hMemoryDC = CreateCompatibleDC(hdc);
    hBitmap = CreateCompatibleBitmap(hdc,nWidth,nHeight);
    hOldBitmap = SelectObject(hMemoryDC, hBitmap);
    GetObject(hBitmap, sizeof(BITMAP), &bmButton);

    // Draw the button face
    hOldBrush = SelectObject(hMemoryDC,g_hbrBtnFace);
    PatBlt(hMemoryDC,0,0,nWidth,nHeight,PATCOPY);

    // Draw the button border
    hBlackBrush = GetStockObject(BLACK_BRUSH);
    SelectObject(hMemoryDC,hBlackBrush);
    if (fRightBorder)
        PatBlt(hMemoryDC,nWidth-1,0,1,nHeight,PATCOPY);
    if (fBottomBorder)
        PatBlt(hMemoryDC,0,nHeight-1,nWidth,1,PATCOPY);

    // subtract out the border
    if (fRightBorder)
        nWidth--;
    if (fBottomBorder)
        nHeight--;

    // Draw the highlights and shadow
    if (fButtonDown)
    {
        SelectObject(hMemoryDC,g_hbrBtnShadow);
        PatBlt(hMemoryDC,0,0,nWidth,1,PATCOPY);
        PatBlt(hMemoryDC,0,0,1,nHeight,PATCOPY);
    }
    else
    {
        SelectObject(hMemoryDC,g_hbrBtnHighlight);
        PatBlt(hMemoryDC,0,0,nWidth,2,PATCOPY);
        PatBlt(hMemoryDC,0,0,2,nHeight,PATCOPY);

        SelectObject(hMemoryDC,g_hbrBtnShadow);
        PatBlt(hMemoryDC,nWidth-2,1,1,nHeight-1,PATCOPY);
        PatBlt(hMemoryDC,nWidth-1,0,1,nHeight,PATCOPY);
        PatBlt(hMemoryDC,1,nHeight-2,nWidth-1,1,PATCOPY);
        PatBlt(hMemoryDC,0,nHeight-1,nWidth,1,PATCOPY);
    }


    // Superimpose the user text in lower 1/3 of button
#ifdef WIN32
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
#else
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf32), &lf32, FALSE);

    lf.lfHeight = (int)lf32.lfHeight;
    lf.lfWidth = (int)lf32.lfWidth;
    lf.lfEscapement = (int)lf32.lfEscapement;
    lf.lfOrientation = (int)lf32.lfOrientation;
    lf.lfWeight = (int)lf32.lfWeight;
    hmemcpy(&lf.lfItalic, &lf32.lfCommon, sizeof(COMMONFONT));
#endif

    hFontText = CreateFontIndirect(&lf);

    cbText = lstrlen(lpszUserText);
#ifdef WIN32
    // BUGBUG: I see no reason why the WIN32 side should assume that
    // lpszUserText will fit into szText.
    lstrcpy(szText, lpszUserText);
#else
    //sizeof(c_szEllipses) is 0, since this is an extern string
    //lstrcpyn(szText, lpszUserText, sizeof(szText) - sizeof(s_szElipsis));
    lstrcpyn(szText, lpszUserText, sizeof(szText));
#endif

    hOldFont = SelectObject(hMemoryDC, hFontText);
    SetTextColor(hMemoryDC, g_clrBtnText);
    SetBkMode(hMemoryDC, TRANSPARENT);
    SetTextAlign(hMemoryDC, TA_TOP);

    MGetTextExtent(hMemoryDC, szText, cbText, &textWidth, &textHeight);

    nWidthT  = nWidth - 2*TEXT_BORDER_WIDTH;
    nHeightT = nHeight - 2*TEXT_BORDER_WIDTH;

    xDest = TEXT_BORDER_WIDTH + ((textWidth < nWidthT ) ?
                                 (nWidthT - textWidth) / 2 : 0);

    yDest = TEXT_BORDER_WIDTH + nHeightT - TEXTLINES*(textHeight);

    rcText->top = yDest;
    rcText->left = xDest;
    rcText->right = rcText->left + min(textWidth,nWidthT);
    rcText->bottom = rcText->top + TEXTLINES*(textHeight);

    // Elipsize text if needed
    if (textWidth > nWidthT)
    {
        int     nWidthText;
        int     cbTextT;
        MGetTextExtent(hMemoryDC, c_szEllipses, lstrlen(c_szEllipses), &nWidthText, NULL);
        nWidthT -= nWidthText;

	// BUGBUG: if the first character's extent is > nWidthT, we clobber cyDest.
	// fortunately it isn't in use yet...  We'll also not truncate the text
	// for the elipsis.
        for (cbTextT = 0; cbTextT < cbText; cbTextT++)
        {
            MGetTextExtent(hMemoryDC, szText, cbTextT, &nWidthText, NULL);
            if (nWidthText > nWidthT)
                break;
        }
        szText[--cbTextT] = 0;
        lstrcat(szText, c_szEllipses);
        cbText = lstrlen(szText);
    }

    if (fButtonDown)
    {
        xDest += 2;
        yDest += 2;
        OffsetRect(rcText,2,2);
    }

    ExtTextOut(hMemoryDC, xDest, yDest, ETO_CLIPPED, rcText, szText, cbText, NULL);

    if (fButtonDown)
        OffsetRect(rcText,-2,-2);

    // if the bitmaps are compatible,
    // Superimpose the user bitmap centered horizontally and
    // vertically in above rcText

    GetObject(hUserBitmap,sizeof(BITMAP),&bm);
    if (bm.bmPlanes == bmButton.bmPlanes)
    {
        nWidthT = nWidth - 2*PICT_BORDER_WIDTH;
        nHeightT = rcText->top - PICT_BORDER_WIDTH;

        xDest = PICT_BORDER_WIDTH + ((bm.bmWidth < nWidthT ) ?
                    (nWidthT - bm.bmWidth) / 2 : 0);
        cxDest = (bm.bmWidth < nWidthT ) ?
                    bm.bmWidth : nWidthT;

        yDest = PICT_BORDER_WIDTH + ((bm.bmHeight < nHeightT) ?
                    (nHeightT - bm.bmHeight) / 2 : 0);
        cyDest = (bm.bmHeight < nHeightT) ?
                    bm.bmHeight : nHeightT;

        if (fButtonDown)
        {
            xDest += 2;
            yDest += 2;
        }

        hUserDC = CreateCompatibleDC(hdc);
        hOldUserBitmap = SelectObject(hUserDC,hUserBitmap);
        if (hOldUserBitmap)
        {
            BitBlt(hMemoryDC,xDest,yDest,cxDest,cyDest,hUserDC,0,0,SRCCOPY);
            SelectObject(hUserDC,hOldUserBitmap);
        }
        DeleteDC(hUserDC);
    }




    // Cleanup
    SelectObject(hMemoryDC, hOldBrush);
    SelectObject(hMemoryDC, hOldBitmap);
    SelectObject(hMemoryDC, hOldFont);
    DeleteObject(hFontText);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hdc);

    return hBitmap;
}


/**********************************************************************\
 *
 *  NAME:       SubListBoxProc
 *
 *  SYNOPSIS:   ListBox subclassing window proc for the button listbox
 *              child listbox.
 *
 *  ENTRY:      hwnd                HWND    Window handle of listbox
 *              uMsg                UINT    Window message
 *              wParam              WPARAM  message dependent param
 *              lParam              LPARAM  message dependent param
 *
 *  EXIT:       return              LRESULT message dependent
 *
 *  NOTES:      This window proc handles messages to perform hit-testing
 *              on the button items in the listbox and does pre-processing
 *              of messages that add or change item data.
 *
 *              Messages that are not explicitly handled are forwarded
 *              to the default listbox window proc.
 *
\**********************************************************************/

LRESULT CALLBACK SubListBoxProc(HWND hLB, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(hLB, WM_LBUTTONDOWN,  Sub_OnLButtonDown);
    HANDLE_MSG(hLB, WM_LBUTTONUP,    Sub_OnLButtonUp);
    HANDLE_MSG(hLB, WM_MOUSEMOVE,    Sub_OnMouseMove);
    HANDLE_MSG(hLB, WM_KEYDOWN,      Sub_OnKey);
    HANDLE_MSG(hLB, WM_KEYUP,        Sub_OnKey);
    }

    return CallWindowProc(lpDefListBoxProc, hLB, uMsg, wParam, lParam);
}


/**********************************************************************\
 *
 *  NAME:       Sub_OnLButtonDown
 *
 *  SYNOPSIS:   Handle the WM_LBUTTONDOWN message for the child listbox
 *
 *  ENTRY:      hLB             HWND    window handle of listbox
 *              fDoubldClick    BOOL    TRUE if double click, else FALSE
 *              x               int     horizontal mouse coordinate
 *              y               int     vertical mouse coordinate
 *              keyFlags        UINT    flags from the VK message
 *
 *  EXIT:       void
 *
 *  NOTES:      On a mouse button down, check to see which item the mouse
 *              is in. Set the fButtonDown flag for the pressed button
 *              and invalidate the button so that it will be drawn pressed.
 *
 *              Save the item number of the pressed button for use in the
 *              Sub_OnLButtonUp and Sub_OnMouseMove handlers.
 *
\**********************************************************************/

void NEAR PASCAL Sub_OnLButtonDown(HWND hLB, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    PLISTBUTTONDATA pLBD;
    int     nItem;
    int     cbItem;
    RECT    rcItem;
    POINT   pt;
    PBLINFO pbli;

    Reference(fDoubleClick);

    pbli = (PBLINFO)GetWindowInt(GetParent(hLB), 0);
    if (!pbli)
        return;

    cbItem = ListBox_GetCount(hLB);
    pt.x = x;
    pt.y = y;

    for (nItem = 0; nItem < cbItem; nItem++)
    {
        pLBD = GetListButtonData(hLB, nItem);
        if (!pLBD)
            return;

        ListBox_GetItemRect(hLB, nItem, &rcItem);
        if (PtInRect(&rcItem,pt))
        {
            pLBD->fButtonDown = TRUE;
            pbli->nTrackButton = nItem;
            InvalidateRect(hLB,&rcItem,FALSE);
        }
        else
            pLBD->fButtonDown = FALSE;
    }

    // tell list box this item was pressed
    CallWindowProc(lpDefListBoxProc,hLB,WM_LBUTTONDOWN,(WPARAM)keyFlags,MAKELPARAM(x,y));
}

/**********************************************************************\
 *
 *  NAME:       Sub_OnLButtonUp
 *
 *  SYNOPSIS:   Handle the WM_LBUTTONUP message for the child listbox
 *
 *  ENTRY:      hLB             HWND    window handle of listbox
 *              x               int     horizontal mouse coordinate
 *              y               int     vertical mouse coordinate
 *              keyFlags        UINT    flags from the VK message
 *
 *  EXIT:       void
 *
 *  NOTES:      If we're not tracking a button press, forward the message
 *              to the default listbox proc.
 *
 *              Otherwise, set all buttons to up and test if the mouse
 *              went up in the pressed button. If so, send the doubld click
 *              message to the listbox to cause a WM_COMMAND:BLN_CLICKED
 *              notification from the listbox.
 *
\**********************************************************************/

LONG NEAR PASCAL Sub_OnLButtonUp(HWND hLB, int x, int y, UINT keyFlags)
{
    PLISTBUTTONDATA pLBD;
    int                 nItem;
    int                 cbItem;
    RECT                rcItem;
    POINT               pt;
    PBLINFO pbli;
    int                 nPressedItem = -1;

    pbli = (PBLINFO)GetWindowInt(GetParent(hLB), 0);
    if (!pbli)
        return 0;

    cbItem = ListBox_GetCount(hLB);
    pt.x = x;
    pt.y = y;

    if (pbli->nTrackButton == -1)
    {
        CallWindowProc(lpDefListBoxProc, hLB, WM_LBUTTONUP, (WPARAM)keyFlags, MAKELPARAM(x,y));
        return 0;
    }

    for (nItem = 0; nItem < cbItem; nItem++)
    {
        pLBD = GetListButtonData(hLB, nItem);
        if (!pLBD)
            return 0;
        if (pLBD->fButtonDown)
            nPressedItem = nItem;
        pLBD->fButtonDown = FALSE;
    }

    if (nPressedItem != -1)
    {
        // BOOM! We got a button press
        pLBD = GetListButtonData(hLB, nPressedItem);
        if (!pLBD)
            return 0;
        ListBox_GetItemRect(hLB, nPressedItem, &rcItem);
        InvalidateRect(hLB,&rcItem,FALSE);

        CallWindowProc(lpDefListBoxProc, hLB, WM_LBUTTONDBLCLK, (WPARAM)keyFlags, MAKELPARAM(x,y));

    }

    pbli->nTrackButton = -1;

    CallWindowProc(lpDefListBoxProc,hLB,WM_LBUTTONUP,(WPARAM)keyFlags,MAKELPARAM(x,y));

    return 0;
}


/**********************************************************************\
 *
 *  NAME:       Sub_OnMouseMove
 *
 *  SYNOPSIS:   Handle the WM_MOUSEMOVE message for the child listbox
 *
 *  ENTRY:      hLB             HWND    window handle of the listbox
 *              x               int     horizontal mouse coordinate
 *              y               int     vertical mouse coordinate
 *              keyFlags        UINT    flags from the VK message
 *
 *  EXIT:       void
 *
 *  NOTES:      If we're not tracking a button press, forward the
 *              message to the default listbox proc.
 *
 *              Otherwise, if the mouse enters or leaves the button rectangle,
 *              redraw the button in the up or down state.
 *
\**********************************************************************/

void NEAR PASCAL Sub_OnMouseMove(HWND hLB, int x, int y, UINT keyFlags)
{
    PLISTBUTTONDATA pLBD;
    int                 nItem;
    RECT                rcItem;
    POINT               pt;
    PBLINFO pbli;
    BOOL                fInRect;

    // Pass to listbox if not button down
    if (!(keyFlags & MK_LBUTTON))
    {
        CallWindowProc(lpDefListBoxProc,hLB,WM_MOUSEMOVE,(WPARAM)keyFlags,MAKELPARAM(x,y));
        return;
    }

    pbli = (PBLINFO)GetWindowInt(GetParent(hLB), 0);
    if (!pbli)
        return;
    pt.x = x;
    pt.y = y;

    nItem = pbli->nTrackButton;
    if (nItem == -1)
        return;

    ListBox_GetItemRect(hLB, nItem, &rcItem);
    pLBD = GetListButtonData(hLB, nItem);
    if (!pLBD)
        return;
    fInRect = PtInRect(&rcItem,pt);

    if (fInRect != pLBD->fButtonDown)
    {
        pLBD->fButtonDown = fInRect;
        InvalidateRect(hLB,&rcItem,FALSE);
    }

}

/**********************************************************************\
 *
 *  NAME:       Sub_OnKey
 *
 *  SYNOPSIS:   Handle the WM_KEYDOWN and WM_KEYUP message for the
 *              child listbox
 *
 *  ENTRY:      hLB             HWND    window handle of the listbox
 *              fDown           BOOL    TRUE if keydown, else false
 *              cRepeat         int     repeat count
 *              flags           UINT    flags from the VK message
 *
 *  EXIT:       void
 *
 *  NOTES:      If the spacebar goes down, paint the current button
 *              as pressed. When it goes up, paint the current button
 *              as uppressed and send a double click to the listbox
 *              for the button to cause a WM_COMMAND:BLN_CLICKED
 *              notification from the listbox.
 *
\**********************************************************************/

void NEAR PASCAL Sub_OnKey(HWND hLB, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    PLISTBUTTONDATA pLBD;
    int                 nItem;
    RECT                rcItem;
    PBLINFO pbli;

    pbli = (PBLINFO)GetWindowInt(GetParent(hLB), 0);
    if (!pbli)
        return;
    if (pbli->nTrackButton >= 0)
        return;

    if (vk == VK_SPACE)
    {
        nItem = ListBox_GetCurSel(hLB);
        if (nItem >= 0)
        {
	    pLBD = GetListButtonData(hLB, nItem);
            if (!pLBD)
                return;

            if (pLBD->fButtonDown != fDown)
            {
                pLBD->fButtonDown = fDown;
                ListBox_GetItemRect(hLB, nItem, &rcItem);
                InvalidateRect(hLB, &rcItem, FALSE);

                // When the key goes up, fake a double click
                // to generate a WM_COMMAND notification
                if (!fDown)
                    CallWindowProc(lpDefListBoxProc,hLB,WM_LBUTTONDBLCLK,(WPARAM)flags,
                                    MAKELPARAM(rcItem.left,rcItem.top));
            }

        }
    }

    CallWindowProc(lpDefListBoxProc,
                    hLB,
                    fDown? WM_KEYDOWN : WM_KEYUP,
                    (WPARAM)vk,
                    MAKELPARAM((UINT)cRepeat,flags));
}


/**********************************************************************\
 *
 *  NAME:       CreateListButton
 *
 *  SYNOPSIS:   Setup internal fields in the LISTBUTTONDATA structure
 *
 *  ENTRY:      hLB             HWND    window handle of child listbox
 *              pLBD           PLISTBUTTONDATA ptr to input structure
 *
 *  EXIT:       return          PLISTBUTTONDATA ptr to output structure
 *                                      NULL if an error occurs
 *
 *  NOTES:      This function should is called in response to the
 *              BL_ADDBUTTON and BL_INSERTBUTTON messages.
 *
\**********************************************************************/

PLISTBUTTONDATA NEAR PASCAL CreateListButton(HWND hLB,CREATELISTBUTTON FAR* lpCLB)
{
    PBLINFO pbli;
    PLISTBUTTONDATA pNewLBD;

    if (!lpCLB)
    {
        DEBUGPRINTF(("CreateListButton: !lpCLB"));
        return NULL;
    }
    if (lpCLB->cbSize != sizeof(CREATELISTBUTTON))
    {
        DEBUGPRINTF(("CreateListButton: lpCLB->cbSize wrong"));
        return NULL;
    }
    if (!lpCLB->hBitmap)
    {
        DEBUGPRINTF(("CreateListButton: !lpCLB->hBitmap"));
        return NULL;
    }
    if (!lpCLB->lpszText)
    {
        DEBUGPRINTF(("CreateListButton: !lpCLB->lpszText"));
        return NULL;
    }

    pbli = (PBLINFO)GetWindowInt(GetParent(hLB), 0);
    if (!pbli)
    {
        DEBUGPRINTF(("CreateListButton: !pbli"));
        return NULL;
    }

    pNewLBD = (PLISTBUTTONDATA)LocalAlloc(LPTR, sizeof(LISTBUTTONDATA) + lstrlen(lpCLB->lpszText));
    if (!pNewLBD)
    {
        DEBUGPRINTF(("CreateListButton: !LocalAlloc pNewLBD"));
        return NULL;
    }

    // copy over user item data
    pNewLBD->dwItemData = lpCLB->dwItemData;

    lstrcpy(pNewLBD->szText, lpCLB->lpszText);

    // init internal fields
    pNewLBD->fButtonDown = FALSE;
    pNewLBD->chUpper = (UINT)LOWORD(AnsiUpper((LPSTR)(DWORD)(BYTE)pNewLBD->szText[0]));
    pNewLBD->chLower = (UINT)LOWORD(AnsiLower((LPSTR)(DWORD)(BYTE)pNewLBD->szText[0]));

    // create the up and down bitmaps

    pNewLBD->hbmpUp = CreateButtonBitmap(hLB,
                                         pbli->cxButton,
                                         pbli->cyButton,
                                         FALSE,  // button not down
                                         lpCLB->hBitmap,
                                         pNewLBD->szText,
                                         &pNewLBD->rcText);
    if (!pNewLBD->hbmpUp)
    {
        DEBUGPRINTF(("CreateListButton: !CreateButtonBitmap() UP"));
        goto error;
    }

    pNewLBD->hbmpDown = CreateButtonBitmap(hLB,
                                           pbli->cxButton,
                                           pbli->cyButton,
                                           TRUE,  // button down
                                           lpCLB->hBitmap,
                                           pNewLBD->szText,
                                           &pNewLBD->rcText);
    if (!pNewLBD->hbmpDown)
    {
        DEBUGPRINTF(("CreateListButton: !CreateButtonBitmap() DOWN"));
        goto error;
    }

    return pNewLBD;

error:

    DeleteListButton(pNewLBD);
    return NULL;
}

/**********************************************************************\
 *
 *  NAME:       DeleteListButton
 *
 *  SYNOPSIS:   Free memory and internal objects in allocated from
 *              CreateListButton
 *
 *  ENTRY:      pLBD               PLISTBUTTONDATA
 *
 *  EXIT:       void
 *
\**********************************************************************/

VOID NEAR PASCAL DeleteListButton(PLISTBUTTONDATA pLBD)
{
    if (pLBD)
    {
        if (pLBD->hbmpUp)
            DeleteObject(pLBD->hbmpUp);
        if (pLBD->hbmpDown)
            DeleteObject(pLBD->hbmpDown);

        LocalFree((HLOCAL)pLBD);
    }
}

#ifdef DEBUG
static void cdecl DebugPrintf(LPCSTR lpsz, int first, ...)
{
    char    sz[256];
    wvsprintf(sz,lpsz,&first);
    OutputDebugString(sz);
    OutputDebugString("\n\r");
}
#endif
