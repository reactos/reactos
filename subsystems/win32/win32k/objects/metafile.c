/*
 * PROJECT:         ReactOS Win32k Subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32k/objects/metafile.c
 * PURPOSE:         Metafile Implementation
 * PROGRAMMERS:     ...
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* System Service Calls ******************************************************/

/*
 * @unimplemented
 */
LONG
APIENTRY
NtGdiConvertMetafileRect(IN HDC hDC,
                         IN OUT PRECTL pRect)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
HDC
APIENTRY
NtGdiCreateMetafileDC(IN HDC hdc)
{

    UNIMPLEMENTED;
    return NULL;


#if 0
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
      OBJECT_ATTRIBUTES ObjectAttributes;
      IO_STATUS_BLOCK IoStatusBlock;
      IO_STATUS_BLOCK Iosb;
      UNICODE_STRING NtPathU;
      NTSTATUS Status;
      ULONG FileAttributes = (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY);

      DPRINT1("Trying Create EnhMetaFile\n");

      /* disk based metafile */
      dwDesiredAccess = GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES;

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
#endif
}

/*
 * @unimplemented
 */
HANDLE
APIENTRY
NtGdiCreateServerMetaFile(IN DWORD iType,
                          IN ULONG cjData,
                          IN PBYTE pjData,
                          IN DWORD mm,
                          IN DWORD xExt,
                          IN DWORD yExt)
{
    UNIMPLEMENTED;
    return NULL;
}
 
/*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiGetServerMetaFileBits(IN HANDLE hmo,
                           IN ULONG cjData,
                           OUT OPTIONAL PBYTE pjData,
                           OUT PDWORD piType,
                           OUT PDWORD pmm,
                           OUT PDWORD pxExt,
                           OUT PDWORD pyExt)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
