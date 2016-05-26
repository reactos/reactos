/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 *           1997 Peter Schlaile
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

#include "progman.h"

#if 0
#define MALLOCHUNK 1000

#define GET_USHORT(buffer, i)\
  (((BYTE)((buffer)[(i)]) + 0x100 * (BYTE)((buffer)[(i)+1])))
#define GET_SHORT(buffer, i)\
  (((BYTE)((buffer)[(i)]) + 0x100 * (signed char)((buffer)[(i)+1])))
#define PUT_SHORT(buffer, i, s)\
  (((buffer)[(i)] = (s) & 0xff, (buffer)[(i)+1] = ((s) >> 8) & 0xff))

static BOOL   GRPFILE_ReadFileToBuffer(LPCSTR, HLOCAL*, INT*);
static HLOCAL GRPFILE_ScanGroup(LPCSTR, INT, LPCSTR, BOOL);
static HLOCAL GRPFILE_ScanProgram(LPCSTR, INT, LPCSTR, INT,
				  LPCSTR, HLOCAL,LPCSTR);
static BOOL GRPFILE_DoWriteGroupFile(HFILE file, PROGGROUP *group);
#endif

/***********************************************************************
 *
 *           GRPFILE_ModifyFileName
 *
 *  Change extension `.grp' to `.gr'
 */

#if 0
static VOID GRPFILE_ModifyFileName(LPSTR lpszNewName, LPCSTR lpszOrigName,
				   INT nSize, BOOL bModify)
{
  lstrcpynA(lpszNewName, lpszOrigName, nSize);
  lpszNewName[nSize-1] = '\0';
  if (!bModify) return;
  if (!lstrcmpiA(lpszNewName + strlen(lpszNewName) - 4, ".grp"))
    lpszNewName[strlen(lpszNewName) - 1] = '\0';
}
#endif

/***********************************************************************
 *
 *           GRPFILE_ReadGroupFile
 */

DWORD GRPFILE_ReadGroupFile(LPCWSTR lpszPath, BOOL bIsCommonGroup)
{
#if 0
  CHAR   szPath_gr[MAX_PATHNAME_LEN];
  BOOL   bFileNameModified = FALSE;
  OFSTRUCT dummy;
  HLOCAL hBuffer, hGroup;
  INT    size;

  /* if `.gr' file exists use that */
  GRPFILE_ModifyFileName(szPath_gr, lpszPath, MAX_PATHNAME_LEN, TRUE);
  if (OpenFile(szPath_gr, &dummy, OF_EXIST) != HFILE_ERROR)
    {
      lpszPath = szPath_gr;
      bFileNameModified = TRUE;
    }

  /* Read the whole file into a buffer */
  if (!GRPFILE_ReadFileToBuffer(lpszPath, &hBuffer, &size))
    {
      MAIN_MessageBoxIDS_s(IDS_GRPFILE_READ_ERROR_s, lpszPath, IDS_ERROR, MB_YESNO);
      return(0);
    }

  /* Interpret buffer */
  hGroup = GRPFILE_ScanGroup(LocalLock(hBuffer), size,
			     lpszPath, bFileNameModified);
  if (!hGroup)
    MAIN_MessageBoxIDS_s(IDS_GRPFILE_READ_ERROR_s, lpszPath, IDS_ERROR, MB_YESNO);

  LocalFree(hBuffer);

  return(hGroup);

#else
    return ERROR_SUCCESS;
#endif
}

/***********************************************************************
 *
 *           GRPFILE_ReadFileToBuffer
 */

