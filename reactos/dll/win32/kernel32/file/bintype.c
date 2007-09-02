/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/bintype.c
 * PURPOSE:         Binary detection functions
 * PROGRAMMER:      Alexandre Julliard (WINE)
 *                  Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *                  02/05/2004 - Ported/Adapted from WINE
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/* Check whether a file is an OS/2 or a very old Windows executable
 * by testing on import of KERNEL.
 *
 * FIXME: is reading the module imports the only way of discerning
 *        old Windows binaries from OS/2 ones ? At least it seems so...
 */
static DWORD STDCALL
InternalIsOS2OrOldWin(HANDLE hFile, IMAGE_DOS_HEADER *mz, IMAGE_OS2_HEADER *ne)
{
  DWORD CurPos;
  LPWORD modtab = NULL;
  LPSTR nametab = NULL;
  DWORD Read, Ret;
  int i;

  Ret = BINARY_OS216;
  CurPos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

  /* read modref table */
  if((SetFilePointer(hFile, mz->e_lfanew + ne->ne_modtab, NULL, FILE_BEGIN) == (DWORD)-1) ||
     (!(modtab = HeapAlloc(GetProcessHeap(), 0, ne->ne_cmod * sizeof(WORD)))) ||
     (!(ReadFile(hFile, modtab, ne->ne_cmod * sizeof(WORD), &Read, NULL))) ||
     (Read != (DWORD)ne->ne_cmod * sizeof(WORD)))
  {
    goto broken;
  }

  /* read imported names table */
  if((SetFilePointer(hFile, mz->e_lfanew + ne->ne_imptab, NULL, FILE_BEGIN) == (DWORD)-1) ||
     (!(nametab = HeapAlloc(GetProcessHeap(), 0, ne->ne_enttab - ne->ne_imptab))) ||
     (!(ReadFile(hFile, nametab, ne->ne_enttab - ne->ne_imptab, &Read, NULL))) ||
     (Read != (DWORD)ne->ne_enttab - ne->ne_imptab))
  {
    goto broken;
  }

  for(i = 0; i < ne->ne_cmod; i++)
  {
    LPSTR module;
    module = &nametab[modtab[i]];
    if(!strncmp(&module[1], "KERNEL", module[0]))
    {
      /* very old windows file */
      Ret = BINARY_WIN16;
      goto done;
    }
  }

  broken:
  DPRINT("InternalIsOS2OrOldWin(): Binary file seems to be broken\n");

  done:
  HeapFree(GetProcessHeap(), 0, modtab);
  HeapFree(GetProcessHeap(), 0, nametab);
  SetFilePointer(hFile, CurPos, NULL, FILE_BEGIN);
  return Ret;
}

