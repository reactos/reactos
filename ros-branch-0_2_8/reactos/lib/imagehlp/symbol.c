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
#include "winerror.h"
#include "wine/debug.h"
#include "imagehlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/***********************************************************************
 *		SymCleanup (IMAGEHLP.@)
 */
BOOL WINAPI SymCleanup(HANDLE hProcess)
{
  FIXME("(%p): stub\n", hProcess);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymEnumerateModules (IMAGEHLP.@)
 */

BOOL WINAPI SymEnumerateModules(
  HANDLE hProcess, PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,
  PVOID UserContext)
{
  FIXME("(%p, %p, %p): stub\n",
    hProcess, EnumModulesCallback, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymEnumerateSymbols (IMAGEHLP.@)
 */
BOOL WINAPI SymEnumerateSymbols(
  HANDLE hProcess, DWORD BaseOfDll,
  PSYM_ENUMSYMBOLS_CALLBACK EnumSymbolsCallback, PVOID UserContext)
{
  FIXME("(%p, 0x%08lx, %p, %p): stub\n",
    hProcess, BaseOfDll, EnumSymbolsCallback, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymFunctionTableAccess (IMAGEHLP.@)
 */
PVOID WINAPI SymFunctionTableAccess(HANDLE hProcess, DWORD AddrBase)
{
  FIXME("(%p, 0x%08lx): stub\n", hProcess, AddrBase);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymGetModuleBase (IMAGEHLP.@)
 */
DWORD WINAPI SymGetModuleBase(HANDLE hProcess, DWORD dwAddr)
{
  FIXME("(%p, 0x%08lx): stub\n", hProcess, dwAddr);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		SymGetModuleInfo (IMAGEHLP.@)
 */
BOOL WINAPI SymGetModuleInfo(
  HANDLE hProcess, DWORD dwAddr,
  PIMAGEHLP_MODULE ModuleInfo)
{
  MEMORY_BASIC_INFORMATION mbi;

  FIXME("(%p, 0x%08lx, %p): hacked stub\n",
    hProcess, dwAddr, ModuleInfo
  );

  /*
   * OpenOffice uses this function to get paths of it's modules
   * from address inside the module. So return at least that for
   * now.
   */
  if (VirtualQuery((PVOID)dwAddr, &mbi, sizeof(mbi)) != sizeof(mbi) ||
      !GetModuleFileNameA((HMODULE)mbi.AllocationBase, ModuleInfo->ImageName, sizeof(ModuleInfo->ImageName)))
  {
    return FALSE;
  }
  return TRUE;
}

/***********************************************************************
 *		SymGetOptions (IMAGEHLP.@)
 */
DWORD WINAPI SymGetOptions()
{
  FIXME("(): stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		SymGetSearchPath (IMAGEHLP.@)
 */
BOOL WINAPI SymGetSearchPath(
  HANDLE hProcess, LPSTR szSearchPath, DWORD SearchPathLength)
{
  FIXME("(%p, %s, %ld): stub\n",
    hProcess, debugstr_an(szSearchPath,SearchPathLength), SearchPathLength
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymGetSymFromAddr (IMAGEHLP.@)
 */
BOOL WINAPI SymGetSymFromAddr(
  HANDLE hProcess, DWORD dwAddr,
  PDWORD pdwDisplacement, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(%p, 0x%08lx, %p, %p): stub\n",
    hProcess, dwAddr, pdwDisplacement, Symbol
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymGetSymFromName (IMAGEHLP.@)
 */
BOOL WINAPI SymGetSymFromName(
  HANDLE hProcess, LPSTR Name, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(%p, %s, %p): stub\n", hProcess, Name, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymGetSymNext (IMAGEHLP.@)
 */
BOOL WINAPI SymGetSymNext(
  HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(%p, %p): stub\n", hProcess, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymGetSymPrev (IMAGEHLP.@)
 */

BOOL WINAPI SymGetSymPrev(
  HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(%p, %p): stub\n", hProcess, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymInitialize (IMAGEHLP.@)
 */
BOOL WINAPI SymInitialize(
  HANDLE hProcess, LPSTR UserSearchPath, BOOL fInvadeProcess)
{
  FIXME("(%p, %s, %d): stub\n",
    hProcess, debugstr_a(UserSearchPath), fInvadeProcess
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymLoadModule (IMAGEHLP.@)
 */

BOOL WINAPI SymLoadModule(
  HANDLE hProcess, HANDLE hFile, LPSTR ImageName, LPSTR ModuleName,
  DWORD BaseOfDll, DWORD SizeOfDll)
{
  FIXME("(%p, %p, %s, %s, %ld, %ld): stub\n",
    hProcess, hFile, debugstr_a(ImageName), debugstr_a(ModuleName),
    BaseOfDll, SizeOfDll
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymRegisterCallback (IMAGEHLP.@)
 */
BOOL WINAPI SymRegisterCallback(
  HANDLE hProcess, PSYMBOL_REGISTERED_CALLBACK CallbackFunction,
  PVOID UserContext)
{
  FIXME("(%p, %p, %p): stub\n",
    hProcess, CallbackFunction, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymSetOptions (IMAGEHLP.@)
 */
DWORD WINAPI SymSetOptions(DWORD SymOptions)
{
  FIXME("(0x%08lx): stub\n", SymOptions);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		SymSetSearchPath (IMAGEHLP.@)
 */
BOOL WINAPI SymSetSearchPath(HANDLE hProcess, LPSTR szSearchPath)
{
  FIXME("(%p, %s): stub\n",
    hProcess, debugstr_a(szSearchPath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymUnDName (IMAGEHLP.@)
 */
BOOL WINAPI SymUnDName(
  PIMAGEHLP_SYMBOL sym, LPSTR UnDecName, DWORD UnDecNameLength)
{
  FIXME("(%p, %s, %ld): stub\n",
    sym, UnDecName, UnDecNameLength
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymUnloadModule (IMAGEHLP.@)
 */
BOOL WINAPI SymUnloadModule(
  HANDLE hProcess, DWORD BaseOfDll)
{
  FIXME("(%p, 0x%08lx): stub\n", hProcess, BaseOfDll);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
