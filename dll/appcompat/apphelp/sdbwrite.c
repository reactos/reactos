/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Shim database manipulation functions
 * COPYRIGHT:   Copyright 2011 André Hentschel
 *              Copyright 2013 Mislav Blažević
 *              Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#if !defined(SDBWRITE_HOSTTOOL)
#define WIN32_NO_STATUS
#include "windef.h"
#include "ntndk.h"
#include <appcompat/sdbtypes.h>
#include <appcompat/sdbtagid.h>
#else
#include <typedefs.h>
#include <guiddef.h>
#include <sdbtypes.h>
#include <sdbtagid.h>
#endif


#include "sdbpapi.h"
#include "sdbstringtable.h"


/* Local functions */
BOOL WINAPI SdbWriteStringRefTag(PDB pdb, TAG tag, TAGID tagid);
BOOL WINAPI SdbWriteStringTag(PDB pdb, TAG tag, LPCWSTR string);
TAGID WINAPI SdbBeginWriteListTag(PDB pdb, TAG tag);
BOOL WINAPI SdbEndWriteListTag(PDB pdb, TAGID tagid);

/* sdbapi.c */
void WINAPI SdbCloseDatabase(PDB);

/* Copy data to the allocated database */
static void WINAPI SdbpWrite(PDB pdb, const void* data, DWORD size)
{
    ASSERT(pdb->for_write);
    if (pdb->write_iter + size > pdb->size)
    {
        DWORD oldSize = pdb->size;
        /* Round to powers of two to prevent too many reallocations */
        while (pdb->size < pdb->write_iter + size) pdb->size <<= 1;
        pdb->data = SdbReAlloc(pdb->data, pdb->size, oldSize);
    }

    memcpy(pdb->data + pdb->write_iter, data, size);
    pdb->write_iter += size;
}

/* Add a string to the string table (creating it when it did not exist yet),
   returning if it succeeded or not */
static BOOL WINAPI SdbpGetOrAddStringRef(PDB pdb, LPCWSTR string, TAGID* tagid)
{
    PDB buf = pdb->string_buffer;
    ASSERT(pdb->for_write);

    if (pdb->string_buffer == NULL)
    {
        pdb->string_buffer = buf = SdbAlloc(sizeof(DB));
        if (buf == NULL)
            return FALSE;
        buf->size = 128;
        buf->data = SdbAlloc(buf->size);
        buf->for_write = TRUE;
        if (buf->data == NULL)
            return FALSE;
    }

   *tagid = buf->write_iter + sizeof(TAG) + sizeof(DWORD);
   if (SdbpAddStringToTable(&pdb->string_lookup, string, tagid))
       return SdbWriteStringTag(buf, TAG_STRINGTABLE_ITEM, string);

    return pdb->string_lookup != NULL;
}

/* Write the in-memory stringtable to the specified db */
static BOOL WINAPI SdbpWriteStringtable(PDB pdb)
{
    TAGID table;
    PDB buf = pdb->string_buffer;
    if (buf == NULL || pdb->string_lookup == NULL)
        return FALSE;

    table = SdbBeginWriteListTag(pdb, TAG_STRINGTABLE);
    SdbpWrite(pdb, buf->data, buf->write_iter);
    return SdbEndWriteListTag(pdb, table);
}

/**
 * Creates new shim database file
 *
 * If a file already exists on specified path, that file shall be overwritten.
 *
 * @note Use SdbCloseDatabaseWrite to close the database opened with this function.
 *
 * @param [in]  path    Path to the new shim database.
 * @param [in]  type    Type of path. Either DOS_PATH or NT_PATH.
 *
 * @return  Success: Handle to the newly created shim database, NULL otherwise.
 */
PDB WINAPI SdbCreateDatabase(LPCWSTR path, PATH_TYPE type)
{
    static const DWORD version_major = 2, version_minor = 1;
    static const char* magic = "sdbf";
    PDB pdb;

    pdb = SdbpCreate(path, type, TRUE);
    if (!pdb)
        return NULL;

    pdb->size = sizeof(DWORD) + sizeof(DWORD) + (DWORD)strlen(magic);
    pdb->data = SdbAlloc(pdb->size);

    SdbpWrite(pdb, &version_major, sizeof(DWORD));
    SdbpWrite(pdb, &version_minor, sizeof(DWORD));
    SdbpWrite(pdb, magic, (DWORD)strlen(magic));

    return pdb;
}

/**
 * Closes specified database and writes data to file.
 *
 * @param [in]  pdb  Handle to the shim database.
 */
void WINAPI SdbCloseDatabaseWrite(PDB pdb)
{
    ASSERT(pdb->for_write);
    SdbpWriteStringtable(pdb);
    SdbpFlush(pdb);
    SdbCloseDatabase(pdb);
}

