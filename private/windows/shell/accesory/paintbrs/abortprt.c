/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*							*
*	file:	AbortPrt.c				*
*	system: PC Paintbrush for MS-Windows		*
*	descr:	print abort proc			*
*	date:	03/17/87 @ 10:10			*
*							*
********************************************************/

#include "onlypbr.h"
#undef NOMSG
#undef NOCTLMGR

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"

extern BOOL bUserAbort;
extern HWND hDlgPrint;


BOOL FAR PASCAL AbortPrt(HDC printDC, short code)
{
   MSG msg;

   while(!bUserAbort && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      if(!hDlgPrint || !IsDialogMessage(hDlgPrint, &msg)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   return(!bUserAbort);
}
