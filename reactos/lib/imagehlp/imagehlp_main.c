/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "imagehlp.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/**********************************************************************/

HANDLE IMAGEHLP_hHeap = NULL;

static API_VERSION IMAGEHLP_ApiVersion = { 4, 0, 0, 5 };

/***********************************************************************
 *           DllMain (IMAGEHLP.init)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hinstDLL);
      IMAGEHLP_hHeap = HeapCreate(0, 0x10000, 0);
      break;
    case DLL_PROCESS_DETACH:
      HeapDestroy(IMAGEHLP_hHeap);
      IMAGEHLP_hHeap = NULL;
      break;
    default:
      break;
    }
  return TRUE;
}

/***********************************************************************
 *           ImagehlpApiVersion (IMAGEHLP.@)
 */
LPAPI_VERSION WINAPI ImagehlpApiVersion(VOID)
{
  return &IMAGEHLP_ApiVersion;
}

/***********************************************************************
 *           ImagehlpApiVersionEx (IMAGEHLP.@)
 */
LPAPI_VERSION WINAPI ImagehlpApiVersionEx(LPAPI_VERSION AppVersion)
{
  if(!AppVersion)
    return NULL;

  AppVersion->MajorVersion = IMAGEHLP_ApiVersion.MajorVersion;
  AppVersion->MinorVersion = IMAGEHLP_ApiVersion.MinorVersion;
  AppVersion->Revision = IMAGEHLP_ApiVersion.Revision;
  AppVersion->Reserved = IMAGEHLP_ApiVersion.Reserved;

  return AppVersion;
}

/***********************************************************************
 *           MakeSureDirectoryPathExists (IMAGEHLP.@)
 *
 * Path may contain a file at the end. If a dir is at the end, the path 
 * must end with a backslash.
 *
 * Path may be absolute or relative (to current dir).
 *
 */
BOOL WINAPI MakeSureDirectoryPathExists(LPCSTR DirPath)
{
   char Path[MAX_PATH];
   char *SlashPos = Path;
   char Slash;
   BOOL bRes;
   
   strcpy(Path, DirPath);
        
   while((SlashPos=strpbrk(SlashPos+1,"\\/")))
   {
      Slash = *SlashPos;
      *SlashPos = 0;
      
      bRes = CreateDirectoryA(Path, NULL);
      if (bRes == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
      {
         return FALSE;
      }
      
      *SlashPos = Slash;
   }

   return TRUE;
}


/***********************************************************************
 *           MarkImageAsRunFromSwap (IMAGEHLP.@)
 * FIXME
 *   No documentation available.
 */

/***********************************************************************
 *           SearchTreeForFile (IMAGEHLP.@)
 */
BOOL WINAPI SearchTreeForFile(
  LPSTR RootPath, LPSTR InputPathName, LPSTR OutputPathBuffer)
{
  FIXME("(%s, %s, %s): stub\n",
    debugstr_a(RootPath), debugstr_a(InputPathName),
    debugstr_a(OutputPathBuffer)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           TouchFileTimes (IMAGEHLP.@)
 */
BOOL WINAPI TouchFileTimes(
  HANDLE FileHandle, LPSYSTEMTIME lpSystemTime)
{
  FIXME("(%p, %p): stub\n",
    FileHandle, lpSystemTime
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
