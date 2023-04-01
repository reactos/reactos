/*
 * PROJECT:    ReactOS Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 * COPYRIGHT:  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *             Copyright 2002 Andriy Palamarchuk
 */

#pragma once

#define MAIN_MENU        0x201
#define DIALOG_PAGESETUP 0x202
#define ID_ACCEL         0x203
#define DIALOG_ENCODING  0x204
#define ID_ENCODING      0x205
#define ID_EOLN          0x206
#define DIALOG_GOTO      0x207
#define ID_LINENUMBER    0x208
#define IDI_NPICON       0x209
#define IDC_LICENSE      0x20A
#define DIALOG_PRINTING  0x20B
#define IDC_PRINTING_STATUS     0x20C
#define IDC_PRINTING_FILENAME   0x20D
#define IDC_PRINTING_PAGE       0x20E

/* Commands */
#define CMD_NEW        0x100
#define CMD_NEW_WINDOW 0x101
#define CMD_OPEN       0x102
#define CMD_SAVE       0x103
#define CMD_SAVE_AS    0x104
#define CMD_PRINT      0x105
#define CMD_PAGE_SETUP 0x106
#define CMD_EXIT       0x108

#define CMD_UNDO       0x110
#define CMD_CUT        0x111
#define CMD_COPY       0x112
#define CMD_PASTE      0x113
#define CMD_DELETE     0x114
#define CMD_SELECT_ALL 0x116
#define CMD_TIME_DATE  0x117

#define CMD_SEARCH      0x120
#define CMD_SEARCH_NEXT 0x121
#define CMD_REPLACE     0x122
#define CMD_GOTO        0x123
#define CMD_SEARCH_PREV 0x124

#define CMD_WRAP 0x119
#define CMD_FONT 0x140

#define CMD_STATUSBAR        0x150
#define CMD_STATUSBAR_WND_ID 0x151

#define CMD_HELP_CONTENTS 0x130
#define CMD_HELP_ABOUT_NOTEPAD    0x135

/* Strings */
#define STRING_PAGESETUP_HEADERVALUE 0x160
#define STRING_PAGESETUP_FOOTERVALUE 0x161

#define STRING_NOTEPAD        0x170
#define STRING_ERROR          0x171
#define STRING_WARNING        0x172
#define STRING_INFO           0x173
#define STRING_UNTITLED       0x174
#define STRING_ALL_FILES      0x175
#define STRING_TEXT_FILES_TXT 0x176
#define STRING_TOOLARGE       0x177
#define STRING_NOTEXT         0x178
#define STRING_DOESNOTEXIST   0x179
#define STRING_NOTSAVED       0x17A

#define STRING_NOTFOUND      0x17B
#define STRING_OUT_OF_MEMORY 0x17C
#define STRING_CANNOTFIND    0x17D

#define STRING_ANSI       0x180
#define STRING_UNICODE    0x181
#define STRING_UNICODE_BE 0x182
#define STRING_UTF8       0x183
#define STRING_UTF8_BOM   0x184

#define STRING_CRLF 0x185
#define STRING_LF   0x186
#define STRING_CR   0x187

#define STRING_LINE_COLUMN 0x188
#define STRING_PRINTERROR  0x189
#define STRING_DEFAULTFONT 0x18A
#define STRING_LINE_NUMBER_OUT_OF_RANGE 0x18B
#define STRING_NOWPRINTING 0x18C
#define STRING_PRINTCANCELING 0x18D
#define STRING_PRINTCOMPLETE 0x18E
#define STRING_PRINTCANCELED 0x18F
#define STRING_PRINTFAILED 0x190

#define STRING_TEXT_DOCUMENT  0x200

#define STRING_NOTEPAD_AUTHORS 0x300
