/*
 *  Notepad
 *
 *  Copyright 2000 Mike McCormack <Mike_McCormack@looksmart.com.au>
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *  To be distributed under the Wine License
 *
 *  FIXME,TODO list:
 *  - Use wine Heap instead of malloc/free (done)
 *  - use scroll bars (vertical done)
 *  - cut 'n paste (clipboard)
 *  - save file
 *  - print file
 *  - find dialog
 *  - encapsulate data structures (?) - half done
 *  - free unused memory
 *  - solve Open problems
 *  - smoother scrolling
 *  - separate view code and document code
 *
 * This program is intended as a testbed for winelib as much as
 * a useful application.
 */

#include <stdio.h>
#include "windows.h"

#ifdef LCC
#include "lcc.h"
#endif

#include "main.h"
#include "license.h"
#include "dialog.h"
#include "language.h"

extern BOOL DoCloseFile(void);
extern void DoOpenFile(LPCSTR szFileName);

NOTEPAD_GLOBALS Globals;


/* Using a pointer to pointer data structure to
   achieve a little more efficiency. Hopefully
   it will be worth it, because it complicates the
   code - mjm 26 Jun 2000 */

#define BUFFERCHUNKSIZE 0xe0
typedef struct TAGLine {
    LPSTR lpLine;
    DWORD dwWidth;
    DWORD dwMaxWidth;
} LINE, *LPLINE;

/* FIXME: make this info into a structure */
/* typedef struct tagBUFFER { */
DWORD dwVOffset=0;
DWORD dwLines=0;
DWORD dwMaxLines=0;
LPLINE lpBuffer=NULL;
DWORD dwXpos=0,dwYpos=0;        /* position of caret in char coords */
DWORD dwCaretXpos=0,dwCaretYpos=0; /* position of caret in pixel coords */
TEXTMETRIC tm;                  /* textmetric for current font */
RECT rectClient;        /* client rectangle of the window we're drawing in */
/* } BUFFER, *LPBUFFER */

VOID InitFontInfo(HWND hWnd)
{
    HDC hDC = GetDC(hWnd);

    if(hDC)
    {
        GetTextMetrics(hDC, &tm);
        ReleaseDC(hWnd,hDC);
    }
}

void InitBuffer(void)
{
    lpBuffer = NULL;
    dwLines = 0;
    dwMaxLines = 0;
    dwXpos=0;
    dwYpos=0;
}

/* convert x,y character co-ords into x pixel co-ord */
DWORD CalcStringWidth(HDC hDC, DWORD x, DWORD y)
{
    DWORD len;
    SIZE size;

    size.cx = 0;
    size.cy = 0;

    if(y>dwLines)
        return size.cx;
    if(lpBuffer == NULL) 
        return size.cx;
    if(lpBuffer[y].lpLine == NULL)
        return size.cx;
    len = (x<lpBuffer[y].dwWidth) ? 
           x : lpBuffer[y].dwWidth;
    GetTextExtentPoint(hDC, lpBuffer[y].lpLine, len, &size);

    return size.cx;
}

void CalcCaretPos(HDC hDC, DWORD dwXpos, DWORD dwYpos)
{
    dwCaretXpos = CalcStringWidth(hDC, dwXpos, dwYpos);
    dwCaretYpos = tm.tmHeight*(dwYpos-dwVOffset);
    SetCaretPos(dwCaretXpos,dwCaretYpos);
}

DWORD GetLinesPerPage(HWND hWnd)
{
    return (rectClient.bottom/tm.tmHeight); /* round down */
}

/* render one line of text and blank space */
void RenderLine(HDC hDC, DWORD lineno)
{
    RECT rect;
    HBRUSH hPrev;

    if(!hDC)
        return;

    /* erase the space at the end of a line using a white rectangle */
    rect.top = tm.tmHeight*(lineno-dwVOffset);
    rect.bottom = tm.tmHeight*(lineno-dwVOffset+1);

    if(lpBuffer && (lineno<dwLines))
        rect.left = CalcStringWidth(hDC, lpBuffer[lineno].dwWidth,lineno);
    else
        rect.left = 0;
    rect.right = rectClient.right;

    /* use the white pen so there's not outline */
    hPrev = SelectObject(hDC, GetStockObject(WHITE_PEN));
    Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(hDC, hPrev);
    
    if(lpBuffer && lpBuffer[lineno].lpLine)
    {
        TextOut(hDC, 0, rect.top, lpBuffer[lineno].lpLine, 
                       lpBuffer[lineno].dwWidth);
    }
}

