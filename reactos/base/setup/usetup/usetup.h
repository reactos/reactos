/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/usetup.h
 * PURPOSE:         Text-mode setup
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __USETUP_H__
#define __USETUP_H__

/* C Headers */
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>

/* PSDK/NDK */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <fmifs/fmifs.h>

/* VFAT */
#include <fslib/vfatlib.h>

/* DDK Disk Headers */
#include <ntddscsi.h>

/* Helper Header */
#include <reactos/helper.h>

/* ReactOS Version */
#include <reactos/buildno.h>

/* Internal Headers */
#include "interface/consup.h"
#include "partlist.h"
#include "inffile.h"
#include "inicache.h"
#include "progress.h"
#ifdef __REACTOS__
#include "filequeue.h"
#endif
#include "bootsup.h"
#include "registry.h"
#include "fslist.h"
#include "chkdsk.h"
#include "format.h"
#include "cabinet.h"
#include "filesup.h"
#include "drivesup.h"
#include "genlist.h"
#include "settings.h"
#include "host.h"
#include "mui.h"
#include "errorcode.h"

extern HANDLE ProcessHeap;
extern UNICODE_STRING SourceRootPath;
extern UNICODE_STRING SourceRootDir;
extern UNICODE_STRING SourcePath;
extern BOOLEAN IsUnattendedSetup;

typedef enum _PAGE_NUMBER
{
  LANGUAGE_PAGE = 0,
  START_PAGE,
  INTRO_PAGE,
  LICENSE_PAGE,
  INSTALL_INTRO_PAGE,

//  SCSI_CONTROLLER_PAGE,

  DEVICE_SETTINGS_PAGE,
  COMPUTER_SETTINGS_PAGE,
  DISPLAY_SETTINGS_PAGE,
  KEYBOARD_SETTINGS_PAGE,
  LAYOUT_SETTINGS_PAGE,

  SELECT_PARTITION_PAGE,
  CREATE_PARTITION_PAGE,
  DELETE_PARTITION_PAGE,

  SELECT_FILE_SYSTEM_PAGE,
  FORMAT_PARTITION_PAGE,
  CHECK_FILE_SYSTEM_PAGE,

  PREPARE_COPY_PAGE,
  INSTALL_DIRECTORY_PAGE,
  FILE_COPY_PAGE,
  REGISTRY_PAGE,
  BOOT_LOADER_PAGE,
  BOOT_LOADER_FLOPPY_PAGE,
  BOOT_LOADER_HARDDISK_PAGE,

  REPAIR_INTRO_PAGE,

  SUCCESS_PAGE,
  QUIT_PAGE,
  FLUSH_PAGE,
  REBOOT_PAGE,			/* virtual page */
} PAGE_NUMBER, *PPAGE_NUMBER;

#define POPUP_WAIT_NONE    0
#define POPUP_WAIT_ANY_KEY 1
#define POPUP_WAIT_ENTER   2


#endif /* __USETUP_H__*/

/* EOF */
