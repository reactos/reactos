/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000  Brian Palmer  <brianp@sginet.com>
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

#ifndef __ROSBOOT_H
#define __ROSBOOT_H

#include "freeldr.h"
#include "stdlib.h"
#include "pe.h"

#define PACKED __attribute__((packed))

void	LoadAndBootReactOS(int nOSToBoot);
void	ReactOSMemInit(void);
void	ReactOSBootKernel(void);
BOOL	ReactOSLoadPEImage(FILE *pImage);
void	enable_a20(void);
void	boot_ros(void);


// WARNING:
// This structure is prototyped here but allocated in ros.S
// if you change this prototype make sure to update ros.S
typedef struct
{
	/*
	 * Magic value (useless really)
	 */
	unsigned int magic;

	/*
	 * Cursor position
	 */
	unsigned int cursorx;
	unsigned int cursory;

	/*
	 * Number of files (including the kernel) loaded
	 */
	unsigned int nr_files;

	/*
	 * Range of physical memory being used by the system
	 */
	unsigned int start_mem;
	unsigned int end_mem;

	/*
	 * List of module lengths (terminated by a 0)
	 */
	unsigned int module_lengths[64];

	/*
	 * Kernel parameter string
	 */
	char kernel_parameters[256];
} boot_param PACKED;

#endif // defined __ROSBOOT_H