/*
 * Paint the buffer onto the window.
 */
void RenderWindow(HDC hDC) {
    DWORD i;

    if(!hDC)
        return;

    /* FIXME: render only necessary lines */
    for(i = dwVOffset; i < (dwVOffset+GetLinesPerPage(0)); i++)
    {
        RenderLine(hDC,i);
    }
}

/* 
 * Check that correct buffers exist to access line y pos x
 * Only manages memory.
 *
 * Returns TRUE if the line is accessable
 *         FALSE if there is a problem
 */
BOOL ValidateLine(DWORD y,DWORD x)
{
    DWORD max;

    /* check to see that the BUFFER has enough lines */
    max = dwMaxLines;
    if( (max<=y) || (lpBuffer == NULL))
    {
        while(max<=y)
            max += BUFFERCHUNKSIZE;
        /* use GlobalAlloc first time round */
        if(lpBuffer)
            lpBuffer = (LPLINE) GlobalReAlloc((HGLOBAL)lpBuffer,GMEM_FIXED,
                                          max*sizeof(LINE)) ;
        else
            lpBuffer = (LPLINE) GlobalAlloc(GMEM_FIXED, max*sizeof(LINE));
        if(lpBuffer == NULL)
            return FALSE;
        ZeroMemory(&lpBuffer[dwLines], sizeof(LINE)*(max-dwLines) );
        dwMaxLines = max;
    }

    /* check to see that the LINE is wide enough */
    max = lpBuffer[y].dwMaxWidth;
    if( (max <= x) || (lpBuffer[y].lpLine == NULL) )
    {
        while(max <= x)
            max += BUFFERCHUNKSIZE;
        /* use GlobalAlloc first */
        if(lpBuffer[y].lpLine)
            lpBuffer[y].lpLine = (LPSTR)GlobalReAlloc((HGLOBAL)lpBuffer[y].lpLine,
                                                  GMEM_FIXED, max) ;
        else
            lpBuffer[y].lpLine = (LPSTR)GlobalAlloc( GMEM_FIXED, max);
        if(lpBuffer[y].lpLine == NULL)
            return FALSE;
        lpBuffer[y].dwWidth = 0;
        lpBuffer[y].dwMaxWidth = max;
    }
    return TRUE;
}

/* inserts a new line into the buffer */
BOOL DoNewLine(HDC hDC)
{
    DWORD i,cnt;
    LPSTR src,dst;

    /* check to see if we need more memory for the buffer pointers */
    if(!ValidateLine(dwLines,0))
        return FALSE;

    /* shuffle up all the lines */
    for(i=dwLines; i>(dwYpos+1); i--)
    {
        lpBuffer[i] = lpBuffer[i-1];
        RenderLine(hDC,i);
    }
    ZeroMemory(&lpBuffer[dwYpos+1],sizeof(LINE));

    /* copy the characters after the carat (if any) to the next line */
    src = &lpBuffer[dwYpos].lpLine[dwXpos];
    cnt = lpBuffer[dwYpos].dwWidth-dwXpos;
    if(!ValidateLine(dwYpos+1,cnt)) /* allocates the buffer */
        return FALSE; /* FIXME */
    dst = &lpBuffer[dwYpos+1].lpLine[0];
    memcpy(dst, src, cnt);
    lpBuffer[dwYpos+1].dwWidth = cnt;
    lpBuffer[dwYpos].dwWidth  -= cnt;

    /* move the cursor */
    dwLines++;
    dwXpos = 0;
    dwYpos++;

    /* update the window */
    RenderLine(hDC, dwYpos-1);
    RenderLine(hDC, dwYpos);
    CalcCaretPos(hDC, dwXpos, dwYpos);
    /* FIXME: don't use globals */
    SetScrollRange(Globals.hMainWnd, SB_VERT, 0, dwLines, TRUE);

    return TRUE;
}

/*
 * Attempt a basic edit buffer
 */
