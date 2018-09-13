/****************************Module*Header******************************\
* Module Name: message.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include <windows.h>
#include "port1632.h"

//#define NOEXTERN
#include "pbrush.h"
#ifdef BLOCKONMSG
#include "pbserver.h"
#endif

#define BUF1LEN 300
#define BUF2LEN 300

extern TCHAR NotEnoughMem[];
extern TCHAR pgmTitle[];
extern HWND pbrushWnd[];
extern int fInvisible;

#ifdef BLOCKONMSG
int fBlocked = FALSE;
#endif

/* puts up error message for StringId, possibly merging in a string,
** and asks for the appropriate buttons and icons
*/
WORD SimpleMessage(WORD StringId, LPTSTR lpText, WORD style)
{
   static TCHAR buf1[BUF1LEN];
   static TCHAR buf2[BUF2LEN];
   int Result;

   if (fInvisible)
        return MB_OK;

   if (StringId == IDSNotEnufMem) {
       lstrcpy(buf1, NotEnoughMem);
       style = MB_SYSTEMMODAL | MB_ICONHAND | MB_OK;
   } else {
       LoadString(hInst, StringId, buf1, CharSizeOf(buf1));
       style |= MB_TASKMODAL;
   }

   if (lpText)
       wsprintf(buf2, buf1, lpText, lpText);
   else
       lstrcpy(buf2, buf1);
#ifdef BLOCKONMSG
   EnterMsgMode();
#endif
   Result = MessageBox(GetActiveWindow(), buf2, pgmTitle, style);
   return Result;
}
#ifdef BLOCKONMSG
/* fBlocked == FALSE, we are not in msg mode, hence server not blocked.
            == TRUE, we are in msg mode, Server is Blocked
*/
void EnterMsgMode(
    void)
{
    PBSRVR FAR * lpOleServer;
    extern HANDLE hServer;

    if (lpOleServer = (PBSRVR FAR *)GlobalLock(hServer))
    {
        if (OleBlockServer(lpOleServer->lhsrvr) == OLE_OK)
            fBlocked = TRUE;    /* we are in msg mode */
        GlobalUnlock(hServer);
    }
}
#endif

/* puts up a dialog box with only an ok button */
void PbrushOkError(WORD StringId, WORD style)
{
   SimpleMessage(StringId, NULL, (WORD)(MB_OK | style));
}
