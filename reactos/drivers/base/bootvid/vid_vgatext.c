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

#define CRTC_COLUMNS       0x01
#define CRTC_OVERFLOW      0x07
#define CRTC_ROWS          0x12
#define CRTC_SCANLINES     0x09
#define CRTC_CURHI         0x0e
#define CRTC_CURLO         0x0f

#define CHAR_ATTRIBUTE_BLACK  0x00  /* black on black */
#define CHAR_ATTRIBUTE        0x1F  /* grey on blue */

/* GLOBALS *******************************************************************/

static BOOLEAN VidpInitialized = FALSE;
static PUCHAR VidpMemory;
static ULONG SizeX, SizeY;

/* FUNCTIONS *****************************************************************/

static VOID NTAPI
VidpVgaTextClearDisplay(VOID)
{
   volatile WORD *ptr = (volatile WORD*)VidpMemory;
   ULONG i;

   for (i = 0; i < SizeX * SizeY; i++, ptr++)
      *ptr = (0x1700 + ' ');
}

static BOOLEAN NTAPI
VidVgaTextInitialize(
   IN BOOLEAN SetMode)
{
   ULONG ScanLines;
   PHYSICAL_ADDRESS PhysicalAddress;
   UCHAR Data;

   if (!VidpInitialized)
   {
      PhysicalAddress.QuadPart = 0xB8000;
      VidpMemory = MmMapIoSpace(PhysicalAddress, 0x2000, MmNonCached);
      if (VidpMemory == NULL)
         return FALSE;
      VidpInitialized = TRUE;

      WRITE_PORT_UCHAR(CRTC, CRTC_COLUMNS);
      SizeX = READ_PORT_UCHAR(CRTCDATA) + 1;
      WRITE_PORT_UCHAR(CRTC, CRTC_ROWS);
      SizeY = READ_PORT_UCHAR(CRTCDATA);
      WRITE_PORT_UCHAR(CRTC, CRTC_OVERFLOW);
      Data = READ_PORT_UCHAR(CRTCDATA);
      SizeY |= (((Data & 0x02) << 7) | ((Data & 0x40) << 3));
      SizeY++;
      WRITE_PORT_UCHAR(CRTC, CRTC_SCANLINES);
      ScanLines = (READ_PORT_UCHAR(CRTCDATA) & 0x1F) + 1;
      SizeY = SizeY / ScanLines;

      VidpVgaTextClearDisplay();

      WRITE_PORT_UCHAR(CRTC, CRTC_CURLO);
      WRITE_PORT_UCHAR(CRTCDATA, 0);
      WRITE_PORT_UCHAR(CRTC, CRTC_CURHI);
      WRITE_PORT_UCHAR(CRTCDATA, 0);
   }

   return TRUE;
}

static VOID NTAPI
VidVgaTextResetDisplay(VOID)
{
   VidVgaTextInitialize(TRUE);
}

static VOID NTAPI
VidVgaTextCleanUp(VOID)
{
   if (VidpInitialized)
   {
      MmUnmapIoSpace(VidpMemory, 0x10000);
      VidpInitialized = FALSE;
   }
}

static VOID NTAPI
VidVgaTextBufferToScreenBlt(
   IN PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN ULONG Delta)
{
}

static VOID NTAPI
VidVgaTextScreenToBufferBlt(
   OUT PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN LONG Delta)
{
}

static VOID NTAPI
VidVgaTextBitBlt(
   IN PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top)
{
}

static VOID NTAPI
VidVgaTextSolidColorFill(
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Right,
   IN ULONG Bottom,
   IN ULONG Color)
{
}

static VOID NTAPI
VidpVgaTextScrollDisplay(VOID)
{
   volatile USHORT *ptr;
   int i;

   RtlMoveMemory(VidpMemory, (PUSHORT)VidpMemory + SizeX, SizeX * (SizeY - 1) * 2);

   ptr = (volatile USHORT *)VidpMemory + (SizeX * (SizeY - 1));
   for (i = 0; i < (int)SizeX; i++, ptr++)
      *ptr = (CHAR_ATTRIBUTE << 8) + ' ';
}

static VOID NTAPI
VidVgaTextDisplayString(
   IN PCSTR String)
{
   PCSTR pch;
   int offset;
   ULONG CursorX;
   ULONG CursorY;

   pch = String;

   WRITE_PORT_UCHAR(CRTC, CRTC_CURHI);
   offset = READ_PORT_UCHAR(CRTCDATA) << 8;
   WRITE_PORT_UCHAR(CRTC, CRTC_CURLO);
   offset += READ_PORT_UCHAR(CRTCDATA);
  
   CursorY = offset / SizeX;
   CursorX = offset % SizeX;
  
   while (*pch != 0)
   {
      if (*pch == '\n')
      {
         CursorY++;
         CursorX = 0;
      }
      else if (*pch == '\b')
      {
         if (CursorX > 0)
            CursorX--;
      }
      else if (*pch != '\r')
      {
         volatile USHORT *ptr;

         ptr = (volatile USHORT *)VidpMemory + ((CursorY * SizeX) + CursorX);
         *ptr = (CHAR_ATTRIBUTE << 8) + *pch;
         CursorX++;
	  
         if (CursorX >= SizeX)
         {
            CursorY++;
            CursorX = 0;
         }
      }
  
      if (CursorY >= SizeY)
      {
         VidpVgaTextScrollDisplay();
         CursorY = SizeY - 1;
      }
  
      pch++;
   }
  
   offset = (CursorY * SizeX) + CursorX;
   WRITE_PORT_UCHAR(CRTC, CRTC_CURLO);
   WRITE_PORT_UCHAR(CRTCDATA, (UCHAR)(offset & 0xff));
   WRITE_PORT_UCHAR(CRTC, CRTC_CURHI);
   WRITE_PORT_UCHAR(CRTCDATA, (UCHAR)((offset >> 8) & 0xff));
}

VID_FUNCTION_TABLE VidVgaTextTable = {
   VidVgaTextInitialize,
   VidVgaTextCleanUp,
   VidVgaTextResetDisplay,
   VidVgaTextBufferToScreenBlt,
   VidVgaTextScreenToBufferBlt,
   VidVgaTextBitBlt,
   VidVgaTextSolidColorFill,
   VidVgaTextDisplayString
};
