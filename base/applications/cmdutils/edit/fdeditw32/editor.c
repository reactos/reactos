/* ------------- editor.c ------------ */
#include "dflat.h"

#define pTab ('\t' + 0x80)
#define sTab ('\f' + 0x80)

/* ---------- SETTEXT Message ------------ */
static int SetTextMsg(WINDOW wnd, char *Buf)
{
    unsigned char *tp,*ep,*ttp;
    int x=0,sz=0,rtn;

    tp=Buf;

    /* Compute the buffer size based on tabs in the text */
    while (*tp)
        {
        if (*tp == '\t')
            {
            /* Tab, adjust the buffer length */
            int sps=cfg.Tabs-(x % cfg.Tabs);

            sz+=sps;
            x+=sps;
            }
        else
            {
            /* Not a tab, count the character */
            sz++;
            x++;
            }

        if (*tp == '\n')
            x=0;                        /* Newline, reset x */

        tp++;
        }

    ep=DFcalloc(1, sz+1);               /* Allocate a buffer */

    /* Detab the input file */
    tp=Buf;
    ttp=ep;
    x=0;
    while (*tp)
        {
        x++;                            /* Put the character (\t, too) into the buffer */
        /* Expand tab into subst tab (\f + 0x80) and expansions (\t + 0x80) */
        if (*tp == '\t')
            {
            *ttp++ = sTab;              /* Substitute tab character */
            while ((x % cfg.Tabs) != 0)
                *ttp++ = pTab, x++;

            }
        else
            {
            *ttp++ = *tp;
            if (*tp == '\n')
                x=0;
            }

        tp++;
        }

    *ttp='\0';
    rtn=BaseWndProc(EDITOR, wnd, SETTEXT, (PARAM) ep, 0);
    free(ep);
    return rtn;

}
void CollapseTabs(WINDOW wnd)
{
    unsigned char *cp = wnd->text, *cp2;

    while (*cp)
        {
        if (*cp == pTab)
            {
            *cp='\t';
            cp2=cp;
            while (*++cp2 == sTab);
                memmove(cp+1, cp2, strlen(cp2)+1);

            }

    	cp++;
        }

}

void ExpandTabs(WINDOW wnd)
{
    int Holdwtop = wnd->wtop, Holdwleft = wnd->wleft, HoldRow = wnd->CurrLine, HoldCol = wnd->CurrCol, HoldwRow = wnd->WndRow;

    SendMessage(wnd, SETTEXT, (PARAM) wnd->text, 0);
    wnd->wtop=Holdwtop;
    wnd->wleft=Holdwleft;
    wnd->CurrLine=HoldRow;
    wnd->CurrCol=HoldCol;
    wnd->WndRow=HoldwRow;
    SendMessage(wnd, PAINT, 0, 0);
    SendMessage(wnd, KEYBOARD_CURSOR, 0, wnd->WndRow);

}

/* --- When inserting or deleting, adjust next following tab, same line --- */
static void AdjustTab(WINDOW wnd)
{
    int col=wnd->CurrCol;

    while (*CurrChar && *CurrChar != '\n')
        {
        if (*CurrChar == sTab)
            {
            int exp=(cfg.Tabs-1)-(wnd->CurrCol % cfg.Tabs);

            wnd->CurrCol++;
            while (*CurrChar == pTab)
                BaseWndProc(EDITOR, wnd, KEYBOARD, DEL, 0);

            while (exp--)
                BaseWndProc(EDITOR, wnd, KEYBOARD, pTab, 0);

            break;
            }

        wnd->CurrCol++;
        }

    wnd->CurrCol=col;

}

static void TurnOffDisplay(WINDOW wnd)
{
    SendMessage(NULL, HIDE_CURSOR, 0, 0);
    ClearVisible(wnd);

}

static void TurnOnDisplay(WINDOW wnd)
{
    SetVisible(wnd);
    SendMessage(NULL, SHOW_CURSOR, 0, 0);

}

static void RepaintLine(WINDOW wnd)
{
    SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    WriteTextLine(wnd, NULL, wnd->CurrLine, FALSE);
}

/* --------- KEYBOARD Message ---------- */
static int KeyboardMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int c=(int) p1;
    BOOL delnl;
    PARAM pn=p1;

    if (WindowMoving || WindowSizing || ((int)p2 & ALTKEY))
        return FALSE;

    switch (c)
        {
        case PGUP:
        case PGDN:
        case UP:
        case DN:
            pn=(PARAM) BS;
        case FWD:
        case LARROW:
            TurnOffDisplay(wnd);
            BaseWndProc(EDITOR, wnd, KEYBOARD, p1, p2);
            while (*CurrChar == pTab)
                BaseWndProc(EDITOR, wnd, KEYBOARD, pn, p2);

            TurnOnDisplay(wnd);
            return TRUE;
        case DEL:
            TurnOffDisplay(wnd);
            delnl = *CurrChar == '\n' || TextBlockMarked(wnd);
            BaseWndProc(EDITOR, wnd, KEYBOARD, p1, p2);
            while (*CurrChar == pTab)
                BaseWndProc(EDITOR, wnd, KEYBOARD, p1, p2);

            AdjustTab(wnd);
            TurnOnDisplay(wnd);
            RepaintLine(wnd);
            if (delnl)
                SendMessage(wnd, PAINT, 0, 0);

            return TRUE;
        case '\t':
            TurnOffDisplay(wnd);
            BaseWndProc(EDITOR, wnd, KEYBOARD, (PARAM) sTab, p2);
            while ((wnd->CurrCol % cfg.Tabs) != 0)
                BaseWndProc(EDITOR, wnd, KEYBOARD, pTab, p2);

            TurnOnDisplay(wnd);
            RepaintLine(wnd);
            return TRUE;
        default:
            if (((c & FKEY) == 0) && (isprint(c) || c == '\r'))
                {
                TurnOffDisplay(wnd);
                BaseWndProc(EDITOR, wnd, KEYBOARD, p1, p2);
                AdjustTab(wnd);
                TurnOnDisplay(wnd);
                RepaintLine(wnd);
                if (c == '\r')
                    SendMessage(wnd, PAINT, 0, 0);

                return TRUE;
                }

            break;

        }

    return FALSE;

}

/* ------- Window processing module for EDITBOX class ------ */
int EditorProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)
        {
        case KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;

            break;
        case SETTEXT:
            return SetTextMsg(wnd, (char *) p1);
        default:
            break;

        }

    return BaseWndProc(EDITOR, wnd, msg, p1, p2);

}
