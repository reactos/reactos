/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shim database query functions
 * COPYRIGHT:   Copyright 2011 André Hentschel
 *              Copyright 2013 Mislav Blaževic
 *              Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "windef.h"
#include "apphelp.h"


BOOL WINAPI SdbpReadData(PDB pdb, PVOID dest, DWORD offset, DWORD num)
{
    DWORD size = offset + num;

    /* Either overflow or no data to read */
    if (size <= offset)
        return FALSE;

    /* Overflow */
    if (pdb->size < size)
        return FALSE;

    memcpy(dest, pdb->data + offset, num);
    return TRUE;
}

static DWORD WINAPI SdbpGetTagSize(PDB pdb, TAGID tagid)
{
    WORD type;
    DWORD size;

    type = SdbGetTagFromTagID(pdb, tagid) & TAG_TYPE_MASK;
    if (type == TAG_NULL)
        return 0;

    size = SdbGetTagDataSize(pdb, tagid);
    if (type <= TAG_TYPE_STRINGREF)
        return size += sizeof(TAG);
    else size += (sizeof(TAG) + sizeof(DWORD));

    return size;
}

LPWSTR WINAPI SdbpGetString(PDB pdb, TAGID tagid, PDWORD size)
{
    TAG tag;
    TAGID offset;

    tag = SdbGetTagFromTagID(pdb, tagid);
    if (tag == TAG_NULL)
        return NULL;

    if ((tag & TAG_TYPE_MASK) == TAG_TYPE_STRINGREF)
    {
        /* No stringtable; all references are invalid */
        if (pdb->stringtable == TAGID_NULL)
            return NULL;

        /* TAG_TYPE_STRINGREF contains offset of string relative to stringtable */
        if (!SdbpReadData(pdb, &tagid, tagid + sizeof(TAG), sizeof(TAGID)))
            return NULL;

        offset = pdb->stringtable + tagid + sizeof(TAG) + sizeof(TAGID);
    }
    else if ((tag & TAG_TYPE_MASK) == TAG_TYPE_STRING)
    {
        offset = tagid + sizeof(TAG) + sizeof(TAGID);
    }
    else
    {
        SHIM_ERR("Tag 0x%u at tagid %u is neither a string or reference to string\n", tag, tagid);
        return NULL;
    }

    /* Optionally read string size */
    if (size && !SdbpReadData(pdb, size, offset - sizeof(TAGID), sizeof(*size)))
        return FALSE;

    return (LPWSTR)(&pdb->data[offset]);
}

/**
 * Searches shim database for the tag associated with specified tagid.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tagid   The TAGID of the tag.
 *
 * @return  Success: The tag associated with specified tagid, Failure: TAG_NULL.
 */
TAG WINAPI SdbGetTagFromTagID(PDB pdb, TAGID tagid)
{
    TAG data;
    if (!SdbpReadData(pdb, &data, tagid, sizeof(data)))
        return TAG_NULL;
    return data;
}

/**
 * Retrieves size of data at specified tagid.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tagid   Tagid of tag whose size is queried.
 *
 * @return  Success: Size of data at specified tagid, Failure: 0.
 */
DWORD WINAPI SdbGetTagDataSize(PDB pdb, TAGID tagid)
{
    /* sizes of data types with fixed size */
    static const SIZE_T sizes[6] = {
        0, /* NULL  */ 1, /* BYTE      */
        2, /* WORD  */ 4, /* DWORD     */
        8, /* QWORD */ 4  /* STRINGREF */
    };
    WORD type;
    DWORD size;

    type = SdbGetTagFromTagID(pdb, tagid) & TAG_TYPE_MASK;
    if (type == TAG_NULL)
        return 0;

    if (type <= TAG_TYPE_STRINGREF)
        return sizes[(type >> 12) - 1];

    /* tag with dynamic size (e.g. list): must read size */
    if (!SdbpReadData(pdb, &size, tagid + sizeof(TAG), sizeof(size)))
        return 0;

    return size;
}

/**
 * Searches shim database for a child of specified parent tag.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  parent  TAGID of parent.
 *
 * @return  Success: TAGID of child tag, Failure: TAGID_NULL.
 */
