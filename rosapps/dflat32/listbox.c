/* ------------- listbox.c ------------ */

#include "dflat.h"

#ifdef INCLUDE_EXTENDEDSELECTIONS
static int ExtendSelections(DFWINDOW, int, int);
static void TestExtended(DFWINDOW, DF_PARAM);
static void ClearAllSelections(DFWINDOW);
static void SetSelection(DFWINDOW, int);
static void FlipSelection(DFWINDOW, int);
static void ClearSelection(DFWINDOW, int);
#else
#define TestExtended(w,p) /**/
#endif
static void ChangeSelection(DFWINDOW, int, int);
static void WriteSelection(DFWINDOW, int, int, DFRECT *);
static BOOL SelectionInWindow(DFWINDOW, int);

static int py = -1;    /* the previous y mouse coordinate */

#ifdef INCLUDE_EXTENDEDSELECTIONS
/* --------- DF_SHIFT_F8 Key ------------ */
static void AddModeKey(DFWINDOW wnd)
{
    if (DfIsMultiLine(wnd))    {
        wnd->AddMode ^= TRUE;
        DfSendMessage(DfGetParent(wnd), DFM_ADDSTATUS,
            wnd->AddMode ? ((DF_PARAM) "Add Mode") : 0, 0);
    }
}
#endif

/* --------- DF_UP (Up Arrow) Key ------------ */
static void UpKey(DFWINDOW wnd, DF_PARAM p2)
{
    if (wnd->selection > 0)    {
        if (wnd->selection == wnd->wtop)    {
            DfBaseWndProc(DF_LISTBOX, wnd, DFM_KEYBOARD, DF_UP, p2);
            DfPostMessage(wnd, DFM_LB_SELECTION, wnd->selection-1,
                DfIsMultiLine(wnd) ? p2 : FALSE);
        }
        else    {
            int newsel = wnd->selection-1;
            if (wnd->wlines == DfClientHeight(wnd))
                while (*DfTextLine(wnd, newsel) == DF_LINE)
                    --newsel;
            DfPostMessage(wnd, DFM_LB_SELECTION, newsel,
#ifdef INCLUDE_EXTENDEDSELECTIONS
                DfIsMultiLine(wnd) ? p2 :
#endif
                FALSE);
        }
    }
}

/* --------- DF_DN (Down Arrow) Key ------------ */
static void DnKey(DFWINDOW wnd, DF_PARAM p2)
{
    if (wnd->selection < wnd->wlines-1)    {
        if (wnd->selection == wnd->wtop+DfClientHeight(wnd)-1)  {
            DfBaseWndProc(DF_LISTBOX, wnd, DFM_KEYBOARD, DF_DN, p2);
            DfPostMessage(wnd, DFM_LB_SELECTION, wnd->selection+1,
                DfIsMultiLine(wnd) ? p2 : FALSE);
        }
        else    {
            int newsel = wnd->selection+1;
            if (wnd->wlines == DfClientHeight(wnd))
                while (*DfTextLine(wnd, newsel) == DF_LINE)
                    newsel++;
            DfPostMessage(wnd, DFM_LB_SELECTION, newsel,
#ifdef INCLUDE_EXTENDEDSELECTIONS
                DfIsMultiLine(wnd) ? p2 :
#endif
                FALSE);
        }
    }
}

/* --------- DF_HOME and DF_PGUP Keys ------------ */
static void HomePgUpKey(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    DfBaseWndProc(DF_LISTBOX, wnd, DFM_KEYBOARD, p1, p2);
    DfPostMessage(wnd, DFM_LB_SELECTION, wnd->wtop,
#ifdef INCLUDE_EXTENDEDSELECTIONS
        DfIsMultiLine(wnd) ? p2 :
#endif
        FALSE);
}

/* --------- DF_END and DF_PGDN Keys ------------ */
static void EndPgDnKey(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int bot;
    DfBaseWndProc(DF_LISTBOX, wnd, DFM_KEYBOARD, p1, p2);
    bot = wnd->wtop+DfClientHeight(wnd)-1;
    if (bot > wnd->wlines-1)
        bot = wnd->wlines-1;
    DfPostMessage(wnd, DFM_LB_SELECTION, bot,
#ifdef INCLUDE_EXTENDEDSELECTIONS
        DfIsMultiLine(wnd) ? p2 :
#endif
        FALSE);
}

