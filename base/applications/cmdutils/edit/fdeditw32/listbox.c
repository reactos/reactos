/* ------------- listbox.c ------------ */

#include "dflat.h"

#ifdef INCLUDE_EXTENDEDSELECTIONS
static int ExtendSelections(WINDOW, int, int);
static void TestExtended(WINDOW, PARAM);
static void ClearAllSelections(WINDOW);
static void SetSelection(WINDOW, int);
static void FlipSelection(WINDOW, int);
static void ClearSelection(WINDOW, int);
#else
#define TestExtended(w,p) /**/
#endif
static void near ChangeSelection(WINDOW, int, int);
static void near WriteSelection(WINDOW, int, int, RECT *);
static BOOL SelectionInWindow(WINDOW, int);

static int py=-1;    /* the previous y mouse coordinate */

#ifdef INCLUDE_EXTENDEDSELECTIONS
/* --------- CTRL_F8 Key ------------ */
static void AddModeKey(WINDOW wnd)
{
    if (isMultiLine(wnd))
        {
        wnd->AddMode ^= TRUE;
        SendMessage(GetParent(wnd), ADDSTATUS, wnd->AddMode ? ((PARAM) "Add Mode") : 0, 0);
        }

}
#endif

/* --------- UP (Up Arrow) Key ------------ */
static void UpKey(WINDOW wnd, PARAM p2)
{
    if (wnd->selection > 0)
        {
        if (wnd->selection == wnd->wtop)
            {
            BaseWndProc(LISTBOX, wnd, KEYBOARD, UP, p2);
            PostMessage(wnd, LB_SELECTION, wnd->selection-1, isMultiLine(wnd) ? p2 : FALSE);
            }
        else
            {
            int newsel=wnd->selection-1;

            if (wnd->wlines == ClientHeight(wnd))
                while (*TextLine(wnd, newsel) == LINE)
                    --newsel;

            PostMessage(wnd, LB_SELECTION, newsel,
#ifdef INCLUDE_EXTENDEDSELECTIONS
            isMultiLine(wnd) ? p2 :
#endif
            FALSE);
            }

        }

}

/* --------- DN (Down Arrow) Key ------------ */
static void DnKey(WINDOW wnd, PARAM p2)
{
    if (wnd->selection < wnd->wlines-1)
        {
        if (wnd->selection == wnd->wtop+ClientHeight(wnd)-1)
            {
            BaseWndProc(LISTBOX, wnd, KEYBOARD, DN, p2);
            PostMessage(wnd, LB_SELECTION, wnd->selection+1, isMultiLine(wnd) ? p2 : FALSE);
            }
        else
            {
            int newsel=wnd->selection+1;

            if (wnd->wlines == ClientHeight(wnd))
                while (*TextLine(wnd, newsel) == LINE)
                    newsel++;

            PostMessage(wnd, LB_SELECTION, newsel,
#ifdef INCLUDE_EXTENDEDSELECTIONS
            isMultiLine(wnd) ? p2 :
#endif
            FALSE);
            }

        }

}

/* --------- HOME and PGUP Keys ------------ */
static void HomePgUpKey(WINDOW wnd, PARAM p1, PARAM p2)
{
    BaseWndProc(LISTBOX, wnd, KEYBOARD, p1, p2);
    PostMessage(wnd, LB_SELECTION, wnd->wtop,
#ifdef INCLUDE_EXTENDEDSELECTIONS
    isMultiLine(wnd) ? p2 :
#endif
    FALSE);

}

/* --------- END and PGDN Keys ------------ */
static void EndPgDnKey(WINDOW wnd, PARAM p1, PARAM p2)
{
    int bot;

    BaseWndProc(LISTBOX, wnd, KEYBOARD, p1, p2);
    bot=wnd->wtop+ClientHeight(wnd)-1;
    if (bot > wnd->wlines-1)
        bot=wnd->wlines-1;

    PostMessage(wnd, LB_SELECTION, bot,
#ifdef INCLUDE_EXTENDEDSELECTIONS
    isMultiLine(wnd) ? p2 :
#endif
    FALSE);

}

