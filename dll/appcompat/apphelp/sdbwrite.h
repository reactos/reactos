/*
 * Copyright 2013 Mislav Blažević
 * Copyright 2015,2016 Mark Jansen
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

#ifndef SDBWRITE_H
#define SDBWRITE_H

#ifdef __cplusplus
extern "C" {
#endif

PDB WINAPI SdbCreateDatabase(LPCWSTR path, PATH_TYPE type);
void WINAPI SdbCloseDatabaseWrite(PDB db);
BOOL WINAPI SdbWriteNULLTag(PDB db, TAG tag);
BOOL WINAPI SdbWriteWORDTag(PDB db, TAG tag, WORD data);
BOOL WINAPI SdbWriteDWORDTag(PDB db, TAG tag, DWORD data);
BOOL WINAPI SdbWriteQWORDTag(PDB db, TAG tag, QWORD data);
BOOL WINAPI SdbWriteStringTag(PDB db, TAG tag, LPCWSTR string);
BOOL WINAPI SdbWriteStringRefTag(PDB db, TAG tag, TAGID tagid);
BOOL WINAPI SdbWriteBinaryTag(PDB db, TAG tag, BYTE* data, DWORD size);
BOOL WINAPI SdbWriteBinaryTagFromFile(PDB db, TAG tag, LPCWSTR path);
TAGID WINAPI SdbBeginWriteListTag(PDB db, TAG tag);
BOOL WINAPI SdbEndWriteListTag(PDB db, TAGID tagid);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SDBWRITE_H