#if 0
static BOOL GRPFILE_ReadFileToBuffer(LPCSTR path, HLOCAL *phBuffer,
				     INT *piSize)
{
  UINT    len, size;
  LPSTR  buffer;
  HLOCAL hBuffer, hNewBuffer;
  HFILE  file;

  file=_lopen(path, OF_READ);
  if (file == HFILE_ERROR) return FALSE;

  size = 0;
  hBuffer = LocalAlloc(LMEM_FIXED, MALLOCHUNK + 1);
  if (!hBuffer) return FALSE;
  buffer = LocalLock(hBuffer);

  while ((len = _lread(file, buffer + size, MALLOCHUNK))
	 == MALLOCHUNK)
    {
      size += len;
      hNewBuffer = LocalReAlloc(hBuffer, size + MALLOCHUNK + 1,
				LMEM_MOVEABLE);
      if (!hNewBuffer)
	{
	  LocalFree(hBuffer);
	  return FALSE;
	}
      hBuffer = hNewBuffer;
      buffer = LocalLock(hBuffer);
    }

  _lclose(file);

  if (len == (UINT)HFILE_ERROR)
    {
      LocalFree(hBuffer);
      return FALSE;
    }

  size += len;
  buffer[size] = 0;

  *phBuffer = hBuffer;
  *piSize   = size;
  return TRUE;
}
#endif

/***********************************************************************
 *           GRPFILE_ScanGroup
 */

#if 0
static HLOCAL GRPFILE_ScanGroup(LPCSTR buffer, INT size,
				LPCSTR lpszGrpFile,
				BOOL bModifiedFileName)
{
  HLOCAL  hGroup;
  INT     i, seqnum;
  LPCSTR  extension;
  LPCSTR  lpszName;
  INT     x, y, width, height, iconx, icony, nCmdShow;
  INT     number_of_programs;
  BOOL    bOverwriteFileOk;

  if (buffer[0] != 'P' || buffer[1] != 'M') return(0);
  if (buffer[2] == 'C' && buffer[3] == 'C')
    /* original with checksum */
    bOverwriteFileOk = FALSE;
  else if (buffer[2] == 'X' && buffer[3] == 'X')
    /* modified without checksum */
    bOverwriteFileOk = TRUE;
  else return(0);

  /* checksum = GET_USHORT(buffer, 4)   (ignored) */

  extension = buffer + GET_USHORT(buffer, 6);
  if (extension == buffer + size) extension = 0;
  else if (extension + 6 > buffer + size) return(0);

  nCmdShow = GET_USHORT(buffer,  8);
  x        = GET_SHORT(buffer,  10);
  y        = GET_SHORT(buffer,  12);
  width    = GET_USHORT(buffer, 14);
  height   = GET_USHORT(buffer, 16);
  iconx    = GET_SHORT(buffer,  18);
  icony    = GET_SHORT(buffer,  20);
  lpszName = buffer + GET_USHORT(buffer, 22);
  if (lpszName >= buffer + size) return(0);

  /* unknown bytes 24 - 31 ignored */
  /*
    Unknown bytes should be:
    wLogPixelsX = GET_SHORT(buffer, 24);
    wLogPixelsY = GET_SHORT(buffer, 26);
    byBitsPerPixel = byte at 28;
    byPlanes     = byte at 29;
    wReserved   = GET_SHORT(buffer, 30);
    */

  hGroup = GROUP_AddGroup(lpszName, lpszGrpFile, nCmdShow, x, y,
			  width, height, iconx, icony,
			  bModifiedFileName, bOverwriteFileOk,
			  TRUE);
  if (!hGroup) return(0);

  number_of_programs = GET_USHORT(buffer, 32);
  if (2 * number_of_programs + 34 > size) return(0);
  for (i=0, seqnum=0; i < number_of_programs; i++, seqnum++)
    {
      LPCSTR program_ptr = buffer + GET_USHORT(buffer, 34 + 2*i);
      if (program_ptr + 24 > buffer + size) return(0);
      if (!GET_USHORT(buffer, 34 + 2*i)) continue;
      if (!GRPFILE_ScanProgram(buffer, size, program_ptr, seqnum,
			       extension, hGroup, lpszGrpFile))
	{
	  GROUP_DeleteGroup(hGroup);
	  return(0);
	}
    }

  /* FIXME shouldn't be necessary */
  GROUP_ShowGroupWindow(hGroup);

  return hGroup;
}
#endif