#ifdef INCLUDE_EXTENDEDSELECTIONS
/* --------- Space Bar Key ------------ */
static void SpacebarKey(WINDOW wnd, PARAM p2)
{
    if (isMultiLine(wnd))
        {
        int sel=SendMessage(wnd, LB_CURRENTSELECTION, 0, 0);

        if (sel != -1)
            {
            if (wnd->AddMode)
                FlipSelection(wnd, sel);

            if (ItemSelected(wnd, sel))
                {
                if (!((int) p2 & (LEFTSHIFT | RIGHTSHIFT)))
                    wnd->AnchorPoint=sel;

                ExtendSelections(wnd, sel, (int) p2);
                }
            else
                wnd->AnchorPoint=-1;

            SendMessage(wnd, PAINT, 0, 0);
            }

        }

}
#endif

/* --------- Enter ('\r') Key ------------ */
static void EnterKey(WINDOW wnd)
{
    if (wnd->selection != -1)
        {
        SendMessage(wnd, LB_SELECTION, wnd->selection, TRUE);
        SendMessage(wnd, LB_CHOOSE, wnd->selection, 0);
        }

}

/* --------- All Other Key Presses ------------ */
static void KeyPress(WINDOW wnd, PARAM p1, PARAM p2)
{
    int sel=wnd->selection+1;

    while (sel < wnd->wlines)
        {
        char *cp=TextLine(wnd, sel);

        if (cp == NULL)
            break;
#ifdef INCLUDE_EXTENDEDSELECTIONS
        if (isMultiLine(wnd))
            cp++;
#endif
        if (tolower(*cp) == (int)p1)
            {
            SendMessage(wnd, LB_SELECTION, sel, isMultiLine(wnd) ? p2 : FALSE);
            if (!SelectionInWindow(wnd, sel))
                {
                wnd->wtop=sel-ClientHeight(wnd)+1;
                SendMessage(wnd, PAINT, 0, 0);
                }

            break;
            }

        sel++;
        }

}

/* --------- KEYBOARD Message ------------ */
static int KeyboardMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    switch ((int) p1)
        {
#ifdef INCLUDE_EXTENDEDSELECTIONS
        case CTRL_F8:
            AddModeKey(wnd);
            return TRUE;
#endif
        case UP:
            TestExtended(wnd, p2);
            UpKey(wnd, p2);
            return TRUE;
        case DN:
            TestExtended(wnd, p2);
            DnKey(wnd, p2);
            return TRUE;
        case PGUP:
        case HOME:
            TestExtended(wnd, p2);
            HomePgUpKey(wnd, p1, p2);
            return TRUE;
        case PGDN:
        case END:
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

/* ------- LEFT_BUTTON Message -------- */
static int LeftButtonMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int my=(int) p2-GetTop(wnd);

    if (my >= wnd->wlines-wnd->wtop)
        my=wnd->wlines-wnd->wtop;

    if (!InsideRect(p1, p2, ClientRect(wnd)))
        return FALSE;

    if (wnd->wlines && my != py)
        {
        int sel=wnd->wtop+my-1;
#ifdef INCLUDE_EXTENDEDSELECTIONS
        int sh=getshift();

        if (!(sh & (LEFTSHIFT | RIGHTSHIFT)))
            {
            if (!(sh & CTRLKEY))
                ClearAllSelections(wnd);

            wnd->AnchorPoint=sel;
            SendMessage(wnd, PAINT, 0, 0);
            }
#endif
        SendMessage(wnd, LB_SELECTION, sel, TRUE);
        py=my;
        }

    return TRUE;

}

