/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
 * FILE:            subsys/system/usetup/errcode.h
 * PURPOSE:         
 * PROGRAMMER:      
 */

#pragma once

typedef enum
{
    NOT_AN_ERROR = 0,
    ERROR_NOT_INSTALLED,
    ERROR_NO_HDD,
    ERROR_NO_SOURCE_DRIVE,
    ERROR_LOAD_TXTSETUPSIF,
    ERROR_CORRUPT_TXTSETUPSIF,
    ERROR_SIGNATURE_TXTSETUPSIF,
    ERROR_DRIVE_INFORMATION,
    ERROR_WRITE_BOOT,
    ERROR_LOAD_COMPUTER,
    ERROR_LOAD_DISPLAY,
    ERROR_LOAD_KEYBOARD,
    ERROR_LOAD_KBLAYOUT,
    ERROR_WARN_PARTITION,
    ERROR_NEW_PARTITION,
    ERROR_DELETE_SPACE,
    ERROR_INSTALL_BOOTCODE,
    ERROR_NO_FLOPPY,
    ERROR_UPDATE_KBSETTINGS,
    ERROR_UPDATE_DISPLAY_SETTINGS,
    ERROR_IMPORT_HIVE,
    ERROR_FIND_REGISTRY,
    ERROR_CREATE_HIVE,
    ERROR_INITIALIZE_REGISTRY,
    ERROR_INVALID_CABINET_INF,
    ERROR_CABINET_MISSING,
    ERROR_CABINET_SCRIPT,
    ERROR_COPY_QUEUE,
    ERROR_CREATE_DIR,
    ERROR_TXTSETUP_SECTION,
    ERROR_CABINET_SECTION,
    ERROR_CREATE_INSTALL_DIR,
    ERROR_FIND_SETUPDATA,
    ERROR_WRITE_PTABLE,
    ERROR_ADDING_CODEPAGE,
    ERROR_UPDATE_LOCALESETTINGS,
    ERROR_ADDING_KBLAYOUTS,
    ERROR_UPDATE_GEOID,
    ERROR_DIRECTORY_NAME,
    ERROR_INSUFFICIENT_PARTITION_SIZE,
    ERROR_PARTITION_TABLE_FULL,
    ERROR_ONLY_ONE_EXTENDED,
    ERROR_FORMATTING_PARTITION,

    ERROR_LAST_ERROR_CODE
} ERROR_NUMBER;

/* EOF */
