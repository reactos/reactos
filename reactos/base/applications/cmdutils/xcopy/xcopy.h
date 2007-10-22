/*
 * XCOPY - Wine-compatible xcopy program
 *
 * Copyright (C) 2007 J. Edmeades
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

/* Local #defines */
#define RC_OK         0
#define RC_NOFILES    1
#define RC_CTRLC      2
#define RC_INITERROR  4
#define RC_WRITEERROR 5

#define OPT_ASSUMEDIR    0x00000001
#define OPT_RECURSIVE    0x00000002
#define OPT_EMPTYDIR     0x00000004
#define OPT_QUIET        0x00000008
#define OPT_FULL         0x00000010
#define OPT_SIMULATE     0x00000020
#define OPT_PAUSE        0x00000040
#define OPT_NOCOPY       0x00000080
#define OPT_NOPROMPT     0x00000100
#define OPT_SHORTNAME    0x00000200
#define OPT_MUSTEXIST    0x00000400
#define OPT_REPLACEREAD  0x00000800
#define OPT_COPYHIDSYS   0x00001000
#define OPT_IGNOREERRORS 0x00002000
#define OPT_SRCPROMPT    0x00004000
#define OPT_ARCHIVEONLY  0x00008000
#define OPT_REMOVEARCH   0x00010000
#define OPT_EXCLUDELIST  0x00020000
#define OPT_DATERANGE    0x00040000
#define OPT_DATENEWER    0x00080000

#define MAXSTRING 8192

/* Translation ids */
#define STRING_INVPARMS         101
#define STRING_INVPARM          102
#define STRING_PAUSE            103
#define STRING_SIMCOPY          104
#define STRING_COPY             105
#define STRING_QISDIR           106
#define STRING_SRCPROMPT        107
#define STRING_OVERWRITE        108
#define STRING_COPYFAIL         109
#define STRING_OPENFAIL         110
#define STRING_READFAIL         111
#define STRING_YES_CHAR         112
#define STRING_NO_CHAR          113
#define STRING_ALL_CHAR         114
#define STRING_FILE_CHAR        115
#define STRING_DIR_CHAR         116
#define STRING_HELP             117
