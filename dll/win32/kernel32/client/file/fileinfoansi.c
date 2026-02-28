/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/fileinfo.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

/* FUNCTIONS ****************************************************************/

#if (NTDDI_VERSION < NTDDI_WINBLUE)
/* moved to kernelbase in Windows 8.1 */
/*
 * @implemented
 */
DWORD WINAPI
GetCompressedFileSizeA(LPCSTR lpFileName,
		       LPDWORD lpFileSizeHigh)
{
   PWCHAR FileNameW;

   if (!(FileNameW = FilenameA2W(lpFileName, FALSE)))
      return INVALID_FILE_SIZE;

   return GetCompressedFileSizeW(FileNameW, lpFileSizeHigh);
}
#endif

/* EOF */
