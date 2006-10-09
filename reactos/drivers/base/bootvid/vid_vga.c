/*
 * ReactOS Boot video driver
 *
 * Copyright (C) 2005 Filip Navara
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
 */

/* INCLUDES ******************************************************************/

#include "bootvid.h"
#define NDEBUG
#include <debug.h>

/* TYPES AND DEFINITIONS *****************************************************/

typedef struct _VGA_REGISTERS
{
   UCHAR CRT[24];
   UCHAR Attribute[21];
   UCHAR Graphics[9];
   UCHAR Sequencer[5];
   UCHAR Misc;
} VGA_REGISTERS, *PVGA_REGISTERS;

/* VGA registers */
#define MISC         (PUCHAR)0x3c2
#define SEQ          (PUCHAR)0x3c4
#define SEQDATA      (PUCHAR)0x3c5
#define CRTC         (PUCHAR)0x3d4
#define CRTCDATA     (PUCHAR)0x3d5
#define GRAPHICS     (PUCHAR)0x3ce
#define GRAPHICSDATA (PUCHAR)0x3cf
#define ATTRIB       (PUCHAR)0x3c0
#define STATUS       (PUCHAR)0x3da
#define PELMASK      (PUCHAR)0x3c6
#define PELINDEX     (PUCHAR)0x3c8
#define PELDATA      (PUCHAR)0x3c9

/* GLOBALS *******************************************************************/

/*
 * NOTE:
 * This is based on SvgaLib 640x480x16 mode definition with the
 * following changes:
 * - Graphics: Data Rotate (Index 3)
 *   Set to zero to indicate that the data written to video memory by
 *   CPU should be processed unmodified.
 * - Graphics: Mode Register (Index 5)
 *   Set to Write Mode 2 and Read Mode 0.
 */

static const VGA_REGISTERS VidpMode12Regs =
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

static const RGBQUAD DefaultPalette[] =
{
   {0, 0, 0},
   {0, 0, 0x80},
   {0, 0x80, 0},
   {0, 0x80, 0x80},
   {0x80, 0, 0},
   {0x80, 0, 0x80},
   {0x80, 0x80, 0},
   {0x80, 0x80, 0x80},
   {0xC0, 0xC0, 0xC0},
   {0, 0, 0xFF},
   {0, 0xFF, 0},
   {0, 0xFF, 0xFF},
   {0xFF, 0, 0},
   {0xFF, 0, 0xFF},
   {0xFF, 0xFF, 0},
   {0xFF, 0xFF, 0xFF}
};

static BOOLEAN VidpInitialized = FALSE;
static PUCHAR VidpMemory;
static CHAR VidpMaskBit[640];
static ULONG VidpCurrentX;
static ULONG VidpCurrentY;

/* FUNCTIONS *****************************************************************/

static VOID FASTCALL
VidpSetRegisters(const VGA_REGISTERS *Registers)
{
   UINT i;

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

   /* Set the PEL mask. */
   WRITE_PORT_UCHAR(PELMASK, 0xff);
}

static BOOLEAN NTAPI
VidVgaInitialize(
   IN BOOLEAN SetMode)
{
   ULONG Index;
   PHYSICAL_ADDRESS PhysicalAddress;

   if (!VidpInitialized)
   {
      PhysicalAddress.QuadPart = 0xA0000;
      VidpMemory = MmMapIoSpace(PhysicalAddress, 0x10000, MmNonCached);
      if (VidpMemory == NULL)
         return FALSE;

      for (Index = 0; Index < 80; Index++)
      {
         VidpMaskBit[Index * 8 + 0] = 128;
         VidpMaskBit[Index * 8 + 1] = 64;
         VidpMaskBit[Index * 8 + 2] = 32;
         VidpMaskBit[Index * 8 + 3] = 16;
         VidpMaskBit[Index * 8 + 4] = 8;
         VidpMaskBit[Index * 8 + 5] = 4;
         VidpMaskBit[Index * 8 + 6] = 2;
         VidpMaskBit[Index * 8 + 7] = 1;
      }

      VidpInitialized = TRUE;
   }

   if (SetMode)
   {
      VidpSetRegisters(&VidpMode12Regs);
      VidpCurrentX = VidpCurrentY = 0;

      /* Disable screen and enable palette access. */
      READ_PORT_UCHAR(STATUS);
      WRITE_PORT_UCHAR(ATTRIB, 0x00);

      for (Index = 0; Index < sizeof(DefaultPalette) / sizeof(RGBQUAD); Index++)
      {
         WRITE_PORT_UCHAR(PELINDEX, Index);
         WRITE_PORT_UCHAR(PELDATA, DefaultPalette[Index].rgbRed >> 2);
         WRITE_PORT_UCHAR(PELDATA, DefaultPalette[Index].rgbGreen >> 2);
         WRITE_PORT_UCHAR(PELDATA, DefaultPalette[Index].rgbBlue >> 2);
      }

      /* Enable screen and disable palette access. */
      READ_PORT_UCHAR(STATUS);
      WRITE_PORT_UCHAR(ATTRIB, 0x20);
   }

   return TRUE;
}

