/*
 * ReactOS Boot video driver
 *
 * Copyright (C) 2003 Casper S. Hornstroup
 * Copyright (C) 2004 Filip Navara
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id: bootvid.c,v 1.8 2004/05/15 22:45:50 hbirr Exp $
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntbootvid.h>
#include <reactos/resource.h>
#include <rosrtl/string.h>
#include "bootvid.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/*
 * NOTE:
 * This is based on SvgaLib 640x480x16 mode definition with the
 * following changes:
 * - Graphics: Data Rotate (Index 3)
 *   Set to zero to indicate that the data written to video memory by
 *   CPU should be processed unmodified.
 * - Graphics: Mode Register (index 5)
 *   Set to Write Mode 2 and Read Mode 0.
 */

static VGA_REGISTERS Mode12Regs =
{
   /* CRT Controller Registers */
   {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3},
   /* Attribute Controller Registers */
   {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x81, 0x00, 0x0F, 0x00, 0x00},
   /* Graphics Controller Registers */
   {0x00, 0x0F, 0x00, 0x00, 0x00, 0x02, 0x05, 0x0F, 0xFF},
   /* Sequencer Registers */
   {0x03, 0x01, 0x0F, 0x00, 0x06},
   /* Misc Output Register */
   0xE3
};

PUCHAR VideoMemory;

/* Must be 4 bytes per entry */
long maskbit[640];

static HANDLE BitmapThreadHandle;
static CLIENT_ID BitmapThreadId;
static PUCHAR BootimageBitmap;

/* DATA **********************************************************************/

static PDRIVER_OBJECT BootVidDriverObject = NULL;

/* FUNCTIONS *****************************************************************/

STATIC BOOLEAN FASTCALL
InbvFindBootimage()
{
   PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
   LDR_RESOURCE_INFO ResourceInfo;
   NTSTATUS Status;
   PVOID BaseAddress = BootVidDriverObject->DriverStart;
   ULONG Size;

   ResourceInfo.Type = RT_BITMAP;
   ResourceInfo.Name = IDB_BOOTIMAGE;
   ResourceInfo.Language = 0x09;

   Status = LdrFindResource_U(
      BaseAddress,
      &ResourceInfo,
      RESOURCE_DATA_LEVEL,
      &ResourceDataEntry);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("LdrFindResource_U() failed with status 0x%.08x\n", Status);
      return FALSE;
   }

   Status = LdrAccessResource(
      BaseAddress,
      ResourceDataEntry,
      (PVOID*)&BootimageBitmap,
      &Size);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("LdrAccessResource() failed with status 0x%.08x\n", Status);
      return FALSE;
   }

   return TRUE;
}


STATIC BOOLEAN FASTCALL
InbvMapVideoMemory(VOID)
{
   PHYSICAL_ADDRESS PhysicalAddress;

   PhysicalAddress.QuadPart = 0xA0000;
   VideoMemory = MmMapIoSpace(PhysicalAddress, 0x10000, MmNonCached);

   return VideoMemory != NULL;
}


STATIC BOOLEAN FASTCALL
InbvUnmapVideoMemory(VOID)
{
   MmUnmapIoSpace(VideoMemory, 0x10000);
   return TRUE;
}


STATIC VOID FASTCALL
vgaPreCalc()
{
   ULONG j;

   for (j = 0; j < 80; j++)
   {
      maskbit[j * 8 + 0] = 128;
      maskbit[j * 8 + 1] = 64;
      maskbit[j * 8 + 2] = 32;
      maskbit[j * 8 + 3] = 16;
      maskbit[j * 8 + 4] = 8;
      maskbit[j * 8 + 5] = 4;
      maskbit[j * 8 + 6] = 2;
      maskbit[j * 8 + 7] = 1;
   }
}


