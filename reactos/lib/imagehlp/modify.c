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

static WORD CalcCheckSum(DWORD StartValue, LPVOID BaseAddress, DWORD WordCount);
extern PIMAGE_NT_HEADERS WINAPI ImageNtHeader(PVOID Base);

/***********************************************************************
 *		BindImageEx (IMAGEHLP.@)
 */
BOOL WINAPI BindImageEx(
  DWORD Flags, LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath,
  PIMAGEHLP_STATUS_ROUTINE StatusRoutine)
{
  FIXME("(%ld, %s, %s, %s, %p): stub\n",
    Flags, debugstr_a(ImageName), debugstr_a(DllPath),
    debugstr_a(SymbolPath), StatusRoutine
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		BindImage (IMAGEHLP.@)
 */
BOOL WINAPI BindImage(
  LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath)
{
  return BindImageEx(0, ImageName, DllPath, SymbolPath, NULL);
}

/***********************************************************************
 *		CheckSum (internal)
 */
static WORD CalcCheckSum(
  DWORD StartValue, LPVOID BaseAddress, DWORD WordCount)
{
   LPWORD Ptr;
   DWORD Sum;
   DWORD i;

   Sum = StartValue;
   Ptr = (LPWORD)BaseAddress;
   for (i = 0; i < WordCount; i++)
     {
	Sum += *Ptr;
	if (HIWORD(Sum) != 0)
	  {
	     Sum = LOWORD(Sum) + HIWORD(Sum);
	  }
	Ptr++;
     }

   return (WORD)(LOWORD(Sum) + HIWORD(Sum));
}


/***********************************************************************
 *		CheckSumMappedFile (IMAGEHLP.@)
 */
PIMAGE_NT_HEADERS WINAPI CheckSumMappedFile(
  LPVOID BaseAddress, DWORD FileLength,
  LPDWORD HeaderSum, LPDWORD CheckSum)
{
  PIMAGE_NT_HEADERS Header;
  DWORD CalcSum;
  DWORD HdrSum;

  FIXME("(%p, %ld, %p, %p): stub\n",
    BaseAddress, FileLength, HeaderSum, CheckSum
  );

  CalcSum = (DWORD)CalcCheckSum(0,
				BaseAddress,
				(FileLength + 1) / sizeof(WORD));

  Header = ImageNtHeader(BaseAddress);
  HdrSum = Header->OptionalHeader.CheckSum;

  /* Subtract image checksum from calculated checksum. */
  /* fix low word of checksum */
  if (LOWORD(CalcSum) >= LOWORD(HdrSum))
  {
    CalcSum -= LOWORD(HdrSum);
  }
  else
  {
    CalcSum = ((LOWORD(CalcSum) - LOWORD(HdrSum)) & 0xFFFF) - 1;
  }

   /* fix high word of checksum */
  if (LOWORD(CalcSum) >= HIWORD(HdrSum))
  {
    CalcSum -= HIWORD(HdrSum);
  }
  else
  {
    CalcSum = ((LOWORD(CalcSum) - HIWORD(HdrSum)) & 0xFFFF) - 1;
  }

  /* add file length */
  CalcSum += FileLength;

  *CheckSum = CalcSum;
  *HeaderSum = Header->OptionalHeader.CheckSum;

  return Header;
}

/***********************************************************************
 *		MapFileAndCheckSumA (IMAGEHLP.@)
 */
DWORD WINAPI MapFileAndCheckSumA(
  LPSTR Filename, LPDWORD HeaderSum, LPDWORD CheckSum)
{
  HANDLE hFile;
  HANDLE hMapping;
  LPVOID BaseAddress;
  DWORD FileLength;

  TRACE("(%s, %p, %p): stub\n",
    debugstr_a(Filename), HeaderSum, CheckSum
  );

  hFile = CreateFileA(Filename,
		      GENERIC_READ,
		      FILE_SHARE_READ | FILE_SHARE_WRITE,
		      NULL,
		      OPEN_EXISTING,
		      FILE_ATTRIBUTE_NORMAL,
		      0);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    return CHECKSUM_OPEN_FAILURE;
  }

  hMapping = CreateFileMappingW(hFile,
			       NULL,
			       PAGE_READONLY,
			       0,
			       0,
			       NULL);
  if (hMapping == 0)
  {
    CloseHandle(hFile);
    return CHECKSUM_MAP_FAILURE;
  }

  BaseAddress = MapViewOfFile(hMapping,
			      FILE_MAP_READ,
			      0,
			      0,
			      0);
  if (hMapping == 0)
  {
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return CHECKSUM_MAPVIEW_FAILURE;
  }

  FileLength = GetFileSize(hFile,
			   NULL);

  CheckSumMappedFile(BaseAddress,
		     FileLength,
		     HeaderSum,
		     CheckSum);

  UnmapViewOfFile(BaseAddress);
  CloseHandle(hMapping);
  CloseHandle(hFile);

  return 0;
}

/***********************************************************************
 *		MapFileAndCheckSumW (IMAGEHLP.@)
 */
DWORD WINAPI MapFileAndCheckSumW(
  LPWSTR Filename, LPDWORD HeaderSum, LPDWORD CheckSum)
{
  HANDLE hFile;
  HANDLE hMapping;
  LPVOID BaseAddress;
  DWORD FileLength;

  TRACE("(%s, %p, %p): stub\n",
    debugstr_w(Filename), HeaderSum, CheckSum
  );

  hFile = CreateFileW(Filename,
		      GENERIC_READ,
		      FILE_SHARE_READ | FILE_SHARE_WRITE,
		      NULL,
		      OPEN_EXISTING,
		      FILE_ATTRIBUTE_NORMAL,
		      0);
  if (hFile == INVALID_HANDLE_VALUE)
  {
  return CHECKSUM_OPEN_FAILURE;
  }

  hMapping = CreateFileMappingW(hFile,
			       NULL,
			       PAGE_READONLY,
			       0,
			       0,
			       NULL);
  if (hMapping == 0)
  {
    CloseHandle(hFile);
    return CHECKSUM_MAP_FAILURE;
  }

  BaseAddress = MapViewOfFile(hMapping,
			      FILE_MAP_READ,
			      0,
			      0,
			      0);
  if (hMapping == 0)
  {
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return CHECKSUM_MAPVIEW_FAILURE;
  }

  FileLength = GetFileSize(hFile,
			   NULL);

  CheckSumMappedFile(BaseAddress,
		     FileLength,
		     HeaderSum,
		     CheckSum);

  UnmapViewOfFile(BaseAddress);
  CloseHandle(hMapping);
  CloseHandle(hFile);

  return 0;
}

/***********************************************************************
 *		ReBaseImage (IMAGEHLP.@)
 */
BOOL WINAPI ReBaseImage(
  LPSTR CurrentImageName, LPSTR SymbolPath, BOOL fReBase,
  BOOL fRebaseSysfileOk, BOOL fGoingDown, ULONG CheckImageSize,
  ULONG *OldImageSize, ULONG *OldImageBase, ULONG *NewImageSize,
  ULONG *NewImageBase, ULONG TimeStamp)
{
  FIXME(
    "(%s, %s, %d, %d, %d, %ld, %p, %p, %p, %p, %ld): stub\n",
      debugstr_a(CurrentImageName),debugstr_a(SymbolPath), fReBase,
      fRebaseSysfileOk, fGoingDown, CheckImageSize, OldImageSize,
      OldImageBase, NewImageSize, NewImageBase, TimeStamp
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		RemovePrivateCvSymbolic (IMAGEHLP.@)
 */
BOOL WINAPI RemovePrivateCvSymbolic(
  PCHAR DebugData, PCHAR *NewDebugData, ULONG *NewDebugSize)
{
  FIXME("(%p, %p, %p): stub\n",
    DebugData, NewDebugData, NewDebugSize
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		RemoveRelocations (IMAGEHLP.@)
 */
VOID WINAPI RemoveRelocations(PCHAR ImageName)
{
  FIXME("(%p): stub\n", ImageName);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/***********************************************************************
 *		SplitSymbols (IMAGEHLP.@)
 */
BOOL WINAPI SplitSymbols(
  LPSTR ImageName, LPSTR SymbolsPath,
  LPSTR SymbolFilePath, DWORD Flags)
{
  FIXME("(%s, %s, %s, %ld): stub\n",
    debugstr_a(ImageName), debugstr_a(SymbolsPath),
    debugstr_a(SymbolFilePath), Flags
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		UpdateDebugInfoFile (IMAGEHLP.@)
 */
BOOL WINAPI UpdateDebugInfoFile(
  LPSTR ImageFileName, LPSTR SymbolPath,
  LPSTR DebugFilePath, PIMAGE_NT_HEADERS NtHeaders)
{
  FIXME("(%s, %s, %s, %p): stub\n",
    debugstr_a(ImageFileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath), NtHeaders
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		UpdateDebugInfoFileEx (IMAGEHLP.@)
 */
BOOL WINAPI UpdateDebugInfoFileEx(
  LPSTR ImageFileName, LPSTR SymbolPath, LPSTR DebugFilePath,
  PIMAGE_NT_HEADERS NtHeaders, DWORD OldChecksum)
{
  FIXME("(%s, %s, %s, %p, %ld): stub\n",
    debugstr_a(ImageFileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath), NtHeaders, OldChecksum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