#ifdef INCLUDE_EXTENDEDSELECTIONS
/* --------- Space Bar Key ------------ */
static void SpacebarKey(DFWINDOW wnd, DF_PARAM p2)
{
    if (DfIsMultiLine(wnd))    {
        int sel = DfSendMessage(wnd, DFM_LB_CURRENTSELECTION, 0, 0);
        if (sel != -1)    {
            if (wnd->AddMode)
                FlipSelection(wnd, sel);
            if (DfItemSelected(wnd, sel))    {
                if (!((int) p2 & (DF_LEFTSHIFT | DF_RIGHTSHIFT)))
                    wnd->AnchorPoint = sel;
                ExtendSelections(wnd, sel, (int) p2);
            }
            else
                wnd->AnchorPoint = -1;
            DfSendMessage(wnd, DFM_PAINT, 0, 0);
        }
    }
}
#endif

/* --------- Enter ('\r') Key ------------ */
static void EnterKey(DFWINDOW wnd)
{
    if (wnd->selection != -1)    {
        DfSendMessage(wnd, DFM_LB_SELECTION, wnd->selection, TRUE);
        DfSendMessage(wnd, DFM_LB_CHOOSE, wnd->selection, 0);
    }
}

/* --------- All Other Key Presses ------------ */
static void KeyPress(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int sel = wnd->selection+1;
    while (sel < wnd->wlines)    {
        char *cp = DfTextLine(wnd, sel);
        if (cp == NULL)
            break;
#ifdef INCLUDE_EXTENDEDSELECTIONS
        if (DfIsMultiLine(wnd))
            cp++;
#endif
        /* --- special for directory list box --- */
        if (*cp == '[')
            cp++;
        if (tolower(*cp) == (int)p1)    {
            DfSendMessage(wnd, DFM_LB_SELECTION, sel,
                DfIsMultiLine(wnd) ? p2 : FALSE);
            if (!SelectionInWindow(wnd, sel))    {
                wnd->wtop = sel-DfClientHeight(wnd)+1;
                DfSendMessage(wnd, DFM_PAINT, 0, 0);
            }
            break;
        }
        sel++;
    }
}

/* --------- DFM_KEYBOARD Message ------------ */
static int KeyboardMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    switch ((int) p1)    {
#ifdef INCLUDE_EXTENDEDSELECTIONS
        case DF_SHIFT_F8:
            AddModeKey(wnd);
            return TRUE;
#endif
        case DF_UP:
            TestExtended(wnd, p2);
            UpKey(wnd, p2);
            return TRUE;
        case DF_DN:
            TestExtended(wnd, p2);
            DnKey(wnd, p2);
            return TRUE;
        case DF_PGUP:
        case DF_HOME:
            TestExtended(wnd, p2);
            HomePgUpKey(wnd, p1, p2);
            return TRUE;
        case DF_PGDN:
        case DF_END:
            TestExtended(wnd, p2);
            EndPgDnKey(wnd, p1, p2);
            return TRUE;
#ifdef INCLUDE_EXTENDEDSELECTIONS
        case ' ':
            SpacebarKey(wnd, p2);
            break;
#endif
        case '\r':
            EnterKey(wnd);
            return TRUE;
        default:
            KeyPress(wnd, p1, p2);
            break;
    }
    return FALSE;
}

/* ------- DFM_LEFT_BUTTON Message -------- */
static int LeftButtonMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int my = (int) p2 - DfGetTop(wnd);
    if (my >= wnd->wlines-wnd->wtop)
        my = wnd->wlines - wnd->wtop;

    if (!DfInsideRect(p1, p2, DfClientRect(wnd)))
        return FALSE;
    if (wnd->wlines && my != py)    {
        int sel = wnd->wtop+my-1;
#ifdef INCLUDE_EXTENDEDSELECTIONS
        int sh = DfGetShift();
        if (!(sh & (DF_LEFTSHIFT | DF_RIGHTSHIFT)))    {
            if (!(sh & DF_CTRLKEY))
                ClearAllSelections(wnd);
            wnd->AnchorPoint = sel;
            DfSendMessage(wnd, DFM_PAINT, 0, 0);
        }
#endif
        DfSendMessage(wnd, DFM_LB_SELECTION, sel, TRUE);
        py = my;
    }
    return TRUE;
}