/***********************************************************************
 *           GRPFILE_ScanProgram
 */

#if 0
static HLOCAL GRPFILE_ScanProgram(LPCSTR buffer, INT size,
				  LPCSTR program_ptr, INT seqnum,
				  LPCSTR extension, HLOCAL hGroup,
				  LPCSTR lpszGrpFile)
{
  INT    icontype;
  HICON  hIcon;
  LPCSTR lpszName, lpszCmdLine, lpszIconFile, lpszWorkDir;
  LPCSTR iconinfo_ptr, iconANDbits_ptr, iconXORbits_ptr;
  INT    x, y, nIconIndex, iconANDsize, iconXORsize;
  INT    nHotKey, nCmdShow;
  UINT width, height, planes, bpp;

  x               = GET_SHORT(program_ptr, 0);
  y               = GET_SHORT(program_ptr, 2);
  nIconIndex      = GET_USHORT(program_ptr, 4);

  /* FIXME is this correct ?? */
  icontype = GET_USHORT(program_ptr,  6);
  switch (icontype)
    {
    default:
      MAIN_MessageBoxIDS_s(IDS_UNKNOWN_FEATURE_s, lpszGrpFile,
			   IDS_WARNING, MB_OK);
    case 0x048c:
      iconXORsize     = GET_USHORT(program_ptr,  8);
      iconANDsize     = GET_USHORT(program_ptr, 10) / 8;
      iconinfo_ptr    = buffer + GET_USHORT(program_ptr, 12);
      iconXORbits_ptr = buffer + GET_USHORT(program_ptr, 14);
      iconANDbits_ptr = buffer + GET_USHORT(program_ptr, 16);
      width           = GET_USHORT(iconinfo_ptr, 4);
      height          = GET_USHORT(iconinfo_ptr, 6);
      planes          = GET_USHORT(iconinfo_ptr, 10);
      bpp             = GET_USHORT(iconinfo_ptr, 11);
      break;
    case 0x000c:
      iconANDsize     = GET_USHORT(program_ptr,  8);
      iconXORsize     = GET_USHORT(program_ptr, 10);
      iconinfo_ptr    = buffer + GET_USHORT(program_ptr, 12);
      iconANDbits_ptr = buffer + GET_USHORT(program_ptr, 14);
      iconXORbits_ptr = buffer + GET_USHORT(program_ptr, 16);
      width           = GET_USHORT(iconinfo_ptr, 4);
      height          = GET_USHORT(iconinfo_ptr, 6);
      planes          = GET_USHORT(iconinfo_ptr, 10);
      bpp             = GET_USHORT(iconinfo_ptr, 11);
    }

  if (iconANDbits_ptr + iconANDsize > buffer + size ||
      iconXORbits_ptr + iconXORsize > buffer + size) return(0);

  hIcon = CreateIcon(Globals.hInstance, width, height, planes, bpp, (PBYTE)iconANDbits_ptr, (PBYTE)iconXORbits_ptr);

  lpszName        = buffer + GET_USHORT(program_ptr, 18);
  lpszCmdLine     = buffer + GET_USHORT(program_ptr, 20);
  lpszIconFile    = buffer + GET_USHORT(program_ptr, 22);
  if (iconinfo_ptr + 6 > buffer + size ||
      lpszName         > buffer + size ||
      lpszCmdLine      > buffer + size ||
      lpszIconFile     > buffer + size) return(0);

  /* Scan Extensions */
  lpszWorkDir = "";
  nHotKey     = 0;
  nCmdShow    = SW_SHOWNORMAL;
  if (extension)
    {
      LPCSTR ptr = extension;
      while (ptr + 6 <= buffer + size)
	{
	  UINT type   = GET_USHORT(ptr, 0);
	  UINT number = GET_USHORT(ptr, 2);
	  UINT skip   = GET_USHORT(ptr, 4);

	  if (number == seqnum)
	    {
	      switch (type)
		{
		case 0x8000:
		  if (ptr + 10 > buffer + size) return(0);
		  if (ptr[6] != 'P' || ptr[7] != 'M' ||
		      ptr[8] != 'C' || ptr[9] != 'C') return(0);
		  break;
		case 0x8101:
		  lpszWorkDir = ptr + 6;
		  break;
		case 0x8102:
		  if (ptr + 8 > buffer + size) return(0);
		  nHotKey = GET_USHORT(ptr, 6);
		  break;
		case 0x8103:
		  if (ptr + 8 > buffer + size) return(0);
		  nCmdShow = GET_USHORT(ptr, 6);
		  break;
		default:
		  MAIN_MessageBoxIDS_s(IDS_UNKNOWN_FEATURE_s,
				       lpszGrpFile, IDS_WARNING, MB_OK);
		}
	    }
	  if (!skip) break;
	  ptr += skip;
	}
    }

  return (PROGRAM_AddProgram(hGroup, hIcon, lpszName, x, y,
			     lpszCmdLine, lpszIconFile,
			     nIconIndex, lpszWorkDir,
			     nHotKey, nCmdShow));
}
#endif