BOOL AddCharToBuffer(HDC hDC, char ch)
{
    /* we can use lpBuffer[dwYpos] */
    if(!ValidateLine(dwYpos,0))
        return FALSE;

    /* shuffle the rest of the line*/
    if(!ValidateLine(dwYpos, lpBuffer[dwYpos].dwWidth))
        return FALSE;
    lpBuffer[dwYpos].dwWidth++;
    memmove(&lpBuffer[dwYpos].lpLine[dwXpos+1],
            &lpBuffer[dwYpos].lpLine[dwXpos],
            lpBuffer[dwYpos].dwWidth-dwXpos);

    /* add the character */
    lpBuffer[dwYpos].lpLine[dwXpos] = ch;
    if(dwLines == 0)
        dwLines++;
    dwXpos++;

    /* update the window and cursor position */
    RenderLine(hDC,dwYpos);
    CalcCaretPos(hDC,dwXpos,dwYpos);

    return TRUE;
}


/* erase a character */
BOOL DoBackSpace(HDC hDC)
{
    DWORD i;

    if(lpBuffer == NULL)
        return FALSE;
    if(lpBuffer[dwYpos].lpLine && (dwXpos>0))
    {
        dwXpos --;
        /* FIXME: use memmove */
        for(i=dwXpos; i<(lpBuffer[dwYpos].dwWidth-1); i++)
            lpBuffer[dwYpos].lpLine[i]=lpBuffer[dwYpos].lpLine[i+1];

        lpBuffer[dwYpos].dwWidth--;
        RenderLine(hDC, dwYpos);
        CalcCaretPos(hDC,dwXpos,dwYpos);
    }
    else 
    {
        /* Erase a newline. To do this we join two lines */
        LPSTR src, dest;
        DWORD len, oldlen;

        if(dwYpos==0)
            return FALSE;

        oldlen = lpBuffer[dwYpos-1].dwWidth;
        if(lpBuffer[dwYpos-1].lpLine&&lpBuffer[dwYpos].lpLine)
        {
            /* concatonate to the end of the line above line */
            src  = &lpBuffer[dwYpos].lpLine[0];
            dest = &lpBuffer[dwYpos-1].lpLine[lpBuffer[dwYpos-1].dwWidth];
            len  = lpBuffer[dwYpos].dwWidth;

            /* check the length of the new line */
            if(!ValidateLine(dwYpos-1,lpBuffer[dwYpos-1].dwWidth + len))
                return FALSE;

            memcpy(dest,src,len);
            lpBuffer[dwYpos-1].dwWidth+=len;
            GlobalFree( (HGLOBAL)lpBuffer[dwYpos].lpLine);
        }
        else if (!lpBuffer[dwYpos-1].lpLine)
        {
            lpBuffer[dwYpos-1]=lpBuffer[dwYpos];
        } /* else both are NULL */
        RenderLine(hDC,dwYpos-1);

        /* don't zero - it's going to get trashed anyhow */

        /* shuffle up all the lines below this one */
        for(i=dwYpos; i<(dwLines-1); i++)
        {
            lpBuffer[i] = lpBuffer[i+1];
            RenderLine(hDC,i);
        }

        /* clear the last line */
        ZeroMemory(&lpBuffer[dwLines-1],sizeof (LINE));
        RenderLine(hDC,dwLines-1);
        dwLines--;

        /* adjust the cursor position to joining point */
        dwYpos--;
        dwXpos = oldlen;

        CalcCaretPos(hDC,dwXpos,dwYpos);
        SetScrollRange(Globals.hMainWnd, SB_VERT, 0, dwLines, TRUE);
    }
    return TRUE;
}

/* as used by File->New */
void TrashBuffer(void)
{
    DWORD i;

    /* variables belonging to the buffer */
    if(lpBuffer)
    {
        for(i=0; i<dwLines; i++)
        {
            if(lpBuffer[i].lpLine)
                GlobalFree ((HGLOBAL)lpBuffer[i].lpLine);
            ZeroMemory(&lpBuffer[i],sizeof (LINE));
        }
        GlobalFree((HGLOBAL)lpBuffer);
        lpBuffer=NULL;
    }
    dwLines = 0;
    dwMaxLines = 0;

    /* variables belonging to the view */
    dwXpos = 0;
    dwYpos = 0;
    dwVOffset = 0 ;
    /* FIXME: don't use globals */
    SetScrollPos(Globals.hMainWnd, SB_VERT, dwVOffset, FALSE);
    SetScrollRange(Globals.hMainWnd, SB_VERT, 0, dwLines, TRUE);
}