/* ------------- DOUBLE_CLICK Message ------------ */
static int DoubleClickMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    if (DfWindowMoving || DfWindowSizing)
        return FALSE;
    if (wnd->wlines)    {
        DFRECT rc = DfClientRect(wnd);
        DfBaseWndProc(DF_LISTBOX, wnd, DOUBLE_CLICK, p1, p2);
        if (DfInsideRect(p1, p2, rc))
            DfSendMessage(wnd, DFM_LB_CHOOSE, wnd->selection, 0);
    }
    return TRUE;
}

/* ------------ DFM_ADDTEXT Message -------------- */
static int AddTextMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int rtn = DfBaseWndProc(DF_LISTBOX, wnd, DFM_ADDTEXT, p1, p2);
    if (wnd->selection == -1)
        DfSendMessage(wnd, DFM_LB_SETSELECTION, 0, 0);
#ifdef INCLUDE_EXTENDEDSELECTIONS
    if (*(char *)p1 == DF_LISTSELECTOR)
        wnd->SelectCount++;
#endif
    return rtn;
}

/* --------- DFM_GETTEXT Message ------------ */
static void GetTextMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    if ((int)p2 != -1)    {
        char *cp1 = (char *)p1;
        char *cp2 = DfTextLine(wnd, (int)p2);
        while (cp2 && *cp2 && *cp2 != '\n')
            *cp1++ = *cp2++;
        *cp1 = '\0';
    }
}

/* --------- DF_LISTBOX Window Processing Module ------------ */
int DfListBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            DfBaseWndProc(DF_LISTBOX, wnd, msg, p1, p2);
            wnd->selection = -1;
#ifdef INCLUDE_EXTENDEDSELECTIONS
            wnd->AnchorPoint = -1;
#endif
            return TRUE;
        case DFM_KEYBOARD:
            if (DfWindowMoving || DfWindowSizing)
                break;
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_LEFT_BUTTON:
            if (LeftButtonMsg(wnd, p1, p2) == TRUE)
                return TRUE;
            break;
        case DOUBLE_CLICK:
            if (DoubleClickMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_BUTTON_RELEASED:
            if (DfWindowMoving || DfWindowSizing || DfVSliding)
                break;
            py = -1;
            return TRUE;
        case DFM_ADDTEXT:
            return AddTextMsg(wnd, p1, p2);
        case DFM_LB_GETTEXT:
            GetTextMsg(wnd, p1, p2);
            return TRUE;
        case DFM_CLEARTEXT:
            wnd->selection = -1;
#ifdef INCLUDE_EXTENDEDSELECTIONS
            wnd->AnchorPoint = -1;
#endif
            wnd->SelectCount = 0;
            break;
        case DFM_PAINT:
            DfBaseWndProc(DF_LISTBOX, wnd, msg, p1, p2);
            WriteSelection(wnd, wnd->selection, TRUE, (DFRECT *)p1);
            return TRUE;
        case DFM_SCROLL:
        case DFM_HORIZSCROLL:
        case DFM_SCROLLPAGE:
        case DFM_HORIZPAGE:
        case DFM_SCROLLDOC:
            DfBaseWndProc(DF_LISTBOX, wnd, msg, p1, p2);
            WriteSelection(wnd,wnd->selection,TRUE,NULL);
            return TRUE;
        case DFM_LB_CHOOSE:
            DfSendMessage(DfGetParent(wnd), DFM_LB_CHOOSE, p1, p2);
            return TRUE;
        case DFM_LB_SELECTION:
            ChangeSelection(wnd, (int) p1, (int) p2);
            DfSendMessage(DfGetParent(wnd), DFM_LB_SELECTION,
                wnd->selection, 0);
            return TRUE;
        case DFM_LB_CURRENTSELECTION:
            return wnd->selection;
        case DFM_LB_SETSELECTION:
            ChangeSelection(wnd, (int) p1, 0);
            return TRUE;
#ifdef INCLUDE_EXTENDEDSELECTIONS
        case DFM_CLOSE_WINDOW:
            if (DfIsMultiLine(wnd) && wnd->AddMode)    {
                wnd->AddMode = FALSE;
                DfSendMessage(DfGetParent(wnd), DFM_ADDSTATUS, 0, 0);
            }
            break;
#endif
        default:
            break;
    }
    return DfBaseWndProc(DF_LISTBOX, wnd, msg, p1, p2);
}

