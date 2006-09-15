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
  UNIMPLEMENTED;
  return 0;
}

HMETAFILE
STDCALL
NtGdiCloseMetaFile(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

HENHMETAFILE
STDCALL
NtGdiCopyEnhMetaFile(HENHMETAFILE  Src,
                                  LPCWSTR  File)
{
  UNIMPLEMENTED;
  return 0;
}

HMETAFILE
STDCALL
NtGdiCopyMetaFile(HMETAFILE  Src,
                            LPCWSTR  File)
{
  UNIMPLEMENTED;
  return 0;
}

HDC
STDCALL
NtGdiCreateEnhMetaFile(HDC  hDCRef,
                           LPCWSTR  File,
                           CONST LPRECT  Rect,
                           LPCWSTR  Description)
{
   PDC Dc;   
   HDC ret = NULL;
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;	   
   DWORD length = 0;
   HDC tempHDC;
   DWORD MemSize;
  
   tempHDC = hDCRef;
   if (hDCRef == NULL)
   {
	   /* FIXME ??
	    * Shall we create hdc NtGdiHdcCompatible hdc ?? 
		*/
	   UNICODE_STRING DriverName;
	   RtlInitUnicodeString(&DriverName, L"DISPLAY");
	   tempHDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE);
   }
   
	  
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

   DPRINT1("File cretae unimplement in NtGdiCreateEnhMetaFile\n");
   if (File)  
   {
    


     /* disk based metafile */
	   
      /* FIXME
      if ((Dc->hFile = CreateFileW(File, GENERIC_WRITE | GENERIC_READ, 0,
				 NULL, CREATE_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE) 
      {
           DC_UnLockDc(Dc);
           if (hDCRef == NULL)
           {
               NtGdiDeleteObjectApp(tempHDC);        
           }
           return 0;
      }
      */

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
		  //ret = Dc->hSelf;
		  ret = tempHDC;
		  DC_UnlockDc(Dc);
      }
      else
      {
          Dc->hFile = NULL;
          /* FIXME use SetLastErrorByStatus */
          //SetLastErrorByStatus(Status);
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




HDC
STDCALL
NtGdiCreateMetaFile(LPCWSTR  File)
{
  UNIMPLEMENTED;
  return 0;
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
NtGdiDeleteMetaFile(HMETAFILE  mf)
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
NtGdiEnumMetaFile(HDC  hDC,
                       HMETAFILE  mf,
                       MFENUMPROC  MetaFunc,
                       LPARAM  lParam)
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

HMETAFILE
STDCALL
NtGdiGetMetaFile(LPCWSTR  MetaFile)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetMetaFileBitsEx(HMETAFILE  hmf,
                            UINT  Size,
                            LPVOID  Data)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetWinMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer,
                             INT  MapMode,
                             HDC  Ref)
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

BOOL
STDCALL
NtGdiPlayMetaFile(HDC  hDC,
                       HMETAFILE  hmf)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiPlayMetaFileRecord(HDC  hDC,
                             LPHANDLETABLE  Handletable,
                             LPMETARECORD  MetaRecord,
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

HMETAFILE
STDCALL
NtGdiSetMetaFileBitsEx(UINT  Size,
                                 CONST PBYTE  Data)
{
  UNIMPLEMENTED;
  return 0;
}

HENHMETAFILE
STDCALL
NtGdiSetWinMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Buffer,
                                     HDC  Ref,
//                                     CONST METAFILEPICT *mfp)
				     PVOID mfp)
{
  UNIMPLEMENTED;
  return 0;
}

/* EOF */