/*
 * Add a line to the buffer
 */
/* FIXME: this breaks lines longer than BUFFERCHUNKSIZE */
DWORD CreateLine(
    LPSTR buffer, /* pointer to buffer with file data */
    DWORD size, /* number of bytes available in buffer */
    BOOL nomore)
{
    DWORD i;
    
    if(size == 0)
        return 0;

    for(i=0; i<size; i++)
    {
        if(buffer[i]==0x0a)
        {
            if(ValidateLine(dwLines,i))
            {
                memcpy(&lpBuffer[dwLines].lpLine[0],&buffer[0],i);
                lpBuffer[dwLines].dwWidth = i;
                dwLines++;
            }
            return i+1;
        }
    }

    /* make a line of the rest */
    if( (i == BUFFERCHUNKSIZE) || nomore )
    {
        if(ValidateLine(dwLines,i))
        {
            memcpy(&lpBuffer[dwLines].lpLine[0],&buffer[0],i);
            lpBuffer[dwLines].dwWidth = i;
            dwLines++;
        }
        return i;
    }

    return 0;
}


/* 
 * This is probably overcomplicated by lpBuffer data structure... 
 * Read blocks from the file, then add each line from the
 *  block to the buffer until there is none left. If all
 *  a slab isn't used, try load some more data from the file.
 */
void LoadBufferFromFile(LPCSTR szFileName)
{
    HANDLE hFile;
    CHAR *pTemp;
    DWORD size,i,len,bytes_left,bytes_read;

    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return;
    size = BUFFERCHUNKSIZE;
    pTemp = (LPSTR) GlobalAlloc(GMEM_FIXED, size);
    if(!pTemp)
        return;
    bytes_read = 1; /* anything non-zero */
    bytes_left = 0;
    while(bytes_read)
    {
        if(!ReadFile(hFile, 
                     &pTemp[bytes_left], 
                     size-bytes_left, 
                     &bytes_read, NULL))
            break;
        bytes_left+=bytes_read;

        /* add strings to buffer */
        for(i = 0; 
            (i<size) &&
            (len  = CreateLine(&pTemp[i], bytes_left, !bytes_read));
            i+= len,bytes_left-=len );

        /* move leftover to front of buffer */
        if(bytes_left)
            memmove(&pTemp[0],&pTemp[i], bytes_left);
    }
    CloseHandle(hFile);
    MessageBox(Globals.hMainWnd, "Finished", "Info", MB_OK);
}

BOOL DoInput(HDC hDC, WPARAM wParam, LPARAM lParam)
{
    switch(wParam)
    {
    case 0x08:
        return DoBackSpace(hDC);
    case 0x0d:
        return DoNewLine(hDC);
    default:
        return AddCharToBuffer(hDC,wParam);
    }
}

BOOL GotoHome(HWND hWnd)
{
    dwXpos = 0;
    dwYpos = 0;
    dwVOffset = 0;
    return TRUE;
}

BOOL GotoEndOfLine(HWND hWnd)
{
    dwXpos = lpBuffer[dwYpos].dwWidth;
    return TRUE;
}

BOOL GotoDown(HWND hWnd)
{
    if((dwYpos+1) >= dwLines)
    {
        return FALSE;
    }
    dwYpos++;
    if (dwXpos>lpBuffer[dwYpos].dwWidth)
        GotoEndOfLine(hWnd);
    return TRUE;
}

BOOL GotoUp(HWND hWnd)
{
    if(dwYpos==0)
        return FALSE;
    dwYpos--;
    if (dwXpos>lpBuffer[dwYpos].dwWidth)
        GotoEndOfLine(hWnd);
    return TRUE;
}

BOOL GotoLeft(HWND hWnd)
{
    if(dwXpos > 0)
    {
        dwXpos--;
        return TRUE;
    }
    if(GotoUp(hWnd))   
        return GotoEndOfLine(hWnd);
    return FALSE;
}

BOOL GotoRight(HWND hWnd)
{
    if(dwXpos<lpBuffer[dwYpos].dwWidth)
    {
        dwXpos++;
        return TRUE;
    }
    if(!GotoDown(hWnd))
        return FALSE;
    dwXpos=0;
    return TRUE;
}

