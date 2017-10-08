/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shim database manipulation interface
 * COPYRIGHT:   Copyright 2011 André Hentschel
 *              Copyright 2013 Mislav Blažević
 *              Copyright 2015-2017 Mark Jansen (mark.jansen@reactos.org)
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
BOOL WINAPI SdbWriteBinaryTag(PDB db, TAG tag, const BYTE* data, DWORD size);
BOOL WINAPI SdbWriteBinaryTagFromFile(PDB db, TAG tag, LPCWSTR path);
TAGID WINAPI SdbBeginWriteListTag(PDB db, TAG tag);
BOOL WINAPI SdbEndWriteListTag(PDB db, TAGID tagid);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SDBWRITE_H
