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

#pragma once

/* Icon */
#define IDI_APPICON 1

/* Resource names */
#define IDD_MAIN      1
#define IDD_NEW       2
#define IDD_OPEN      3
#define IDD_MOVE      4
#define IDD_COPY      5
#define IDD_DELETE    6
#define IDD_GROUP     7
#define IDD_PROGRAM   8
#define IDD_SYMBOL    9
#define IDD_EXECUTE   10

#define IDA_ACCEL     1

/* Stringtable index */
#define IDS_PROGRAM_MANAGER            0x02
#define IDS_ERROR                      0x03
#define IDS_WARNING                    0x04
#define IDS_INFO                       0x05
#define IDS_DELETE                     0x06
#define IDS_DELETE_GROUP_s             0x07
#define IDS_DELETE_PROGRAM_s           0x08
#define IDS_NOT_IMPLEMENTED            0x09
#define IDS_FILE_READ_ERROR_s          0x0a
#define IDS_FILE_WRITE_ERROR_s         0x0b
#define IDS_GRPFILE_READ_ERROR_s       0x0c
#define IDS_OUT_OF_MEMORY              0x0d
#define IDS_WINHELP_ERROR              0x0e
#define IDS_UNKNOWN_FEATURE_s          0x0f
#define IDS_FILE_NOT_OVERWRITTEN_s     0x10
#define IDS_SAVE_GROUP_AS_s            0x11
#define IDS_NO_HOT_KEY                 0x12
#define IDS_ALL_FILES                  0x13
#define IDS_PROGRAMS                   0x14
#define IDS_LIBRARIES_DLL              0x15
#define IDS_SYMBOL_FILES               0x16
#define IDS_SYMBOLS_ICO                0x17

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
#define PM_SAVE_SETTINGS    0x113

#define PM_OVERLAP          0x120
#define PM_SIDE_BY_SIDE     0x121
#define PM_ARRANGE          0x122
#define PM_FIRST_CHILD      0x3030

/*
 *#define PM_FIRST_LANGUAGE   0x400
 *#define PM_LAST_LANGUAGE    0x499
 */

#define PM_CONTENTS         0x131

#define PM_ABOUT_WINE       0x142

/* Dialog `New' */

/* RADIOBUTTON: The next two must be in sequence */
#define PM_NEW_GROUP        0x150
#define PM_NEW_PROGRAM      0x151
#define PM_NEW_GROUP_TXT    0x152
#define PM_NEW_PROGRAM_TXT  0x153

/* Dialogs `Copy', `Move' */

#define PM_PROGRAM          0x160
#define PM_FROM_GROUP       0x161
#define PM_TO_GROUP         0x162
#define PM_TO_GROUP_TXT     0x163

/* Dialogs `Group attributes' */

#define PM_DESCRIPTION      0x170
#define PM_DESCRIPTION_TXT  0x171
#define PM_FILE             0x172
#define PM_FILE_TXT         0x173

/* Dialogs `Program attributes' */
#define PM_COMMAND_LINE     0x180
#define PM_COMMAND_LINE_TXT 0x181
#define PM_DIRECTORY        0x182
#define PM_DIRECTORY_TXT    0x183
#define PM_HOT_KEY          0x184
#define PM_HOT_KEY_TXT      0x185
#define PM_ICON             0x186
#define PM_OTHER_SYMBOL     0x187

/* Dialog `Symbol' */

#define PM_ICON_FILE        0x190
#define PM_ICON_FILE_TXT    0x191
#define PM_SYMBOL_LIST      0x192
#define PM_SYMBOL_LIST_TXT  0x193

/* Dialog `Execute' */

#define PM_COMMAND          0x1a0
#define PM_SYMBOL           0x1a1
#define PM_BROWSE           0x1a2
#define PM_HELP             0x1a3
