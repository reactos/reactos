/****************************Module*Header***********************************\
* Module Name: WASSERT
*
* Module Descripton: Quick Win32 assert code.
*
* Warnings:
*
* Created: 15 July 1993
*
* Author: Raymond E. Endres   [rayen@microsoft.com]
\****************************************************************************/

#include <windows.h>
#include "wassert.h"

#ifdef _DEBUG

void vAssert(TCHAR * pszExp, TCHAR * pszFile, int iLine)
{
   TCHAR  szTmp[1024];
   int iReply;

   wsprintf(szTmp, TEXT("Assertion (%s) at line %d, file %s failed.\nPress Abort to quit the program, Retry to debug the program, or Ignore to continue."),
            pszExp, iLine, pszFile);
   iReply = MessageBox(NULL, szTmp, TEXT("Assertion failed:"), MB_ABORTRETRYIGNORE | MB_TASKMODAL );
   switch ( iReply )
   {
   case IDABORT:
       PostQuitMessage( -1 );
       break;
 
   case IDRETRY:
       DebugBreak();
       break;

   case IDIGNORE:
   default:
       // do nothing, program will try to continue;
       break;
   }
}

#endif