STATIC VOID FASTCALL
vgaSetRegisters(PVGA_REGISTERS Registers)
{
   int i;

   /* Update misc output register */
   WRITE_PORT_UCHAR(MISC, Registers->Misc);

   /* Synchronous reset on */
   WRITE_PORT_UCHAR(SEQ, 0x00);
   WRITE_PORT_UCHAR(SEQDATA, 0x01);

   /* Write sequencer registers */
   for (i = 1; i < sizeof(Registers->Sequencer); i++)
   {
      WRITE_PORT_UCHAR(SEQ, i);
      WRITE_PORT_UCHAR(SEQDATA, Registers->Sequencer[i]);
   }

   /* Synchronous reset off */
   WRITE_PORT_UCHAR(SEQ, 0x00);
   WRITE_PORT_UCHAR(SEQDATA, 0x03);

   /* Deprotect CRT registers 0-7 */
   WRITE_PORT_UCHAR(CRTC, 0x11);
   WRITE_PORT_UCHAR(CRTCDATA, Registers->CRT[0x11] & 0x7f);

   /* Write CRT registers */
   for (i = 0; i < sizeof(Registers->CRT); i++)
   {
      WRITE_PORT_UCHAR(CRTC, i);
      WRITE_PORT_UCHAR(CRTCDATA, Registers->CRT[i]);
   }

   /* Write graphics controller registers */
   for (i = 0; i < sizeof(Registers->Graphics); i++)
   {
      WRITE_PORT_UCHAR(GRAPHICS, i);
      WRITE_PORT_UCHAR(GRAPHICSDATA, Registers->Graphics[i]);
   }

   /* Write attribute controller registers */
   for (i = 0; i < sizeof(Registers->Attribute); i++)
   {
      READ_PORT_UCHAR(STATUS);
      WRITE_PORT_UCHAR(ATTRIB, i);
      WRITE_PORT_UCHAR(ATTRIB, Registers->Attribute[i]);
   }
}


static VOID
InbvInitVGAMode(VOID)
{
   /* Zero out video memory (clear a possibly trashed screen) */
   RtlZeroMemory(VideoMemory, 0x10000);

   vgaSetRegisters(&Mode12Regs);

   /* Set the PEL mask. */
   WRITE_PORT_UCHAR(PELMASK, 0xff);

   vgaPreCalc();
}


BOOL STDCALL
VidResetDisplay(VOID)
{
   /* 
    * We are only using standard VGA facilities so we can rely on the
    * HAL 'int10mode3' reset to cleanup the hardware state.
    */

   return FALSE;
}


VOID STDCALL
VidCleanUp(VOID)
{
   InbvUnmapVideoMemory();
}


STATIC VOID FASTCALL
InbvSetColor(INT Index, UCHAR Red, UCHAR Green, UCHAR Blue)
{
   WRITE_PORT_UCHAR(PELINDEX, Index);
   WRITE_PORT_UCHAR(PELDATA, Red >> 2);
   WRITE_PORT_UCHAR(PELDATA, Green >> 2);
   WRITE_PORT_UCHAR(PELDATA, Blue >> 2);
}


STATIC VOID FASTCALL
InbvSetBlackPalette()
{
   register ULONG r = 0;

   /* Disable screen and enable palette access. */
   READ_PORT_UCHAR(STATUS);
   WRITE_PORT_UCHAR(ATTRIB, 0x00);

   for (r = 0; r < 16; r++)
   {
      InbvSetColor(r, 0, 0, 0);
   }

   /* Enable screen and enable palette access. */
   READ_PORT_UCHAR(STATUS);
   WRITE_PORT_UCHAR(ATTRIB, 0x20);
}


STATIC VOID FASTCALL
InbvDisplayBitmap(ULONG Width, ULONG Height, PCHAR ImageData)
{
   ULONG j, k, y;
   register ULONG i;
   register ULONG x;
   register ULONG c;

   k = 0;
   for (y = 0; y < Height; y++)
   {
      for (j = 0; j < 8; j++)
      {
         x = j;

         /*
          * Loop through the line and process every 8th pixel.
          * This way we can get a way with using the same bit mask
          * for several pixels and thus not need to do as much I/O
          * communication.
          */
         while (x < 640)
         {
            c = 0;

            if (x < Width)
            {
               c = ImageData[k + x];
               for (i = 1; i < 4; i++)
               {
                  if (x + i * 8 < Width)
                  {
                     c |= (ImageData[k + x + i * 8] << i * 8);
                  }
               }
            }

            InbvPutPixels(x, 479 - y, c);
            x += 8 * 4;
         }
      }
      k += Width;
   }
}


