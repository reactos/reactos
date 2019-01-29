/*
 *      Typelib v2 (MSFT) generation
 *
 *	Copyright 2004  Alastair Bridgewater
 *                2004, 2005 Huw Davies
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
 *
 * --------------------------------------------------------------------------------------
 *  Known problems:
 *
 *    Badly incomplete.
 *
 *    Only works on little-endian systems.
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#define NONAMELESSUNION

#ifdef __REACTOS__
#include <typedefs.h>
#include <nls.h>
#else
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#endif

#include "widl.h"
#include "typelib.h"
#include "typelib_struct.h"
#include "utils.h"
#include "header.h"
#include "hash.h"
#include "typetree.h"
#include "parser.h"
#include "typegen.h"

#ifdef __REACTOS__
#define S_OK           0
#define S_FALSE        1
#define E_OUTOFMEMORY  ((HRESULT)0x8007000EL)
#endif

enum MSFT_segment_index {
    MSFT_SEG_TYPEINFO = 0,  /* type information */
    MSFT_SEG_IMPORTINFO,    /* import information */
    MSFT_SEG_IMPORTFILES,   /* import filenames */
    MSFT_SEG_REFERENCES,    /* references (?) */
    MSFT_SEG_GUIDHASH,      /* hash table for guids? */
    MSFT_SEG_GUID,          /* guid storage */
    MSFT_SEG_NAMEHASH,      /* hash table for names */
    MSFT_SEG_NAME,          /* name storage */
    MSFT_SEG_STRING,        /* string storage */
    MSFT_SEG_TYPEDESC,      /* type descriptions */
    MSFT_SEG_ARRAYDESC,     /* array descriptions */
    MSFT_SEG_CUSTDATA,      /* custom data */
    MSFT_SEG_CUSTDATAGUID,  /* custom data guids */
    MSFT_SEG_UNKNOWN,       /* ??? */
    MSFT_SEG_UNKNOWN2,      /* ??? */
    MSFT_SEG_MAX            /* total number of segments */
};

typedef struct tagMSFT_ImpFile {
    int guid;
    LCID lcid;
    int version;
    char filename[0]; /* preceded by two bytes of encoded (length << 2) + flags in the low two bits. */
} MSFT_ImpFile;

typedef struct _msft_typelib_t
{
    typelib_t *typelib;
    MSFT_Header typelib_header;
    MSFT_pSeg typelib_segdir[MSFT_SEG_MAX];
    unsigned char *typelib_segment_data[MSFT_SEG_MAX];
    int typelib_segment_block_length[MSFT_SEG_MAX];

    INT typelib_typeinfo_offsets[0x200]; /* Hope that's enough. */

    INT *typelib_namehash_segment;
    INT *typelib_guidhash_segment;

    INT help_string_dll_offset;

    struct _msft_typeinfo_t *typeinfos;
    struct _msft_typeinfo_t *last_typeinfo;
} msft_typelib_t;

typedef struct _msft_typeinfo_t
{
    msft_typelib_t *typelib;
    MSFT_TypeInfoBase *typeinfo;

    int typekind;

    unsigned int var_data_allocated;
    int *var_data;

    unsigned int func_data_allocated;
    int *func_data;

    int vars_allocated;
    int *var_indices;
    int *var_names;
    int *var_offsets;

    int funcs_allocated;
    int *func_indices;
    int *func_names;
    int *func_offsets;

    int datawidth;

    struct _msft_typeinfo_t *next_typeinfo;
} msft_typeinfo_t;



/*================== Internal functions ===================================*/

/****************************************************************************
 *	ctl2_init_header
 *
 *  Initializes the type library header of a new typelib.
 */
static void ctl2_init_header(
	msft_typelib_t *typelib) /* [I] The typelib to initialize. */
{
    typelib->typelib_header.magic1 = 0x5446534d;
    typelib->typelib_header.magic2 = 0x00010002;
    typelib->typelib_header.posguid = -1;
    typelib->typelib_header.lcid = 0x0409;
    typelib->typelib_header.lcid2 = 0x0;
    typelib->typelib_header.varflags = 0x40;
    typelib->typelib_header.version = 0;
    typelib->typelib_header.flags = 0;
    typelib->typelib_header.nrtypeinfos = 0;
    typelib->typelib_header.helpstring = -1;
    typelib->typelib_header.helpstringcontext = 0;
    typelib->typelib_header.helpcontext = 0;
    typelib->typelib_header.nametablecount = 0;
    typelib->typelib_header.nametablechars = 0;
    typelib->typelib_header.NameOffset = -1;
    typelib->typelib_header.helpfile = -1;
    typelib->typelib_header.CustomDataOffset = -1;
    typelib->typelib_header.res44 = 0x20;
    typelib->typelib_header.res48 = 0x80;
    typelib->typelib_header.dispatchpos = -1;
    typelib->typelib_header.nimpinfos = 0;
}

/****************************************************************************
 *	ctl2_init_segdir
 *
 *  Initializes the segment directory of a new typelib.
 */
static void ctl2_init_segdir(
	msft_typelib_t *typelib) /* [I] The typelib to initialize. */
{
    int i;
    MSFT_pSeg *segdir;

    segdir = &typelib->typelib_segdir[MSFT_SEG_TYPEINFO];

    for (i = 0; i < MSFT_SEG_MAX; i++) {
	segdir[i].offset = -1;
	segdir[i].length = 0;
	segdir[i].res08 = -1;
	segdir[i].res0c = 0x0f;
    }
}

/****************************************************************************
 *	ctl2_hash_guid
 *
 *  Generates a hash key from a GUID.
 *
 * RETURNS
 *
 *  The hash key for the GUID.
 */
static int ctl2_hash_guid(
	REFGUID guid)                /* [I] The guid to hash. */
{
    int hash;
    int i;

    hash = 0;
    for (i = 0; i < 8; i ++) {
	hash ^= ((const short *)guid)[i];
    }

    return hash & 0x1f;
}

/****************************************************************************
 *	ctl2_find_guid
 *
 *  Locates a guid in a type library.
 *
 * RETURNS
 *
 *  The offset into the GUID segment of the guid, or -1 if not found.
 */
static int ctl2_find_guid(
	msft_typelib_t *typelib,   /* [I] The typelib to operate against. */
	int hash_key,              /* [I] The hash key for the guid. */
	REFGUID guid)              /* [I] The guid to find. */
{
    int offset;
    MSFT_GuidEntry *guidentry;

    offset = typelib->typelib_guidhash_segment[hash_key];
    while (offset != -1) {
	guidentry = (MSFT_GuidEntry *)&typelib->typelib_segment_data[MSFT_SEG_GUID][offset];

	if (!memcmp(guidentry, guid, sizeof(GUID))) return offset;

	offset = guidentry->next_hash;
    }

    return offset;
}

/****************************************************************************
 *	ctl2_find_name
 *
 *  Locates a name in a type library.
 *
 * RETURNS
 *
 *  The offset into the NAME segment of the name, or -1 if not found.
 *
 * NOTES
 *
 *  The name must be encoded as with ctl2_encode_name().
 */
static int ctl2_find_name(
	msft_typelib_t *typelib,   /* [I] The typelib to operate against. */
	char *name)                /* [I] The encoded name to find. */
{
    int offset;
    int *namestruct;

    offset = typelib->typelib_namehash_segment[name[2] & 0x7f];
    while (offset != -1) {
	namestruct = (int *)&typelib->typelib_segment_data[MSFT_SEG_NAME][offset];

	if (!((namestruct[2] ^ *((int *)name)) & 0xffff00ff)) {
	    /* hash codes and lengths match, final test */
	    if (!strncasecmp(name+4, (void *)(namestruct+3), name[0])) break;
	}

	/* move to next item in hash bucket */
	offset = namestruct[1];
    }

    return offset;
}

/****************************************************************************
 *	ctl2_encode_name
 *
 *  Encodes a name string to a form suitable for storing into a type library
 *  or comparing to a name stored in a type library.
 *
 * RETURNS
 *
 *  The length of the encoded name, including padding and length+hash fields.
 *
 * NOTES
 *
 *  Will throw an exception if name or result are NULL. Is not multithread
 *  safe in the slightest.
 */
static int ctl2_encode_name(
	msft_typelib_t *typelib,   /* [I] The typelib to operate against (used for LCID only). */
	const char *name,          /* [I] The name string to encode. */
	char **result)             /* [O] A pointer to a pointer to receive the encoded name. */
{
    int length;
    static char converted_name[0x104];
    int offset;
    int value;

    length = strlen(name);
    memcpy(converted_name + 4, name, length);

    converted_name[length + 4] = 0;


    value = lhash_val_of_name_sys(typelib->typelib_header.varflags & 0x0f, typelib->typelib_header.lcid, converted_name + 4);

#ifdef WORDS_BIGENDIAN
    converted_name[3] = length & 0xff;
    converted_name[2] = 0x00;
    converted_name[1] = value;
    converted_name[0] = value >> 8;
#else
    converted_name[0] = length & 0xff;
    converted_name[1] = 0x00;
    converted_name[2] = value;
    converted_name[3] = value >> 8;
#endif

    for (offset = (4 - length) & 3; offset; offset--) converted_name[length + offset + 3] = 0x57;

    *result = converted_name;

    return (length + 7) & ~3;
}

/****************************************************************************
 *	ctl2_encode_string
 *
 *  Encodes a string to a form suitable for storing into a type library or
 *  comparing to a string stored in a type library.
 *
 * RETURNS
 *
 *  The length of the encoded string, including padding and length fields.
 *
 * NOTES
 *
 *  Will throw an exception if string or result are NULL. Is not multithread
 *  safe in the slightest.
 */
static int ctl2_encode_string(
	const char *string,        /* [I] The string to encode. */
	char **result)             /* [O] A pointer to a pointer to receive the encoded string. */
{
    int length;
    static char converted_string[0x104];
    int offset;

    length = strlen(string);
    memcpy(converted_string + 2, string, length);

#ifdef WORDS_BIGENDIAN
    converted_string[1] = length & 0xff;
    converted_string[0] = (length >> 8) & 0xff;
#else
    converted_string[0] = length & 0xff;
    converted_string[1] = (length >> 8) & 0xff;
#endif

    if(length < 3) { /* strings of this length are padded with up to 8 bytes incl the 2 byte length */
        for(offset = 0; offset < 4; offset++)
            converted_string[length + offset + 2] = 0x57;
        length += 4;
    }
    for (offset = (4 - (length + 2)) & 3; offset; offset--) converted_string[length + offset + 1] = 0x57;

    *result = converted_string;

    return (length + 5) & ~3;
}

/****************************************************************************
 *	ctl2_alloc_segment
 *
 *  Allocates memory from a segment in a type library.
 *
 * RETURNS
 *
 *  Success: The offset within the segment of the new data area.
 *
 * BUGS
 *
 *  Does not (yet) handle the case where the allocated segment memory needs to grow.
 */
