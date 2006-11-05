/*
 * ReactOS Boot video driver
 *
 * Copyright (C) 2003 Casper S. Hornstroup
 * Copyright (C) 2004, 2005 Filip Navara
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
 * $Id$
 */

/* INCLUDES ******************************************************************/

#include "bootvid.h"
#include "resource.h"

#define NDEBUG
#include <debug.h>

//#define USE_PROGRESS_BAR

/* GLOBALS *******************************************************************/

static volatile LONG ShutdownNotify;
static KEVENT ShutdownCompleteEvent;

/* DATA **********************************************************************/

static RGBQUAD _MainPalette[16];
static UCHAR _Square1[9 * 4];
static UCHAR _Square2[9 * 4];
static UCHAR _Square3[9 * 4];

/* FUNCTIONS *****************************************************************/

static VOID NTAPI
BootVidAnimationThread(PVOID Ignored)
{
   UCHAR PaletteBitmapBuffer[sizeof(BITMAPINFOHEADER) + sizeof(_MainPalette)];
   PBITMAPINFOHEADER PaletteBitmap = (PBITMAPINFOHEADER)PaletteBitmapBuffer;
   LPRGBQUAD Palette = (LPRGBQUAD)(PaletteBitmapBuffer + sizeof(BITMAPINFOHEADER));
   ULONG Iteration, Index, ClrUsed;
   UINT AnimBarPos;
   LARGE_INTEGER Interval;
   
   /*
    * Build a bitmap containing the fade in palette. The palette entries
    * are then processed in a loop and set using VidBitBlt function.
    */
   
   ClrUsed = sizeof(_MainPalette) / sizeof(_MainPalette[0]);
   RtlZeroMemory(PaletteBitmap, sizeof(BITMAPINFOHEADER));
   PaletteBitmap->biSize = sizeof(BITMAPINFOHEADER);
   PaletteBitmap->biBitCount = 4; 
   PaletteBitmap->biClrUsed = ClrUsed;

   /*
    * Main animation loop.
    */

   for (Iteration = 0, AnimBarPos = 0; !ShutdownNotify; Iteration++)
   {
      if (Iteration <= PALETTE_FADE_STEPS)
      {
         for (Index = 0; Index < ClrUsed; Index++)
         {
            Palette[Index].rgbRed =
               _MainPalette[Index].rgbRed * Iteration / PALETTE_FADE_STEPS;
            Palette[Index].rgbGreen =
               _MainPalette[Index].rgbGreen * Iteration / PALETTE_FADE_STEPS;
            Palette[Index].rgbBlue =
               _MainPalette[Index].rgbBlue * Iteration / PALETTE_FADE_STEPS;
         }

         VidBitBlt(PaletteBitmapBuffer, 0, 0);
      }
#ifdef USE_PROGRESS_BAR
      else
      {
         break;
      }

      Interval.QuadPart = -PALETTE_FADE_TIME;
#else

#if 0
      if (AnimBarPos == 0)
      {
         VidSolidColorFill(0x173, 354, 0x178, 354 + 9, 0);
      }
      else if (AnimBarPos > 3)
      {
         VidSolidColorFill(0xe3 + AnimBarPos * 8, 354,
                           0xe8 + AnimBarPos * 8, 354 + 9,
                           0);
      }

      if (AnimBarPos >= 3)
      {
         VidBufferToScreenBlt(_Square1, 0xeb + AnimBarPos * 8, 354, 6, 9, 4);
      }
      if (AnimBarPos >= 2 && AnimBarPos <= 16)
      {
         VidBufferToScreenBlt(_Square2, 0xf3 + AnimBarPos * 8, 354, 6, 9, 4);
      }
      if (AnimBarPos >= 1 && AnimBarPos <= 15)
      {
         VidBufferToScreenBlt(_Square3, 0xfb + AnimBarPos * 8, 354, 6, 9, 4);
      }
#endif

      if (Iteration <= PALETTE_FADE_STEPS)
      {
         Interval.QuadPart = -PALETTE_FADE_TIME;
         if ((Iteration % 5) == 0)
            AnimBarPos++;
      }
      else
      {
         Interval.QuadPart = -PALETTE_FADE_TIME * 5;
         AnimBarPos++;
      }
      AnimBarPos = Iteration % 18;
#endif

      /* Wait for a bit. */
      KeDelayExecutionThread(KernelMode, FALSE, &Interval);
   }

   DPRINT("Finishing bootvid thread.\n");
   KeSetEvent(&ShutdownCompleteEvent, 0, FALSE);

   PsTerminateSystemThread(0);
}


