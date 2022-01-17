/*
 * REG.EXE - Wine-compatible reg program.
 *
 * Copyright 2008 Andrew Riedi
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

#include <windef.h>

/* Translation IDs */

/* Shared */
#define STRING_YES                    100
#define STRING_NO                     101
#define STRING_YESNO                  103
#define STRING_INVALID_SYNTAX         105
#define STRING_FUNC_HELP              106
#define STRING_ACCESS_DENIED          107
#define STRING_SUCCESS                108
#define STRING_CANCELLED              109
#define STRING_KEY_NONEXIST           110
#define STRING_VALUE_NONEXIST         111
#define STRING_DEFAULT_VALUE          112

/* reg.c */
#define STRING_REG_HELP               150
#define STRING_USAGE                  151
#define STRING_ADD_USAGE              152
#define STRING_COPY_USAGE             153
#define STRING_DELETE_USAGE           154
#define STRING_EXPORT_USAGE           155
#define STRING_IMPORT_USAGE           156
#define STRING_QUERY_USAGE            157
#define STRING_REG_VIEW_USAGE         164
#define STRING_INVALID_KEY            165
#define STRING_NO_REMOTE              166
#define STRING_INVALID_SYSTEM_KEY     167
#define STRING_INVALID_OPTION         168

/* add.c */
#define STRING_MISSING_NUMBER         200
#define STRING_MISSING_HEXDATA        201
#define STRING_INVALID_STRING         202
#define STRING_UNHANDLED_TYPE         203
#define STRING_UNSUPPORTED_TYPE       204
#define STRING_OVERWRITE_VALUE        205
#define STRING_INVALID_CMDLINE        206

/* delete.c */
#define STRING_DELETE_VALUE           300
#define STRING_DELETE_VALUEALL        301
#define STRING_DELETE_SUBKEY          302
#define STRING_VALUEALL_FAILED        303

/* export.c */
#define STRING_OVERWRITE_FILE         350

/* import.c */
#define STRING_ESCAPE_SEQUENCE        400
#define STRING_KEY_IMPORT_FAILED      401
#define STRING_FILE_NOT_FOUND         402

/* query.c */
#define STRING_VALUE_NOT_SET          450
#define STRING_MATCHES_FOUND          451