static int ctl2_alloc_segment(
	msft_typelib_t *typelib,         /* [I] The type library in which to allocate. */
	enum MSFT_segment_index segment, /* [I] The segment in which to allocate. */
	int size,                        /* [I] The amount to allocate. */
	int block_size)                  /* [I] Initial allocation block size, or 0 for default. */
{
    int offset;

    if(!typelib->typelib_segment_data[segment]) {
	if (!block_size) block_size = 0x2000;

	typelib->typelib_segment_block_length[segment] = block_size;
	typelib->typelib_segment_data[segment] = xmalloc(block_size);
	if (!typelib->typelib_segment_data[segment]) return -1;
	memset(typelib->typelib_segment_data[segment], 0x57, block_size);
    }

    while ((typelib->typelib_segdir[segment].length + size) > typelib->typelib_segment_block_length[segment]) {
	unsigned char *block;

	block_size = typelib->typelib_segment_block_length[segment];
	block = xrealloc(typelib->typelib_segment_data[segment], block_size << 1);

	if (segment == MSFT_SEG_TYPEINFO) {
	    /* TypeInfos have a direct pointer to their memory space, so we have to fix them up. */
	    msft_typeinfo_t *typeinfo;

	    for (typeinfo = typelib->typeinfos; typeinfo; typeinfo = typeinfo->next_typeinfo) {
		typeinfo->typeinfo = (void *)&block[((unsigned char *)typeinfo->typeinfo) - typelib->typelib_segment_data[segment]];
	    }
	}

	memset(block + block_size, 0x57, block_size);
	typelib->typelib_segment_block_length[segment] = block_size << 1;
	typelib->typelib_segment_data[segment] = block;
    }

    offset = typelib->typelib_segdir[segment].length;
    typelib->typelib_segdir[segment].length += size;

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_typeinfo
 *
 *  Allocates and initializes a typeinfo structure in a type library.
 *
 * RETURNS
 *
 *  Success: The offset of the new typeinfo.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_typeinfo(
	msft_typelib_t *typelib,   /* [I] The type library to allocate in. */
	int nameoffset)            /* [I] The offset of the name for this typeinfo. */
{
    int offset;
    MSFT_TypeInfoBase *typeinfo;

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEINFO, sizeof(MSFT_TypeInfoBase), 0);

    typelib->typelib_typeinfo_offsets[typelib->typelib_header.nrtypeinfos++] = offset;

    typeinfo = (void *)(typelib->typelib_segment_data[MSFT_SEG_TYPEINFO] + offset);

    typeinfo->typekind = (typelib->typelib_header.nrtypeinfos - 1) << 16;
    typeinfo->memoffset = -1; /* should be EOF if no elements */
    typeinfo->res2 = 0;
    typeinfo->res3 = -1;
    typeinfo->res4 = 3;
    typeinfo->res5 = 0;
    typeinfo->cElement = 0;
    typeinfo->res7 = 0;
    typeinfo->res8 = 0;
    typeinfo->res9 = 0;
    typeinfo->resA = 0;
    typeinfo->posguid = -1;
    typeinfo->flags = 0;
    typeinfo->NameOffset = nameoffset;
    typeinfo->version = 0;
    typeinfo->docstringoffs = -1;
    typeinfo->helpstringcontext = 0;
    typeinfo->helpcontext = 0;
    typeinfo->oCustData = -1;
    typeinfo->cbSizeVft = 0;
    typeinfo->cImplTypes = 0;
    typeinfo->size = 0;
    typeinfo->datatype1 = -1;
    typeinfo->datatype2 = 0;
    typeinfo->res18 = 0;
    typeinfo->res19 = -1;

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_guid
 *
 *  Allocates and initializes a GUID structure in a type library. Also updates
 *  the GUID hash table as needed.
 *
 * RETURNS
 *
 *  Success: The offset of the new GUID.
 */
static int ctl2_alloc_guid(
	msft_typelib_t *typelib,   /* [I] The type library to allocate in. */
	MSFT_GuidEntry *guid)      /* [I] The GUID to store. */
{
    int offset;
    MSFT_GuidEntry *guid_space;
    int hash_key;

    chat("adding uuid {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
         guid->guid.Data1, guid->guid.Data2, guid->guid.Data3,
         guid->guid.Data4[0], guid->guid.Data4[1], guid->guid.Data4[2], guid->guid.Data4[3],
         guid->guid.Data4[4], guid->guid.Data4[5], guid->guid.Data4[6], guid->guid.Data4[7]);

    hash_key = ctl2_hash_guid(&guid->guid);

    offset = ctl2_find_guid(typelib, hash_key, &guid->guid);
    if (offset != -1)
    {
        if (is_warning_enabled(2368))
            warning("duplicate uuid {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                    guid->guid.Data1, guid->guid.Data2, guid->guid.Data3,
                    guid->guid.Data4[0], guid->guid.Data4[1], guid->guid.Data4[2], guid->guid.Data4[3],
                    guid->guid.Data4[4], guid->guid.Data4[5], guid->guid.Data4[6], guid->guid.Data4[7]);
        return -1;
    }

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_GUID, sizeof(MSFT_GuidEntry), 0);

    guid_space = (void *)(typelib->typelib_segment_data[MSFT_SEG_GUID] + offset);
    *guid_space = *guid;

    guid_space->next_hash = typelib->typelib_guidhash_segment[hash_key];
    typelib->typelib_guidhash_segment[hash_key] = offset;

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_name
 *
 *  Allocates and initializes a name within a type library. Also updates the
 *  name hash table as needed.
 *
 * RETURNS
 *
 *  Success: The offset within the segment of the new name.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_name(
	msft_typelib_t *typelib,  /* [I] The type library to allocate in. */
	const char *name)         /* [I] The name to store. */
{
    int length;
    int offset;
    MSFT_NameIntro *name_space;
    char *encoded_name;

    length = ctl2_encode_name(typelib, name, &encoded_name);

    offset = ctl2_find_name(typelib, encoded_name);
    if (offset != -1) return offset;

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_NAME, length + 8, 0);

    name_space = (void *)(typelib->typelib_segment_data[MSFT_SEG_NAME] + offset);
    name_space->hreftype = -1;
    name_space->next_hash = -1;
    memcpy(&name_space->namelen, encoded_name, length);

    if (typelib->typelib_namehash_segment[encoded_name[2] & 0x7f] != -1)
	name_space->next_hash = typelib->typelib_namehash_segment[encoded_name[2] & 0x7f];

    typelib->typelib_namehash_segment[encoded_name[2] & 0x7f] = offset;

    typelib->typelib_header.nametablecount += 1;
    typelib->typelib_header.nametablechars += *encoded_name;

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_string
 *
 *  Allocates and initializes a string in a type library.
 *
 * RETURNS
 *
 *  Success: The offset within the segment of the new string.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_string(
	msft_typelib_t *typelib,  /* [I] The type library to allocate in. */
	const char *string)       /* [I] The string to store. */
{
    int length;
    int offset;
    unsigned char *string_space;
    char *encoded_string;

    length = ctl2_encode_string(string, &encoded_string);

    for (offset = 0; offset < typelib->typelib_segdir[MSFT_SEG_STRING].length;
	 offset += (((typelib->typelib_segment_data[MSFT_SEG_STRING][offset + 1] << 8) |
	      typelib->typelib_segment_data[MSFT_SEG_STRING][offset + 0]) + 5) & ~3) {
	if (!memcmp(encoded_string, typelib->typelib_segment_data[MSFT_SEG_STRING] + offset, length)) return offset;
    }

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_STRING, length, 0);

    string_space = typelib->typelib_segment_data[MSFT_SEG_STRING] + offset;
    memcpy(string_space, encoded_string, length);

    return offset;
}

/****************************************************************************
 *	alloc_msft_importinfo
 *
 *  Allocates and initializes an import information structure in a type library.
 *
 * RETURNS
 *
 *  Success: The offset of the new importinfo.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int alloc_msft_importinfo(
        msft_typelib_t *typelib,   /* [I] The type library to allocate in. */
        MSFT_ImpInfo *impinfo)     /* [I] The import information to store. */
{
    int offset;
    MSFT_ImpInfo *impinfo_space;

    for (offset = 0;
	 offset < typelib->typelib_segdir[MSFT_SEG_IMPORTINFO].length;
	 offset += sizeof(MSFT_ImpInfo)) {
	if (!memcmp(&(typelib->typelib_segment_data[MSFT_SEG_IMPORTINFO][offset]),
		    impinfo, sizeof(MSFT_ImpInfo))) {
	    return offset;
	}
    }

    impinfo->flags |= typelib->typelib_header.nimpinfos++;

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_IMPORTINFO, sizeof(MSFT_ImpInfo), 0);

    impinfo_space = (void *)(typelib->typelib_segment_data[MSFT_SEG_IMPORTINFO] + offset);
    *impinfo_space = *impinfo;

    return offset;
}

/****************************************************************************
 *	alloc_importfile
 *
 *  Allocates and initializes an import file definition in a type library.
 *
 * RETURNS
 *
 *  Success: The offset of the new importinfo.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int alloc_importfile(
        msft_typelib_t *typelib,   /* [I] The type library to allocate in. */
        int guidoffset,            /* [I] The offset to the GUID for the imported library. */
        int major_version,         /* [I] The major version number of the imported library. */
        int minor_version,         /* [I] The minor version number of the imported library. */
        const char *filename)      /* [I] The filename of the imported library. */
{
    int length;
    int offset;
    MSFT_ImpFile *importfile;
    char *encoded_string;

    length = ctl2_encode_string(filename, &encoded_string);

    encoded_string[0] <<= 2;
    encoded_string[0] |= 1;

    for (offset = 0; offset < typelib->typelib_segdir[MSFT_SEG_IMPORTFILES].length;
	 offset += (((typelib->typelib_segment_data[MSFT_SEG_IMPORTFILES][offset + 0xd] << 8) |
	              typelib->typelib_segment_data[MSFT_SEG_IMPORTFILES][offset + 0xc]) >> 2) + 0xc) {
	if (!memcmp(encoded_string, typelib->typelib_segment_data[MSFT_SEG_IMPORTFILES] + offset + 0xc, length)) return offset;
    }

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_IMPORTFILES, length + 0xc, 0);

    importfile = (MSFT_ImpFile *)&typelib->typelib_segment_data[MSFT_SEG_IMPORTFILES][offset];
    importfile->guid = guidoffset;
    importfile->lcid = typelib->typelib_header.lcid2;
    importfile->version = major_version | (minor_version << 16);
    memcpy(&importfile->filename, encoded_string, length);

    return offset;
}

static void alloc_importinfo(msft_typelib_t *typelib, importinfo_t *importinfo)
{
    importlib_t *importlib = importinfo->importlib;

    chat("alloc_importinfo: %s\n", importinfo->name);

    if(!importlib->allocated) {
        MSFT_GuidEntry guid;
        int guid_idx;

        chat("allocating importlib %s\n", importlib->name);

        importlib->allocated = -1;

        memcpy(&guid.guid, &importlib->guid, sizeof(GUID));
        guid.hreftype = 2;

        guid_idx = ctl2_alloc_guid(typelib, &guid);

        alloc_importfile(typelib, guid_idx, importlib->version&0xffff,
                         importlib->version>>16, importlib->name);
    }

    if(importinfo->offset == -1 || !(importinfo->flags & MSFT_IMPINFO_OFFSET_IS_GUID)) {
        MSFT_ImpInfo impinfo;

        impinfo.flags = importinfo->flags;
        impinfo.oImpFile = 0;

        if(importinfo->flags & MSFT_IMPINFO_OFFSET_IS_GUID) {
            MSFT_GuidEntry guid;

            guid.hreftype = 0;
            memcpy(&guid.guid, &importinfo->guid, sizeof(GUID));

            impinfo.oGuid = ctl2_alloc_guid(typelib, &guid);

            importinfo->offset = alloc_msft_importinfo(typelib, &impinfo);

            typelib->typelib_segment_data[MSFT_SEG_GUID][impinfo.oGuid+sizeof(GUID)]
                = importinfo->offset+1;

            if(!strcmp(importinfo->name, "IDispatch"))
                typelib->typelib_header.dispatchpos = importinfo->offset+1;
        }else {
            impinfo.oGuid = importinfo->id;
            importinfo->offset = alloc_msft_importinfo(typelib, &impinfo);
        }
    }
}

static importinfo_t *find_importinfo(msft_typelib_t *typelib, const char *name)
{
    importlib_t *importlib;
    int i;

    chat("search importlib %s\n", name);

    if(!name)
        return NULL;

    LIST_FOR_EACH_ENTRY( importlib, &typelib->typelib->importlibs, importlib_t, entry )
    {
        for(i=0; i < importlib->ntypeinfos; i++) {
            if(!strcmp(name, importlib->importinfos[i].name)) {
                chat("Found %s in importlib.\n", name);
                return importlib->importinfos+i;
            }
        }
    }

    return NULL;
}

static void add_structure_typeinfo(msft_typelib_t *typelib, type_t *structure);
static void add_interface_typeinfo(msft_typelib_t *typelib, type_t *interface);
static void add_enum_typeinfo(msft_typelib_t *typelib, type_t *enumeration);
static void add_union_typeinfo(msft_typelib_t *typelib, type_t *tunion);
static void add_coclass_typeinfo(msft_typelib_t *typelib, type_t *cls);
static void add_dispinterface_typeinfo(msft_typelib_t *typelib, type_t *dispinterface);


