/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

HENHMETAFILE
STDCALL
NtGdiCloseEnhMetaFile(HDC  hDC)
{
  LPENHMETAHEADER emh;
  HANDLE hmf = 0;
  PDD_ENHMETAFILEOBJ phmf;
  HANDLE hMapping = 0;
  EMREOF emr;
  PDC Dc;

  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;



  Dc = DC_LockDc(hDC);
  if (Dc == NULL)
  {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return NULL;
  }

  emr.emr.iType = EMR_EOF;
  emr.emr.nSize = sizeof(EMREOF);
  emr.nPalEntries = 0;
  emr.offPalEntries = 0;
  emr.nSizeLast = emr.emr.nSize;

  if(Dc->hFile)
  {
     Status = NtWriteFile(Dc->hFile, NULL, NULL, NULL, &Iosb, (PVOID)&emr, emr.emr.nSize, NULL, NULL);
     if (Status == STATUS_PENDING)
      {
          Status = NtWaitForSingleObject(Dc->hFile,FALSE,NULL);
          if (NT_SUCCESS(Status))
          {
              Status = Iosb.Status;
          }
      }

      if (NT_SUCCESS(Status))
      {
		  DWORD len = Dc->emh->nBytes + emr.emr.nSize;
		  /* always resize the buffer */
		  emh = EngAllocMem(FL_ZERO_MEMORY, len, 0);
		  if (emh != NULL)
	      {
              memcpy(emh,Dc->emh,Dc->emh->nBytes);
	          EngFreeMem(Dc->emh);
	          Dc->emh = emh;

			  memcpy(Dc->emh + Dc->emh->nBytes, &emr, emr.emr.nSize);
	      }
	      else
	      {
	          EngFreeMem(Dc->emh);
	          Dc->emh=NULL;
	      }

      }
      else
      {
          Dc->hFile = NULL;
		  DPRINT1("Write to EnhMetaFile fail\n");
      }
  }

  Dc->emh->nBytes += emr.emr.nSize;
  Dc->emh->nRecords++;

  if(Dc->emh->rclFrame.left > Dc->emh->rclFrame.right)
  {
     Dc->emh->rclFrame.left = Dc->emh->rclBounds.left * Dc->emh->szlMillimeters.cx * 100 / Dc->emh->szlDevice.cx;
     Dc->emh->rclFrame.top = Dc->emh->rclBounds.top * Dc->emh->szlMillimeters.cy * 100 / Dc->emh->szlDevice.cy;
     Dc->emh->rclFrame.right = Dc->emh->rclBounds.right * Dc->emh->szlMillimeters.cx * 100 / Dc->emh->szlDevice.cx;
     Dc->emh->rclFrame.bottom = Dc->emh->rclBounds.bottom * Dc->emh->szlMillimeters.cy * 100 / Dc->emh->szlDevice.cy;
  }

  if (Dc->hFile)  /* disk based metafile */
  {
	  FILE_POSITION_INFORMATION FilePosition;
	  LARGE_INTEGER Distance ;
	  IO_STATUS_BLOCK IoStatusBlock;

	  POBJECT_ATTRIBUTES ObjectAttributes = NULL;
      ACCESS_MASK DesiredAccess;
	  PLARGE_INTEGER SectionSize = NULL;
	  DWORD flProtect;
	  ULONG Attributes;
	  LARGE_INTEGER SectionOffset;
      ULONG ViewSize;
      ULONG Protect;
      LPVOID ViewBase;

	  Distance.u.LowPart = 0;
      Distance.u.HighPart = 0;
	  FilePosition.CurrentByteOffset.QuadPart = Distance.QuadPart;

	  DPRINT1("Trying write to metafile and map it\n");

	  Status = NtSetInformationFile(Dc->hFile, &IoStatusBlock, &FilePosition,
		                             sizeof(FILE_POSITION_INFORMATION), FilePositionInformation);

	 if (!NT_SUCCESS(Status))
     {
		 // SetLastErrorByStatus(Status);
         SetLastWin32Error(ERROR_INVALID_HANDLE);

		 NtClose( Dc->hFile );
		 DC_UnlockDc(Dc);
		 NtGdiDeleteObjectApp(hDC);

		 DPRINT1("NtSetInformationFile fail\n");
	     return hmf;
     }

	 if (FilePosition.CurrentByteOffset.u.LowPart != 0)
	 {
		 // SetLastErrorByStatus(Status);
		 SetLastWin32Error(ERROR_INVALID_HANDLE);

		 NtClose( Dc->hFile );
		 DC_UnlockDc(Dc);
		 NtGdiDeleteObjectApp(hDC);
		 DPRINT1("FilePosition.CurrentByteOffset.u.LowPart is not 0\n");
	     return hmf;
	 }

	 Status = NtWriteFile(Dc->hFile, NULL, NULL, NULL, &Iosb, (PVOID)&Dc->emh,  sizeof(*Dc->emh), NULL, NULL);
     if (Status == STATUS_PENDING)
     {
          Status = NtWaitForSingleObject(Dc->hFile,FALSE,NULL);
          if (NT_SUCCESS(Status))
          {
              Status = Iosb.Status;
          }
      }

      if (!NT_SUCCESS(Status))
      {
         NtClose( Dc->hFile );
		 DC_UnlockDc(Dc);
         NtGdiDeleteObjectApp(hDC);
		 DPRINT1("fail to write 0\n");
         return hmf;
      }

	  EngFreeMem(Dc->emh);

      /* create maping */
      DesiredAccess = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ;
      Attributes = (PAGE_READONLY & (SEC_FILE | SEC_IMAGE | SEC_RESERVE | SEC_NOCACHE | SEC_COMMIT));
      flProtect = PAGE_READONLY ^ (PAGE_READONLY & (SEC_FILE | SEC_IMAGE | SEC_RESERVE | SEC_NOCACHE | SEC_COMMIT));

      if (!Attributes) Attributes = SEC_COMMIT;

      if (Dc->hFile == INVALID_HANDLE_VALUE)
      {
          Dc->hFile = NULL;
          if (!SectionSize)
          {
			 SetLastWin32Error(ERROR_INVALID_PARAMETER);
             hMapping = NULL;
			 DPRINT1("fail !SectionSize \n");
          }
      }
	  else
	  {
          Status = NtCreateSection(&hMapping, DesiredAccess, ObjectAttributes, SectionSize, flProtect, Attributes, Dc->hFile);
          if (!NT_SUCCESS(Status))
          {
          //SetLastErrorByStatus(Status);
		      SetLastWin32Error(ERROR_INVALID_HANDLE);
              hMapping =  NULL;
			  DPRINT1("fail NtCreateSection \n");
          }
	  }

      /* MapViewOfFile */
      SectionOffset.LowPart = 0;
      SectionOffset.HighPart = 0;
      ViewBase = NULL;
      ViewSize = 0;

      Protect = PAGE_READONLY;

      Status = ZwMapViewOfSection(&hMapping, NtCurrentProcess(), &ViewBase, 0,
		                          0, &SectionOffset, &ViewSize, ViewShare, 0, Protect);
      if (!NT_SUCCESS(Status))
      {
          //SetLastErrorByStatus(Status);
		  SetLastWin32Error(ERROR_INVALID_HANDLE);
          Dc->emh = NULL;
		  DPRINT1("fail ZwMapViewOfSection \n");
      }
	  else
      {
          Dc->emh = ViewBase;
	  }
      /* Close */
	  if (hMapping != NULL)
          NtClose( hMapping );
	  if (Dc->hFile != NULL)
          NtClose( Dc->hFile );
    }

  hmf = GDIOBJ_AllocObj(GdiHandleTable, GDI_OBJECT_TYPE_ENHMETAFILE);
  if (hmf != NULL)
  {
     phmf = GDIOBJ_LockObj(GdiHandleTable, hmf, GDI_OBJECT_TYPE_ENHMETAFILE);
	 if (phmf != NULL)
	 {
         if (Dc->hFile != NULL)
         {
             phmf->on_disk = TRUE;
	     }
         else
         {
	         phmf->on_disk = FALSE;
         }
		 GDIOBJ_UnlockObjByPtr(GdiHandleTable, phmf);
		 phmf->emh = Dc->emh;
	 }
  }

  Dc->emh = NULL;  /* So it won't be deleted */
  DC_UnlockDc(Dc);
  NtGdiDeleteObjectApp(hDC);
  return hmf;
}