/* ------------- DOUBLE_CLICK Message ------------ */
static int DoubleClickMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    if (WindowMoving || WindowSizing)
        return FALSE;

    if (wnd->wlines)
        {
        RECT rc=ClientRect(wnd);

        BaseWndProc(LISTBOX, wnd, DOUBLE_CLICK, p1, p2);
        if (InsideRect(p1, p2, rc))
            SendMessage(wnd, LB_CHOOSE, wnd->selection, 0);

        }

    return TRUE;

}

/* ------------ ADDTEXT Message -------------- */
static int AddTextMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int rtn=BaseWndProc(LISTBOX, wnd, ADDTEXT, p1, p2);

    if (wnd->selection == -1)
        SendMessage(wnd, LB_SETSELECTION, 0, 0);
#ifdef INCLUDE_EXTENDEDSELECTIONS
    if (*(char *)p1 == LISTSELECTOR)
        wnd->SelectCount++;
#endif
    return rtn;

}

/* --------- GETTEXT Message ------------ */
static void GetTextMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    if ((int)p2 != -1)
        {
        char *cp1=(char *)p1, *cp2=TextLine(wnd, (int)p2);

        while (cp2 && *cp2 && *cp2 != '\n')
            *cp1++ = *cp2++;

        *cp1='\0';
        }

}

/* --------- LISTBOX Window Processing Module ------------ */
int ListBoxProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)
        {
        case CREATE_WINDOW:
            BaseWndProc(LISTBOX, wnd, msg, p1, p2);
            wnd->selection=-1;
#ifdef INCLUDE_EXTENDEDSELECTIONS
            wnd->AnchorPoint=-1;
#endif
            return TRUE;
        case KEYBOARD:
            if (WindowMoving || WindowSizing)
                break;

            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;

            break;
        case LEFT_BUTTON:
            if (LeftButtonMsg(wnd, p1, p2) == TRUE)
                return TRUE;

            break;
        case DOUBLE_CLICK:
            if (DoubleClickMsg(wnd, p1, p2))
                return TRUE;

            break;
        case BUTTON_RELEASED:
            if (WindowMoving || WindowSizing || VSliding)
                break;

            py=-1;
            return TRUE;
        case ADDTEXT:
            return AddTextMsg(wnd, p1, p2);
        case LB_GETTEXT:
            GetTextMsg(wnd, p1, p2);
            return TRUE;
        case CLEARTEXT:
            wnd->selection=-1;
#ifdef INCLUDE_EXTENDEDSELECTIONS
            wnd->AnchorPoint=-1;
#endif
            wnd->SelectCount=0;
            break;
        case PAINT:
            BaseWndProc(LISTBOX, wnd, msg, p1, p2);
            WriteSelection(wnd, wnd->selection, TRUE, (RECT *)p1);
            return TRUE;
        case SETFOCUS:
            BaseWndProc(LISTBOX, wnd, msg, p1, p2);
            if ((int)p1)
            	WriteSelection(wnd, wnd->selection, TRUE, NULL);

            return TRUE;
        case SCROLL:
        case HORIZSCROLL:
        case SCROLLPAGE:
        case HORIZPAGE:
        case SCROLLDOC:
            BaseWndProc(LISTBOX, wnd, msg, p1, p2);
            WriteSelection(wnd,wnd->selection,TRUE,NULL);
            return TRUE;
        case LB_CHOOSE:
            SendMessage(GetParent(wnd), LB_CHOOSE, p1, p2);
            return TRUE;
        case LB_SELECTION:
            ChangeSelection(wnd, (int) p1, (int) p2);
            SendMessage(GetParent(wnd), LB_SELECTION, wnd->selection, 0);
            return TRUE;
        case LB_CURRENTSELECTION:
            return wnd->selection;
        case LB_SETSELECTION:
            ChangeSelection(wnd, (int) p1, 0);
            return TRUE;
#ifdef INCLUDE_EXTENDEDSELECTIONS
        case CLOSE_WINDOW:
            if (isMultiLine(wnd) && wnd->AddMode)
                {
                wnd->AddMode=FALSE;
                SendMessage(GetParent(wnd), ADDSTATUS, 0, 0);
                }

            break;
#endif
        default:
            break;

        }

    return BaseWndProc(LISTBOX, wnd, msg, p1, p2);

}