/**
 * Writes a tag-only (NULL) entry to the specified shim database.
 *
 * @param [in]  pdb  Handle to the shim database.
 * @param [in]  tag A tag for the entry.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteNULLTag(PDB pdb, TAG tag)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_NULL))
        return FALSE;

    SdbpWrite(pdb, &tag, sizeof(TAG));
    return TRUE;
}

/**
 * Writes a WORD entry to the specified shim database.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  data    WORD entry which will be written to the database.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteWORDTag(PDB pdb, TAG tag, WORD data)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_WORD))
        return FALSE;

    SdbpWrite(pdb, &tag, sizeof(TAG));
    SdbpWrite(pdb, &data, sizeof(data));
    return TRUE;
}

/**
 * Writes a DWORD entry to the specified shim database.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  data    DWORD entry which will be written to the database.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteDWORDTag(PDB pdb, TAG tag, DWORD data)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_DWORD))
        return FALSE;

    SdbpWrite(pdb, &tag, sizeof(TAG));
    SdbpWrite(pdb, &data, sizeof(data));
    return TRUE;
}

/**
 * Writes a DWORD entry to the specified shim database.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  data    QWORD entry which will be written to the database.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteQWORDTag(PDB pdb, TAG tag, QWORD data)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_QWORD))
        return FALSE;

    SdbpWrite(pdb, &tag, sizeof(TAG));
    SdbpWrite(pdb, &data, sizeof(data));
    return TRUE;
}

/**
 * Writes a wide string entry to the specified shim database.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  string  Wide string entry which will be written to the database.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteStringTag(PDB pdb, TAG tag, LPCWSTR string)
{
    DWORD size;

    if (SdbpCheckTagType(tag, TAG_TYPE_STRINGREF))
    {
        TAGID tagid = 0;
        if (!SdbpGetOrAddStringRef(pdb, string, &tagid))
            return FALSE;

        return SdbWriteStringRefTag(pdb, tag, tagid);
    }

    if (!SdbpCheckTagType(tag, TAG_TYPE_STRING))
        return FALSE;

    size = SdbpStrsize(string);
    SdbpWrite(pdb, &tag, sizeof(TAG));
    SdbpWrite(pdb, &size, sizeof(size));
    SdbpWrite(pdb, string, size);
    return TRUE;
}

/**
 * Writes a stringref tag to specified database
 * @note Reference (tagid) is not checked for validity.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tag     TAG which will be written.
 * @param [in]  tagid   TAGID of the string tag refers to.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteStringRefTag(PDB pdb, TAG tag, TAGID tagid)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_STRINGREF))
        return FALSE;

    SdbpWrite(pdb, &tag, sizeof(TAG));
    SdbpWrite(pdb, &tagid, sizeof(tagid));
    return TRUE;
}

/**
 * Writes data the specified shim database.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  data    Pointer to data.
 * @param [in]  size    Number of bytes to write.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteBinaryTag(PDB pdb, TAG tag, const BYTE* data, DWORD size)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_BINARY))
        return FALSE;

    SdbpWrite(pdb, &tag, sizeof(TAG));
    SdbpWrite(pdb, &size, sizeof(size));
    SdbpWrite(pdb, data, size);
    return TRUE;
}

#if !defined(SDBWRITE_HOSTTOOL)
/**
 * Writes data from a file to the specified shim database.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  path    Path of the input file.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteBinaryTagFromFile(PDB pdb, TAG tag, LPCWSTR path)
{
    MEMMAPPED mapped;

    if (!SdbpCheckTagType(tag, TAG_TYPE_BINARY))
        return FALSE;

    if (!SdbpOpenMemMappedFile(path, &mapped))
        return FALSE;

    SdbWriteBinaryTag(pdb, tag, mapped.view, mapped.size);
    SdbpCloseMemMappedFile(&mapped);
    return TRUE;
}
#endif

/**
 * Writes a list tag to specified database All subsequent SdbWrite* functions shall write to
 * newly created list untill TAGID of that list is passed to SdbEndWriteListTag.
 *
 * @param [in]  pdb  Handle to the shim database.
 * @param [in]  tag TAG for the list
 *
 *                  RETURNS Success: TAGID of the newly created list, or TAGID_NULL on failure.
 *
 * @return  A TAGID.
 */
TAGID WINAPI SdbBeginWriteListTag(PDB pdb, TAG tag)
{
    TAGID list_id;
    DWORD dum = 0;

    if (!SdbpCheckTagType(tag, TAG_TYPE_LIST))
        return TAGID_NULL;

    list_id = pdb->write_iter;
    SdbpWrite(pdb, &tag, sizeof(TAG));
    SdbpWrite(pdb, &dum, sizeof(dum)); /* reserve some memory for storing list size */
    return list_id;
}

/**
 * Marks end of the specified list.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tagid   TAGID of the list.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbEndWriteListTag(PDB pdb, TAGID tagid)
{
    ASSERT(pdb->for_write);

    if (!SdbpCheckTagIDType(pdb, tagid, TAG_TYPE_LIST))
        return FALSE;

    /* Write size of list to list tag header */
    *(DWORD*)&pdb->data[tagid + sizeof(TAG)] = pdb->write_iter - tagid - sizeof(TAG) - sizeof(TAGID);
    return TRUE;
}