static DWORD STDCALL
InternalGetBinaryType(HANDLE hFile)
{
  union
  {
    struct
    {
      unsigned char magic[4];
      unsigned char ignored[12];
      unsigned short type;
    } elf;
    struct
    {
      unsigned long magic;
      unsigned long cputype;
      unsigned long cpusubtype;
      unsigned long filetype;
    } macho;
    IMAGE_DOS_HEADER mz;
  } Header;
  char magic[4];
  DWORD Read;

  if((SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == (DWORD)-1) ||
     (!ReadFile(hFile, &Header, sizeof(Header), &Read, NULL) ||
      (Read != sizeof(Header))))
  {
    return BINARY_UNKNOWN;
  }

  if(!memcmp(Header.elf.magic, "\177ELF", sizeof(Header.elf.magic)))
  {
    /* FIXME: we don't bother to check byte order, architecture, etc. */
    switch(Header.elf.type)
    {
      case 2:
        return BINARY_UNIX_EXE;
      case 3:
        return BINARY_UNIX_LIB;
    }
    return BINARY_UNKNOWN;
  }

  /* Mach-o File with Endian set to Big Endian  or Little Endian*/
  if(Header.macho.magic == 0xFEEDFACE ||
     Header.macho.magic == 0xCEFAEDFE)
  {
    switch(Header.macho.filetype)
    {
      case 0x8:
        /* MH_BUNDLE */
        return BINARY_UNIX_LIB;
    }
    return BINARY_UNKNOWN;
  }

  /* Not ELF, try DOS */
  if(Header.mz.e_magic == IMAGE_DOS_SIGNATURE)
  {
    /* We do have a DOS image so we will now try to seek into
     * the file by the amount indicated by the field
     * "Offset to extended header" and read in the
     * "magic" field information at that location.
     * This will tell us if there is more header information
     * to read or not.
     */
    if((SetFilePointer(hFile, Header.mz.e_lfanew, NULL, FILE_BEGIN) == (DWORD)-1) ||
       (!ReadFile(hFile, magic, sizeof(magic), &Read, NULL) ||
        (Read != sizeof(magic))))
    {
      return BINARY_DOS;
    }

    /* Reading the magic field succeeded so
     * we will try to determine what type it is.
     */
    if(!memcmp(magic, "PE\0\0", sizeof(magic)))
    {
      IMAGE_FILE_HEADER FileHeader;
      if(!ReadFile(hFile, &FileHeader, sizeof(IMAGE_FILE_HEADER), &Read, NULL) ||
         (Read != sizeof(IMAGE_FILE_HEADER)))
      {
        return BINARY_DOS;
      }

      /* FIXME - detect 32/64 bit */

      if(FileHeader.Characteristics & IMAGE_FILE_DLL)
        return BINARY_PE_DLL32;
      return BINARY_PE_EXE32;
    }

    if(!memcmp(magic, "NE", 1))
    {
      /* This is a Windows executable (NE) header.  This can
       * mean either a 16-bit OS/2 or a 16-bit Windows or even a
       * DOS program (running under a DOS extender).  To decide
       * which, we'll have to read the NE header.
       */
      IMAGE_OS2_HEADER ne;
      if((SetFilePointer(hFile, Header.mz.e_lfanew, NULL, FILE_BEGIN) == 1) ||
         !ReadFile(hFile, &ne, sizeof(IMAGE_OS2_HEADER), &Read, NULL) ||
         (Read != sizeof(IMAGE_OS2_HEADER)))
      {
        /* Couldn't read header, so abort. */
        return BINARY_DOS;
      }

      switch(ne.ne_exetyp)
      {
        case 2:
          return BINARY_WIN16;
        case 5:
          return BINARY_DOS;
        default:
          return InternalIsOS2OrOldWin(hFile, &Header.mz, &ne);
      }
    }
    return BINARY_DOS;
  }
  return BINARY_UNKNOWN;
}

/*
 * @implemented
 */
BOOL
STDCALL
GetBinaryTypeW (
    LPCWSTR lpApplicationName,
    LPDWORD lpBinaryType
    )
{
  HANDLE hFile;
  DWORD BinType;

  if(!lpApplicationName || !lpBinaryType)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  hFile = CreateFileW(lpApplicationName, GENERIC_READ, FILE_SHARE_READ, NULL,
                      OPEN_EXISTING, 0, 0);
  if(hFile == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }

  BinType = InternalGetBinaryType(hFile);
  CloseHandle(hFile);

  switch(BinType)
  {
    case BINARY_UNKNOWN:
    {
      WCHAR *dot;

      /*
       * guess from filename
       */
      if(!(dot = wcsrchr(lpApplicationName, L'.')))
      {
        return FALSE;
      }
      if(!lstrcmpiW(dot, L".COM"))
      {
        *lpBinaryType = SCS_DOS_BINARY;
        return TRUE;
      }
      if(!lstrcmpiW(dot, L".PIF"))
      {
        *lpBinaryType = SCS_PIF_BINARY;
        return TRUE;
      }
      return FALSE;
    }
    case BINARY_PE_EXE32:
    case BINARY_PE_DLL32:
    {
      *lpBinaryType = SCS_32BIT_BINARY;
      return TRUE;
    }
    case BINARY_PE_EXE64:
    case BINARY_PE_DLL64:
    {
      *lpBinaryType = SCS_64BIT_BINARY;
      return TRUE;
    }
    case BINARY_WIN16:
    {
      *lpBinaryType = SCS_WOW_BINARY;
      return TRUE;
    }
    case BINARY_OS216:
    {
      *lpBinaryType = SCS_OS216_BINARY;
      return TRUE;
    }
    case BINARY_DOS:
    {
      *lpBinaryType = SCS_DOS_BINARY;
      return TRUE;
    }
    case BINARY_UNIX_EXE:
    case BINARY_UNIX_LIB:
    {
      return FALSE;
    }
  }

  DPRINT1("Invalid binary type returned!\n", BinType);
  return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetBinaryTypeA (
    LPCSTR  lpApplicationName,
    LPDWORD lpBinaryType
    )
{
  PWCHAR ApplicationNameW;

  if(!lpApplicationName || !lpBinaryType)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  if (!(ApplicationNameW = FilenameA2W(lpApplicationName, FALSE)))
     return FALSE;

  return GetBinaryTypeW(ApplicationNameW, lpBinaryType);
}

/* EOF */