/****************************************************************************
 *	encode_type
 *
 *  Encodes a type, storing information in the TYPEDESC and ARRAYDESC
 *  segments as needed.
 *
 * RETURNS
 *
 *  Success: 0.
 *  Failure: -1.
 */
static int encode_type(
	msft_typelib_t *typelib,   /* [I] The type library in which to encode the TYPEDESC. */
        int vt,                    /* [I] vt to encode */
	type_t *type,              /* [I] type */
	int *encoded_type,         /* [O] The encoded type description. */
	int *decoded_size)         /* [O] The total size of the unencoded TYPEDESCs, including nested descs. */
{
    int default_type;
    int scratch;
    int typeoffset;
    int *typedata;
    int target_type;
    int child_size = 0;

    chat("encode_type vt %d type %p\n", vt, type);

    default_type = 0x80000000 | (vt << 16) | vt;
    if (!decoded_size) decoded_size = &scratch;

    *decoded_size = 0;

    switch (vt) {
    case VT_I1:
    case VT_UI1:
	*encoded_type = default_type;
	break;

    case VT_INT:
	*encoded_type = 0x80000000 | (VT_I4 << 16) | VT_INT;
	break;

    case VT_UINT:
	*encoded_type = 0x80000000 | (VT_UI4 << 16) | VT_UINT;
	break;

    case VT_UI2:
    case VT_I2:
    case VT_BOOL:
	*encoded_type = default_type;
	break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
    case VT_HRESULT:
	*encoded_type = default_type;
	break;

    case VT_R8:
    case VT_I8:
    case VT_UI8:
	*encoded_type = default_type;
	break;

    case VT_CY:
    case VT_DATE:
	*encoded_type = default_type;
	break;

    case VT_DECIMAL:
        *encoded_type = default_type;
        break;

    case VT_VOID:
	*encoded_type = 0x80000000 | (VT_EMPTY << 16) | vt;
	break;

    case VT_UNKNOWN:
    case VT_DISPATCH:
    case VT_BSTR:
        *encoded_type = default_type;
        break;

    case VT_VARIANT:
        *encoded_type = default_type;
        break;

    case VT_LPSTR:
    case VT_LPWSTR:
        *encoded_type = 0xfffe0000 | vt;
        break;

    case VT_PTR:
      {
        int next_vt;
        for(next_vt = 0; is_ptr(type); type = type_pointer_get_ref(type)) {
            next_vt = get_type_vt(type_pointer_get_ref(type));
            if (next_vt != 0)
                break;
        }
        /* if no type found then it must be void */
        if (next_vt == 0)
            next_vt = VT_VOID;

        encode_type(typelib, next_vt, type_pointer_get_ref(type),
                    &target_type, &child_size);
        /* these types already have an implicit pointer, so we don't need to
         * add another */
        if(next_vt == VT_DISPATCH || next_vt == VT_UNKNOWN) {
            chat("encode_type: skipping ptr\n");
            *encoded_type = target_type;
            *decoded_size = child_size;
            break;
        }

	for (typeoffset = 0; typeoffset < typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if (((typedata[0] & 0xffff) == VT_PTR) && (typedata[1] == target_type)) break;
	}

	if (typeoffset == typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    int mix_field;
	    
	    if (target_type & 0x80000000) {
		mix_field = ((target_type >> 16) & 0x3fff) | VT_BYREF;
	    } else {
		typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][target_type];
		mix_field = ((typedata[0] >> 16) == 0x7fff)? 0x7fff: 0x7ffe;
	    }

	    typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (mix_field << 16) | VT_PTR;
	    typedata[1] = target_type;
	}

	*encoded_type = typeoffset;

	*decoded_size = 8 /*sizeof(TYPEDESC)*/ + child_size;
        break;
    }

    case VT_SAFEARRAY:
	{
	type_t *element_type = type_alias_get_aliasee(type_array_get_element(type));
	int next_vt = get_type_vt(element_type);

	encode_type(typelib, next_vt, type_alias_get_aliasee(type_array_get_element(type)),
        &target_type, &child_size);

	for (typeoffset = 0; typeoffset < typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if (((typedata[0] & 0xffff) == VT_SAFEARRAY) && (typedata[1] == target_type)) break;
	}

	if (typeoffset == typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    int mix_field;
	    
	    if (target_type & 0x80000000) {
		mix_field = ((target_type >> 16) & VT_TYPEMASK) | VT_ARRAY;
	    } else {
		typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][target_type];
		mix_field = ((typedata[0] >> 16) == 0x7fff)? 0x7fff: 0x7ffe;
	    }

	    typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (mix_field << 16) | VT_SAFEARRAY;
	    typedata[1] = target_type;
	}

	*encoded_type = typeoffset;

	*decoded_size = 8 /*sizeof(TYPEDESC)*/ + child_size;
	break;
	}

    case VT_USERDEFINED:
      {
        importinfo_t *importinfo;
        int typeinfo_offset;

        if (type->typelib_idx > -1)
        {
            chat("encode_type: VT_USERDEFINED - found already defined type %s at %d\n",
                type->name, type->typelib_idx);
            typeinfo_offset = typelib->typelib_typeinfo_offsets[type->typelib_idx];
        }
        else if ((importinfo = find_importinfo(typelib, type->name)))
        {
            chat("encode_type: VT_USERDEFINED - found imported type %s in %s\n",
                type->name, importinfo->importlib->name);
            alloc_importinfo(typelib, importinfo);
            typeinfo_offset = importinfo->offset | 0x1;
        }
        else
        {
            /* typedef'd types without public attribute aren't included in the typelib */
            while (type_is_alias(type) && !is_attr(type->attrs, ATTR_PUBLIC))
                type = type_alias_get_aliasee(type);

            chat("encode_type: VT_USERDEFINED - adding new type %s, real type %d\n",
                 type->name, type_get_type(type));

            switch (type_get_type(type))
            {
            case TYPE_STRUCT:
                add_structure_typeinfo(typelib, type);
                break;
            case TYPE_INTERFACE:
                add_interface_typeinfo(typelib, type);
                break;
            case TYPE_ENUM:
                add_enum_typeinfo(typelib, type);
                break;
            case TYPE_UNION:
                add_union_typeinfo(typelib, type);
                break;
            case TYPE_COCLASS:
                add_coclass_typeinfo(typelib, type);
                break;
            default:
                error("encode_type: VT_USERDEFINED - unhandled type %d\n",
                      type_get_type(type));
            }

            typeinfo_offset = typelib->typelib_typeinfo_offsets[type->typelib_idx];
        }
	for (typeoffset = 0; typeoffset < typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if ((typedata[0] == ((0x7fff << 16) | VT_USERDEFINED)) && (typedata[1] == typeinfo_offset)) break;
	}

	if (typeoffset == typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (0x7fff << 16) | VT_USERDEFINED;
	    typedata[1] = typeinfo_offset;
	}

	*encoded_type = typeoffset;
        break;
      }

    default:
	error("encode_type: unrecognized type %d.\n", vt);
	*encoded_type = default_type;
	break;
    }

    return 0;
}

static void dump_type(type_t *t)
{
    chat("dump_type: %p name %s type %d attrs %p\n", t, t->name, type_get_type(t), t->attrs);
}

static int encode_var(
	msft_typelib_t *typelib,   /* [I] The type library in which to encode the TYPEDESC. */
	type_t *type,              /* [I] The type description to encode. */
	var_t *var,                /* [I] The var to encode. */
	int *encoded_type,         /* [O] The encoded type description. */
	int *decoded_size)         /* [O] The total size of the unencoded TYPEDESCs, including nested descs. */
{
    int typeoffset;
    int *typedata;
    int target_type;
    int child_size;
    int vt;
    int scratch;

    if (!decoded_size) decoded_size = &scratch;
    *decoded_size = 0;

    chat("encode_var: var %p type %p type->name %s\n",
         var, type, type->name ? type->name : "NULL");

    if (is_array(type) && !type_array_is_decl_as_ptr(type)) {
        int num_dims, elements = 1, arrayoffset;
        type_t *atype;
        int *arraydata;

        num_dims = 0;
        for (atype = type;
             is_array(atype) && !type_array_is_decl_as_ptr(atype);
             atype = type_array_get_element(atype))
            ++num_dims;

        chat("array with %d dimensions\n", num_dims);
        encode_var(typelib, atype, var, &target_type, NULL);
        arrayoffset = ctl2_alloc_segment(typelib, MSFT_SEG_ARRAYDESC, (2 + 2 * num_dims) * sizeof(int), 0);
        arraydata = (void *)&typelib->typelib_segment_data[MSFT_SEG_ARRAYDESC][arrayoffset];

        arraydata[0] = target_type;
        arraydata[1] = num_dims;
        arraydata[1] |= ((num_dims * 2 * sizeof(int)) << 16);

        arraydata += 2;
        for (atype = type;
             is_array(atype) && !type_array_is_decl_as_ptr(atype);
             atype = type_array_get_element(atype))
        {
            arraydata[0] = type_array_get_dim(atype);
            arraydata[1] = 0;
            arraydata += 2;
            elements *= type_array_get_dim(atype);
        }

        typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
        typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

        typedata[0] = (0x7ffe << 16) | VT_CARRAY;
        typedata[1] = arrayoffset;

        *encoded_type = typeoffset;
        *decoded_size = 20 /*sizeof(ARRAYDESC)*/ + (num_dims - 1) * 8 /*sizeof(SAFEARRAYBOUND)*/;
        return 0;
    }

    vt = get_type_vt(type);
    if (vt == VT_PTR) {
        type_t *ref = is_ptr(type) ?
            type_pointer_get_ref(type) : type_array_get_element(type);
        int skip_ptr = encode_var(typelib, ref, var, &target_type, &child_size);

        if(skip_ptr == 2) {
            chat("encode_var: skipping ptr\n");
            *encoded_type = target_type;
            *decoded_size = child_size;
            return 0;
        }

	for (typeoffset = 0; typeoffset < typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if (((typedata[0] & 0xffff) == VT_PTR) && (typedata[1] == target_type)) break;
	}

	if (typeoffset == typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    int mix_field;

	    if (target_type & 0x80000000) {
		mix_field = ((target_type >> 16) & 0x3fff) | VT_BYREF;
	    } else if (get_type_vt(ref) == VT_SAFEARRAY) {
		type_t *element_type = type_alias_get_aliasee(type_array_get_element(ref));
		mix_field = get_type_vt(element_type) | VT_ARRAY | VT_BYREF;
	    } else {
		typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][target_type];
		mix_field = ((typedata[0] >> 16) == 0x7fff)? 0x7fff: 0x7ffe;
	    }

	    typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (mix_field << 16) | VT_PTR;
	    typedata[1] = target_type;
	}

	*encoded_type = typeoffset;

	*decoded_size = 8 /*sizeof(TYPEDESC)*/ + child_size;
        return 0;
    }

    dump_type(type);

    encode_type(typelib, vt, type, encoded_type, decoded_size);
    /* these types already have an implicit pointer, so we don't need to
     * add another */
    if(vt == VT_DISPATCH || vt == VT_UNKNOWN) return 2;
    return 0;
}

static unsigned int get_ulong_val(unsigned int val, int vt)
{
    switch(vt) {
    case VT_I2:
    case VT_BOOL:
    case VT_UI2:
        return val & 0xffff;
    case VT_I1:
    case VT_UI1:
        return val & 0xff;
    }

    return val;
}

static void write_int_value(msft_typelib_t *typelib, int *out, int vt, int value)
{
    const unsigned int lv = get_ulong_val(value, vt);
    if ((lv & 0x3ffffff) == lv) {
        *out = 0x80000000;
        *out |= vt << 26;
        *out |= lv;
    } else {
        int offset = ctl2_alloc_segment(typelib, MSFT_SEG_CUSTDATA, 8, 0);
        *((unsigned short *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset]) = vt;
        memcpy(&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+2], &value, 4);
        *((unsigned short *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+6]) = 0x5757;
        *out = offset;
    }
}

