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

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "imagehlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/***********************************************************************
 *		FindDebugInfoFile (IMAGEHLP.@)
 */
HANDLE WINAPI FindDebugInfoFile(
  LPSTR FileName, LPSTR SymbolPath, LPSTR DebugFilePath)
{
  FIXME("(%s, %s, %s): stub\n",
    debugstr_a(FileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *		FindExecutableImage (IMAGEHLP.@)
 */
HANDLE WINAPI FindExecutableImage(
  LPSTR FileName, LPSTR SymbolPath, LPSTR ImageFilePath)
{
  FIXME("(%s, %s, %s): stub\n",
    debugstr_a(FileName), debugstr_a(SymbolPath),
    debugstr_a(ImageFilePath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *		MapDebugInformation (IMAGEHLP.@)
 */
PIMAGE_DEBUG_INFORMATION WINAPI MapDebugInformation(
  HANDLE FileHandle, LPSTR FileName,
  LPSTR SymbolPath, DWORD ImageBase)
{
  FIXME("(%p, %s, %s, 0x%08lx): stub\n",
    FileHandle, FileName, SymbolPath, ImageBase
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *		StackWalk (IMAGEHLP.@)
 */
BOOL WINAPI StackWalk(
  DWORD MachineType, HANDLE hProcess, HANDLE hThread,
  LPSTACKFRAME StackFrame, LPVOID ContextRecord,
  PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
  PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
  PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
  PTRANSLATE_ADDRESS_ROUTINE TranslateAddress)
{
  FIXME(
    "(%ld, %p, %p, %p, %p, %p, %p, %p, %p): stub\n",
      MachineType, hProcess, hThread, StackFrame, ContextRecord,
      ReadMemoryRoutine, FunctionTableAccessRoutine,
      GetModuleBaseRoutine, TranslateAddress
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		UnDecorateSymbolName (IMAGEHLP.@)
 */
DWORD WINAPI UnDecorateSymbolName(
  LPCSTR DecoratedName, LPSTR UnDecoratedName,
  DWORD UndecoratedLength, DWORD Flags)
{
  FIXME("(%s, %s, %ld, 0x%08lx): stub\n",
    debugstr_a(DecoratedName), debugstr_a(UnDecoratedName),
    UndecoratedLength, Flags
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		UnmapDebugInformation (IMAGEHLP.@)
 */
BOOL WINAPI UnmapDebugInformation(
  PIMAGE_DEBUG_INFORMATION DebugInfo)
{
  FIXME("(%p): stub\n", DebugInfo);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
