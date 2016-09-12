/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Sylvain Petreolle
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * PROJECT:         ReactOS Program Manager
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            base/shell/progman/resource.h
 * PURPOSE:         ProgMan resource header
 * PROGRAMMERS:     Ulrich Schmid
 *                  Sylvain Petreolle
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#ifndef IDC_STATIC
#define IDC_STATIC  -1
#endif

/* Icons */
#define IDI_APPICON               1
#define IDI_GROUP_PERSONAL_ICON   8
#define IDI_GROUP_COMMON_ICON     9
#define IDI_ROSLOGO               10
#define IDI_GROUP_ICON            48
#define IDI_TERMINAL              49

/* Accelerators */
#define IDA_ACCEL     1

/* String table index */
#define IDS_PROGRAM_MANAGER            0x02
#define IDS_ERROR                      0x03
#define IDS_WARNING                    0x04
#define IDS_INFO                       0x05
#define IDS_DELETE                     0x06
#define IDS_DELETE_GROUP_s             0x07
#define IDS_DELETE_PROGRAM_s           0x08
#define IDS_MOVE_PROGRAM_1             0x09 // FIXME: rename me!
#define IDS_MOVE_PROGRAM_2             0x0a // FIXME: rename me!
#define IDS_NOT_IMPLEMENTED            0x0b
#define IDS_FILE_READ_ERROR_s          0x0c
#define IDS_FILE_WRITE_ERROR_s         0x0d
#define IDS_GRPFILE_READ_ERROR_s       0x0e
#define IDS_OUT_OF_MEMORY              0x0f
#define IDS_WINHELP_ERROR              0x10
#define IDS_UNKNOWN_FEATURE_s          0x11
#define IDS_FILE_NOT_OVERWRITTEN_s     0x12
#define IDS_SAVE_GROUP_AS_s            0x13
#define IDS_ALL_FILES                  0x14
#define IDS_PROGRAMS                   0x15
#define IDS_LIBRARIES_DLL              0x16
#define IDS_SYMBOL_FILES               0x17
#define IDS_SYMBOLS_ICO                0x18

/* Menu */

#define MAIN_MENU           0x109
#define PM_NEW              0x100
#define PM_OPEN             0x101
#define PM_MOVE             0x102
#define PM_COPY             0x103
#define PM_DELETE           0x104
#define PM_ATTRIBUTES       0x105
#define PM_EXECUTE          0x107
#define PM_EXIT             0x108

#define PM_AUTO_ARRANGE     0x110
#define PM_MIN_ON_RUN       0x111
#define PM_SAVE_SETTINGS    0x112
#define PM_SAVE_SETTINGS_NOW    0x113

#define PM_OVERLAP          0x120
#define PM_SIDE_BY_SIDE     0x121
#define PM_ARRANGE          0x122
#define PM_FIRST_CHILD      0x3030

/*
 *#define PM_FIRST_LANGUAGE   0x400
 *#define PM_LAST_LANGUAGE    0x499
 */

#define PM_CONTENTS         0x131
#define PM_ABOUT            0x142


/* Dialogs */
#define IDD_MAIN      1
#define IDD_NEW       2
#define IDD_COPY_MOVE 4
#define IDD_GROUP     6
#define IDD_PROGRAM   7
#define IDD_SYMBOL    8
#define IDD_EXECUTE   9

/* Dialog `New' */

/* RADIOBUTTON: The next two must be in sequence */
#define PM_NEW_GROUP        0x150
#define PM_NEW_PROGRAM      0x151
#define PM_PERSONAL_GROUP   1001
#define PM_COMMON_GROUP     1002
#define PM_FORMAT_TXT       1003
#define PM_FORMAT           1004

/* Dialogs `Copy', `Move' */
#define PM_COPY_MOVE_TXT    0x160
#define PM_PROGRAM          0x161
#define PM_FROM_GROUP       0x162
#define PM_TO_GROUP         0x163

/* Dialogs `Group attributes' */
#define PM_DESCRIPTION      0x170
#define PM_FILE             0x172

/* Dialogs `Program attributes' */
#define PM_COMMAND_LINE     0x180
#define PM_DIRECTORY        0x182
#define PM_HOT_KEY          0x184
#define PM_ICON             0x186
#define PM_OTHER_SYMBOL     0x187

/* Dialog `Symbol' */
#define PM_ICON_FILE        0x190
#define PM_SYMBOL_LIST      0x192

/* Dialog `Execute' */
#define PM_COMMAND          0x1a0 // FIXME: May be merged with PM_COMMAND_LINE ?
#define PM_SYMBOL           0x1a1 // FIXME: Rename: PM_RUN_MINIMIZED
#define PM_NEW_VDM          0x1a2
#define PM_BROWSE           0x1a3