static void write_string_value(msft_typelib_t *typelib, int *out, const char *value)
{
    int len = strlen(value), seg_len = (len + 6 + 3) & ~0x3;
    int offset = ctl2_alloc_segment(typelib, MSFT_SEG_CUSTDATA, seg_len, 0);
    *((unsigned short *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset]) = VT_BSTR;
    memcpy(&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+2], &len, sizeof(len));
    memcpy(&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+6], value, len);
    len += 6;
    while(len < seg_len) {
        *((char *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+len]) = 0x57;
        len++;
    }
    *out = offset;
}

static void write_default_value(msft_typelib_t *typelib, type_t *type, expr_t *expr, int *out)
{
    int vt;

    if (expr->type == EXPR_STRLIT || expr->type == EXPR_WSTRLIT) {
        if (get_type_vt(type) != VT_BSTR)
            error("string default value applied to non-string type\n");
        chat("default value '%s'\n", expr->u.sval);
        write_string_value(typelib, out, expr->u.sval);
        return;
    }

    if (type_get_type(type) == TYPE_ENUM) {
        vt = VT_I4;
    } else if (is_ptr(type)) {
        vt = get_type_vt(type_pointer_get_ref(type));
        if (vt == VT_USERDEFINED)
            vt = VT_I4;
        if (expr->cval)
            warning("non-null pointer default value\n");
    } else {
        vt = get_type_vt(type);
        switch(vt) {
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_BOOL:
        case VT_I1:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_INT:
        case VT_UINT:
        case VT_HRESULT:
            break;
        default:
            warning("can't write value of type %d yet\n", vt);
            return;
        }
    }

    write_int_value(typelib, out, vt, expr->cval);
}

static HRESULT set_custdata(msft_typelib_t *typelib, REFGUID guid,
                            int vt, const void *value, int *offset)
{
    MSFT_GuidEntry guidentry;
    int guidoffset;
    int custoffset;
    int *custdata;
    int data_out;

    guidentry.guid = *guid;

    guidentry.hreftype = -1;
    guidentry.next_hash = -1;

    guidoffset = ctl2_alloc_guid(typelib, &guidentry);
    if(vt == VT_BSTR)
        write_string_value(typelib, &data_out, value);
    else
        write_int_value(typelib, &data_out, vt, *(int*)value);

    custoffset = ctl2_alloc_segment(typelib, MSFT_SEG_CUSTDATAGUID, 12, 0);

    custdata = (int *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATAGUID][custoffset];
    custdata[0] = guidoffset;
    custdata[1] = data_out;
    custdata[2] = *offset;
    *offset = custoffset;

    return S_OK;
}

static HRESULT add_func_desc(msft_typeinfo_t* typeinfo, var_t *func, int index)
{
    int offset, name_offset;
    int *typedata, typedata_size;
    int i, id, next_idx;
    int decoded_size, extra_attr = 0;
    int num_params = 0, num_optional = 0, num_defaults = 0;
    var_t *arg;
    unsigned char *namedata;
    const attr_t *attr;
    unsigned int funcflags = 0, callconv = 4 /* CC_STDCALL */;
    unsigned int funckind, invokekind = 1 /* INVOKE_FUNC */;
    int help_context = 0, help_string_context = 0, help_string_offset = -1;
    int entry = -1, entry_is_ord = 0;
    int lcid_retval_count = 0;

    chat("add_func_desc(%p,%d)\n", typeinfo, index);

    id = ((0x6000 | (typeinfo->typeinfo->datatype2 & 0xffff)) << 16) | index;

    switch(typeinfo->typekind) {
    case TKIND_DISPATCH:
        funckind = 0x4; /* FUNC_DISPATCH */
        break;
    case TKIND_MODULE:
        funckind = 0x3; /* FUNC_STATIC */
        break;
    default:
        funckind = 0x1; /* FUNC_PUREVIRTUAL */
        break;
    }

    if (is_local( func->attrs )) {
        chat("add_func_desc: skipping local function\n");
        return S_FALSE;
    }

    if (type_get_function_args(func->type))
      LIST_FOR_EACH_ENTRY( arg, type_get_function_args(func->type), var_t, entry )
      {
        num_params++;
        if (arg->attrs) LIST_FOR_EACH_ENTRY( attr, arg->attrs, const attr_t, entry ) {
            if(attr->type == ATTR_DEFAULTVALUE)
                num_defaults++;
            else if(attr->type == ATTR_OPTIONAL)
                num_optional++;
        }
      }

    chat("add_func_desc: num of params %d\n", num_params);

    name_offset = ctl2_alloc_name(typeinfo->typelib, func->name);

    if (func->attrs) LIST_FOR_EACH_ENTRY( attr, func->attrs, const attr_t, entry ) {
        expr_t *expr = attr->u.pval;
        switch(attr->type) {
        case ATTR_BINDABLE:
            funcflags |= 0x4; /* FUNCFLAG_FBINDABLE */
            break;
        case ATTR_DEFAULTBIND:
            funcflags |= 0x20; /* FUNCFLAG_FDEFAULTBIND */
            break;
        case ATTR_DEFAULTCOLLELEM:
            funcflags |= 0x100; /* FUNCFLAG_FDEFAULTCOLLELEM */
            break;
        case ATTR_DISPLAYBIND:
            funcflags |= 0x10; /* FUNCFLAG_FDISPLAYBIND */
            break;
        case ATTR_ENTRY:
            extra_attr = max(extra_attr, 3);
            if (expr->type == EXPR_STRLIT || expr->type == EXPR_WSTRLIT)
              entry = ctl2_alloc_string(typeinfo->typelib, attr->u.pval);
            else {
              entry = expr->cval;
              entry_is_ord = 1;
            }
            break;
        case ATTR_HELPCONTEXT:
            extra_attr = max(extra_attr, 1);
            help_context = expr->u.lval;
            break;
        case ATTR_HELPSTRING:
            extra_attr = max(extra_attr, 2);
            help_string_offset = ctl2_alloc_string(typeinfo->typelib, attr->u.pval);
            break;
        case ATTR_HELPSTRINGCONTEXT:
            extra_attr = max(extra_attr, 6);
            help_string_context = expr->u.lval;
            break;
        case ATTR_HIDDEN:
            funcflags |= 0x40; /* FUNCFLAG_FHIDDEN */
            break;
        case ATTR_ID:
            id = expr->cval;
            break;
        case ATTR_IMMEDIATEBIND:
            funcflags |= 0x1000; /* FUNCFLAG_FIMMEDIATEBIND */
            break;
        case ATTR_NONBROWSABLE:
            funcflags |= 0x400; /* FUNCFLAG_FNONBROWSABLE */
            break;
        case ATTR_OUT:
            break;
        case ATTR_PROPGET:
            invokekind = 0x2; /* INVOKE_PROPERTYGET */
            break;
        case ATTR_PROPPUT:
            invokekind = 0x4; /* INVOKE_PROPERTYPUT */
            break;
        case ATTR_PROPPUTREF:
            invokekind = 0x8; /* INVOKE_PROPERTYPUTREF */
            break;
        /* FIXME: FUNCFLAG_FREPLACEABLE */
        case ATTR_REQUESTEDIT:
            funcflags |= 0x8; /* FUNCFLAG_FREQUESTEDIT */
            break;
        case ATTR_RESTRICTED:
            funcflags |= 0x1; /* FUNCFLAG_FRESTRICTED */
            break;
        case ATTR_SOURCE:
            funcflags |= 0x2; /* FUNCFLAG_FSOURCE */
            break;
        case ATTR_UIDEFAULT:
            funcflags |= 0x200; /* FUNCFLAG_FUIDEFAULT */
            break;
        case ATTR_USESGETLASTERROR:
            funcflags |= 0x80; /* FUNCFLAG_FUSESGETLASTERROR */
            break;
        case ATTR_VARARG:
            if (num_optional || num_defaults)
                warning("add_func_desc: ignoring vararg in function with optional or defaultvalue params\n");
            else
                num_optional = -1;
            break;
        default:
            break;
        }
    }

    /* allocate type data space for us */
    typedata_size = 0x18 + extra_attr * sizeof(int) + (num_params * (num_defaults ? 16 : 12));

    if (!typeinfo->func_data) {
        typeinfo->func_data = xmalloc(0x100);
        typeinfo->func_data_allocated = 0x100;
        typeinfo->func_data[0] = 0;
    }

    if(typeinfo->func_data[0] + typedata_size + sizeof(int) > typeinfo->func_data_allocated) {
        typeinfo->func_data_allocated = max(typeinfo->func_data_allocated * 2,
                                            typeinfo->func_data[0] + typedata_size + sizeof(int));
        typeinfo->func_data = xrealloc(typeinfo->func_data, typeinfo->func_data_allocated);
    }

    offset = typeinfo->func_data[0];
    typeinfo->func_data[0] += typedata_size;
    typedata = typeinfo->func_data + (offset >> 2) + 1;


    /* find func with the same name - if it exists use its id */
    for(i = 0; i < (typeinfo->typeinfo->cElement & 0xffff); i++) {
        if(name_offset == typeinfo->func_names[i]) {
            id = typeinfo->func_indices[i];
            break;
        }
    }

    /* find the first func with the same id and link via the hiword of typedata[4] */
    next_idx = index;
    for(i = 0; i < (typeinfo->typeinfo->cElement & 0xffff); i++) {
        if(id == typeinfo->func_indices[i]) {
            next_idx = typeinfo->func_data[(typeinfo->func_offsets[i] >> 2) + 1 + 4] >> 16;
            typeinfo->func_data[(typeinfo->func_offsets[i] >> 2) + 1 + 4] &= 0xffff;
            typeinfo->func_data[(typeinfo->func_offsets[i] >> 2) + 1 + 4] |= (index << 16);
            break;
        }
    }

    /* fill out the basic type information */
    typedata[0] = typedata_size | (index << 16);
    encode_var(typeinfo->typelib, type_function_get_rettype(func->type), func,
        &typedata[1], &decoded_size);
    typedata[2] = funcflags;
    typedata[3] = ((52 /*sizeof(FUNCDESC)*/ + decoded_size) << 16) | typeinfo->typeinfo->cbSizeVft;
    typedata[4] = (next_idx << 16) | (callconv << 8) | (invokekind << 3) | funckind;
    if(num_defaults) typedata[4] |= 0x1000;
    if(entry_is_ord) typedata[4] |= 0x2000;
    typedata[5] = (num_optional << 16) | num_params;

    /* NOTE: High word of typedata[3] is total size of FUNCDESC + size of all ELEMDESCs for params + TYPEDESCs for pointer params and return types. */
    /* That is, total memory allocation required to reconstitute the FUNCDESC in its entirety. */
    typedata[3] += (16 /*sizeof(ELEMDESC)*/ * num_params) << 16;
    typedata[3] += (24 /*sizeof(PARAMDESCEX)*/ * num_defaults) << 16;

    switch(extra_attr) {
    case 6: typedata[11] = help_string_context;
    case 5: typedata[10] = -1;
    case 4: typedata[9] = -1;
    case 3: typedata[8] = entry;
    case 2: typedata[7] = help_string_offset;
    case 1: typedata[6] = help_context;
    case 0:
        break;
    default:
        warning("unknown number of optional attrs\n");
    }

    if (type_get_function_args(func->type))
    {
      i = 0;
      LIST_FOR_EACH_ENTRY( arg, type_get_function_args(func->type), var_t, entry )
      {
        int paramflags = 0;
        int *paramdata = typedata + 6 + extra_attr + (num_defaults ? num_params : 0) + i * 3;
        int *defaultdata = num_defaults ? typedata + 6 + extra_attr + i : NULL;

        if(defaultdata) *defaultdata = -1;

	encode_var(typeinfo->typelib, arg->type, arg, paramdata, &decoded_size);
        if (arg->attrs) LIST_FOR_EACH_ENTRY( attr, arg->attrs, const attr_t, entry ) {
            switch(attr->type) {
            case ATTR_DEFAULTVALUE:
              {
                paramflags |= 0x30; /* PARAMFLAG_FHASDEFAULT | PARAMFLAG_FOPT */
                write_default_value(typeinfo->typelib, arg->type, (expr_t *)attr->u.pval, defaultdata);
                break;
              }
            case ATTR_IN:
                paramflags |= 0x01; /* PARAMFLAG_FIN */
                break;
            case ATTR_OPTIONAL:
                paramflags |= 0x10; /* PARAMFLAG_FOPT */
                break;
            case ATTR_OUT:
                paramflags |= 0x02; /* PARAMFLAG_FOUT */
                break;
            case ATTR_PARAMLCID:
                paramflags |= 0x04; /* PARAMFLAG_LCID */
                lcid_retval_count++;
                break;
            case ATTR_RETVAL:
                paramflags |= 0x08; /* PARAMFLAG_FRETVAL */
                lcid_retval_count++;
                break;
            default:
                chat("unhandled param attr %d\n", attr->type);
                break;
            }
        }
	paramdata[1] = -1;
	paramdata[2] = paramflags;
	typedata[3] += decoded_size << 16;

        i++;
      }
    }

    if(lcid_retval_count == 1)
        typedata[4] |= 0x4000;
    else if(lcid_retval_count == 2)
        typedata[4] |= 0x8000;

    if(typeinfo->funcs_allocated == 0) {
        typeinfo->funcs_allocated = 10;
        typeinfo->func_indices = xmalloc(typeinfo->funcs_allocated * sizeof(int));
        typeinfo->func_names   = xmalloc(typeinfo->funcs_allocated * sizeof(int));
        typeinfo->func_offsets = xmalloc(typeinfo->funcs_allocated * sizeof(int));
    }
    if(typeinfo->funcs_allocated == (typeinfo->typeinfo->cElement & 0xffff)) {
        typeinfo->funcs_allocated *= 2;
        typeinfo->func_indices = xrealloc(typeinfo->func_indices, typeinfo->funcs_allocated * sizeof(int));
        typeinfo->func_names   = xrealloc(typeinfo->func_names,   typeinfo->funcs_allocated * sizeof(int));
        typeinfo->func_offsets = xrealloc(typeinfo->func_offsets, typeinfo->funcs_allocated * sizeof(int));
    }

    /* update the index data */
    typeinfo->func_indices[typeinfo->typeinfo->cElement & 0xffff] = id; 
    typeinfo->func_offsets[typeinfo->typeinfo->cElement & 0xffff] = offset;
    typeinfo->func_names[typeinfo->typeinfo->cElement & 0xffff] = name_offset;

    /* ??? */
    if (!typeinfo->typeinfo->res2) typeinfo->typeinfo->res2 = 0x20;
    typeinfo->typeinfo->res2 <<= 1;
    /* ??? */
    if (index < 2) typeinfo->typeinfo->res2 += num_params << 4;

    if (typeinfo->typeinfo->res3 == -1) typeinfo->typeinfo->res3 = 0;
    typeinfo->typeinfo->res3 += 0x38 + num_params * 0x10;
    if(num_defaults) typeinfo->typeinfo->res3 += num_params * 0x4;

    /* adjust size of VTBL */
    if(funckind != 0x3 /* FUNC_STATIC */)
        typeinfo->typeinfo->cbSizeVft += pointer_size;

    /* Increment the number of function elements */
    typeinfo->typeinfo->cElement += 1;

    namedata = typeinfo->typelib->typelib_segment_data[MSFT_SEG_NAME] + name_offset;
    if (*((INT *)namedata) == -1) {
	*((INT *)namedata) = typeinfo->typelib->typelib_typeinfo_offsets[typeinfo->typeinfo->typekind >> 16];
        if(typeinfo->typekind == TKIND_MODULE)
            namedata[9] |= 0x10;
    } else
        namedata[9] &= ~0x10;

    if(typeinfo->typekind == TKIND_MODULE)
        namedata[9] |= 0x20;

    if (type_get_function_args(func->type))
    {
        i = 0;
        LIST_FOR_EACH_ENTRY( arg, type_get_function_args(func->type), var_t, entry )
        {
            /* don't give the last arg of a [propput*] func a name */
            if(i != num_params - 1 || (invokekind != 0x4 /* INVOKE_PROPERTYPUT */ && invokekind != 0x8 /* INVOKE_PROPERTYPUTREF */))
            {
                int *paramdata = typedata + 6 + extra_attr + (num_defaults ? num_params : 0) + i * 3;
                offset = ctl2_alloc_name(typeinfo->typelib, arg->name);
                paramdata[1] = offset;
            }
            i++;
        }
    }
    return S_OK;
}

