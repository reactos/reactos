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

#define MB_INFO_FLAG_MEM_SIZE			0x00000001
#define MB_INFO_FLAG_BOOT_DEVICE		0x00000002
#define MB_INFO_FLAG_COMMAND_LINE		0x00000004
#define MB_INFO_FLAG_MODULES			0x00000008
#define MB_INFO_FLAG_AOUT_SYMS			0x00000010
#define MB_INFO_FLAG_ELF_SYMS			0x00000020
#define MB_INFO_FLAG_MEMORY_MAP			0x00000040
#define MB_INFO_FLAG_DRIVES				0x00000080
#define MB_INFO_FLAG_CONFIG_TABLE		0x00000100
#define MB_INFO_FLAG_BOOT_LOADER_NAME	0x00000200
#define MB_INFO_FLAG_APM_TABLE			0x00000400
#define MB_INFO_FLAG_GRAPHICS_TABLE		0x00000800

void	LoadAndBootReactOS(char *OperatingSystemName);
BOOL	MultiBootLoadKernel(FILE *KernelImage);
BOOL	MultiBootLoadModule(FILE *ModuleImage, char *ModuleName);
void	enable_a20(void);
void	boot_ros(void);


#endif // defined __ROSBOOT_H