/***********************************************************************
 *
 *           GRPFILE_WriteGroupFile
 */

BOOL GRPFILE_WriteGroupFile(PROGGROUP* hGroup)
{
#if 0
  CHAR szPath[MAX_PATHNAME_LEN];
  PROGGROUP *group = LocalLock(hGroup);
  OFSTRUCT dummy;
  HFILE file;
  BOOL ret;

  GRPFILE_ModifyFileName(szPath, LocalLock(group->hGrpFile),
			 MAX_PATHNAME_LEN,
			 group->bFileNameModified);

  /* Try not to overwrite original files */

  /* group->bOverwriteFileOk == TRUE only if a file has the modified format */
  if (!group->bOverwriteFileOk &&
      OpenFile(szPath, &dummy, OF_EXIST) != HFILE_ERROR)
    {
      /* Original file exists, try `.gr' extension */
      GRPFILE_ModifyFileName(szPath, LocalLock(group->hGrpFile),
			     MAX_PATHNAME_LEN, TRUE);
      if (OpenFile(szPath, &dummy, OF_EXIST) != HFILE_ERROR)
	{
	  /* File exists. Do not overwrite */
	  MAIN_MessageBoxIDS_s(IDS_FILE_NOT_OVERWRITTEN_s, szPath,
			       IDS_INFO, MB_OK);
	  return FALSE;
	}
      /* Inform about the modified file name */
      if (IDCANCEL ==
	  MAIN_MessageBoxIDS_s(IDS_SAVE_GROUP_AS_s, szPath, IDS_INFO,
			       MB_OKCANCEL | MB_ICONINFORMATION))
	return FALSE;
    }

  {
    /* Warn about the (possible) incompatibility */
    CHAR msg[MAX_PATHNAME_LEN + 200];
    wsprintfA(msg,
	     "Group files written by this DRAFT Program Manager "
	     "possibly cannot be read by the Microsoft Program Manager!!\n"
	     "Are you sure to write %s?", szPath);
    if (IDOK != MessageBoxA(Globals.hMainWnd, msg, "WARNING",
                            MB_OKCANCEL | MB_DEFBUTTON2)) return FALSE;
  }

  /* Open file */
  file = _lcreat(szPath, 0);
  if (file != HFILE_ERROR)
    {
      ret = GRPFILE_DoWriteGroupFile(file, group);
      _lclose(file);
    }
  else ret = FALSE;

  if (!ret)
    MAIN_MessageBoxIDS_s(IDS_FILE_WRITE_ERROR_s, szPath, IDS_ERROR, MB_OK);

  return(ret);

#else
    return TRUE;
#endif
}

#if 0

/***********************************************************************
 *
 *           GRPFILE_CalculateSizes
 */