static HRESULT add_var_desc(msft_typeinfo_t *typeinfo, UINT index, var_t* var)
{
    int offset, id;
    unsigned int typedata_size;
    INT *typedata;
    unsigned int var_datawidth, var_alignment = 0;
    int var_type_size, var_kind = 0 /* VAR_PERINSTANCE */; 
    int alignment;
    int varflags = 0;
    const attr_t *attr;
    unsigned char *namedata;
    int var_num = (typeinfo->typeinfo->cElement >> 16) & 0xffff;

    chat("add_var_desc(%d, %s)\n", index, var->name);

    id = 0x40000000 + index;

    if (var->attrs) LIST_FOR_EACH_ENTRY( attr, var->attrs, const attr_t, entry ) {
        expr_t *expr = attr->u.pval;
        switch(attr->type) {
        case ATTR_BINDABLE:
            varflags |= 0x04; /* VARFLAG_FBINDABLE */
            break;
        case ATTR_DEFAULTBIND:
            varflags |= 0x20; /* VARFLAG_FDEFAULTBIND */
            break;
        case ATTR_DEFAULTCOLLELEM:
            varflags |= 0x100; /* VARFLAG_FDEFAULTCOLLELEM */
            break;
        case ATTR_DISPLAYBIND:
            varflags |= 0x10; /* VARFLAG_FDISPLAYBIND */
            break;
        case ATTR_HIDDEN:
            varflags |= 0x40; /* VARFLAG_FHIDDEN */
            break;
        case ATTR_ID:
            id = expr->cval;
            break;
        case ATTR_IMMEDIATEBIND:
            varflags |= 0x1000; /* VARFLAG_FIMMEDIATEBIND */
            break;
        case ATTR_NONBROWSABLE:
            varflags |= 0x400; /* VARFLAG_FNONBROWSABLE */
            break;
        case ATTR_READONLY:
            varflags |= 0x01; /* VARFLAG_FREADONLY */
            break;
        /* FIXME: VARFLAG_FREPLACEABLE */
        case ATTR_REQUESTEDIT:
            varflags |= 0x08; /* VARFLAG_FREQUESTEDIT */
            break;
        case ATTR_RESTRICTED:
            varflags |= 0x80; /* VARFLAG_FRESTRICTED */
            break;
        case ATTR_SOURCE:
            varflags |= 0x02; /* VARFLAG_FSOURCE */
            break;
        case ATTR_UIDEFAULT:
            varflags |= 0x0200; /* VARFLAG_FUIDEFAULT */
            break;
        default:
            break;
        }
    }

    /* allocate type data space for us */
    typedata_size = 0x14;

    if (!typeinfo->var_data) {
        typeinfo->var_data = xmalloc(0x100);
        typeinfo->var_data_allocated = 0x100;
        typeinfo->var_data[0] = 0;
    }

    if(typeinfo->var_data[0] + typedata_size + sizeof(int) > typeinfo->var_data_allocated) {
        typeinfo->var_data_allocated = max(typeinfo->var_data_allocated * 2,
                                            typeinfo->var_data[0] + typedata_size + sizeof(int));
        typeinfo->var_data = xrealloc(typeinfo->var_data, typeinfo->var_data_allocated);
    }

    offset = typeinfo->var_data[0];
    typeinfo->var_data[0] += typedata_size;
    typedata = typeinfo->var_data + (offset >> 2) + 1;

    /* fill out the basic type information */
    typedata[0] = typedata_size | (index << 16);
    typedata[2] = varflags;
    typedata[3] = (36 /*sizeof(VARDESC)*/ << 16) | 0;

    if(typeinfo->vars_allocated == 0) {
        typeinfo->vars_allocated = 10;
        typeinfo->var_indices = xmalloc(typeinfo->vars_allocated * sizeof(int));
        typeinfo->var_names   = xmalloc(typeinfo->vars_allocated * sizeof(int));
        typeinfo->var_offsets = xmalloc(typeinfo->vars_allocated * sizeof(int));
    }
    if(typeinfo->vars_allocated == var_num) {
        typeinfo->vars_allocated *= 2;
        typeinfo->var_indices = xrealloc(typeinfo->var_indices, typeinfo->vars_allocated * sizeof(int));
        typeinfo->var_names   = xrealloc(typeinfo->var_names,   typeinfo->vars_allocated * sizeof(int));
        typeinfo->var_offsets = xrealloc(typeinfo->var_offsets, typeinfo->vars_allocated * sizeof(int));
    }
    /* update the index data */
    typeinfo->var_indices[var_num] = id;
    typeinfo->var_names[var_num] = -1;
    typeinfo->var_offsets[var_num] = offset;

    /* figure out type widths and whatnot */
    var_datawidth = type_memsize_and_alignment(var->type, &var_alignment);
    encode_var(typeinfo->typelib, var->type, var, &typedata[1], &var_type_size);

    /* pad out starting position to data width */
    typeinfo->datawidth += var_alignment - 1;
    typeinfo->datawidth &= ~(var_alignment - 1);

    switch(typeinfo->typekind) {
    case TKIND_ENUM:
        write_int_value(typeinfo->typelib, &typedata[4], VT_I4, var->eval->cval);
        var_kind = 2; /* VAR_CONST */
        var_type_size += 16; /* sizeof(VARIANT) */
        typeinfo->datawidth = var_datawidth;
        break;
    case TKIND_RECORD:
        typedata[4] = typeinfo->datawidth;
        typeinfo->datawidth += var_datawidth;
        break;
    case TKIND_UNION:
        typedata[4] = typeinfo->datawidth;
        typeinfo->datawidth = max(typeinfo->datawidth, var_datawidth);
        break;
    case TKIND_DISPATCH:
        var_kind = 3; /* VAR_DISPATCH */
        typeinfo->datawidth = pointer_size;
        break;
    default:
        error("add_var_desc: unhandled type kind %d\n", typeinfo->typekind);
        break;
    }

    /* add type description size to total required allocation */
    typedata[3] += var_type_size << 16 | var_kind;

    /* fix type alignment */
    alignment = (typeinfo->typeinfo->typekind >> 11) & 0x1f;
    if (alignment < var_alignment) {
	alignment = var_alignment;
	typeinfo->typeinfo->typekind &= ~0xffc0;
	typeinfo->typeinfo->typekind |= alignment << 11 | alignment << 6;
    }

    /* ??? */
    if (!typeinfo->typeinfo->res2) typeinfo->typeinfo->res2 = 0x1a;
    if ((index == 0) || (index == 1) || (index == 2) || (index == 4) || (index == 9)) {
	typeinfo->typeinfo->res2 <<= 1;
    }

    /* ??? */
    if (typeinfo->typeinfo->res3 == -1) typeinfo->typeinfo->res3 = 0;
    typeinfo->typeinfo->res3 += 0x2c;

    /* increment the number of variable elements */
    typeinfo->typeinfo->cElement += 0x10000;

    /* pad data width to alignment */
    typeinfo->typeinfo->size = (typeinfo->datawidth + (alignment - 1)) & ~(alignment - 1);

    offset = ctl2_alloc_name(typeinfo->typelib, var->name);
    if (offset == -1) return E_OUTOFMEMORY;

    namedata = typeinfo->typelib->typelib_segment_data[MSFT_SEG_NAME] + offset;
    if (*((INT *)namedata) == -1) {
	*((INT *)namedata) = typeinfo->typelib->typelib_typeinfo_offsets[typeinfo->typeinfo->typekind >> 16];
        if(typeinfo->typekind != TKIND_DISPATCH)
            namedata[9] |= 0x10;
    } else
        namedata[9] &= ~0x10;

    if (typeinfo->typekind == TKIND_ENUM) {
	namedata[9] |= 0x20;
    }
    typeinfo->var_names[var_num] = offset;

    return S_OK;
}