HENHMETAFILE
STDCALL
NtGdiCopyEnhMetaFile(HENHMETAFILE  Src,
                                  LPCWSTR  File)
{
  UNIMPLEMENTED;
  return 0;
}

//
//
// Rewrite is in progress, this function is subject to change at any time.
// 04/30/2007
//
HDC
STDCALL
NtGdiCreateEnhMetaFile(HDC  hDCRef,
                           LPCWSTR  File,
                           CONST LPRECT  Rect,
                           LPCWSTR  Description)
{
   PDC Dc;
   HDC ret = NULL;
   DWORD length = 0;
   HDC tempHDC;
   DWORD MemSize;
   DWORD dwDesiredAccess;

   tempHDC = hDCRef;
   if (hDCRef == NULL)
   {
       /* FIXME ??
        * Shall we create hdc NtGdiHdcCompatible hdc ??
        */
       UNICODE_STRING DriverName;
       RtlInitUnicodeString(&DriverName, L"DISPLAY");
       //IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE);
           tempHDC = NtGdiOpenDCW( &DriverName,
                                          NULL,
                                          NULL,
                                             0,  // DCW 0 and ICW 1.
                                          NULL,
                                  (PVOID) NULL,
                                  (PVOID) NULL );
   }

   GDIOBJ_SetOwnership(GdiHandleTable, tempHDC, PsGetCurrentProcess());
   DC_SetOwnership(tempHDC, PsGetCurrentProcess());

   Dc = DC_LockDc(tempHDC);
   if (Dc == NULL)
   {
	  if (hDCRef == NULL)
	  {
          NtGdiDeleteObjectApp(tempHDC);
	  }
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return NULL;
   }

   if(Description)
   {
      length = wcslen(Description);
      length += wcslen(Description + length + 1);
      length += 3;
      length *= 2;
   }

   MemSize = sizeof(ENHMETAHEADER) + (length + 3) / 4 * 4;

   if (!(Dc->emh = EngAllocMem(FL_ZERO_MEMORY, MemSize, 0)))
   {
       DC_UnlockDc(Dc);
       if (hDCRef == NULL)
       {
           NtGdiDeleteObjectApp(tempHDC);
       }
       SetLastWin32Error(ERROR_INVALID_HANDLE);
       return NULL;
   }

   Dc->emh->iType = EMR_HEADER;
   Dc->emh->nSize = MemSize;

   Dc->emh->rclBounds.left = Dc->emh->rclBounds.top = 0;
   Dc->emh->rclBounds.right = Dc->emh->rclBounds.bottom = -1;

   if(Rect)
   {
      Dc->emh->rclFrame.left   = Rect->left;
      Dc->emh->rclFrame.top    = Rect->top;
      Dc->emh->rclFrame.right  = Rect->right;
      Dc->emh->rclFrame.bottom = Rect->bottom;
   }
   else
   {
      /* Set this to {0,0 - -1,-1} and update it at the end */
      Dc->emh->rclFrame.left = Dc->emh->rclFrame.top = 0;
      Dc->emh->rclFrame.right = Dc->emh->rclFrame.bottom = -1;
   }

   Dc->emh->dSignature = ENHMETA_SIGNATURE;
   Dc->emh->nVersion = 0x10000;
   Dc->emh->nBytes = Dc->emh->nSize;
   Dc->emh->nRecords = 1;
   Dc->emh->nHandles = 1;

   Dc->emh->sReserved = 0; /* According to docs, this is reserved and must be 0 */
   Dc->emh->nDescription = length / 2;

   Dc->emh->offDescription = length ? sizeof(ENHMETAHEADER) : 0;

   Dc->emh->nPalEntries = 0; /* I guess this should start at 0 */

   /* Size in pixels */
   Dc->emh->szlDevice.cx = NtGdiGetDeviceCaps(tempHDC, HORZRES);
   Dc->emh->szlDevice.cy = NtGdiGetDeviceCaps(tempHDC, VERTRES);

   /* Size in millimeters */
   Dc->emh->szlMillimeters.cx = NtGdiGetDeviceCaps(tempHDC, HORZSIZE);
   Dc->emh->szlMillimeters.cy = NtGdiGetDeviceCaps(tempHDC, VERTSIZE);

   /* Size in micrometers */
   Dc->emh->szlMicrometers.cx = Dc->emh->szlMillimeters.cx * 1000;
   Dc->emh->szlMicrometers.cy = Dc->emh->szlMillimeters.cy * 1000;

   if(Description)
   {
      memcpy((char *)Dc->emh + sizeof(ENHMETAHEADER), Description, length);
   }

   ret = tempHDC;
   if (File)
   {
      DPRINT1("Trying Create EnhMetaFile\n");

      /* disk based metafile */
      dwDesiredAccess = GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES;
      OBJECT_ATTRIBUTES ObjectAttributes;
      IO_STATUS_BLOCK IoStatusBlock;
      IO_STATUS_BLOCK Iosb;
      UNICODE_STRING NtPathU;
      NTSTATUS Status;
      ULONG FileAttributes = (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY);

      if (!RtlDosPathNameToNtPathName_U (File, &NtPathU, NULL, NULL))
      {
         DC_UnlockDc(Dc);
         if (hDCRef == NULL)
         {
             NtGdiDeleteObjectApp(tempHDC);
         }
         DPRINT1("Can not Create EnhMetaFile\n");
         SetLastWin32Error(ERROR_PATH_NOT_FOUND);
         return NULL;
      }

      InitializeObjectAttributes(&ObjectAttributes, &NtPathU, 0, NULL, NULL);

      Status = NtCreateFile (&Dc->hFile, dwDesiredAccess, &ObjectAttributes, &IoStatusBlock,
                             NULL, FileAttributes, 0, FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE,
                             NULL, 0);

      RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathU.Buffer);

      if (!NT_SUCCESS(Status))
      {
         Dc->hFile = NULL;
         DC_UnlockDc(Dc);
         if (hDCRef == NULL)
         {
             NtGdiDeleteObjectApp(tempHDC);
         }
         DPRINT1("Create EnhMetaFile fail\n");
         SetLastWin32Error(ERROR_INVALID_HANDLE);
         return NULL;
      }

      SetLastWin32Error(IoStatusBlock.Information == FILE_OVERWRITTEN ? ERROR_ALREADY_EXISTS : 0);

      Status = NtWriteFile(Dc->hFile, NULL, NULL, NULL, &Iosb, (PVOID)&Dc->emh, Dc->emh->nSize, NULL, NULL);
      if (Status == STATUS_PENDING)
      {
          Status = NtWaitForSingleObject(Dc->hFile,FALSE,NULL);
          if (NT_SUCCESS(Status))
          {
              Status = Iosb.Status;
          }
      }

      if (NT_SUCCESS(Status))
      {
          ret = tempHDC;
          DC_UnlockDc(Dc);
      }
      else
      {
          Dc->hFile = NULL;
          DPRINT1("Write to EnhMetaFile fail\n");
          SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
          ret = NULL;
          DC_UnlockDc(Dc);
          if (hDCRef == NULL)
          {
             NtGdiDeleteObjectApp(tempHDC);
          }
      }
    }
    else
    {
      DC_UnlockDc(Dc);
    }

    return ret;
}

