/*
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
#include "arch.h"
#include "i386boot.h"
#include "machine.h"
#include "machxen.h"

VOID
XenBootReactOS(VOID)
  {
  BOOLEAN PaeModeEnabled;

  XenVideoClearScreen(0x07);

  /* Disable events */
#ifdef TODO
  XenEvtchnDisableEvents();
#endif

  /* Re-initalize EFLAGS */
  Ke386EraseFlags();

  /* PAE Mode not supported yet */
  PaeModeEnabled = FALSE;

  /* Initialize the page directory */
  i386BootSetupPageDirectory(PaeModeEnabled);

  /* Initialize Paging, Write-Protection and Load NTOSKRNL */
  i386BootSetupPae(PaeModeEnabled, 0x2badb002);
  }

/* EOF */