static HRESULT add_impl_type(msft_typeinfo_t *typeinfo, type_t *ref, importinfo_t *importinfo)
{
    if(importinfo) {
        alloc_importinfo(typeinfo->typelib, importinfo);
        typeinfo->typeinfo->datatype1 = importinfo->offset+1;
    }else {
        if(ref->typelib_idx == -1)
            add_interface_typeinfo(typeinfo->typelib, ref);
        if(ref->typelib_idx == -1)
            error("add_impl_type: unable to add inherited interface\n");

        typeinfo->typeinfo->datatype1 = typeinfo->typelib->typelib_typeinfo_offsets[ref->typelib_idx];
    }

    typeinfo->typeinfo->cImplTypes++;
    return S_OK;
}

static msft_typeinfo_t *create_msft_typeinfo(msft_typelib_t *typelib, enum type_kind kind,
                                             const char *name, const attr_list_t *attrs)
{
    const attr_t *attr;
    msft_typeinfo_t *msft_typeinfo;
    int nameoffset;
    int typeinfo_offset;
    MSFT_TypeInfoBase *typeinfo;
    MSFT_GuidEntry guidentry;

    chat("create_msft_typeinfo: name %s kind %d index %d\n", name, kind, typelib->typelib_header.nrtypeinfos);

    msft_typeinfo = xmalloc(sizeof(*msft_typeinfo));
    memset( msft_typeinfo, 0, sizeof(*msft_typeinfo) );

    msft_typeinfo->typelib = typelib;

    nameoffset = ctl2_alloc_name(typelib, name);
    typeinfo_offset = ctl2_alloc_typeinfo(typelib, nameoffset);
    typeinfo = (MSFT_TypeInfoBase *)&typelib->typelib_segment_data[MSFT_SEG_TYPEINFO][typeinfo_offset];

    typelib->typelib_segment_data[MSFT_SEG_NAME][nameoffset + 9] = 0x38;
    *((int *)&typelib->typelib_segment_data[MSFT_SEG_NAME][nameoffset]) = typeinfo_offset;

    msft_typeinfo->typekind = kind;
    msft_typeinfo->typeinfo = typeinfo;

    typeinfo->typekind |= kind | 0x20;

    if(kind == TKIND_COCLASS)
        typeinfo->flags |= 0x2; /* TYPEFLAG_FCANCREATE */

    if (attrs) LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry ) {
        switch(attr->type) {
        case ATTR_AGGREGATABLE:
            if (kind == TKIND_COCLASS)
                typeinfo->flags |= 0x400; /* TYPEFLAG_FAGGREGATABLE */
            break;

        case ATTR_APPOBJECT:
            if (kind == TKIND_COCLASS)
                typeinfo->flags |= 0x1; /* TYPEFLAG_FAPPOBJECT */
            break;

        case ATTR_CONTROL:
            if (kind == TKIND_COCLASS)
                typeinfo->flags |= 0x20; /* TYPEFLAG_FCONTROL */
            break;

        case ATTR_DLLNAME:
          {
            int offset = ctl2_alloc_string(typelib, attr->u.pval);
            typeinfo->datatype1 = offset;
            break;
          }

        case ATTR_DUAL:
            /* FIXME: check interface is compatible */
            typeinfo->typekind = (typeinfo->typekind & ~0xff) | 0x34;
            typeinfo->flags |= 0x140; /* TYPEFLAG_FDUAL | TYPEFLAG_FOLEAUTOMATION */
            break;

        case ATTR_HELPCONTEXT:
          {
            expr_t *expr = (expr_t*)attr->u.pval;
            typeinfo->helpcontext = expr->cval;
            break;
          }
        case ATTR_HELPSTRING:
          {
            int offset = ctl2_alloc_string(typelib, attr->u.pval);
            if (offset == -1) break;
            typeinfo->docstringoffs = offset;
            break;
          }
        case ATTR_HELPSTRINGCONTEXT:
          {
            expr_t *expr = (expr_t*)attr->u.pval;
            typeinfo->helpstringcontext = expr->cval;
            break;
          }
        case ATTR_HIDDEN:
            typeinfo->flags |= 0x10; /* TYPEFLAG_FHIDDEN */
            break;

        case ATTR_LICENSED:
            typeinfo->flags |= 0x04; /* TYPEFLAG_FLICENSED */
            break;

        case ATTR_NONCREATABLE:
            typeinfo->flags &= ~0x2; /* TYPEFLAG_FCANCREATE */
            break;

        case ATTR_NONEXTENSIBLE:
            typeinfo->flags |= 0x80; /* TYPEFLAG_FNONEXTENSIBLE */
            break;

        case ATTR_OLEAUTOMATION:
            typeinfo->flags |= 0x100; /* TYPEFLAG_FOLEAUTOMATION */
            break;

        /* FIXME: TYPEFLAG_FPREDCLID */

        case ATTR_PROXY:
            typeinfo->flags |= 0x4000; /* TYPEFLAG_FPROXY */
            break;

        /* FIXME: TYPEFLAG_FREPLACEABLE */

        case ATTR_RESTRICTED:
            typeinfo->flags |= 0x200; /* TYPEFLAG_FRESTRICTED */
            break;

        case ATTR_UUID:
            guidentry.guid = *(GUID*)attr->u.pval;
            guidentry.hreftype = typelib->typelib_typeinfo_offsets[typeinfo->typekind >> 16];
            guidentry.next_hash = -1;
            typeinfo->posguid = ctl2_alloc_guid(typelib, &guidentry);
#if 0
            if (IsEqualIID(guid, &IID_IDispatch)) {
                typelib->typelib_header.dispatchpos = typelib->typelib_typeinfo_offsets[typeinfo->typekind >> 16];
            }
#endif
            break;

        case ATTR_VERSION:
            typeinfo->version = attr->u.ival;
            break;

        default:
            break;
        }
    }

    if (typelib->last_typeinfo) typelib->last_typeinfo->next_typeinfo = msft_typeinfo;
    typelib->last_typeinfo = msft_typeinfo;
    if (!typelib->typeinfos) typelib->typeinfos = msft_typeinfo;


    return msft_typeinfo;
}