static VOID STDCALL
VidVgaResetDisplay(VOID)
{
   VidVgaInitialize(TRUE);
}

static VOID NTAPI
VidVgaCleanUp(VOID)
{
   if (VidpInitialized)
   {
      MmUnmapIoSpace(VidpMemory, 0x10000);
      VidpInitialized = FALSE;
   }
}

static VOID NTAPI
VidVgaBufferToScreenBlt(
   IN PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN ULONG Delta)
{
   ULONG x, y;
   PUCHAR BufferPtr;
   ULONG VidOffset;

   for (x = Left; x < Left + Width; x++)
   {
      WRITE_PORT_UCHAR(GRAPHICS, 0x08);
      WRITE_PORT_UCHAR(GRAPHICSDATA, VidpMaskBit[x]);

      BufferPtr = Buffer;
      VidOffset = (x >> 3) + (Top * 80);

      if (((x - Left) % 2) == 0)
      {
         for (y = Top; y < Top + Height; y++)
         {
            READ_REGISTER_UCHAR(VidpMemory + VidOffset);
            WRITE_REGISTER_UCHAR(VidpMemory + VidOffset, *BufferPtr >> 4);
            VidOffset += 80;
            BufferPtr += Delta;
         }
      }
      else
      {
         for (y = Top; y < Top + Height; y++)
         {
            READ_REGISTER_UCHAR(VidpMemory + VidOffset);
            WRITE_REGISTER_UCHAR(VidpMemory + VidOffset, *BufferPtr & 0xf);
            VidOffset += 80;
            BufferPtr += Delta;
         }

         Buffer++;
      }
   }
}

static VOID NTAPI
VidVgaScreenToBufferBlt(
   OUT PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN ULONG Delta)
{
   UCHAR Plane;
   UCHAR b;
   ULONG x, y;

   /* Reset the destination. */
   RtlZeroMemory(Buffer, (Delta > 0 ? Delta : -Delta) * Height);

   for (Plane = 0; Plane < 4; Plane++)
   {
      WRITE_PORT_UCHAR(GRAPHICS, 0x04);
      WRITE_PORT_UCHAR(GRAPHICSDATA, Plane);

      for (y = Top; y < Top + Height; y++)
      {
         for (x = Left; x < Left + Width; x++)
         {
            b = READ_REGISTER_UCHAR(VidpMemory + (y * 80 + (x >> 3)));
            b >>= 7 - (x & 7);
            b &= 1;
            b <<= Plane + ((~(x - Left) & 1) << 2);
            Buffer[(y - Top) * Delta + ((x - Left) >> 1)] |= b;
         }
      }
   }
}

