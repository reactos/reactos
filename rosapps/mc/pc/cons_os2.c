/* Client interface for General purpose OS/2 console save/restore server.
        1997 Alexander Dong <ado@software-ag.de>
   Having the same interface as its Linux counterpart:
   	Copyright (C) 1994 Janne Kukonlehto <jtklehto@stekt.oulu.fi> 
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
   
*/

#include <config.h>

#ifdef __os2__
#define INCL_BASE
#define INCL_NOPM
#define INCL_VIO
#define INCL_KBD
#define INCL_DOS
#define INCL_SUB
#define INCL_DOSERRORS
#include <os2.h>
#endif

#include "../src/tty.h"
#include "../src/util.h"
#include "../src/win.h"
#include "../src/cons.saver.h"

signed char console_flag = 1;
static unsigned char *scr_buffer;
static unsigned char *pointer;

static int GetScrRows();
static int GetScrCols();

static int GetScrRows()
{
   VIOMODEINFO pvMode = {80};
   unsigned int hVio = 0;
   VioGetMode(&pvMode, hVio);
   return (pvMode.row ? pvMode.row: 25);
}

static int GetScrCols()
{
   VIOMODEINFO pvMode = {80};
   unsigned int hVio = 0;
   VioGetMode(&pvMode, hVio);
   return (pvMode.col ? pvMode.col: 80);
}

void show_console_contents (int starty, unsigned char begin_line, unsigned char end_line)
{
   int col = GetScrCols();
   int row = GetScrRows();
   int n;
   register int z;

   pointer = scr_buffer;
   for (z=0; z<(begin_line * col); z++) {
      pointer++; pointer++;
   }
   n = (end_line - begin_line + 1) * col;
   VioWrtCellStr((PCH) pointer, (USHORT) n, begin_line, 0, 0);
   return;
}

void handle_console (unsigned char action)
{
   static int col;
   static int row;
   int        n;

   switch (action) {
   case CONSOLE_INIT:           /* Initialize */
      col = GetScrCols();
      row = GetScrRows();
      scr_buffer = (unsigned char *) malloc(col * row * 2);  /* short values */
      n = col * row * 2;
      VioReadCellStr((PCH) scr_buffer, (USHORT *) &n, 0, 0, 0); /* Just save it */
      break;
   case CONSOLE_DONE:
      free(scr_buffer);
      break;
   case CONSOLE_SAVE:           /* Save the screen */
      n = col * row * 2;
      VioReadCellStr((PCH) scr_buffer, (USHORT *) &n, 0, 0, 0);
      break;
   case CONSOLE_RESTORE:
      n = col * row * 2;
      VioWrtCellStr ((PCH) scr_buffer, (USHORT) n, 0, 0, 0); /* Write it back */
      break;
   default: 
      /* This is not possible, but if we are here, just save the screen */
      handle_console(CONSOLE_SAVE);
     break;
   }
   return;
}