static BOOL SelectionInWindow(DFWINDOW wnd, int sel)
{
    return (wnd->wlines && sel >= wnd->wtop &&
            sel < wnd->wtop+DfClientHeight(wnd));
}

static void WriteSelection(DFWINDOW wnd, int sel,
                                    int reverse, DFRECT *rc)
{
    if (DfIsVisible(wnd))
        if (SelectionInWindow(wnd, sel))
            DfWriteTextLine(wnd, rc, sel, reverse);
}

#ifdef INCLUDE_EXTENDEDSELECTIONS
/* ----- Test for extended selections in the listbox ----- */
static void TestExtended(DFWINDOW wnd, DF_PARAM p2)
{
    if (DfIsMultiLine(wnd) && !wnd->AddMode &&
            !((int) p2 & (DF_LEFTSHIFT | DF_RIGHTSHIFT)))    {
        if (wnd->SelectCount > 1)    {
            ClearAllSelections(wnd);
            DfSendMessage(wnd, DFM_PAINT, 0, 0);
        }
    }
}

/* ----- Clear selections in the listbox ----- */
static void ClearAllSelections(DFWINDOW wnd)
{
    if (DfIsMultiLine(wnd) && wnd->SelectCount > 0)    {
        int sel;
        for (sel = 0; sel < wnd->wlines; sel++)
            ClearSelection(wnd, sel);
    }
}

/* ----- Invert a selection in the listbox ----- */
static void FlipSelection(DFWINDOW wnd, int sel)
{
    if (DfIsMultiLine(wnd))    {
        if (DfItemSelected(wnd, sel))
            ClearSelection(wnd, sel);
        else
            SetSelection(wnd, sel);
    }
}

static int ExtendSelections(DFWINDOW wnd, int sel, int shift)
{    
    if (shift & (DF_LEFTSHIFT | DF_RIGHTSHIFT) &&
                        wnd->AnchorPoint != -1)    {
        int i = sel;
        int j = wnd->AnchorPoint;
        int rtn;
        if (j > i)
            swap(i,j);
        rtn = i - j;
        while (j <= i)
            SetSelection(wnd, j++);
        return rtn;
    }
    return 0;
}

static void SetSelection(DFWINDOW wnd, int sel)
{
    if (DfIsMultiLine(wnd) && !DfItemSelected(wnd, sel))    {
        char *lp = DfTextLine(wnd, sel);
        *lp = DF_LISTSELECTOR;
        wnd->SelectCount++;
    }
}

static void ClearSelection(DFWINDOW wnd, int sel)
{
    if (DfIsMultiLine(wnd) && DfItemSelected(wnd, sel))    {
        char *lp = DfTextLine(wnd, sel);
        *lp = ' ';
        --wnd->SelectCount;
    }
}

BOOL DfItemSelected(DFWINDOW wnd, int sel)
{
	if (sel != -1 && DfIsMultiLine(wnd) && sel < wnd->wlines)    {
        char *cp = DfTextLine(wnd, sel);
        return (int)((*cp) & 255) == DF_LISTSELECTOR;
    }
    return FALSE;
}
#endif

static void ChangeSelection(DFWINDOW wnd,int sel,int shift)
{
    if (sel != wnd->selection)    {
#ifdef INCLUDE_EXTENDEDSELECTIONS
        if (DfIsMultiLine(wnd))        {
            int sels;
            if (!wnd->AddMode)
                ClearAllSelections(wnd);
            sels = ExtendSelections(wnd, sel, shift);
            if (sels > 1)
                DfSendMessage(wnd, DFM_PAINT, 0, 0);
            if (sels == 0 && !wnd->AddMode)    {
                ClearSelection(wnd, wnd->selection);
                SetSelection(wnd, sel);
                wnd->AnchorPoint = sel;
            }
        }
#endif
        WriteSelection(wnd, wnd->selection, FALSE, NULL);
        wnd->selection = sel;
        WriteSelection(wnd, sel, TRUE, NULL);
     }
}

/* EOF */