NTSTATUS NTAPI
BootVidDisplayBootLogo(PVOID ImageBase)
{
   PBITMAPINFOHEADER BitmapInfoHeader;
   LPRGBQUAD Palette;
   static const ULONG BitmapIds[2] = {IDB_BOOTIMAGE, IDB_BAR};
   PUCHAR BitmapData[2];
   ULONG Index;
   HANDLE BitmapThreadHandle;
   CLIENT_ID BitmapThreadId;
   NTSTATUS Status;

   KeInitializeEvent(&ShutdownCompleteEvent, NotificationEvent, FALSE);

   /*
    * Get the bitmaps from the executable.
    */

   for (Index = 0; Index < sizeof(BitmapIds) / sizeof(BitmapIds[0]); Index++)
   {
      PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
      LDR_RESOURCE_INFO ResourceInfo;
      ULONG Size;

      ResourceInfo.Type = /* RT_BITMAP */ 2;
      ResourceInfo.Name = BitmapIds[Index];
      ResourceInfo.Language = 0x09;

      Status = LdrFindResource_U(
         ImageBase,
         &ResourceInfo,
         RESOURCE_DATA_LEVEL,
         &ResourceDataEntry);

      if (!NT_SUCCESS(Status))
      {
         DPRINT("LdrFindResource_U() failed with status 0x%.08x\n", Status);
         return Status;
      }

      Status = LdrAccessResource(
         ImageBase,
         ResourceDataEntry,
         (PVOID*)&BitmapData[Index],
         &Size);

      if (!NT_SUCCESS(Status))
      {
         DPRINT("LdrAccessResource() failed with status 0x%.08x\n", Status);
         return Status;
      }
   }


   /*
    * Initialize the graphics output.
    */

   if (!VidInitialize(TRUE))
   {
      return STATUS_UNSUCCESSFUL;
   }

   /*
    * Load the bar bitmap and get the square data from it.
    */

   VidBitBlt(BitmapData[1], 0, 0);   
   VidScreenToBufferBlt(_Square1, 0, 0, 6, 9, 4);
   VidScreenToBufferBlt(_Square2, 8, 0, 6, 9, 4);
   VidScreenToBufferBlt(_Square3, 16, 0, 6, 9, 4);

   /*
    * Save the main image palette and replace it with black palette, so
    * we can do fade in effect later.
    */

   BitmapInfoHeader = (PBITMAPINFOHEADER)BitmapData[0];
   Palette = (LPRGBQUAD)(BitmapData[0] + BitmapInfoHeader->biSize);
   RtlCopyMemory(_MainPalette, Palette, sizeof(_MainPalette));
   RtlZeroMemory(Palette, sizeof(_MainPalette));

   /*
    * Display the main image.
    */

   VidBitBlt(BitmapData[0], 0, 0);

   /*
    * Start a thread that handles the fade in and bar animation effects.
    */

   Status = PsCreateSystemThread(
      &BitmapThreadHandle,
      THREAD_ALL_ACCESS,
      NULL,
      NULL,
      &BitmapThreadId,
      BootVidAnimationThread,
      NULL);

   if (!NT_SUCCESS(Status))
   {
      VidCleanUp();
      return Status;
   }

   ZwClose(BitmapThreadHandle);

   return STATUS_SUCCESS;
}


VOID NTAPI
BootVidUpdateProgress(ULONG Progress)
{
#ifdef USE_PROGRESS_BAR
   if (ShutdownNotify == 0)
   {
      VidSolidColorFill(0x103, 354, 0x103 + (Progress * 120 / 100), 354 + 9, 1);
   }
#endif
}


VOID NTAPI
BootVidFinalizeBootLogo(VOID)
{
   InterlockedIncrement(&ShutdownNotify);
   DPRINT1("Waiting for bootvid thread to finish.\n");
   KeWaitForSingleObject(&ShutdownCompleteEvent, Executive, KernelMode,
                         FALSE, NULL);
   DPRINT1("Bootvid thread to finish.\n");
   //VidResetDisplay();
}


NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
   return STATUS_SUCCESS;
}
