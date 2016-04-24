/*
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Andriy Palamarchuk
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

#define KEY_MAX_LEN             1024

#define REG_FORMAT_5 1
#define REG_FORMAT_4 2

BOOL export_registry_key(WCHAR *file_name, WCHAR *reg_key_name, DWORD format);
BOOL import_registry_file(FILE *in);
void delete_registry_key(WCHAR *reg_key_name);
WCHAR* GetWideString(const char* strA);
CHAR* GetMultiByteString(const WCHAR* strW);