static VOID GRPFILE_CalculateSizes(PROGRAM *program, INT *Progs, INT *Icons,
                                   UINT *sizeAnd, UINT *sizeXor)
{
  ICONINFO info;
  BITMAP bmp;

  GetIconInfo( program->hIcon, &info );
  GetObjectW( info.hbmMask, sizeof(bmp), &bmp );
  *sizeAnd = bmp.bmHeight * ((bmp.bmWidth + 15) / 16 * 2);
  GetObjectW( info.hbmColor, sizeof(bmp), &bmp );
  *sizeXor = bmp.bmHeight * bmp.bmWidthBytes;
  DeleteObject( info.hbmMask );
  DeleteObject( info.hbmColor );

  *Progs += 24;
  *Progs += strlen(LocalLock(program->hName)) + 1;
  *Progs += strlen(LocalLock(program->hCmdLine)) + 1;
  *Progs += strlen(LocalLock(program->hIconFile)) + 1;

  *Icons += 12; /* IconInfo */
  *Icons += *sizeAnd;
  *Icons += *sizeXor;
}

/***********************************************************************/
UINT16 GRPFILE_checksum;
BOOL GRPFILE_checksum_half_word;
BYTE GRPFILE_checksum_last_byte;
/***********************************************************************
 *
 *           GRPFILE_InitChecksum
 */

static void GRPFILE_InitChecksum(void)
{
	GRPFILE_checksum = 0;
	GRPFILE_checksum_half_word = 0;
}

/***********************************************************************
 *
 *           GRPFILE_GetChecksum
 */

static UINT16 GRPFILE_GetChecksum(void)
{
	return GRPFILE_checksum;
}

/***********************************************************************
 *
 *           GRPFILE_WriteWithChecksum
 *
 * Looks crazier than it is:
 *
 * chksum = 0;
 * chksum = cksum - 1. word;
 * chksum = cksum - 2. word;
 * ...
 *
 * if (filelen is even)
 *      great I'm finished
 * else
 *      ignore last byte
 */

static UINT GRPFILE_WriteWithChecksum(HFILE file, LPCSTR str, UINT size)
{
	UINT i;
	if (GRPFILE_checksum_half_word) {
		GRPFILE_checksum -= GRPFILE_checksum_last_byte;
	}
	for (i=0; i < size; i++) {
		if (GRPFILE_checksum_half_word) {
			GRPFILE_checksum -= str[i] << 8;
		} else {
			GRPFILE_checksum -= str[i];
		}
		GRPFILE_checksum_half_word ^= 1;
	}

	if (GRPFILE_checksum_half_word) {
		GRPFILE_checksum_last_byte = str[size-1];
		GRPFILE_checksum += GRPFILE_checksum_last_byte;
	}

	return _lwrite(file, str, size);
}


/***********************************************************************
 *
 *           GRPFILE_DoWriteGroupFile
 */