/* check the caret is still on the screen */
BOOL ScrollABit(HWND hWnd)
{
    if(dwYpos<dwVOffset)
    {
        dwVOffset = dwYpos;
        return TRUE;
    }
    if(dwYpos>(dwVOffset+GetLinesPerPage(hWnd)))
    {
        dwVOffset = dwYpos - GetLinesPerPage(hWnd) + 1;
        return TRUE;
    }
    return FALSE;
}

/* FIXME: move the window around so we can still see the caret */
VOID DoEdit(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HDC hDC;

    if(lpBuffer==NULL)
        return;
    switch(wParam)
    {
    case VK_HOME: GotoHome(hWnd);
        break;

    case VK_END: GotoEndOfLine(hWnd);
        break;

    case VK_LEFT: GotoLeft(hWnd);
        break;

    case VK_RIGHT: GotoRight(hWnd);
        break;

    case VK_DOWN: GotoDown(hWnd);
        break;

    case VK_UP: GotoUp(hWnd);
        break;

    default:
        return;
    }

    hDC = GetDC(hWnd);
    if(hDC)
    {
        CalcCaretPos(hDC, dwXpos, dwYpos);
        ReleaseDC(hWnd,hDC);
    }
    if(ScrollABit(hWnd))
        InvalidateRect(hWnd, NULL, FALSE);
}

void ButtonDownToCaretPos(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    DWORD x, y, caretx, carety;
    BOOL refine_guess = TRUE;
    HDC hDC;

    x = LOWORD(lParam);
    y = HIWORD(lParam);

    caretx = x/tm.tmAveCharWidth; /* guess */
    carety = dwVOffset + y/tm.tmHeight;

    hDC = GetDC(hWnd);

    if(lpBuffer == NULL)
    {
        caretx = 0;
        carety = 0;
        refine_guess = FALSE;
    }

    /* if the cursor is past the bottom, put it after the last char */
    if(refine_guess && (carety>=dwLines) )
    {
        carety=dwLines-1;
        caretx=lpBuffer[carety].dwWidth;
        refine_guess = FALSE;
    }

    /* cursor past end of line? */
    if(refine_guess && (x>CalcStringWidth(hDC,lpBuffer[carety].dwWidth,carety)))
    {
        caretx = lpBuffer[carety].dwWidth;
        refine_guess = FALSE;
    }

    /* FIXME: doesn't round properly */
    if(refine_guess)
    {
        if(CalcStringWidth(hDC,caretx,carety)<x)
        {
            while( (caretx<lpBuffer[carety].dwWidth) &&
                   (CalcStringWidth(hDC,caretx+1,carety)<x))
                caretx++;
        }
        else
        {
            while((caretx>0)&&(CalcStringWidth(hDC,caretx-1,carety)>x))
                caretx--;
        }
    }
    
    /* set the caret's position */
    dwXpos = caretx;
    dwYpos = carety;
    CalcCaretPos(hDC, caretx, carety);
    ReleaseDC(hWnd,hDC);
}

void DoScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    DWORD dy = GetLinesPerPage(hWnd);

    switch(wParam) /* vscroll code */
    {
    case SB_LINEUP:
        if(dwVOffset)
            dwVOffset--;
        break;
    case SB_LINEDOWN:
        if(dwVOffset<dwLines)
            dwVOffset++;
        break;
    case SB_PAGEUP:
        if( (dy+dwVOffset) > dwLines)
            dwVOffset = dwLines - 1;
        break;
    case SB_PAGEDOWN:
        if( dy > dwVOffset)
            dwVOffset=0;
        break;
    }
    /* position scroll */
    SetScrollPos(hWnd, SB_VERT, dwVOffset, TRUE);
}

/***********************************************************************
 *
 *           NOTEPAD_MenuCommand
 *
 *  All handling of main menu events
 */