STATIC VOID FASTCALL
InbvDisplayCompressedBitmap()
{
   PBITMAPV5HEADER bminfo;
   ULONG i,j,k;
   ULONG x,y;
   ULONG curx,cury;
   ULONG bfOffBits;
   ULONG clen;
   PCHAR ImageData;

   bminfo = (PBITMAPV5HEADER) &BootimageBitmap[0];
   DPRINT("bV5Size = %d\n", bminfo->bV5Size);
   DPRINT("bV5Width = %d\n", bminfo->bV5Width);
   DPRINT("bV5Height = %d\n", bminfo->bV5Height);
   DPRINT("bV5Planes = %d\n", bminfo->bV5Planes);
   DPRINT("bV5BitCount = %d\n", bminfo->bV5BitCount);
   DPRINT("bV5Compression = %d\n", bminfo->bV5Compression);
   DPRINT("bV5SizeImage = %d\n", bminfo->bV5SizeImage);
   DPRINT("bV5XPelsPerMeter = %d\n", bminfo->bV5XPelsPerMeter);
   DPRINT("bV5YPelsPerMeter = %d\n", bminfo->bV5YPelsPerMeter);
   DPRINT("bV5ClrUsed = %d\n", bminfo->bV5ClrUsed);
   DPRINT("bV5ClrImportant = %d\n", bminfo->bV5ClrImportant);

   bfOffBits = bminfo->bV5Size + bminfo->bV5ClrUsed * sizeof(RGBQUAD);
   DPRINT("bfOffBits = %d\n", bfOffBits);
   DPRINT("size of color indices = %d\n", bminfo->bV5ClrUsed * sizeof(RGBQUAD));
   DPRINT("first byte of data = %d\n", BootimageBitmap[bfOffBits]);

   InbvSetBlackPalette();

   ImageData = ExAllocatePool(NonPagedPool, bminfo->bV5Width * bminfo->bV5Height);
   RtlZeroMemory(ImageData, bminfo->bV5Width * bminfo->bV5Height);

   /*
    * ImageData has 1 pixel per byte.
    * bootimage has 2 pixels per byte.
    */

   if (bminfo->bV5Compression == 2)
   {
      k = 0;
      j = 0;
      while ((j < bminfo->bV5SizeImage) && (k < (ULONG) (bminfo->bV5Width * bminfo->bV5Height)))
      {
         unsigned char b;
    
         clen = BootimageBitmap[bfOffBits + j];
         j++;
    
         if (clen > 0)
         {
            /* Encoded mode */
    
            b = BootimageBitmap[bfOffBits + j];
            j++;
    
            for (i = 0; i < (clen / 2); i++)
            {
               ImageData[k] = (b & 0xf0) >> 4;
               k++;
               ImageData[k] = b & 0xf;
               k++;
            }
            if ((clen & 1) > 0)
            {
               ImageData[k] = (b & 0xf0) >> 4;
               k++;
            }
         }
         else
         {
            /* Absolute mode */
            b = BootimageBitmap[bfOffBits + j];
            j++;
    
            if (b == 0)
            {
               /* End of line */
            }
            else if (b == 1)
            {
               /* End of image */
               break;
            }
            else if (b == 2)
            {
               x = BootimageBitmap[bfOffBits + j];
               j++;
               y = BootimageBitmap[bfOffBits + j];
               j++;
               curx = k % bminfo->bV5Width;
               cury = k / bminfo->bV5Width;
               k = (cury + y) * bminfo->bV5Width + (curx + x);
            }
            else
            {
               if ((j & 1) > 0)
               {
                  DPRINT("Unaligned copy!\n");
               }
    
               clen = b;
               for (i = 0; i < (clen / 2); i++)
               {
                  b = BootimageBitmap[bfOffBits + j];
                  j++;
       
                  ImageData[k] = (b & 0xf0) >> 4;
                  k++;
                  ImageData[k] = b & 0xf;
                  k++;
               }
               if ((clen & 1) > 0)
               {
                  b = BootimageBitmap[bfOffBits + j];
                  j++;
                  ImageData[k] = (b & 0xf0) >> 4;
                  k++;
               }
               /* Word align */
               j += (j & 1);
            }
         }
      }

      InbvDisplayBitmap(bminfo->bV5Width, bminfo->bV5Height, ImageData);
   }
   else
   {
      DbgPrint("Warning boot image need to be compressed using RLE4\n");
   }

   ExFreePool(ImageData);
}


