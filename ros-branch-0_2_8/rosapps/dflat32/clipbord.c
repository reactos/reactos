/* ----------- clipbord.c ------------ */
#include "dflat.h"

char *DfClipboard;
unsigned DfClipboardLength;

void DfCopyTextToClipboard(char *text)
{
    DfClipboardLength = strlen(text);
    DfClipboard = DfRealloc(DfClipboard, DfClipboardLength);
    memmove(DfClipboard, text, DfClipboardLength);
}

void DfCopyToClipboard(DFWINDOW wnd)
{
    if (DfTextBlockMarked(wnd))    {
        char *bbl=DfTextLine(wnd,wnd->BlkBegLine)+wnd->BlkBegCol;
        char *bel=DfTextLine(wnd,wnd->BlkEndLine)+wnd->BlkEndCol;
        DfClipboardLength = (int) (bel - bbl);
        DfClipboard = DfRealloc(DfClipboard, DfClipboardLength);
        memmove(DfClipboard, bbl, DfClipboardLength);
    }
}

void DfClearClipboard(void)
{
    if (DfClipboard != NULL)  {
        free(DfClipboard);
        DfClipboard = NULL;
    }
}


BOOL DfPasteText(DFWINDOW wnd, char *SaveTo, unsigned len)
{
    if (SaveTo != NULL && len > 0)    {
        unsigned plen = strlen(wnd->text) + len;

		if (plen <= wnd->MaxTextLength)	{
        	if (plen+1 > wnd->textlen)    {
            	wnd->text = DfRealloc(wnd->text, plen+3);
            	wnd->textlen = plen+1;
        	}
          	memmove(DfCurrChar+len, DfCurrChar, strlen(DfCurrChar)+1);
           	memmove(DfCurrChar, SaveTo, len);
           	DfBuildTextPointers(wnd);
           	wnd->TextChanged = TRUE;
			return TRUE;
		}
    }
	return FALSE;
}

/* EOF */