static BOOL SelectionInWindow(WINDOW wnd, int sel)
{
    return (wnd->wlines && sel >= wnd->wtop && sel < wnd->wtop+ClientHeight(wnd));
}

static void near WriteSelection(WINDOW wnd, int sel, int reverse, RECT *rc)
{
    if (isVisible(wnd))
        if (SelectionInWindow(wnd, sel))
            WriteTextLine(wnd, rc, sel, reverse);

}

#ifdef INCLUDE_EXTENDEDSELECTIONS
/* ----- Test for extended selections in the listbox ----- */
static void TestExtended(WINDOW wnd, PARAM p2)
{
    if (isMultiLine(wnd) && !wnd->AddMode && !((int) p2 & (LEFTSHIFT | RIGHTSHIFT)))
        {
        if (wnd->SelectCount > 1)
            {
            ClearAllSelections(wnd);
            SendMessage(wnd, PAINT, 0, 0);
            }

        }

}

/* ----- Clear selections in the listbox ----- */
static void ClearAllSelections(WINDOW wnd)
{
    if (isMultiLine(wnd) && wnd->SelectCount > 0)
        {
        int sel;

        for (sel=0;sel<wnd->wlines;sel++)
            ClearSelection(wnd, sel);

        }

}

/* ----- Invert a selection in the listbox ----- */
static void FlipSelection(WINDOW wnd, int sel)
{
    if (isMultiLine(wnd))
        {
        if (ItemSelected(wnd, sel))
            ClearSelection(wnd, sel);
        else
            SetSelection(wnd, sel);

        }

}

static int ExtendSelections(WINDOW wnd, int sel, int shift)
{    
    if (shift & (LEFTSHIFT | RIGHTSHIFT) && wnd->AnchorPoint != -1)
        {
        int i=sel,j=wnd->AnchorPoint,rtn;

        if (j > i)
            swap(i,j);

        rtn=i-j;
        while (j <= i)
            SetSelection(wnd, j++);

        return rtn;
        }

    return 0;

}

static void SetSelection(WINDOW wnd, int sel)
{
    if (isMultiLine(wnd) && !ItemSelected(wnd, sel))
        {
        char *lp=TextLine(wnd, sel);

        *lp=LISTSELECTOR;
        wnd->SelectCount++;
        }

}

static void ClearSelection(WINDOW wnd, int sel)
{
    if (isMultiLine(wnd) && ItemSelected(wnd, sel))
        {
        char *lp=TextLine(wnd, sel);

        *lp=' ';
        --wnd->SelectCount;
        }

}

BOOL ItemSelected(WINDOW wnd, int sel)
{
    if (sel != -1 && isMultiLine(wnd) && sel < wnd->wlines)
        {
        char *cp=TextLine(wnd, sel);

        return (int)((*cp) & 255) == LISTSELECTOR;
        }

    return FALSE;

}
#endif

static void near ChangeSelection(WINDOW wnd,int sel,int shift)
{
    if (sel != wnd->selection)
        {
#ifdef INCLUDE_EXTENDEDSELECTIONS
        if (sel != -1 && isMultiLine(wnd))
            {
            int sels;

            if (!wnd->AddMode)
                ClearAllSelections(wnd);

            sels=ExtendSelections(wnd, sel, shift);
            if (sels > 1)
                SendMessage(wnd, PAINT, 0, 0);

            if (sels == 0 && !wnd->AddMode)
                {
                ClearSelection(wnd, wnd->selection);
                SetSelection(wnd, sel);
                wnd->AnchorPoint=sel;
                }

            }
#endif
        WriteSelection(wnd, wnd->selection, FALSE, NULL);
        wnd->selection=sel;
        if (sel != -1)
            WriteSelection(wnd, sel, TRUE, NULL);

        }

}
