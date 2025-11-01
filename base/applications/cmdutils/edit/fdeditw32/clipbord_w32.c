/*  Edit's Clipboard

*/
#include <windows.h>

extern char *Clipboard;
extern unsigned int ClipboardLength;

void CopyTextToClipboard(char *str)
{
    if (Clipboard != NULL) {
        free(Clipboard);
        Clipboard = NULL;
    }
    if(OpenClipboard(NULL)) {
	    HGLOBAL  clipbuffer;
	    char    *buffer;

	    EmptyClipboard();
	    clipbuffer = GlobalAlloc(GMEM_DDESHARE, strlen(str)+1);
	    buffer = (char *)GlobalLock(clipbuffer);
	    strcpy(buffer, str);
	    GlobalUnlock(clipbuffer);
	    SetClipboardData(CF_TEXT,clipbuffer);
	    CloseClipboard();
    }
} 

void CopyMemToClipboard(void *ptr, unsigned int size)
{
    if (Clipboard != NULL) {
        free(Clipboard);
        Clipboard = NULL;
    }
    if(OpenClipboard(NULL)) {
	    HGLOBAL  clipbuffer;
	    char    *buffer;

	    EmptyClipboard();
	    clipbuffer = GlobalAlloc(GMEM_DDESHARE, size+1);
	    buffer = (char *)GlobalLock(clipbuffer);
	    memcpy(buffer, ptr, size);
        buffer[size] = '\0';
	    GlobalUnlock(clipbuffer);
	    SetClipboardData(CF_TEXT,clipbuffer);
	    CloseClipboard();
    }
} 

char *ReadClipboard(void)
{
    char *buffer = NULL;

    if (Clipboard != NULL)
        return Clipboard;

    if (OpenClipboard(NULL)) {
	    HANDLE  hData = GetClipboardData(CF_TEXT);
        char   *fromClipboard;

        if (hData != NULL) {
            fromClipboard = (char *)GlobalLock(hData);
	        buffer = Clipboard = strdup(fromClipboard);
	        GlobalUnlock( hData );
            ClipboardLength = strlen(buffer);
        }
	    CloseClipboard();
    }
    return buffer;
} 

void ClearClipboard(void)
{
    if(OpenClipboard(NULL)) {
	    EmptyClipboard();
	    CloseClipboard();
    }
    if (Clipboard != NULL) {
        free(Clipboard);
        Clipboard = NULL;
    }
}

