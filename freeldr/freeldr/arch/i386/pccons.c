/* $Id: pccons.c,v 1.2 2004/11/10 23:45:37 gvg Exp $
 *
 *  FreeLoader
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

#include "freeldr.h"
#include "machine.h"
#include "arch.h"
#include "debug.h"
#include "machpc.h"
#include "rtl.h"

#define TEXTMODE_BUFFER      0xb8000
#define TEXTMODE_BUFFER_SIZE 0x8000

#define TEXT_COLS  80
#define TEXT_LINES 25

VOID
PcConsClearScreenAttr(U8 Attr)
{
  U16 AttrChar;
  U16 *BufPtr;

  AttrChar = ((U16) Attr << 8) | ' ';
  for (BufPtr = (U16 *) TEXTMODE_BUFFER;
       BufPtr < (U16 *) (TEXTMODE_BUFFER + TEXTMODE_BUFFER_SIZE);
       BufPtr++)
    {
      *BufPtr = AttrChar;
    }
}

VOID
PcConsPutChar(int Ch)
{
  REGS Regs;

  /* If we are displaying a CR '\n' then do a LF also */
  if ('\n' == Ch)
    {
      /* Display the LF */
      PcConsPutChar('\r');
    }

  /* If we are displaying a TAB '\t' then display 8 spaces ' ' */
  if ('\t' == Ch)
    {
      /* Display the 8 spaces ' ' */
      PcConsPutChar(' ');
      PcConsPutChar(' ');
      PcConsPutChar(' ');
      PcConsPutChar(' ');
      PcConsPutChar(' ');
      PcConsPutChar(' ');
      PcConsPutChar(' ');
      PcConsPutChar(' ');
      return;
    }

  /* Int 10h AH=0Eh
   * VIDEO - TELETYPE OUTPUT
   *
   * AH = 0Eh
   * AL = character to write
   * BH = page number
   * BL = foreground color (graphics modes only)
   */
  Regs.b.ah = 0x0E;
  Regs.b.al = Ch;
  Regs.w.bx = 1;
  Int386(0x10, &Regs, &Regs);
}

VOID
PcConsPutCharAttrAtLoc(int Ch, U8 Attr, unsigned X, unsigned Y)
{
  U16 *BufPtr;

  BufPtr = (U16 *) (TEXTMODE_BUFFER + (Y * TEXT_COLS + X) * 2);
  *BufPtr = ((U16) Attr << 8) | (Ch & 0xff);
}

/* EOF */