TAGID WINAPI SdbGetFirstChild(PDB pdb, TAGID parent)
{
    /* if we are at beginning of database */
    if (parent == TAGID_ROOT)
    {
        /* header only database: no tags */
        if (pdb->size <= _TAGID_ROOT)
            return TAGID_NULL;
        /* return *real* root tagid */
        else return _TAGID_ROOT;
    }

    /* only list tag can have children */
    if ((SdbGetTagFromTagID(pdb, parent) & TAG_TYPE_MASK) != TAG_TYPE_LIST)
        return TAGID_NULL;

    /* first child is sizeof(TAG) + sizeof(DWORD) bytes after beginning of list */
    return parent + sizeof(TAG) + sizeof(DWORD);
}

/**
 * Searches shim database for next child of specified parent tag.
 *
 * @param [in]  pdb          Handle to the shim database.
 * @param [in]  parent      TAGID of parent.
 * @param [in]  prev_child  TAGID of previous child.
 *
 * @return  Success: TAGID of next child tag, Failure: TAGID_NULL.
 */
TAGID WINAPI SdbGetNextChild(PDB pdb, TAGID parent, TAGID prev_child)
{
    TAGID next_child;
    DWORD prev_child_size, parent_size;

    prev_child_size = SdbpGetTagSize(pdb, prev_child);
    if (prev_child_size == 0)
        return TAGID_NULL;

    /* Bound check */
    next_child = prev_child + prev_child_size;
    if (next_child >= pdb->size)
        return TAGID_NULL;

    if (parent == TAGID_ROOT)
        return next_child;

    parent_size = SdbpGetTagSize(pdb, parent);
    if (parent_size == 0)
        return TAGID_NULL;

    /* Specified parent has no more children */
    if (next_child >= parent + parent_size)
        return TAGID_NULL;

    return next_child;
}

/**
 * Searches shim database for a tag within specified domain.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  parent  TAGID of parent.
 * @param [in]  tag     TAG to be located.
 *
 * @return  Success: TAGID of first matching tag, Failure: TAGID_NULL.
 */
TAGID WINAPI SdbFindFirstTag(PDB pdb, TAGID parent, TAG tag)
{
    TAGID iter;

    iter = SdbGetFirstChild(pdb, parent);
    while (iter != TAGID_NULL)
    {
        if (SdbGetTagFromTagID(pdb, iter) == tag)
            return iter;
        iter = SdbGetNextChild(pdb, parent, iter);
    }
    return TAGID_NULL;
}

/**
 * Searches shim database for a next tag which matches prev_child within parent's domain.
 *
 * @param [in]  pdb          Handle to the shim database.
 * @param [in]  parent      TAGID of parent.
 * @param [in]  prev_child  TAGID of previous match.
 *
 * @return  Success: TAGID of next match, Failure: TAGID_NULL.
 */
TAGID WINAPI SdbFindNextTag(PDB pdb, TAGID parent, TAGID prev_child)
{
    TAG tag;
    TAGID iter;

    tag = SdbGetTagFromTagID(pdb, prev_child);
    iter = SdbGetNextChild(pdb, parent, prev_child);

    while (iter != TAGID_NULL)
    {
        if (SdbGetTagFromTagID(pdb, iter) == tag)
            return iter;
        iter = SdbGetNextChild(pdb, parent, iter);
    }
    return TAGID_NULL;
}

/**
 * Searches shim database for string associated with specified tagid and copies string into a
 * buffer.
 *
 * If size parameter is less than number of characters in string, this function shall fail and
 * no data shall be copied.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tagid   TAGID of string or stringref associated with the string.
 * @param [out] buffer  Buffer in which string will be copied.
 * @param [in]  size    Number of characters to copy.
 *
 * @return  TRUE if string was successfully copied to the buffer FALSE if string was not copied
 *          to the buffer.
 */
BOOL WINAPI SdbReadStringTag(PDB pdb, TAGID tagid, LPWSTR buffer, DWORD size)
{
    LPWSTR string;
    DWORD string_size;

    string = SdbpGetString(pdb, tagid, &string_size);
    if (!string)
        return FALSE;

    /* Check if buffer is too small */
    if (size * sizeof(WCHAR) < string_size)
        return FALSE;

    memcpy(buffer, string, string_size);
    return TRUE;
}

/**
 * Reads WORD value at specified tagid.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tagid   TAGID of WORD value.
 * @param [in]  ret     Default return value in case function fails.
 *
 * @return  Success: WORD value at specified tagid, or ret on failure.
 */
