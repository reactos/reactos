/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#ifndef __FREELDR_H
#define __FREELDR_H

#include <ddk/ntddk.h>
#include <ndk/ntndk.h>
#include <arch.h>
#include <rtl.h>
#include <disk.h>
#include <fs.h>
#include <ui.h>
#include <multiboot.h>
#include <mm.h>
#include <machine.h>
#include <inifile.h>
#include <video.h>
#include <portio.h>
#include <reactos.h>

#define ROUND_UP(N, S) (((N) + (S) - 1) & ~((S) - 1))
#define ROUND_DOWN(N, S) ((N) & ~((S) - 1))
#define Ke386EraseFlags(x)     __asm__ __volatile__("pushl $0 ; popfl\n")

extern BOOL UserInterfaceUp;	/* Tells us if the user interface is displayed */

VOID BootMain(LPSTR CmdLine);
VOID RunLoader(VOID);

#endif  // defined __FREELDR_H
