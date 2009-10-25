/* $Id$
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

#include <freeldr.h>

#define TEXTMODE_BUFFER      0xb8000
#define TEXTMODE_BUFFER_SIZE 0x8000

#define TEXT_COLS  80
#define TEXT_LINES 25

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

BOOLEAN
PcConsKbHit(VOID)
{
  REGS Regs;

  /* Int 16h AH=01h
   * KEYBOARD - CHECK FOR KEYSTROKE
   *
   * AH = 01h
   * Return:
   * ZF set if no keystroke available
   * ZF clear if keystroke available
   * AH = BIOS scan code
   * AL = ASCII character
   */
  Regs.b.ah = 0x01;
  Int386(0x16, &Regs, &Regs);

  return 0 == (Regs.x.eflags & I386FLAG_ZF);
}

int
PcConsGetCh(void)
{
  REGS Regs;
  static BOOLEAN ExtendedKey = FALSE;
  static char ExtendedScanCode = 0;

  /* If the last time we were called an
   * extended key was pressed then return
   * that keys scan code. */
  if (ExtendedKey)
    {
      ExtendedKey = FALSE;
      return ExtendedScanCode;
    }

  /* Int 16h AH=00h
   * KEYBOARD - GET KEYSTROKE
   *
   * AH = 00h
   * Return:
   * AH = BIOS scan code
   * AL = ASCII character
   */
  Regs.b.ah = 0x00;
  Int386(0x16, &Regs, &Regs);

  /* Check for an extended keystroke */
  if (0 == Regs.b.al)
    {
      ExtendedKey = TRUE;
      ExtendedScanCode = Regs.b.ah;
    }

  /* Return keystroke */
  return Regs.b.al;
}

/* EOF */