BOOL
STDCALL
NtGdiDeleteEnhMetaFile(HENHMETAFILE  emf)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiEnumEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  emf,
                          ENHMFENUMPROC  EnhMetaFunc,
                          LPVOID  Data,
                          CONST LPRECT  Rect)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiGdiComment(HDC  hDC,
                     UINT  Size,
                     CONST LPBYTE  Data)
{
  UNIMPLEMENTED;
  return FALSE;
}

HENHMETAFILE
STDCALL
NtGdiGetEnhMetaFile(LPCWSTR  MetaFile)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetEnhMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetEnhMetaFileDescription(HENHMETAFILE  hemf,
                                    UINT  BufSize,
                                    LPWSTR  Description)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetEnhMetaFileHeader(HENHMETAFILE  hemf,
                               UINT  BufSize,
                               LPENHMETAHEADER  emh)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetEnhMetaFilePaletteEntries(HENHMETAFILE  hemf,
                                       UINT  Entries,
                                       LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL
STDCALL
NtGdiPlayEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  hemf,
                          CONST PRECT  Rect)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiPlayEnhMetaFileRecord(HDC  hDC,
                                LPHANDLETABLE  Handletable,
                                CONST ENHMETARECORD *EnhMetaRecord,
                                UINT  Handles)
{
  UNIMPLEMENTED;
  return FALSE;
}

HENHMETAFILE
STDCALL
NtGdiSetEnhMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Data)
{
  UNIMPLEMENTED;
  return 0;
}

/* EOF */