int NOTEPAD_MenuCommand (WPARAM wParam)
{  
   switch (wParam) {
     case NP_FILE_NEW:          DIALOG_FileNew(); break;
     case NP_FILE_OPEN:         DIALOG_FileOpen(); break;
     case NP_FILE_SAVE:         DIALOG_FileSave(); break;
     case NP_FILE_SAVEAS:       DIALOG_FileSaveAs(); break;
     case NP_FILE_PRINT:        DIALOG_FilePrint(); break;
     case NP_FILE_PAGESETUP:    DIALOG_FilePageSetup(); break;
     case NP_FILE_PRINTSETUP:   DIALOG_FilePrinterSetup();break;
     case NP_FILE_EXIT:         DIALOG_FileExit(); break;

     case NP_EDIT_UNDO:         DIALOG_EditUndo(); break;
     case NP_EDIT_CUT:          DIALOG_EditCut(); break;
     case NP_EDIT_COPY:         DIALOG_EditCopy(); break;
     case NP_EDIT_PASTE:        DIALOG_EditPaste(); break;
     case NP_EDIT_DELETE:       DIALOG_EditDelete(); break;
     case NP_EDIT_SELECTALL:    DIALOG_EditSelectAll(); break;
     case NP_EDIT_TIMEDATE:     DIALOG_EditTimeDate();break;
     case NP_EDIT_WRAP:         DIALOG_EditWrap(); break;

     case NP_SEARCH_SEARCH:     DIALOG_Search(); break;
     case NP_SEARCH_NEXT:       DIALOG_SearchNext(); break;

     case NP_HELP_CONTENTS:     DIALOG_HelpContents(); break;
     case NP_HELP_SEARCH:       DIALOG_HelpSearch(); break;
     case NP_HELP_ON_HELP:      DIALOG_HelpHelp(); break;
     case NP_HELP_LICENSE:      DIALOG_HelpLicense(); break;
     case NP_HELP_NO_WARRANTY:  DIALOG_HelpNoWarranty(); break;
     case NP_HELP_ABOUT_WINE:   DIALOG_HelpAboutWine(); break;
     
     /* Handle languages */
     default:
      LANGUAGE_DefaultHandle(wParam);
   }
   return 0;
}



/***********************************************************************
 *
 *           NOTEPAD_WndProc
 */

LRESULT WINAPI NOTEPAD_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hContext;
    HANDLE hDrop;                      /* drag & drop */
    CHAR szFileName[MAX_STRING_LEN];
    RECT Windowsize;

    lstrcpy(szFileName, "");

    switch (msg) {

       case WM_CREATE:
          GetClientRect(hWnd, &rectClient);
          InitFontInfo(hWnd);
          break;

       case WM_SETFOCUS:
          CreateCaret(Globals.hMainWnd, 0, 1, tm.tmHeight);
          SetCaretPos(dwCaretXpos, dwCaretYpos);
          ShowCaret(Globals.hMainWnd);
          break;

       case WM_KILLFOCUS:
          DestroyCaret();
          break;

       case WM_PAINT:
          GetClientRect(hWnd, &rectClient);
          hContext = BeginPaint(hWnd, &ps);
          RenderWindow(hContext);
          EndPaint(hWnd, &ps);
        break;

       case WM_KEYDOWN:
          DoEdit(hWnd, wParam, lParam);
          break;

       case WM_CHAR:
          GetClientRect(hWnd, &rectClient);
          HideCaret(hWnd);
          hContext = GetDC(hWnd);
          DoInput(hContext,wParam,lParam);
          ReleaseDC(hWnd,hContext);
          ShowCaret(hWnd);
          break;

       case WM_LBUTTONDOWN:
          /* figure out where the mouse was clicked */
          ButtonDownToCaretPos(hWnd, wParam, lParam);
          break;

       case WM_VSCROLL:
          DoScroll(hWnd, wParam, lParam);
	  InvalidateRect(hWnd, NULL, FALSE); /* force a redraw */
          break;

       case WM_COMMAND:
          /* FIXME: this is a bit messy */
          NOTEPAD_MenuCommand(wParam);
          InvalidateRect(hWnd, NULL, FALSE); /* force a redraw */
          hContext = GetDC(hWnd);
          CalcCaretPos(hContext,dwXpos,dwYpos);
          ReleaseDC(hWnd,hContext);
          break;

       case WM_DESTROYCLIPBOARD:
          MessageBox(Globals.hMainWnd, "Empty clipboard", "Debug", MB_ICONEXCLAMATION);
          break;

       case WM_CLOSE:
          if (DoCloseFile()) {
             PostQuitMessage(0);
          }
          break;

       case WM_DESTROY:
             PostQuitMessage (0);
          break;

       case WM_SIZE:
          GetClientRect(Globals.hMainWnd, &Windowsize);
          break;

       case WM_DROPFILES:
          /* User has dropped a file into main window */
          hDrop = (HANDLE) wParam;
          DragQueryFile(hDrop, 0, (CHAR *) &szFileName, sizeof(szFileName));
          DragFinish(hDrop);
          DoOpenFile(szFileName);
          break;        

       default:
          return DefWindowProc (hWnd, msg, wParam, lParam);
    }
    return 0l;
}



