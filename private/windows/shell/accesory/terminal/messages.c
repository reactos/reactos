/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"


/*---------------------------------------------------------------------------*/
/* testBox() - Display parametered messaged box from any using any type. [mg]*/
/*---------------------------------------------------------------------------*/

INT testBox(HWND  window, INT   choice, INT   captionID, BYTE  *str, 
            INT   arg0, INT arg1, INT arg2, INT arg3, INT arg4, INT arg5, 
            INT arg6, INT arg7, INT arg8, INT arg9, INT arga, INT argb, 
            INT argc, INT argd, INT arge, INT argf)
{
   INT   result;
   BYTE  caption[TMPNSTR+1];
   BYTE  workstr[STR255];

   if(window == NULL)
        window = GetActiveWindow(); /* jtf 3.14 */

   LoadString(hInst, captionID, (LPSTR) caption, TMPNSTR);

   sprintf(workstr, str, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,
                         arg8, arg9, arga, argb, argc, argd, arge, argf);

   if(choice < 0)
   {
      choice = -choice;
      MessageBeep(0);
   }

   result = MessageBox(window, (LPSTR) workstr, (LPSTR) caption, choice);

   return(result);
}


/*---------------------------------------------------------------------------*/
/* testMsg() - Display parametered messaged box from main window.      [mg]  */
/*---------------------------------------------------------------------------*/

INT testMsg(BYTE *str0, BYTE *str1, BYTE *str2)
{
   return(testBox(NULL, -MB_ICONHAND, STR_ERRCAPTION, str0,
                  str1, str2,    0,    0,    0,    0,    0,    0,
                     0,    0,    0,    0,    0,    0,    0,    0));
}


/*---------------------------------------------------------------------------*/
/* testResMsg() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/
INT testResMsg(WORD wResID)
{
   BYTE  work[80];

   LoadString(hInst, wResID, (LPSTR) work, 80);
   return(testMsg(work, NULL, NULL));
}

/*---------------------------------------------------------------------------*/
/* testMsgAux() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

VOID testMsgAux(BYTE  *str, INT   arg0, INT arg1, INT arg2, INT arg3, INT arg4, 
                INT arg5, INT arg6, INT arg7, INT arg8, INT arg9, INT arga, 
                INT argb, INT argc, INT argd, INT arge, INT argf)
{
   INT   hFile, len, ndx;
   BYTE  work[STR255];

   if((hFile = _open("aux", 2)) != -1)
   {
      len = sprintf(work, str, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,
                               arg8, arg9, arga, argb, argc, argd, arge, argf);

      _write(hFile, work, len);
      _write(hFile, "\r\n", 2);

      _close(hFile);
   }
}