static VOID NTAPI
VidVgaBitBlt(
   IN PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top)
{
   PBITMAPINFOHEADER BitmapInfoHeader;
   LPRGBQUAD Palette;
   ULONG bfOffBits;
   UCHAR ClrUsed;
   ULONG Index;
   ULONG Delta;

   BitmapInfoHeader = (PBITMAPINFOHEADER)Buffer;
   Palette = (LPRGBQUAD)(Buffer + BitmapInfoHeader->biSize);

   if (BitmapInfoHeader->biClrUsed)
      ClrUsed = BitmapInfoHeader->biClrUsed;
   else
      ClrUsed = 1 << BitmapInfoHeader->biBitCount;

   bfOffBits = BitmapInfoHeader->biSize + ClrUsed * sizeof(RGBQUAD);

   /* Disable screen and enable palette access. */
   READ_PORT_UCHAR(STATUS);
   WRITE_PORT_UCHAR(ATTRIB, 0x00);

   for (Index = 0; Index < ClrUsed; Index++)
   {
      WRITE_PORT_UCHAR(PELINDEX, Index);
      WRITE_PORT_UCHAR(PELDATA, Palette[Index].rgbRed >> 2);
      WRITE_PORT_UCHAR(PELDATA, Palette[Index].rgbGreen >> 2);
      WRITE_PORT_UCHAR(PELDATA, Palette[Index].rgbBlue >> 2);
   }

   /* Enable screen and disable palette access. */
   READ_PORT_UCHAR(STATUS);
   WRITE_PORT_UCHAR(ATTRIB, 0x20);
   
   if (BitmapInfoHeader->biCompression == 2)
   {
      PUCHAR OutputBuffer;
      ULONG InputPos, OutputPos;
      ULONG x, y;
      UCHAR b;
      ULONG Length;

      Delta = (BitmapInfoHeader->biWidth + 1) >> 1;
      OutputBuffer = ExAllocatePool(NonPagedPool, Delta * BitmapInfoHeader->biHeight);
      RtlZeroMemory(OutputBuffer, Delta * BitmapInfoHeader->biHeight);
      OutputPos = InputPos = 0;
      Buffer += bfOffBits;

      while (InputPos < BitmapInfoHeader->biSizeImage &&
             OutputPos < Delta * BitmapInfoHeader->biHeight * 2)
      {
         Length = Buffer[InputPos++];
         if (Length > 0)
         {
            /* Encoded mode */
            b = Buffer[InputPos++];
            if (OutputPos % 2)
            {
               OutputBuffer[OutputPos >> 1] |= b & 0xf;
               b = (b >> 4) | (b << 4);
               Length--;
               OutputPos++;
            }

            memset(OutputBuffer + (OutputPos >> 1), b, Length / 2);
            OutputPos += Length;

            if (Length & 1)
            {
               OutputBuffer[OutputPos >> 1] |= b & 0xf;
               OutputPos++;
            }
         }
         else
         {
            /* Absolute mode */
            b = Buffer[InputPos++];
            if (b == 0)
            {
               /* End of line */
               if (OutputPos % Delta)
                  OutputPos = ((OutputPos / Delta) + 1) * Delta;
            }
            else if (b == 1)
            {
               /* End of image */
               break;
            }
            else if (b == 2)
            {
               x = Buffer[InputPos++];
               y = Buffer[InputPos++];
               OutputPos = ((OutputPos / Delta) + y) * Delta +
                           ((OutputPos % Delta) + x);
            }
            else
            {
               Length = b;
               if (Length)
               {
                  if (OutputPos % 2)
                  {
                     ASSERT(FALSE);
                  }

                  for (Index = 0; Index < (Length / 2); Index++)
                  {
                     b = Buffer[InputPos++];
                     OutputBuffer[OutputPos >> 1] = b;
                     OutputPos += 2;
                  }
                  if (Length & 1)
                  {
                     b = Buffer[InputPos++];
                     OutputBuffer[OutputPos >> 1] |= b >> 4;
                     OutputPos++;
                  }
               }
    
               /* Word align */
               InputPos += (InputPos & 1);
            }
         }
      }

      VidBufferToScreenBlt(OutputBuffer + 
                           (Delta * (BitmapInfoHeader->biHeight - 1)),
                           0, 0, BitmapInfoHeader->biWidth,
                           BitmapInfoHeader->biHeight, -Delta);
      
      ExFreePool(OutputBuffer);
   }
   else
   {
      Delta = ((BitmapInfoHeader->biWidth + 31) & ~31) >> 1;
      if (BitmapInfoHeader->biHeight < 0)
      {
         VidBufferToScreenBlt(Buffer + bfOffBits,
                              0, 0, BitmapInfoHeader->biWidth,
                              -BitmapInfoHeader->biHeight, Delta);
      }
      else
      {
         VidBufferToScreenBlt(Buffer + bfOffBits +
                              (Delta * (BitmapInfoHeader->biHeight - 1)),
                              0, 0, BitmapInfoHeader->biWidth,
                              BitmapInfoHeader->biHeight, -Delta);
      }
   }
}

static VOID NTAPI
VidVgaSolidColorFill(
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Right,
   IN ULONG Bottom,
   IN ULONG Color)
{
   ULONG x, y;
   ULONG VidOffset;

   for (x = Left; x <= Right; x++)
   {
      WRITE_PORT_UCHAR(GRAPHICS, 0x08);
      WRITE_PORT_UCHAR(GRAPHICSDATA, VidpMaskBit[x]);

      VidOffset = (x >> 3) + (Top * 80);

      for (y = Top; y <= Bottom; y++)
      {
         READ_REGISTER_UCHAR(VidpMemory + VidOffset);
         WRITE_REGISTER_UCHAR(VidpMemory + VidOffset, Color & 0xF);
         VidOffset += 80;
      }
   }
}

static VOID NTAPI
VidVgaDisplayString(
   IN PUCHAR String)
{
}

VID_FUNCTION_TABLE VidVgaTable =
{
   VidVgaInitialize,
   VidVgaCleanUp,
   VidVgaResetDisplay,
   VidVgaBufferToScreenBlt,
   VidVgaScreenToBufferBlt,
   VidVgaBitBlt,
   VidVgaSolidColorFill,
   VidVgaDisplayString
};