STATIC VOID FASTCALL
InbvFadeUpPalette()
{
   PBITMAPV5HEADER bminfo;
   PRGBQUAD Palette;
   ULONG i;
   unsigned char r, g, b;
   register ULONG c;
   LARGE_INTEGER Interval;
   FADER_PALETTE_ENTRY FaderPalette[16];
   FADER_PALETTE_ENTRY FaderPaletteDelta[16];

   RtlZeroMemory(&FaderPalette, sizeof(FaderPalette));
   RtlZeroMemory(&FaderPaletteDelta, sizeof(FaderPaletteDelta));

   bminfo = (PBITMAPV5HEADER)&BootimageBitmap[0];
   Palette = (PRGBQUAD)&BootimageBitmap[bminfo->bV5Size];

   for (i = 0; i < 16; i++)
   {
      if (i < bminfo->bV5ClrUsed)
      {
         FaderPaletteDelta[i].r = ((Palette[i].rgbRed << 8) / PALETTE_FADE_STEPS);
         FaderPaletteDelta[i].g = ((Palette[i].rgbGreen << 8) / PALETTE_FADE_STEPS);
         FaderPaletteDelta[i].b = ((Palette[i].rgbBlue << 8) / PALETTE_FADE_STEPS);
      }
   }

   for (i = 0; i < PALETTE_FADE_STEPS; i++)
   {
      /* Disable screen and enable palette access. */
      READ_PORT_UCHAR(STATUS);
      WRITE_PORT_UCHAR(ATTRIB, 0x00);

      for (c = 0; c < bminfo->bV5ClrUsed; c++)
      {
         /* Add the delta */
         FaderPalette[c].r += FaderPaletteDelta[c].r;
         FaderPalette[c].g += FaderPaletteDelta[c].g;
         FaderPalette[c].b += FaderPaletteDelta[c].b;

         /* Get the integer values */
         r = FaderPalette[c].r >> 8;
         g = FaderPalette[c].g >> 8;
         b = FaderPalette[c].b >> 8;

         /* Don't go too far */
         if (r > Palette[c].rgbRed)
            r = Palette[c].rgbRed;
         if (g > Palette[c].rgbGreen)
            g = Palette[c].rgbGreen;
         if (b > Palette[c].rgbBlue)
            b = Palette[c].rgbBlue;

         /* Update the hardware */
         InbvSetColor(c, r, g, b);
      }

      /* Enable screen and disable palette access. */
      READ_PORT_UCHAR(STATUS);
      WRITE_PORT_UCHAR(ATTRIB, 0x20);

      /* Wait for a bit. */
      Interval.QuadPart = -PALETTE_FADE_TIME;
      KeDelayExecutionThread(KernelMode, FALSE, &Interval);
   }
}


STATIC VOID STDCALL
InbvBitmapThreadMain(PVOID Ignored)
{
   if (InbvFindBootimage())
   {
      InbvDisplayCompressedBitmap();
      InbvFadeUpPalette();
   }
   else
   {
      DbgPrint("Warning: Cannot find boot image\n");
   }
}


STATIC BOOLEAN STDCALL
VidInitialize(VOID)
{
   NTSTATUS Status;

   InbvMapVideoMemory();
   InbvInitVGAMode();
  
   Status = PsCreateSystemThread(
      &BitmapThreadHandle,
      THREAD_ALL_ACCESS,
      NULL,
      NULL,
      &BitmapThreadId,
      InbvBitmapThreadMain,
      NULL);
 
   if (!NT_SUCCESS(Status))
   {
      return FALSE;
   }

   NtClose(BitmapThreadHandle);

   return TRUE;
}


NTSTATUS STDCALL
VidDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   PIO_STACK_LOCATION IrpSp;
   NTSTATUS Status;
   NTBOOTVID_FUNCTION_TABLE* FunctionTable;
 
   IrpSp = IoGetCurrentIrpStackLocation(Irp);
   Status = STATUS_SUCCESS;

   switch(IrpSp->MajorFunction)
   {
      /* Opening and closing handles to the device */
      case IRP_MJ_CREATE:
      case IRP_MJ_CLOSE:
         break;

      case IRP_MJ_DEVICE_CONTROL:
         switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
         {
            case IOCTL_BOOTVID_INITIALIZE:
               VidInitialize();
               FunctionTable = (NTBOOTVID_FUNCTION_TABLE *)
                  Irp->AssociatedIrp.SystemBuffer;
               FunctionTable->ResetDisplay = VidResetDisplay;
               break;

            case IOCTL_BOOTVID_CLEANUP:
               VidCleanUp();	  
               break;

            default:
               Status = STATUS_NOT_IMPLEMENTED;
               break;
         }
         break;

      /* Unsupported operations */
      default:
         Status = STATUS_NOT_IMPLEMENTED;
   }

   Irp->IoStatus.Status = Status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return Status;
}


NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
   PDEVICE_OBJECT BootVidDevice;
   UNICODE_STRING DeviceName;
   NTSTATUS Status;

   BootVidDriverObject = DriverObject;

   /* Register driver routines */
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = VidDispatch;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = VidDispatch;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VidDispatch;
   DriverObject->DriverUnload = NULL;

   DriverObject->Flags |= DO_BUFFERED_IO;

   /* Create device */
   RtlRosInitUnicodeStringFromLiteral(&DeviceName, L"\\Device\\BootVid");

   Status = IoCreateDevice(
      DriverObject,
      0,
      &DeviceName,
      FILE_DEVICE_BOOTVID,
      0,
      FALSE,
      &BootVidDevice);

   return Status;
}