/***********************************************************************
 *
 *           WinMain
 */

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    MSG      msg;
    WNDCLASS class;
    char className[] = "NPClass";  /* To make sure className >= 0x10000 */
    char winName[]   = "Notepad";

    /* setup buffer */
    InitBuffer();

    /* Setup Globals */

    Globals.lpszIniFile = "notepad.ini";
    Globals.lpszIcoFile = "notepad.ico";

    Globals.hInstance       = hInstance;

#ifndef LCC
    Globals.hMainIcon       = ExtractIcon(Globals.hInstance, 
                                        Globals.lpszIcoFile, 0);
#endif
    /* icon breakage in ros
    if (!Globals.hMainIcon) {
        Globals.hMainIcon = LoadIcon(0, MAKEINTRESOURCE(DEFAULTICON));
    }
    */
    
    lstrcpy(Globals.szFindText,     "");
    lstrcpy(Globals.szFileName,     "");
    lstrcpy(Globals.szMarginTop,    "25 mm");
    lstrcpy(Globals.szMarginBottom, "25 mm");
    lstrcpy(Globals.szMarginLeft,   "20 mm");
    lstrcpy(Globals.szMarginRight,  "20 mm");
    lstrcpy(Globals.szHeader,       "&n");
    lstrcpy(Globals.szFooter,       "Page &s");
    lstrcpy(Globals.Buffer, "Hello World");

    if (!prev){
        class.style         = CS_HREDRAW | CS_VREDRAW;
        class.lpfnWndProc   = NOTEPAD_WndProc;
        class.cbClsExtra    = 0;
        class.cbWndExtra    = 0;
        class.hInstance     = Globals.hInstance;
        class.hIcon         = LoadIcon (0, IDI_APPLICATION);
        class.hCursor       = LoadCursor (0, IDC_ARROW);
        class.hbrBackground = GetStockObject (WHITE_BRUSH);
        class.lpszMenuName  = 0;
        class.lpszClassName = className;
    }

    if (!RegisterClass (&class)) return FALSE;

    /* Setup windows */


    Globals.hMainWnd = CreateWindow (className, winName, 
       WS_OVERLAPPEDWINDOW + WS_HSCROLL + WS_VSCROLL,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 
       LoadMenu(Globals.hInstance, STRING_MENU_Xx),
       Globals.hInstance, 0);

    Globals.hFindReplaceDlg = 0;

    LANGUAGE_SelectByNumber(0);

    SetMenu(Globals.hMainWnd, Globals.hMainMenu);               
                        
    ShowWindow (Globals.hMainWnd, show);
    UpdateWindow (Globals.hMainWnd);

    /* Set up dialogs */

    /* Identify Messages originating from FindReplace */

    Globals.nCommdlgFindReplaceMsg = RegisterWindowMessage("commdlg_FindReplace");
    if (Globals.nCommdlgFindReplaceMsg==0) {
       MessageBox(Globals.hMainWnd, "Could not register commdlg_FindReplace window message", 
                  "Error", MB_ICONEXCLAMATION);
    }

    /* now handle command line */
    
    while (*cmdline && (*cmdline == ' ' || *cmdline == '-')) 
    
    {
        CHAR   option;
/*      LPCSTR topic_id; */

        if (*cmdline++ == ' ') continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline && *cmdline == ' ') cmdline++;

        switch(option)
        {
            case 'p':
            case 'P': printf("Print file: ");
                      /* Not yet able to print a file */
                      break;
        }
    }

    /* Set up Drag&Drop */

    DragAcceptFiles(Globals.hMainWnd, TRUE);

    /* now enter mesage loop     */
    
    while (GetMessage (&msg, 0, 0, 0)) {
    
        /* Message belongs to FindReplace dialog */
        /* We just let IsDialogMessage handle it */
        /* BTW - DnD is broken under ROS */
        
      /*  if (IsDialogMessage(Globals.hFindReplaceDlg, &msg)!=0) {
        } 
          else */               
        { 
          /* Message belongs to the Notepad Main Window */
          TranslateMessage (&msg);
          DispatchMessage (&msg);
        }
    }
    return 0;
}
