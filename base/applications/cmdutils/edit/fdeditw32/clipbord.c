/*  Edit's Clipboard

*/

#include "dflat.h"

char *Clipboard;
unsigned int ClipboardLength;

#ifndef _FDEDIT_PRIVATE_CLIPBOARD
void CopyTextToClipboard(char *text)
{
    ClipboardLength = strlen(text);
    Clipboard = DFrealloc(Clipboard, ClipboardLength);
    memmove(Clipboard, text, ClipboardLength);
}

void CopyMemToClipboard(void *text, int size)
{
    ClipboardLength = size;
    Clipboard = DFrealloc(Clipboard, ClipboardLength);
    memmove(Clipboard, text, ClipboardLength);
}

char *ReadClipboard(void)
{
    return Clipboard;
}

void ClearClipboard(void)
{
    if (Clipboard != NULL)
        {
        free(Clipboard);
        Clipboard = NULL;
        }

}
#endif

void CopyToClipboard(WINDOW wnd)
{
    if (TextBlockMarked(wnd))
        {
/*      char *bbl=TextLine(wnd,wnd->BlkBegLine)+wnd->BlkBegCol;
        char *bel=TextLine(wnd,wnd->BlkEndLine)+wnd->BlkEndCol;
*/
        char *bbl=TextBlockBegin(wnd);
        char *bel=TextBlockEnd(wnd);

        if (bbl>=bel)
            {
            ErrorMessage("Not copied to clipboard");
            return;
            }

        CopyMemToClipboard(bbl, bel-bbl);
/*
        ClipboardLength=(unsigned) (bel-bbl);
        Clipboard = DFrealloc(Clipboard, ClipboardLength);
        memmove(Clipboard, bbl, ClipboardLength);
*/
        }

}

BOOL PasteText(WINDOW wnd, char *SaveTo, unsigned len)
{
    if (cfg.read_only)
        {
        MessageBox("Edit", "You cannot modify a read only file");
        wnd->TextChanged=FALSE;
        return TRUE;
        }

    if (SaveTo != NULL && len > 0)
        {
        unsigned plen = strlen(wnd->text) + len;

        if (plen <= wnd->MaxTextLength)
            {
            if (plen+1 > wnd->textlen)
                {
                wnd->text = DFrealloc(wnd->text, plen+3);
                wnd->textlen = plen+1;
            }

            memmove(CurrChar+len, CurrChar, strlen(CurrChar)+1);
            memmove(CurrChar, SaveTo, len);
            BuildTextPointers(wnd);
            wnd->TextChanged = TRUE;
            return TRUE;
            }

        }

    return FALSE;

}