static void add_dispatch(msft_typelib_t *typelib)
{
    int guid_offset, impfile_offset, hash_key;
    MSFT_GuidEntry guidentry;
    MSFT_ImpInfo impinfo;
    GUID stdole =        {0x00020430,0x0000,0x0000,{0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
    GUID iid_idispatch = {0x00020400,0x0000,0x0000,{0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

    if(typelib->typelib_header.dispatchpos != -1) return;

    guidentry.guid = stdole;
    guidentry.hreftype = 2;
    guidentry.next_hash = -1;
    hash_key = ctl2_hash_guid(&guidentry.guid);
    guid_offset = ctl2_find_guid(typelib, hash_key, &guidentry.guid);
    if (guid_offset == -1)
        guid_offset = ctl2_alloc_guid(typelib, &guidentry);
    impfile_offset = alloc_importfile(typelib, guid_offset, 2, 0, "stdole2.tlb");

    guidentry.guid = iid_idispatch;
    guidentry.hreftype = 1;
    guidentry.next_hash = -1;
    impinfo.flags = TKIND_INTERFACE << 24 | MSFT_IMPINFO_OFFSET_IS_GUID;
    impinfo.oImpFile = impfile_offset;
    hash_key = ctl2_hash_guid(&guidentry.guid);
    guid_offset = ctl2_find_guid(typelib, hash_key, &guidentry.guid);
    if (guid_offset == -1)
        guid_offset = ctl2_alloc_guid(typelib, &guidentry);
    impinfo.oGuid = guid_offset;
    typelib->typelib_header.dispatchpos = alloc_msft_importinfo(typelib, &impinfo) | 0x01;
}

static void add_dispinterface_typeinfo(msft_typelib_t *typelib, type_t *dispinterface)
{
    int num_parents = 0, num_funcs = 0;
    importinfo_t *importinfo = NULL;
    const statement_t *stmt_func;
    type_t *inherit, *ref;
    int idx = 0;
    var_t *func;
    var_t *var;
    msft_typeinfo_t *msft_typeinfo;

    if (-1 < dispinterface->typelib_idx)
        return;

    inherit = type_dispiface_get_inherit(dispinterface);

    if (inherit)
    {
        importinfo = find_importinfo(typelib, inherit->name);

        if (!importinfo && type_iface_get_inherit(inherit) && inherit->typelib_idx == -1)
            add_interface_typeinfo(typelib, inherit);
    }

    /* check typelib_idx again, it could have been added while resolving the parent interface */
    if (-1 < dispinterface->typelib_idx)
        return;

    dispinterface->typelib_idx = typelib->typelib_header.nrtypeinfos;
    msft_typeinfo = create_msft_typeinfo(typelib, TKIND_DISPATCH, dispinterface->name,
                                         dispinterface->attrs);

    msft_typeinfo->typeinfo->size = pointer_size;
    msft_typeinfo->typeinfo->typekind |= pointer_size << 11 | pointer_size << 6;

    msft_typeinfo->typeinfo->flags |= 0x1000; /* TYPEFLAG_FDISPATCHABLE */
    add_dispatch(typelib);

    if (inherit)
    {
        add_impl_type(msft_typeinfo, inherit, importinfo);
        msft_typeinfo->typeinfo->typekind |= 0x10;
    }

    /* count the number of inherited interfaces and non-local functions */
    for (ref = inherit; ref; ref = type_iface_get_inherit(ref))
    {
        num_parents++;
        STATEMENTS_FOR_EACH_FUNC( stmt_func, type_iface_get_stmts(ref) )
        {
            var_t *func = stmt_func->u.var;
            if (!is_local(func->attrs)) num_funcs++;
        }
    }
    msft_typeinfo->typeinfo->datatype2 = num_funcs << 16 | num_parents;
    msft_typeinfo->typeinfo->cbSizeVft = num_funcs * pointer_size;

    msft_typeinfo->typeinfo->cImplTypes = 1;    /* IDispatch */

    /* count the no of methods, as the variable indices come after the funcs */
    if (dispinterface->details.iface->disp_methods)
        LIST_FOR_EACH_ENTRY( func, dispinterface->details.iface->disp_methods, var_t, entry )
            idx++;

    if (type_dispiface_get_props(dispinterface))
        LIST_FOR_EACH_ENTRY( var, type_dispiface_get_props(dispinterface), var_t, entry )
            add_var_desc(msft_typeinfo, idx++, var);

    if (type_dispiface_get_methods(dispinterface))
    {
        idx = 0;
        LIST_FOR_EACH_ENTRY( func, type_dispiface_get_methods(dispinterface), var_t, entry )
            if(add_func_desc(msft_typeinfo, func, idx) == S_OK)
                idx++;
    }
}

static void add_interface_typeinfo(msft_typelib_t *typelib, type_t *interface)
{
    int idx = 0;
    const statement_t *stmt_func;
    type_t *ref;
    msft_typeinfo_t *msft_typeinfo;
    importinfo_t *ref_importinfo = NULL;
    int num_parents = 0, num_funcs = 0;
    type_t *inherit;
    const type_t *derived;

    if (-1 < interface->typelib_idx)
        return;

    if (!interface->details.iface)
    {
        error( "interface %s is referenced but not defined\n", interface->name );
        return;
    }

    if (is_attr(interface->attrs, ATTR_DISPINTERFACE)) {
        add_dispinterface_typeinfo(typelib, interface);
        return;
    }

    /* midl adds the parent interface first, unless the parent itself
       has no parent (i.e. it stops before IUnknown). */

    inherit = type_iface_get_inherit(interface);

    if(inherit) {
        ref_importinfo = find_importinfo(typelib, inherit->name);

        if(!ref_importinfo && type_iface_get_inherit(inherit) &&
           inherit->typelib_idx == -1)
            add_interface_typeinfo(typelib, inherit);
    }

    /* check typelib_idx again, it could have been added while resolving the parent interface */
    if (-1 < interface->typelib_idx)
        return;

    interface->typelib_idx = typelib->typelib_header.nrtypeinfos;
    msft_typeinfo = create_msft_typeinfo(typelib, TKIND_INTERFACE, interface->name, interface->attrs);
    msft_typeinfo->typeinfo->size = pointer_size;
    msft_typeinfo->typeinfo->typekind |= 0x0200;
    msft_typeinfo->typeinfo->typekind |= pointer_size << 11;

    for (derived = inherit; derived; derived = type_iface_get_inherit(derived))
        if (derived->name && !strcmp(derived->name, "IDispatch"))
            msft_typeinfo->typeinfo->flags |= 0x1000; /* TYPEFLAG_FDISPATCHABLE */

    if(type_iface_get_inherit(interface))
        add_impl_type(msft_typeinfo, type_iface_get_inherit(interface),
                      ref_importinfo);

    /* count the number of inherited interfaces and non-local functions */
    for(ref = inherit; ref; ref = type_iface_get_inherit(ref)) {
        num_parents++;
        STATEMENTS_FOR_EACH_FUNC( stmt_func, type_iface_get_stmts(ref) ) {
            var_t *func = stmt_func->u.var;
            if (!is_local(func->attrs)) num_funcs++;
        }
    }
    msft_typeinfo->typeinfo->datatype2 = num_funcs << 16 | num_parents;
    msft_typeinfo->typeinfo->cbSizeVft = num_funcs * pointer_size;

    STATEMENTS_FOR_EACH_FUNC( stmt_func, type_iface_get_stmts(interface) ) {
        var_t *func = stmt_func->u.var;
        if(add_func_desc(msft_typeinfo, func, idx) == S_OK)
            idx++;
    }
}

static void add_structure_typeinfo(msft_typelib_t *typelib, type_t *structure)
{
    int idx = 0;
    var_t *cur;
    msft_typeinfo_t *msft_typeinfo;

    if (-1 < structure->typelib_idx)
        return;

    structure->typelib_idx = typelib->typelib_header.nrtypeinfos;
    msft_typeinfo = create_msft_typeinfo(typelib, TKIND_RECORD, structure->name, structure->attrs);
    msft_typeinfo->typeinfo->size = 0;

    if (type_struct_get_fields(structure))
        LIST_FOR_EACH_ENTRY( cur, type_struct_get_fields(structure), var_t, entry )
            add_var_desc(msft_typeinfo, idx++, cur);
}

static void add_enum_typeinfo(msft_typelib_t *typelib, type_t *enumeration)
{
    int idx = 0;
    var_t *cur;
    msft_typeinfo_t *msft_typeinfo;

    if (-1 < enumeration->typelib_idx)
        return;

    enumeration->typelib_idx = typelib->typelib_header.nrtypeinfos;
    msft_typeinfo = create_msft_typeinfo(typelib, TKIND_ENUM, enumeration->name, enumeration->attrs);
    msft_typeinfo->typeinfo->size = 0;

    if (type_enum_get_values(enumeration))
        LIST_FOR_EACH_ENTRY( cur, type_enum_get_values(enumeration), var_t, entry )
            add_var_desc(msft_typeinfo, idx++, cur);
}

static void add_union_typeinfo(msft_typelib_t *typelib, type_t *tunion)
{
    int idx = 0;
    var_t *cur;
    msft_typeinfo_t *msft_typeinfo;

    if (-1 < tunion->typelib_idx)
        return;

    tunion->typelib_idx = typelib->typelib_header.nrtypeinfos;
    msft_typeinfo = create_msft_typeinfo(typelib, TKIND_UNION, tunion->name, tunion->attrs);
    msft_typeinfo->typeinfo->size = 0;

    if (type_union_get_cases(tunion))
        LIST_FOR_EACH_ENTRY(cur, type_union_get_cases(tunion), var_t, entry)
            add_var_desc(msft_typeinfo, idx++, cur);
}

static void add_typedef_typeinfo(msft_typelib_t *typelib, type_t *tdef)
{
    msft_typeinfo_t *msft_typeinfo = NULL;
    int datatype1, datatype2, duplicate = 0;
    unsigned int size, alignment = 0;
    type_t *type;

    if (-1 < tdef->typelib_idx)
        return;

    type = type_alias_get_aliasee(tdef);

    if (!type->name || strcmp(tdef->name, type->name) != 0)
    {
        tdef->typelib_idx = typelib->typelib_header.nrtypeinfos;
        msft_typeinfo = create_msft_typeinfo(typelib, TKIND_ALIAS, tdef->name, tdef->attrs);
    }
    else
        duplicate = 1;

    encode_type(typelib, get_type_vt(type), type, &datatype1, &datatype2);
    size = type_memsize_and_alignment(type, &alignment);

    if (msft_typeinfo)
    {
        msft_typeinfo->typeinfo->datatype1 = datatype1;
        msft_typeinfo->typeinfo->size = size;
        msft_typeinfo->typeinfo->datatype2 = datatype2;
        msft_typeinfo->typeinfo->typekind |= (alignment << 11 | alignment << 6);
    }

    /* avoid adding duplicate type definitions */
    if (duplicate)
        tdef->typelib_idx = type->typelib_idx;
}

static void add_coclass_typeinfo(msft_typelib_t *typelib, type_t *cls)
{
    msft_typeinfo_t *msft_typeinfo;
    ifref_t *iref;
    int num_ifaces = 0, offset, i;
    MSFT_RefRecord *ref, *first = NULL, *first_source = NULL;
    int have_default = 0, have_default_source = 0;
    const attr_t *attr;
    ifref_list_t *ifaces;

    if (-1 < cls->typelib_idx)
        return;

    cls->typelib_idx = typelib->typelib_header.nrtypeinfos;
    msft_typeinfo = create_msft_typeinfo(typelib, TKIND_COCLASS, cls->name, cls->attrs);

    ifaces = type_coclass_get_ifaces(cls);
    if (ifaces) LIST_FOR_EACH_ENTRY( iref, ifaces, ifref_t, entry ) num_ifaces++;

    offset = msft_typeinfo->typeinfo->datatype1 = ctl2_alloc_segment(typelib, MSFT_SEG_REFERENCES,
                                                                     num_ifaces * sizeof(*ref), 0);

    i = 0;
    if (ifaces) LIST_FOR_EACH_ENTRY( iref, ifaces, ifref_t, entry ) {
        if(iref->iface->typelib_idx == -1)
            add_interface_typeinfo(typelib, iref->iface);
        ref = (MSFT_RefRecord*) (typelib->typelib_segment_data[MSFT_SEG_REFERENCES] + offset + i * sizeof(*ref));
        ref->reftype = typelib->typelib_typeinfo_offsets[iref->iface->typelib_idx];
        ref->flags = 0;
        ref->oCustData = -1;
        ref->onext = -1;
        if(i < num_ifaces - 1)
            ref->onext = offset + (i + 1) * sizeof(*ref);

        if (iref->attrs) LIST_FOR_EACH_ENTRY( attr, iref->attrs, const attr_t, entry ) {
            switch(attr->type) {
            case ATTR_DEFAULT:
                ref->flags |= 0x1; /* IMPLTYPEFLAG_FDEFAULT */
                break;
            case ATTR_DEFAULTVTABLE:
                ref->flags |= 0x8; /* IMPLTYPEFLAG_FDEFAULTVTABLE */
                break;
            case ATTR_RESTRICTED:
                ref->flags |= 0x4; /* IMPLTYPEFLAG_FRESTRICTED */
                break;
            case ATTR_SOURCE:
                ref->flags |= 0x2; /* IMPLTYPEFLAG_FSOURCE */
                break;
            default:
                warning("add_coclass_typeinfo: unhandled attr %d\n", attr->type);
            }
        }
        if(ref->flags & 0x1) { /* IMPLTYPEFLAG_FDEFAULT */
            if(ref->flags & 0x2) /* IMPLTYPEFLAG_SOURCE */
                have_default_source = 1;
            else
                have_default = 1;
        }

        /* If the interface is non-restricted and we haven't already had one then
           remember it so that we can use it as a default later */
        if((ref->flags & 0x4) == 0) { /* IMPLTYPEFLAG_FRESTRICTED */
            if(ref->flags & 0x2) { /* IMPLTYPEFLAG_FSOURCE */
                if(!first_source)
                    first_source = ref;
            }
            else if(!first)
                first = ref;
        }
        i++;
    }

    /* If we haven't had a default interface, then set the default flags on the
       first ones */
    if(!have_default && first)
        first->flags |= 0x1;
    if(!have_default_source && first_source)
        first_source->flags |= 0x1;

    msft_typeinfo->typeinfo->cImplTypes = num_ifaces;
    msft_typeinfo->typeinfo->size = pointer_size;
    msft_typeinfo->typeinfo->typekind |= 0x2200;
}

static void add_module_typeinfo(msft_typelib_t *typelib, type_t *module)
{
    int idx = 0;
    const statement_t *stmt;
    msft_typeinfo_t *msft_typeinfo;

    if (-1 < module->typelib_idx)
        return;

    module->typelib_idx = typelib->typelib_header.nrtypeinfos;
    msft_typeinfo = create_msft_typeinfo(typelib, TKIND_MODULE, module->name, module->attrs);
    msft_typeinfo->typeinfo->typekind |= 0x0a00;

    STATEMENTS_FOR_EACH_FUNC( stmt, module->details.module->stmts ) {
        var_t *func = stmt->u.var;
        if(add_func_desc(msft_typeinfo, func, idx) == S_OK)
            idx++;
    }

    msft_typeinfo->typeinfo->size = idx;
}

static void add_type_typeinfo(msft_typelib_t *typelib, type_t *type)
{
    switch (type_get_type(type)) {
    case TYPE_INTERFACE:
        add_interface_typeinfo(typelib, type);
        break;
    case TYPE_STRUCT:
        add_structure_typeinfo(typelib, type);
        break;
    case TYPE_ENUM:
        add_enum_typeinfo(typelib, type);
        break;
    case TYPE_UNION:
        add_union_typeinfo(typelib, type);
        break;
    case TYPE_COCLASS:
        add_coclass_typeinfo(typelib, type);
        break;
    case TYPE_BASIC:
    case TYPE_POINTER:
    case TYPE_ARRAY:
        break;
    default:
        error("add_entry: unhandled type 0x%x for %s\n",
              type_get_type(type), type->name);
        break;
    }
}

static void add_entry(msft_typelib_t *typelib, const statement_t *stmt)
{
    switch(stmt->type) {
    case STMT_LIBRARY:
    case STMT_IMPORT:
    case STMT_PRAGMA:
    case STMT_CPPQUOTE:
    case STMT_DECLARATION:
        /* not included in typelib */
        break;
    case STMT_IMPORTLIB:
        /* not processed here */
        break;
    case STMT_TYPEDEF:
    {
        const type_list_t *type_entry = stmt->u.type_list;
        for (; type_entry; type_entry = type_entry->next) {
            /* if the type is public then add the typedef, otherwise attempt
             * to add the aliased type */
            if (is_attr(type_entry->type->attrs, ATTR_PUBLIC))
                add_typedef_typeinfo(typelib, type_entry->type);
            else
                add_type_typeinfo(typelib, type_alias_get_aliasee(type_entry->type));
        }
        break;
    }
    case STMT_MODULE:
        add_module_typeinfo(typelib, stmt->u.type);
        break;
    case STMT_TYPE:
    case STMT_TYPEREF:
    {
        type_t *type = stmt->u.type;
        add_type_typeinfo(typelib, type);
        break;
    }
    }
}

static void set_name(msft_typelib_t *typelib)
{
    int offset;

    offset = ctl2_alloc_name(typelib, typelib->typelib->name);
    if (offset == -1) return;
    typelib->typelib_header.NameOffset = offset;
    return;
}

static void set_version(msft_typelib_t *typelib)
{
    typelib->typelib_header.version = get_attrv( typelib->typelib->attrs, ATTR_VERSION );
}

static void set_guid(msft_typelib_t *typelib)
{
    MSFT_GuidEntry guidentry;
    int offset;
    void *ptr;
    GUID guid = {0,0,0,{0,0,0,0,0,0}};

    guidentry.guid = guid;
    guidentry.hreftype = -2;
    guidentry.next_hash = -1;

    ptr = get_attrp( typelib->typelib->attrs, ATTR_UUID );
    if (ptr) guidentry.guid = *(GUID *)ptr;

    offset = ctl2_alloc_guid(typelib, &guidentry);
    typelib->typelib_header.posguid = offset;

    return;
}

static void set_doc_string(msft_typelib_t *typelib)
{
    char *str = get_attrp( typelib->typelib->attrs, ATTR_HELPSTRING );

    if (str)
    {
        int offset = ctl2_alloc_string(typelib, str);
        if (offset != -1) typelib->typelib_header.helpstring = offset;
    }
}

static void set_help_file_name(msft_typelib_t *typelib)
{
    char *str = get_attrp( typelib->typelib->attrs, ATTR_HELPFILE );

    if (str)
    {
        int offset = ctl2_alloc_string(typelib, str);
        if (offset != -1)
        {
            typelib->typelib_header.helpfile = offset;
            typelib->typelib_header.varflags |= 0x10;
        }
    }
}

static void set_help_context(msft_typelib_t *typelib)
{
    const expr_t *expr = get_attrp( typelib->typelib->attrs, ATTR_HELPCONTEXT );
    if (expr) typelib->typelib_header.helpcontext = expr->cval;
}

static void set_help_string_dll(msft_typelib_t *typelib)
{
    char *str = get_attrp( typelib->typelib->attrs, ATTR_HELPSTRINGDLL );

    if (str)
    {
        int offset = ctl2_alloc_string(typelib, str);
        if (offset != -1)
        {
            typelib->help_string_dll_offset = offset;
            typelib->typelib_header.varflags |= 0x100;
        }
    }
}

static void set_help_string_context(msft_typelib_t *typelib)
{
    const expr_t *expr = get_attrp( typelib->typelib->attrs, ATTR_HELPSTRINGCONTEXT );
    if (expr) typelib->typelib_header.helpstringcontext = expr->cval;
}

static void set_lcid(msft_typelib_t *typelib)
{
    const expr_t *lcid_expr = get_attrp( typelib->typelib->attrs, ATTR_LIBLCID );
    if(lcid_expr)
    {
        typelib->typelib_header.lcid  = lcid_expr->cval;
        typelib->typelib_header.lcid2 = lcid_expr->cval;
    }
}

static void set_lib_flags(msft_typelib_t *typelib)
{
    const attr_t *attr;

    typelib->typelib_header.flags = 0;
    if (!typelib->typelib->attrs) return;
    LIST_FOR_EACH_ENTRY( attr, typelib->typelib->attrs, const attr_t, entry )
    {
        switch(attr->type) {
        case ATTR_CONTROL:
            typelib->typelib_header.flags |= 0x02; /* LIBFLAG_FCONTROL */
            break;
        case ATTR_HIDDEN:
            typelib->typelib_header.flags |= 0x04; /* LIBFLAG_FHIDDEN */
            break;
        case ATTR_RESTRICTED:
            typelib->typelib_header.flags |= 0x01; /* LIBFLAG_FRESTRICTED */
            break;
        default:
            break;
        }
    }
    return;
}

static void ctl2_write_segment(msft_typelib_t *typelib, int segment)
{
    put_data(typelib->typelib_segment_data[segment], typelib->typelib_segdir[segment].length);
}

static void ctl2_finalize_typeinfos(msft_typelib_t *typelib, int filesize)
{
    msft_typeinfo_t *typeinfo;

    for (typeinfo = typelib->typeinfos; typeinfo; typeinfo = typeinfo->next_typeinfo) {
	typeinfo->typeinfo->memoffset = filesize;
	if (typeinfo->func_data)
	    filesize += typeinfo->func_data[0] + ((typeinfo->typeinfo->cElement & 0xffff) * 12);
	if (typeinfo->var_data)
	    filesize += typeinfo->var_data[0] + (((typeinfo->typeinfo->cElement >> 16) & 0xffff) * 12);
        if (typeinfo->func_data || typeinfo->var_data)
            filesize += 4;
    }
}

static int ctl2_finalize_segment(msft_typelib_t *typelib, int filepos, int segment)
{
    if (typelib->typelib_segdir[segment].length) {
	typelib->typelib_segdir[segment].offset = filepos;
    } else {
	typelib->typelib_segdir[segment].offset = -1;
    }

    return typelib->typelib_segdir[segment].length;
}


static void ctl2_write_typeinfos(msft_typelib_t *typelib)
{
    msft_typeinfo_t *typeinfo;
    int typedata_size;

    for (typeinfo = typelib->typeinfos; typeinfo; typeinfo = typeinfo->next_typeinfo) {
        if (!typeinfo->func_data && !typeinfo->var_data) continue;
        typedata_size = 0;
	if (typeinfo->func_data)
            typedata_size = typeinfo->func_data[0];
	if (typeinfo->var_data)
            typedata_size += typeinfo->var_data[0];
	put_data(&typedata_size, sizeof(int));
        if (typeinfo->func_data)
            put_data(typeinfo->func_data + 1, typeinfo->func_data[0]);
        if (typeinfo->var_data)
            put_data(typeinfo->var_data + 1, typeinfo->var_data[0]);
        if (typeinfo->func_indices)
            put_data(typeinfo->func_indices, (typeinfo->typeinfo->cElement & 0xffff) * 4);
        if (typeinfo->var_indices)
            put_data(typeinfo->var_indices, (typeinfo->typeinfo->cElement >> 16) * 4);
        if (typeinfo->func_names)
            put_data(typeinfo->func_names,   (typeinfo->typeinfo->cElement & 0xffff) * 4);
        if (typeinfo->var_names)
            put_data(typeinfo->var_names,   (typeinfo->typeinfo->cElement >> 16) * 4);
        if (typeinfo->func_offsets)
            put_data(typeinfo->func_offsets, (typeinfo->typeinfo->cElement & 0xffff) * 4);
        if (typeinfo->var_offsets) {
            int add = 0, i, offset;
            if(typeinfo->func_data)
                add = typeinfo->func_data[0];
            for(i = 0; i < (typeinfo->typeinfo->cElement >> 16); i++) {
                offset = typeinfo->var_offsets[i];
                offset += add;
                put_data(&offset, 4);
            }
        }
    }
}

static void save_all_changes(msft_typelib_t *typelib)
{
    int filepos;

    chat("save_all_changes(%p)\n", typelib);

    filepos = sizeof(MSFT_Header) + sizeof(MSFT_SegDir);
    if(typelib->typelib_header.varflags & 0x100) filepos += 4; /* helpstringdll */
    filepos += typelib->typelib_header.nrtypeinfos * 4;

    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_TYPEINFO);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_GUIDHASH);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_GUID);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_REFERENCES);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_IMPORTINFO);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_IMPORTFILES);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_NAMEHASH);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_NAME);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_STRING);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_TYPEDESC);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_ARRAYDESC);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_CUSTDATA);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_CUSTDATAGUID);

    ctl2_finalize_typeinfos(typelib, filepos);

    byte_swapped = 0;
    init_output_buffer();

    put_data(&typelib->typelib_header, sizeof(typelib->typelib_header));
    if(typelib->typelib_header.varflags & 0x100)
        put_data(&typelib->help_string_dll_offset, sizeof(typelib->help_string_dll_offset));

    put_data(typelib->typelib_typeinfo_offsets, typelib->typelib_header.nrtypeinfos * 4);
    put_data(&typelib->typelib_segdir, sizeof(typelib->typelib_segdir));
    ctl2_write_segment( typelib, MSFT_SEG_TYPEINFO );
    ctl2_write_segment( typelib, MSFT_SEG_GUIDHASH );
    ctl2_write_segment( typelib, MSFT_SEG_GUID );
    ctl2_write_segment( typelib, MSFT_SEG_REFERENCES );
    ctl2_write_segment( typelib, MSFT_SEG_IMPORTINFO );
    ctl2_write_segment( typelib, MSFT_SEG_IMPORTFILES );
    ctl2_write_segment( typelib, MSFT_SEG_NAMEHASH );
    ctl2_write_segment( typelib, MSFT_SEG_NAME );
    ctl2_write_segment( typelib, MSFT_SEG_STRING );
    ctl2_write_segment( typelib, MSFT_SEG_TYPEDESC );
    ctl2_write_segment( typelib, MSFT_SEG_ARRAYDESC );
    ctl2_write_segment( typelib, MSFT_SEG_CUSTDATA );
    ctl2_write_segment( typelib, MSFT_SEG_CUSTDATAGUID );

    ctl2_write_typeinfos(typelib);

    if (strendswith( typelib_name, ".res" ))  /* create a binary resource file */
    {
        char typelib_id[13] = "#1";

        expr_t *expr = get_attrp( typelib->typelib->attrs, ATTR_ID );
        if (expr)
            sprintf( typelib_id, "#%d", expr->cval );
        add_output_to_resources( "TYPELIB", typelib_id );
        output_typelib_regscript( typelib->typelib );
#ifdef __REACTOS__
        flush_output_resources( typelib_name );
#endif
    }
    else flush_output_buffer( typelib_name );
}