static BOOL GRPFILE_DoWriteGroupFile(HFILE file, PROGGROUP *group)
{
  CHAR buffer[34];
  HLOCAL hProgram;
  INT    NumProg, Title, Progs, Icons, Extension;
  INT    CurrProg, CurrIcon, nCmdShow, ptr, seqnum;

  UINT sizeAnd, sizeXor;

  BOOL   need_extension;
  LPCSTR lpszTitle = LocalLock(group->hName);

  UINT16 checksum;

  GRPFILE_InitChecksum();

  /* Calculate offsets */
  NumProg = 0;
  Icons   = 0;
  Extension = 0;
  need_extension = FALSE;
  hProgram = group->hPrograms;
  while(hProgram)
    {
      PROGRAM *program = LocalLock(hProgram);
      LPCSTR lpszWorkDir = LocalLock(program->hWorkDir);

      NumProg++;
      GRPFILE_CalculateSizes(program, &Icons, &Extension, &sizeAnd, &sizeXor);

      /* Set a flag if an extension is needed */
      if (lpszWorkDir[0] || program->nHotKey ||
	  program->nCmdShow != SW_SHOWNORMAL) need_extension = TRUE;

      hProgram = program->hNext;
    }
  Title      = 34 + NumProg * 2;
  Progs      = Title + strlen(lpszTitle) + 1;
  Icons     += Progs;
  Extension += Icons;

  /* Header */
  buffer[0] = 'P';
  buffer[1] = 'M';
  buffer[2] = 'C';
  buffer[3] = 'C';

  PUT_SHORT(buffer,  4, 0); /* Checksum zero for now, written later */
  PUT_SHORT(buffer,  6, Extension);
  /* Update group->nCmdShow */
  if (IsIconic(group->hWnd))      nCmdShow = SW_SHOWMINIMIZED;
  else if (IsZoomed(group->hWnd)) nCmdShow = SW_SHOWMAXIMIZED;
  else                            nCmdShow = SW_SHOWNORMAL;
  PUT_SHORT(buffer,  8, nCmdShow);
  PUT_SHORT(buffer, 10, group->x);
  PUT_SHORT(buffer, 12, group->y);
  PUT_SHORT(buffer, 14, group->width);
  PUT_SHORT(buffer, 16, group->height);
  PUT_SHORT(buffer, 18, group->iconx);
  PUT_SHORT(buffer, 20, group->icony);
  PUT_SHORT(buffer, 22, Title);
  PUT_SHORT(buffer, 24, 0x0020); /* unknown */
  PUT_SHORT(buffer, 26, 0x0020); /* unknown */
  PUT_SHORT(buffer, 28, 0x0108); /* unknown */
  PUT_SHORT(buffer, 30, 0x0000); /* unknown */
  PUT_SHORT(buffer, 32, NumProg);

  if ((UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, buffer, 34)) return FALSE;

  /* Program table */
  CurrProg = Progs;
  CurrIcon = Icons;
  hProgram = group->hPrograms;
  while(hProgram)
    {
      PROGRAM *program = LocalLock(hProgram);

      PUT_SHORT(buffer, 0, CurrProg);
      if ((UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, buffer, 2))
	      return FALSE;

      GRPFILE_CalculateSizes(program, &CurrProg, &CurrIcon, &sizeAnd, &sizeXor);
      hProgram = program->hNext;
    }

  /* Title */
  if ((UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, lpszTitle, strlen(lpszTitle) + 1))
    return FALSE;

  /* Program entries */
  CurrProg = Progs;
  CurrIcon = Icons;
  hProgram = group->hPrograms;
  while(hProgram)
    {
      PROGRAM *program = LocalLock(hProgram);
      LPCSTR Name     = LocalLock(program->hName);
      LPCSTR CmdLine  = LocalLock(program->hCmdLine);
      LPCSTR IconFile = LocalLock(program->hIconFile);
      INT next_prog = CurrProg;
      INT next_icon = CurrIcon;

      GRPFILE_CalculateSizes(program, &next_prog, &next_icon, &sizeAnd, &sizeXor);
      PUT_SHORT(buffer,  0, program->x);
      PUT_SHORT(buffer,  2, program->y);
      PUT_SHORT(buffer,  4, program->nIconIndex);
      PUT_SHORT(buffer,  6, 0x048c);            /* unknown */
      PUT_SHORT(buffer,  8, sizeXor);
      PUT_SHORT(buffer, 10, sizeAnd * 8);
      PUT_SHORT(buffer, 12, CurrIcon);
      PUT_SHORT(buffer, 14, CurrIcon + 12 + sizeAnd);
      PUT_SHORT(buffer, 16, CurrIcon + 12);
      ptr = CurrProg + 24;
      PUT_SHORT(buffer, 18, ptr);
      ptr += strlen(Name) + 1;
      PUT_SHORT(buffer, 20, ptr);
      ptr += strlen(CmdLine) + 1;
      PUT_SHORT(buffer, 22, ptr);

      if ((UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, buffer, 24) ||
	  (UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, Name, strlen(Name) + 1) ||
	  (UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, CmdLine, strlen(CmdLine) + 1) ||
	  (UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, IconFile, strlen(IconFile) + 1))
	return FALSE;

      CurrProg = next_prog;
      CurrIcon = next_icon;
      hProgram = program->hNext;
    }

  /* Icons */
#if 0  /* FIXME: this is broken anyway */
  hProgram = group->hPrograms;
  while(hProgram)
    {
      PROGRAM *program = LocalLock(hProgram);
      CURSORICONINFO *iconinfo = LocalLock(program->hIcon);
      LPVOID XorBits, AndBits;
      INT sizeXor = iconinfo->nHeight * iconinfo->nWidthBytes;
      INT sizeAnd = iconinfo->nHeight * ((iconinfo->nWidth + 15) / 16 * 2);
      /* DumpIcon16(LocalLock(program->hIcon), 0, &XorBits, &AndBits);*/

      PUT_SHORT(buffer, 0, iconinfo->ptHotSpot.x);
      PUT_SHORT(buffer, 2, iconinfo->ptHotSpot.y);
      PUT_SHORT(buffer, 4, iconinfo->nWidth);
      PUT_SHORT(buffer, 6, iconinfo->nHeight);
      PUT_SHORT(buffer, 8, iconinfo->nWidthBytes);
      buffer[10] = iconinfo->bPlanes;
      buffer[11] = iconinfo->bBitsPerPixel;

      if ((UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, buffer, 12) ||
	  (UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, AndBits, sizeAnd) ||
	  (UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, XorBits, sizeXor)) return FALSE;

      hProgram = program->hNext;
    }
#endif

  if (need_extension)
    {
      /* write `PMCC' extension */
      PUT_SHORT(buffer, 0, 0x8000);
      PUT_SHORT(buffer, 2, 0xffff);
      PUT_SHORT(buffer, 4, 0x000a);
      buffer[6] = 'P', buffer[7] = 'M';
      buffer[8] = 'C', buffer[9] = 'C';
      if ((UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, buffer, 10))
	      return FALSE;

      seqnum = 0;
      hProgram = group->hPrograms;
      while(hProgram)
	{
	  PROGRAM *program = LocalLock(hProgram);
	  LPCSTR lpszWorkDir = LocalLock(program->hWorkDir);

	  /* Working directory */
	  if (lpszWorkDir[0])
	    {
	      PUT_SHORT(buffer, 0, 0x8101);
	      PUT_SHORT(buffer, 2, seqnum);
	      PUT_SHORT(buffer, 4, 7 + strlen(lpszWorkDir));
	      if ((UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, buffer, 6) ||
		  (UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, lpszWorkDir, strlen(lpszWorkDir) + 1))
		return FALSE;
	    }

	  /* Hot key */
	  if (program->nHotKey)
	    {
	      PUT_SHORT(buffer, 0, 0x8102);
	      PUT_SHORT(buffer, 2, seqnum);
	      PUT_SHORT(buffer, 4, 8);
	      PUT_SHORT(buffer, 6, program->nHotKey);
	      if ((UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, buffer, 8)) return FALSE;
	    }

	  /* Show command */
	  if (program->nCmdShow)
	    {
	      PUT_SHORT(buffer, 0, 0x8103);
	      PUT_SHORT(buffer, 2, seqnum);
	      PUT_SHORT(buffer, 4, 8);
	      PUT_SHORT(buffer, 6, program->nCmdShow);
	      if ((UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, buffer, 8)) return FALSE;
	    }

	  seqnum++;
	  hProgram = program->hNext;
	}

      /* Write `End' extension */
      PUT_SHORT(buffer, 0, 0xffff);
      PUT_SHORT(buffer, 2, 0xffff);
      PUT_SHORT(buffer, 4, 0x0000);
      if ((UINT)HFILE_ERROR == GRPFILE_WriteWithChecksum(file, buffer, 6)) return FALSE;
    }

  checksum = GRPFILE_GetChecksum();
  _llseek(file, 4, SEEK_SET);
  PUT_SHORT(buffer, 0, checksum);
  _lwrite(file, buffer, 2);

  return TRUE;
}

#endif
