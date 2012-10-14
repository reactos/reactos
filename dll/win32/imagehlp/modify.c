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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/debug.h"
#include "imagehlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

static WORD CalcCheckSum(DWORD StartValue, LPVOID BaseAddress, DWORD WordCount);


/***********************************************************************
 *		BindImage (IMAGEHLP.@)
 */
BOOL WINAPI BindImage(
  PCSTR ImageName, PCSTR DllPath, PCSTR SymbolPath)
{
  return BindImageEx(0, ImageName, DllPath, SymbolPath, NULL);
}

/***********************************************************************
 *		BindImageEx (IMAGEHLP.@)
 */
BOOL WINAPI BindImageEx(
  DWORD Flags, PCSTR ImageName, PCSTR DllPath, PCSTR SymbolPath,
  PIMAGEHLP_STATUS_ROUTINE StatusRoutine)
{
  FIXME("(%d, %s, %s, %s, %p): stub\n",
    Flags, debugstr_a(ImageName), debugstr_a(DllPath),
    debugstr_a(SymbolPath), StatusRoutine
  );
  return TRUE;
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
  IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *) BaseAddress;
  PIMAGE_NT_HEADERS32 Header32;
  PIMAGE_NT_HEADERS64 Header64;
  DWORD *ChecksumFile;
  DWORD CalcSum;
  DWORD HdrSum;

  TRACE("(%p, %d, %p, %p)\n",
    BaseAddress, FileLength, HeaderSum, CheckSum
  );

  CalcSum = (DWORD)CalcCheckSum(0,
				BaseAddress,
				(FileLength + 1) / sizeof(WORD));

  if (dos->e_magic != IMAGE_DOS_SIGNATURE)
    return NULL;

  Header32 = (IMAGE_NT_HEADERS32 *)((char *)dos + dos->e_lfanew);

  if (Header32->Signature != IMAGE_NT_SIGNATURE)
    return NULL;

  if (Header32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    ChecksumFile = &Header32->OptionalHeader.CheckSum;
  else if (Header32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
  {
    Header64 = (IMAGE_NT_HEADERS64 *)Header32;
    ChecksumFile = &Header64->OptionalHeader.CheckSum;
  }
  else
    return NULL;

  HdrSum = *ChecksumFile;

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
  *HeaderSum = *ChecksumFile;

  return (PIMAGE_NT_HEADERS) Header32;
}

/***********************************************************************
 *		MapFileAndCheckSumA (IMAGEHLP.@)
 */
DWORD WINAPI MapFileAndCheckSumA(
  PCSTR Filename, PDWORD HeaderSum, PDWORD CheckSum)
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
  if (BaseAddress == 0)
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
  PCWSTR Filename, PDWORD HeaderSum, PDWORD CheckSum)
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
  if (BaseAddress == 0)
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
  PCSTR CurrentImageName, PCSTR SymbolPath, BOOL fReBase,
  BOOL fRebaseSysfileOk, BOOL fGoingDown, ULONG CheckImageSize,
  ULONG *OldImageSize, ULONG_PTR *OldImageBase, ULONG *NewImageSize,
  ULONG_PTR *NewImageBase, ULONG TimeStamp)
{
  FIXME(
    "(%s, %s, %d, %d, %d, %d, %p, %p, %p, %p, %d): stub\n",
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
  PSTR ImageName, PCSTR SymbolsPath,
  PSTR SymbolFilePath, ULONG Flags)
{
  FIXME("(%s, %s, %s, %d): stub\n",
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
  PCSTR ImageFileName, PCSTR SymbolPath,
  PSTR DebugFilePath, PIMAGE_NT_HEADERS32 NtHeaders)
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
  PCSTR ImageFileName, PCSTR SymbolPath, PSTR DebugFilePath,
  PIMAGE_NT_HEADERS32 NtHeaders, DWORD OldChecksum)
{
  FIXME("(%s, %s, %s, %p, %d): stub\n",
    debugstr_a(ImageFileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath), NtHeaders, OldChecksum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