WORD WINAPI SdbReadWORDTag(PDB pdb, TAGID tagid, WORD ret)
{
    if (SdbpCheckTagIDType(pdb, tagid, TAG_TYPE_WORD))
        SdbpReadData(pdb, &ret, tagid + 2, sizeof(WORD));
    return ret;
}

/**
 * Reads DWORD value at specified tagid.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tagid   TAGID of DWORD value.
 * @param [in]  ret     Default return value in case function fails.
 *
 * @return  Success: DWORD value at specified tagid, otherwise ret.
 */
DWORD WINAPI SdbReadDWORDTag(PDB pdb, TAGID tagid, DWORD ret)
{
    if (SdbpCheckTagIDType(pdb, tagid, TAG_TYPE_DWORD))
        SdbpReadData(pdb, &ret, tagid + 2, sizeof(DWORD));
    return ret;
}

/**
 * Reads QWORD value at specified tagid.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tagid   TAGID of QWORD value.
 * @param [in]  ret     Default return value in case function fails.
 *
 * @return  Success: QWORD value at specified tagid, otherwise ret.
 */
QWORD WINAPI SdbReadQWORDTag(PDB pdb, TAGID tagid, QWORD ret)
{
    if (SdbpCheckTagIDType(pdb, tagid, TAG_TYPE_QWORD))
        SdbpReadData(pdb, &ret, tagid + sizeof(TAG), sizeof(QWORD));
    return ret;
}

/**
 * Reads binary data at specified tagid.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tagid   TAGID of binary data.
 * @param [out] buffer  Buffer in which data will be copied.
 * @param [in]  size    Size of the buffer.
 *
 * @return  TRUE if data was successfully written, or FALSE otherwise.
 */
BOOL WINAPI SdbReadBinaryTag(PDB pdb, TAGID tagid, PBYTE buffer, DWORD size)
{
    DWORD data_size = 0;

    if (SdbpCheckTagIDType(pdb, tagid, TAG_TYPE_BINARY))
    {
        SdbpReadData(pdb, &data_size, tagid + sizeof(TAG), sizeof(data_size));
        if (size >= data_size)
            return SdbpReadData(pdb, buffer, tagid + sizeof(TAG) + sizeof(data_size), data_size);
    }

    return FALSE;
}

/**
 * Retrieves binary data at specified tagid.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tagid   TAGID of binary data.
 *
 * @return  Success: Pointer to binary data at specified tagid, or NULL on failure.
 */
PVOID WINAPI SdbGetBinaryTagData(PDB pdb, TAGID tagid)
{
    if (!SdbpCheckTagIDType(pdb, tagid, TAG_TYPE_BINARY))
        return NULL;
    return &pdb->data[tagid + sizeof(TAG) + sizeof(DWORD)];
}

/**
 * Searches shim database for string associated with specified tagid.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [in]  tagid   TAGID of string or stringref associated with the string.
 *
 * @return  the LPWSTR associated with specified tagid, or NULL on failure.
 */
LPWSTR WINAPI SdbGetStringTagPtr(PDB pdb, TAGID tagid)
{
    return SdbpGetString(pdb, tagid, NULL);
}

/**
 * Reads binary data at specified tagid.
 *
 * @param [in]  pdb      Handle to the shim database.
 * @param [out] Guid    Database ID.
 *
 * @return  true if the ID was found FALSE otherwise.
 */
BOOL WINAPI SdbGetDatabaseID(PDB pdb, GUID* Guid)
{
    if(SdbIsNullGUID(&pdb->database_id))
    {
        TAGID root = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
        if(root != TAGID_NULL)
        {
            TAGID id = SdbFindFirstTag(pdb, root, TAG_DATABASE_ID);
            if(id != TAGID_NULL)
            {
                if(!SdbReadBinaryTag(pdb, id, (PBYTE)&pdb->database_id, sizeof(pdb->database_id)))
                {
                    memset(&pdb->database_id, 0, sizeof(pdb->database_id));
                }
            }
            else
            {
                /* Should we silence this if we are opening a system db? */
                SHIM_ERR("Failed to get the database id\n");
            }
        }
        else
        {
            /* Should we silence this if we are opening a system db? */
            SHIM_ERR("Failed to get root tag\n");
        }
    }
    if(!SdbIsNullGUID(&pdb->database_id))
    {
        memcpy(Guid, &pdb->database_id, sizeof(pdb->database_id));
        return TRUE;
    }
    return FALSE;
}

