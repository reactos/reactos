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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/usetup.h
 * PURPOSE:         Text-mode setup
 * PROGRAMMER:
 */

#ifndef _USETUP_PCH_
#define _USETUP_PCH_

/* C Headers */
#include <stdio.h>
#include <stdlib.h>

/* PSDK/NDK */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>

#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>

#include <ntstrsafe.h>


/* Setup library headers */
#include <reactos/rosioctl.h>
#include <../lib/setuplib.h>

/* Internal Headers */
#include "consup.h"
#include "progress.h"
#include "fslist.h"
#include "partlist.h"
#include "genlist.h"
#include "mui.h"

#include "spapisup/inffile.h"
#include "spapisup/cabinet.h"


extern HANDLE ProcessHeap;
extern BOOLEAN IsUnattendedSetup;

/* Settings lists *****/
extern PGENERIC_LIST ComputerList;
extern PGENERIC_LIST DisplayList;
extern PGENERIC_LIST KeyboardList;
extern PGENERIC_LIST LanguageList;
extern PGENERIC_LIST LayoutList;

typedef enum _PAGE_NUMBER
{
    SETUP_INIT_PAGE,    /* Virtual page */
    LANGUAGE_PAGE,
    WELCOME_PAGE,
    LICENSE_PAGE,
    INSTALL_INTRO_PAGE,

//    SCSI_CONTROLLER_PAGE,
//    OEM_DRIVER_PAGE,

    REPAIR_INTRO_PAGE,
    UPGRADE_REPAIR_PAGE,

    DEVICE_SETTINGS_PAGE,
    COMPUTER_SETTINGS_PAGE,
    DISPLAY_SETTINGS_PAGE,
    KEYBOARD_SETTINGS_PAGE,
    LAYOUT_SETTINGS_PAGE,

    SELECT_PARTITION_PAGE,
    CREATE_PARTITION_PAGE,
    CHANGE_SYSTEM_PARTITION,
    CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
    DELETE_PARTITION_PAGE,

    START_PARTITION_OPERATIONS_PAGE,    /* Virtual page */
    SELECT_FILE_SYSTEM_PAGE,    /* Virtual page */
    FORMAT_PARTITION_PAGE,      /* Virtual page */
    CHECK_FILE_SYSTEM_PAGE,
    BOOTLOADER_SELECT_PAGE,

    PREPARE_COPY_PAGE,
    INSTALL_DIRECTORY_PAGE,
    FILE_COPY_PAGE,
    REGISTRY_PAGE,
    BOOTLOADER_INSTALL_PAGE,
    BOOTLOADER_REMOVABLE_DISK_PAGE,

    SUCCESS_PAGE,
    QUIT_PAGE,
    FLUSH_PAGE,
    REBOOT_PAGE,    /* Virtual page */
    RECOVERY_PAGE,  /* Virtual page */
} PAGE_NUMBER, *PPAGE_NUMBER;

#define POPUP_WAIT_NONE    0
#define POPUP_WAIT_ANY_KEY 1
#define POPUP_WAIT_ENTER   2

VOID
PopupError(IN PCCH Text,
           IN PCCH Status,
           IN PINPUT_RECORD Ir,
           IN ULONG WaitEvent);

#endif /* _USETUP_PCH_ */
