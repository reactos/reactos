/* $Id: break.c,v 1.1 2003/03/31 22:28:59 hyperion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/debug/debugger.c
 * PURPOSE:         DebugBreakProcess()
 * PROGRAMMER:      KJK::Hyperion <noog@libero.it>
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

/* FUNCTIONS *****************************************************************/

BOOL WINAPI DebugBreakProcess(HANDLE Process)
{
 NTSTATUS nErrCode = DbgUiIssueRemoteBreakin(Process);
 
 if(!NT_SUCCESS(nErrCode))
 {
  SetLastErrorByStatus(nErrCode);
  return FALSE;
 }

 return TRUE;
}

/* EOF */
