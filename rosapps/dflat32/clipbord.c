/* ----------- clipbord.c ------------ */
#include "dflat.h"

char *Clipboard;
unsigned ClipboardLength;

void CopyTextToClipboard(char *text)
{
    ClipboardLength = strlen(text);
    Clipboard = DFrealloc(Clipboard, ClipboardLength);
    memmove(Clipboard, text, ClipboardLength);
}

void CopyToClipboard(DFWINDOW wnd)
{
    if (TextBlockMarked(wnd))    {
        char *bbl=TextLine(wnd,wnd->BlkBegLine)+wnd->BlkBegCol;
        char *bel=TextLine(wnd,wnd->BlkEndLine)+wnd->BlkEndCol;
        ClipboardLength = (int) (bel - bbl);
        Clipboard = DFrealloc(Clipboard, ClipboardLength);
        memmove(Clipboard, bbl, ClipboardLength);
    }
}

void ClearClipboard(void)
{
    if (Clipboard != NULL)  {
        free(Clipboard);
        Clipboard = NULL;
    }
}


BOOL PasteText(DFWINDOW wnd, char *SaveTo, unsigned len)
{
    if (SaveTo != NULL && len > 0)    {
        unsigned plen = strlen(wnd->text) + len;

		if (plen <= wnd->MaxTextLength)	{
        	if (plen+1 > wnd->textlen)    {
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

/* EOF */