int create_msft_typelib(typelib_t *typelib)
{
    msft_typelib_t *msft;
    int failed = 0;
    const statement_t *stmt;
    time_t cur_time;
    char *time_override;
    unsigned int version = 7 << 24 | 555; /* 7.00.0555 */
    GUID midl_time_guid    = {0xde77ba63,0x517c,0x11d1,{0xa2,0xda,0x00,0x00,0xf8,0x77,0x3c,0xe9}};
    GUID midl_version_guid = {0xde77ba64,0x517c,0x11d1,{0xa2,0xda,0x00,0x00,0xf8,0x77,0x3c,0xe9}};
    GUID midl_info_guid = {0xde77ba65,0x517c,0x11d1,{0xa2,0xda,0x00,0x00,0xf8,0x77,0x3c,0xe9}};
    char info_string[128];

    msft = xmalloc(sizeof(*msft));
    memset(msft, 0, sizeof(*msft));
    msft->typelib = typelib;

    ctl2_init_header(msft);
    ctl2_init_segdir(msft);

    msft->typelib_header.varflags |= (pointer_size == 8) ? SYS_WIN64 : SYS_WIN32;

    /*
     * The following two calls return an offset or -1 if out of memory. We
     * specifically need an offset of 0, however, so...
     */
    if (ctl2_alloc_segment(msft, MSFT_SEG_GUIDHASH, 0x80, 0x80)) { failed = 1; }
    if (ctl2_alloc_segment(msft, MSFT_SEG_NAMEHASH, 0x200, 0x200)) { failed = 1; }

    if(failed)
    {
        free(msft);
        return 0;
    }

    msft->typelib_guidhash_segment = (int *)msft->typelib_segment_data[MSFT_SEG_GUIDHASH];
    msft->typelib_namehash_segment = (int *)msft->typelib_segment_data[MSFT_SEG_NAMEHASH];

    memset(msft->typelib_guidhash_segment, 0xff, 0x80);
    memset(msft->typelib_namehash_segment, 0xff, 0x200);

    set_lib_flags(msft);
    set_lcid(msft);
    set_help_file_name(msft);
    set_doc_string(msft);
    set_guid(msft);
    set_version(msft);
    set_name(msft);
    set_help_context(msft);
    set_help_string_dll(msft);
    set_help_string_context(msft);
    
    /* midl adds two sets of custom data to the library: the current unix time
       and midl's version number */
    time_override = getenv( "WIDL_TIME_OVERRIDE");
    cur_time = time_override ? atol( time_override) : time(NULL);
    sprintf(info_string, "Created by WIDL version %s at %s\n", PACKAGE_VERSION, ctime(&cur_time));
    set_custdata(msft, &midl_info_guid, VT_BSTR, info_string, &msft->typelib_header.CustomDataOffset);
    set_custdata(msft, &midl_time_guid, VT_UI4, &cur_time, &msft->typelib_header.CustomDataOffset);
    set_custdata(msft, &midl_version_guid, VT_UI4, &version, &msft->typelib_header.CustomDataOffset);

    if (typelib->stmts)
        LIST_FOR_EACH_ENTRY( stmt, typelib->stmts, const statement_t, entry )
            add_entry(msft, stmt);

    save_all_changes(msft);
    free(msft);
    return 1;
}
