/*
 *	TYPELIB2
 *
 *	Copyright 2004  Alastair Bridgewater
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"

#include "wine/unicode.h"
#include "objbase.h"
#include "typelib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(typelib2);
/* WINE_DEFAULT_DEBUG_CHANNEL(ole); */


/******************************************************************************
 * ICreateTypeLib2 {OLEAUT32}
 *
 * NOTES
 *  The ICreateTypeLib2 interface provides an interface whereby one may create
 *  new type library (.tlb) files.
 *
 *  This interface inherits from ICreateTypeLib, and can be freely cast back
 *  and forth between an ICreateTypeLib and an ICreateTypeLib2 on local clients.
 *  This dispensation applies only to ICreateTypeLib objects obtained on MSFT
 *  format type libraries (those made through CreateTypeLib2).
 *
 * METHODS
 */

/******************************************************************************
 * ICreateTypeInfo2 {OLEAUT32}
 *
 * NOTES
 *  The ICreateTypeInfo2 interface provides an interface whereby one may add
 *  type information to type library (.tlb) files.
 *
 *  This interface inherits from ICreateTypeInfo, and can be freely cast back
 *  and forth between an ICreateTypeInfo and an ICreateTypeInfo2 on local clients.
 *  This dispensation applies only to ICreateTypeInfo objects obtained on MSFT
 *  format type libraries (those made through CreateTypeLib2).
 *
 * METHODS
 */

/******************************************************************************
 * ITypeLib2 {OLEAUT32}
 *
 * NOTES
 *  The ITypeLib2 interface provides an interface whereby one may query MSFT
 *  format type library (.tlb) files.
 *
 *  This interface inherits from ITypeLib, and can be freely cast back and
 *  forth between an ITypeLib and an ITypeLib2 on local clients. This
 *  dispensation applies only to ITypeLib objects obtained on MSFT format type
 *  libraries (those made through CreateTypeLib2).
 *
 * METHODS
 */

/******************************************************************************
 * ITypeInfo2 {OLEAUT32}
 *
 * NOTES
 *  The ITypeInfo2 interface provides an interface whereby one may query type
 *  information stored in MSFT format type library (.tlb) files.
 *
 *  This interface inherits from ITypeInfo, and can be freely cast back and
 *  forth between an ITypeInfo and an ITypeInfo2 on local clients. This
 *  dispensation applies only to ITypeInfo objects obtained on MSFT format type
 *  libraries (those made through CreateTypeLib2).
 *
 * METHODS
 */

/*================== Implementation Structures ===================================*/

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

typedef struct tagICreateTypeLib2Impl
{
    const ICreateTypeLib2Vtbl *lpVtbl;
    const ITypeLib2Vtbl       *lpVtblTypeLib2;

    LONG ref;

    WCHAR *filename;

    MSFT_Header typelib_header;
    MSFT_pSeg typelib_segdir[MSFT_SEG_MAX];
    char *typelib_segment_data[MSFT_SEG_MAX];
    int typelib_segment_block_length[MSFT_SEG_MAX];

    INT typelib_typeinfo_offsets[0x200]; /* Hope that's enough. */

    INT *typelib_namehash_segment;
    INT *typelib_guidhash_segment;

    struct tagICreateTypeInfo2Impl *typeinfos;
    struct tagICreateTypeInfo2Impl *last_typeinfo;
} ICreateTypeLib2Impl;

static inline ICreateTypeLib2Impl *impl_from_ITypeLib2( ITypeLib2 *iface )
{
    return (ICreateTypeLib2Impl *)((char*)iface - FIELD_OFFSET(ICreateTypeLib2Impl, lpVtblTypeLib2));
}

typedef struct tagICreateTypeInfo2Impl
{
    const ICreateTypeInfo2Vtbl *lpVtbl;
    const ITypeInfo2Vtbl       *lpVtblTypeInfo2;

    LONG ref;

    ICreateTypeLib2Impl *typelib;
    MSFT_TypeInfoBase *typeinfo;

    INT *typedata;
    int typedata_allocated;
    int typedata_length;

    int indices[42];
    int names[42];
    int offsets[42];

    int datawidth;

    struct tagICreateTypeInfo2Impl *next_typeinfo;
} ICreateTypeInfo2Impl;

static inline ICreateTypeInfo2Impl *impl_from_ITypeInfo2( ITypeInfo2 *iface )
{
    return (ICreateTypeInfo2Impl *)((char*)iface - FIELD_OFFSET(ICreateTypeInfo2Impl, lpVtblTypeInfo2));
}

static ULONG WINAPI ICreateTypeLib2_fnRelease(ICreateTypeLib2 *iface);


/*================== Internal functions ===================================*/

/****************************************************************************
 *	ctl2_init_header
 *
 *  Initializes the type library header of a new typelib.
 */
static void ctl2_init_header(
	ICreateTypeLib2Impl *This) /* [I] The typelib to initialize. */
{
    This->typelib_header.magic1 = 0x5446534d;
    This->typelib_header.magic2 = 0x00010002;
    This->typelib_header.posguid = -1;
    This->typelib_header.lcid = 0x0409; /* or do we use the current one? */
    This->typelib_header.lcid2 = 0x0409;
    This->typelib_header.varflags = 0x40;
    This->typelib_header.version = 0;
    This->typelib_header.flags = 0;
    This->typelib_header.nrtypeinfos = 0;
    This->typelib_header.helpstring = -1;
    This->typelib_header.helpstringcontext = 0;
    This->typelib_header.helpcontext = 0;
    This->typelib_header.nametablecount = 0;
    This->typelib_header.nametablechars = 0;
    This->typelib_header.NameOffset = -1;
    This->typelib_header.helpfile = -1;
    This->typelib_header.CustomDataOffset = -1;
    This->typelib_header.res44 = 0x20;
    This->typelib_header.res48 = 0x80;
    This->typelib_header.dispatchpos = -1;
    This->typelib_header.nimpinfos = 0;
}

/****************************************************************************
 *	ctl2_init_segdir
 *
 *  Initializes the segment directory of a new typelib.
 */
static void ctl2_init_segdir(
	ICreateTypeLib2Impl *This) /* [I] The typelib to initialize. */
{
    int i;
    MSFT_pSeg *segdir;

    segdir = &This->typelib_segdir[MSFT_SEG_TYPEINFO];

    for (i = 0; i < 15; i++) {
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
	REFGUID guid)                /* [I] The guid to find. */
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
	ICreateTypeLib2Impl *This, /* [I] The typelib to operate against. */
	int hash_key,              /* [I] The hash key for the guid. */
	REFGUID guid)                /* [I] The guid to find. */
{
    int offset;
    MSFT_GuidEntry *guidentry;

    offset = This->typelib_guidhash_segment[hash_key];
    while (offset != -1) {
	guidentry = (MSFT_GuidEntry *)&This->typelib_segment_data[MSFT_SEG_GUID][offset];

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
	ICreateTypeLib2Impl *This, /* [I] The typelib to operate against. */
	char *name)                /* [I] The encoded name to find. */
{
    int offset;
    int *namestruct;

    offset = This->typelib_namehash_segment[name[2] & 0x7f];
    while (offset != -1) {
	namestruct = (int *)&This->typelib_segment_data[MSFT_SEG_NAME][offset];

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
	ICreateTypeLib2Impl *This, /* [I] The typelib to operate against (used for LCID only). */
	const WCHAR *name,         /* [I] The name string to encode. */
	char **result)             /* [O] A pointer to a pointer to receive the encoded name. */
{
    int length;
    static char converted_name[0x104];
    int offset;
    int value;

    length = WideCharToMultiByte(CP_ACP, 0, name, strlenW(name), converted_name+4, 0x100, NULL, NULL);
    converted_name[0] = length & 0xff;

    converted_name[length + 4] = 0;

    converted_name[1] = 0x00;

    value = LHashValOfNameSysA(This->typelib_header.varflags & 0x0f, This->typelib_header.lcid, converted_name + 4);

    converted_name[2] = value;
    converted_name[3] = value >> 8;

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
	ICreateTypeLib2Impl *This, /* [I] The typelib to operate against (not used?). */
	const WCHAR *string,       /* [I] The string to encode. */
	char **result)             /* [O] A pointer to a pointer to receive the encoded string. */
{
    int length;
    static char converted_string[0x104];
    int offset;

    length = WideCharToMultiByte(CP_ACP, 0, string, strlenW(string), converted_string+2, 0x102, NULL, NULL);
    converted_string[0] = length & 0xff;
    converted_string[1] = (length >> 8) & 0xff;

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
 *  Failure: -1 (this is invariably an out of memory condition).
 *
 * BUGS
 *
 *  Does not (yet) handle the case where the allocated segment memory needs to grow.
 */
static int ctl2_alloc_segment(
	ICreateTypeLib2Impl *This,       /* [I] The type library in which to allocate. */
	enum MSFT_segment_index segment, /* [I] The segment in which to allocate. */
	int size,                        /* [I] The amount to allocate. */
	int block_size)                  /* [I] Initial allocation block size, or 0 for default. */
{
    int offset;

    if(!This->typelib_segment_data[segment]) {
	if (!block_size) block_size = 0x2000;

	This->typelib_segment_block_length[segment] = block_size;
	This->typelib_segment_data[segment] = HeapAlloc(GetProcessHeap(), 0, block_size);
	if (!This->typelib_segment_data[segment]) return -1;
	memset(This->typelib_segment_data[segment], 0x57, block_size);
    }

    while ((This->typelib_segdir[segment].length + size) > This->typelib_segment_block_length[segment]) {
	char *block;

	block_size = This->typelib_segment_block_length[segment];
	block = HeapReAlloc(GetProcessHeap(), 0, This->typelib_segment_data[segment], block_size << 1);
	if (!block) return -1;

	if (segment == MSFT_SEG_TYPEINFO) {
	    /* TypeInfos have a direct pointer to their memory space, so we have to fix them up. */
	    ICreateTypeInfo2Impl *typeinfo;

	    for (typeinfo = This->typeinfos; typeinfo; typeinfo = typeinfo->next_typeinfo) {
		typeinfo->typeinfo = (void *)&block[((char *)typeinfo->typeinfo) - This->typelib_segment_data[segment]];
	    }
	}

	memset(block + block_size, 0x57, block_size);
	This->typelib_segment_block_length[segment] = block_size << 1;
	This->typelib_segment_data[segment] = block;
    }

    offset = This->typelib_segdir[segment].length;
    This->typelib_segdir[segment].length += size;

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
	ICreateTypeLib2Impl *This, /* [I] The type library to allocate in. */
	int nameoffset)            /* [I] The offset of the name for this typeinfo. */
{
    int offset;
    MSFT_TypeInfoBase *typeinfo;

    offset = ctl2_alloc_segment(This, MSFT_SEG_TYPEINFO, sizeof(MSFT_TypeInfoBase), 0);
    if (offset == -1) return -1;

    This->typelib_typeinfo_offsets[This->typelib_header.nrtypeinfos++] = offset;

    typeinfo = (void *)(This->typelib_segment_data[MSFT_SEG_TYPEINFO] + offset);

    typeinfo->typekind = (This->typelib_header.nrtypeinfos - 1) << 16;
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
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_guid(
	ICreateTypeLib2Impl *This, /* [I] The type library to allocate in. */
	MSFT_GuidEntry *guid)      /* [I] The GUID to store. */
{
    int offset;
    MSFT_GuidEntry *guid_space;
    int hash_key;

    hash_key = ctl2_hash_guid(&guid->guid);

    offset = ctl2_find_guid(This, hash_key, &guid->guid);
    if (offset != -1) return offset;

    offset = ctl2_alloc_segment(This, MSFT_SEG_GUID, sizeof(MSFT_GuidEntry), 0);
    if (offset == -1) return -1;

    guid_space = (void *)(This->typelib_segment_data[MSFT_SEG_GUID] + offset);
    *guid_space = *guid;

    guid_space->next_hash = This->typelib_guidhash_segment[hash_key];
    This->typelib_guidhash_segment[hash_key] = offset;

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
	ICreateTypeLib2Impl *This, /* [I] The type library to allocate in. */
	const WCHAR *name)         /* [I] The name to store. */
{
    int length;
    int offset;
    MSFT_NameIntro *name_space;
    char *encoded_name;

    length = ctl2_encode_name(This, name, &encoded_name);

    offset = ctl2_find_name(This, encoded_name);
    if (offset != -1) return offset;

    offset = ctl2_alloc_segment(This, MSFT_SEG_NAME, length + 8, 0);
    if (offset == -1) return -1;

    name_space = (void *)(This->typelib_segment_data[MSFT_SEG_NAME] + offset);
    name_space->hreftype = -1;
    name_space->next_hash = -1;
    memcpy(&name_space->namelen, encoded_name, length);

    if (This->typelib_namehash_segment[encoded_name[2] & 0x7f] != -1)
	name_space->next_hash = This->typelib_namehash_segment[encoded_name[2] & 0x7f];

    This->typelib_namehash_segment[encoded_name[2] & 0x7f] = offset;

    This->typelib_header.nametablecount += 1;
    This->typelib_header.nametablechars += *encoded_name;

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
	ICreateTypeLib2Impl *This, /* [I] The type library to allocate in. */
	const WCHAR *string)       /* [I] The string to store. */
{
    int length;
    int offset;
    char *string_space;
    char *encoded_string;

    length = ctl2_encode_string(This, string, &encoded_string);

    for (offset = 0; offset < This->typelib_segdir[MSFT_SEG_STRING].length;
	 offset += ((((This->typelib_segment_data[MSFT_SEG_STRING][offset + 1] << 8) & 0xff)
	     | (This->typelib_segment_data[MSFT_SEG_STRING][offset + 0] & 0xff)) + 5) & ~3) {
	if (!memcmp(encoded_string, This->typelib_segment_data[MSFT_SEG_STRING] + offset, length)) return offset;
    }

    offset = ctl2_alloc_segment(This, MSFT_SEG_STRING, length, 0);
    if (offset == -1) return -1;

    string_space = This->typelib_segment_data[MSFT_SEG_STRING] + offset;
    memcpy(string_space, encoded_string, length);

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_importinfo
 *
 *  Allocates and initializes an import information structure in a type library.
 *
 * RETURNS
 *
 *  Success: The offset of the new importinfo.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_importinfo(
	ICreateTypeLib2Impl *This, /* [I] The type library to allocate in. */
	MSFT_ImpInfo *impinfo)     /* [I] The import information to store. */
{
    int offset;
    MSFT_ImpInfo *impinfo_space;

    for (offset = 0;
	 offset < This->typelib_segdir[MSFT_SEG_IMPORTINFO].length;
	 offset += sizeof(MSFT_ImpInfo)) {
	if (!memcmp(&(This->typelib_segment_data[MSFT_SEG_IMPORTINFO][offset]),
		    impinfo, sizeof(MSFT_ImpInfo))) {
	    return offset;
	}
    }

    impinfo->flags |= This->typelib_header.nimpinfos++;

    offset = ctl2_alloc_segment(This, MSFT_SEG_IMPORTINFO, sizeof(MSFT_ImpInfo), 0);
    if (offset == -1) return -1;

    impinfo_space = (void *)(This->typelib_segment_data[MSFT_SEG_IMPORTINFO] + offset);
    *impinfo_space = *impinfo;

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_importfile
 *
 *  Allocates and initializes an import file definition in a type library.
 *
 * RETURNS
 *
 *  Success: The offset of the new importinfo.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_importfile(
	ICreateTypeLib2Impl *This, /* [I] The type library to allocate in. */
	int guidoffset,            /* [I] The offset to the GUID for the imported library. */
	int major_version,         /* [I] The major version number of the imported library. */
	int minor_version,         /* [I] The minor version number of the imported library. */
	const WCHAR *filename)     /* [I] The filename of the imported library. */
{
    int length;
    int offset;
    MSFT_ImpFile *importfile;
    char *encoded_string;

    length = ctl2_encode_string(This, filename, &encoded_string);

    encoded_string[0] <<= 2;
    encoded_string[0] |= 1;

    for (offset = 0; offset < This->typelib_segdir[MSFT_SEG_IMPORTFILES].length;
	 offset += ((((This->typelib_segment_data[MSFT_SEG_IMPORTFILES][offset + 0xd] << 8) & 0xff)
	     | (This->typelib_segment_data[MSFT_SEG_IMPORTFILES][offset + 0xc] & 0xff)) >> 2) + 0xc) {
	if (!memcmp(encoded_string, This->typelib_segment_data[MSFT_SEG_IMPORTFILES] + offset + 0xc, length)) return offset;
    }

    offset = ctl2_alloc_segment(This, MSFT_SEG_IMPORTFILES, length + 0xc, 0);
    if (offset == -1) return -1;

    importfile = (MSFT_ImpFile *)&This->typelib_segment_data[MSFT_SEG_IMPORTFILES][offset];
    importfile->guid = guidoffset;
    importfile->lcid = This->typelib_header.lcid2;
    importfile->version = major_version | (minor_version << 16);
    memcpy(&importfile->filename, encoded_string, length);

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_custdata
 *
 *  Allocates and initializes a "custom data" value in a type library.
 *
 * RETURNS
 *
 *  Success: The offset of the new custdata.
 *  Failure:
 *
 *    -1: Out of memory.
 *    -2: Unable to encode VARIANT data (typically a bug).
 */
static int ctl2_alloc_custdata(
	ICreateTypeLib2Impl *This, /* [I] The type library in which to encode the value. */
	VARIANT *pVarVal)          /* [I] The value to encode. */
{
    int offset;

    TRACE("(%p,%p(%d))\n",This,pVarVal,V_VT(pVarVal));

    switch (V_VT(pVarVal)) {
    case VT_UI4:
	offset = ctl2_alloc_segment(This, MSFT_SEG_CUSTDATA, 8, 0);
	if (offset == -1) return offset;

	*((unsigned short *)&This->typelib_segment_data[MSFT_SEG_CUSTDATA][offset]) = VT_UI4;
	*((unsigned long *)&This->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+2]) = V_UI4(pVarVal);
	break;

    default:
	FIXME("Unknown variable encoding vt %d.\n", V_VT(pVarVal));
	return -2;
    }

    return offset;
}

/****************************************************************************
 *	ctl2_set_custdata
 *
 *  Adds a custom data element to an object in a type library.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_INVALIDARG or E_OUTOFMEMORY.
 */
static HRESULT ctl2_set_custdata(
	ICreateTypeLib2Impl *This, /* [I] The type library to store the custom data in. */
	REFGUID guid,              /* [I] The GUID used as a key to retrieve the custom data. */
	VARIANT *pVarVal,          /* [I] The custom data itself. */
	int *offset)               /* [I/O] The list of custom data to prepend to. */
{
    MSFT_GuidEntry guidentry;
    int dataoffset;
    int guidoffset;
    int custoffset;
    int *custdata;

    guidentry.guid = *guid;

    guidentry.hreftype = -1;
    guidentry.next_hash = -1;

    guidoffset = ctl2_alloc_guid(This, &guidentry);
    if (guidoffset == -1) return E_OUTOFMEMORY;
    dataoffset = ctl2_alloc_custdata(This, pVarVal);
    if (dataoffset == -1) return E_OUTOFMEMORY;
    if (dataoffset == -2) return E_INVALIDARG;

    custoffset = ctl2_alloc_segment(This, MSFT_SEG_CUSTDATAGUID, 12, 0);
    if (custoffset == -1) return E_OUTOFMEMORY;

    custdata = (int *)&This->typelib_segment_data[MSFT_SEG_CUSTDATAGUID][custoffset];
    custdata[0] = guidoffset;
    custdata[1] = dataoffset;
    custdata[2] = *offset;
    *offset = custoffset;

    return S_OK;
}

/****************************************************************************
 *	ctl2_encode_typedesc
 *
 *  Encodes a type description, storing information in the TYPEDESC and ARRAYDESC
 *  segments as needed.
 *
 * RETURNS
 *
 *  Success: 0.
 *  Failure: -1.
 */
static int ctl2_encode_typedesc(
	ICreateTypeLib2Impl *This, /* [I] The type library in which to encode the TYPEDESC. */
	TYPEDESC *tdesc,           /* [I] The type description to encode. */
	int *encoded_tdesc,        /* [O] The encoded type description. */
	int *width,                /* [O] The width of the type, or NULL. */
	int *alignment,            /* [O] The alignment of the type, or NULL. */
	int *decoded_size)         /* [O] The total size of the unencoded TYPEDESCs, including nested descs. */
{
    int default_tdesc;
    int scratch;
    int typeoffset;
    int arrayoffset;
    int *typedata;
    int *arraydata;
    int target_type;
    int child_size;

    default_tdesc = 0x80000000 | (tdesc->vt << 16) | tdesc->vt;
    if (!width) width = &scratch;
    if (!alignment) alignment = &scratch;
    if (!decoded_size) decoded_size = &scratch;

    *decoded_size = 0;

    switch (tdesc->vt) {
    case VT_UI1:
    case VT_I1:
	*encoded_tdesc = default_tdesc;
	*width = 1;
	*alignment = 1;
	break;

    case VT_INT:
	*encoded_tdesc = 0x80000000 | (VT_I4 << 16) | VT_INT;
	if ((This->typelib_header.varflags & 0x0f) == SYS_WIN16) {
	    *width = 2;
	    *alignment = 2;
	} else {
	    *width = 4;
	    *alignment = 4;
	}
	break;

    case VT_UINT:
	*encoded_tdesc = 0x80000000 | (VT_UI4 << 16) | VT_UINT;
	if ((This->typelib_header.varflags & 0x0f) == SYS_WIN16) {
	    *width = 2;
	    *alignment = 2;
	} else {
	    *width = 4;
	    *alignment = 4;
	}
	break;

    case VT_UI2:
    case VT_I2:
    case VT_BOOL:
	*encoded_tdesc = default_tdesc;
	*width = 2;
	*alignment = 2;
	break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
    case VT_BSTR:
    case VT_HRESULT:
	*encoded_tdesc = default_tdesc;
	*width = 4;
	*alignment = 4;
	break;

    case VT_CY:
	*encoded_tdesc = default_tdesc;
	*width = 8;
	*alignment = 4; /* guess? */
	break;

    case VT_VOID:
	*encoded_tdesc = 0x80000000 | (VT_EMPTY << 16) | tdesc->vt;
	*width = 0;
	*alignment = 1;
	break;

    case VT_PTR:
	/* FIXME: Make with the error checking. */
	FIXME("PTR vartype, may not work correctly.\n");

	ctl2_encode_typedesc(This, tdesc->u.lptdesc, &target_type, NULL, NULL, &child_size);

	for (typeoffset = 0; typeoffset < This->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&This->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if (((typedata[0] & 0xffff) == VT_PTR) && (typedata[1] == target_type)) break;
	}

	if (typeoffset == This->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    int mix_field;
	    
	    if (target_type & 0x80000000) {
		mix_field = ((target_type >> 16) & 0x3fff) | VT_BYREF;
	    } else {
		typedata = (void *)&This->typelib_segment_data[MSFT_SEG_TYPEDESC][target_type];
		mix_field = ((typedata[0] >> 16) == 0x7fff)? 0x7fff: 0x7ffe;
	    }

	    typeoffset = ctl2_alloc_segment(This, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&This->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (mix_field << 16) | VT_PTR;
	    typedata[1] = target_type;
	}

	*encoded_tdesc = typeoffset;

	*width = 4;
	*alignment = 4;
	*decoded_size = sizeof(TYPEDESC) + child_size;
	break;

    case VT_SAFEARRAY:
	/* FIXME: Make with the error checking. */
	FIXME("SAFEARRAY vartype, may not work correctly.\n");

	ctl2_encode_typedesc(This, tdesc->u.lptdesc, &target_type, NULL, NULL, &child_size);

	for (typeoffset = 0; typeoffset < This->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&This->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if (((typedata[0] & 0xffff) == VT_SAFEARRAY) && (typedata[1] == target_type)) break;
	}

	if (typeoffset == This->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    int mix_field;
	    
	    if (target_type & 0x80000000) {
		mix_field = ((target_type >> 16) & VT_TYPEMASK) | VT_ARRAY;
	    } else {
		typedata = (void *)&This->typelib_segment_data[MSFT_SEG_TYPEDESC][target_type];
		mix_field = ((typedata[0] >> 16) == 0x7fff)? 0x7fff: 0x7ffe;
	    }

	    typeoffset = ctl2_alloc_segment(This, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&This->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (mix_field << 16) | VT_SAFEARRAY;
	    typedata[1] = target_type;
	}

	*encoded_tdesc = typeoffset;

	*width = 4;
	*alignment = 4;
	*decoded_size = sizeof(TYPEDESC) + child_size;
	break;

    case VT_CARRAY:
      {
	/* FIXME: Make with the error checking. */
        int num_dims = tdesc->u.lpadesc->cDims, elements = 1, dim;

	ctl2_encode_typedesc(This, &tdesc->u.lpadesc->tdescElem, &target_type, width, alignment, NULL);
	arrayoffset = ctl2_alloc_segment(This, MSFT_SEG_ARRAYDESC, (2 + 2 * num_dims) * sizeof(int), 0);
	arraydata = (void *)&This->typelib_segment_data[MSFT_SEG_ARRAYDESC][arrayoffset];

	arraydata[0] = target_type;
	arraydata[1] = num_dims;
        arraydata[1] |= ((num_dims * 2 * sizeof(int)) << 16);
        arraydata += 2;

        for(dim = 0; dim < num_dims; dim++) {
            arraydata[0] = tdesc->u.lpadesc->rgbounds[dim].cElements;
            arraydata[1] = tdesc->u.lpadesc->rgbounds[dim].lLbound;
            elements *= tdesc->u.lpadesc->rgbounds[dim].cElements;
            arraydata += 2;
        }
	typeoffset = ctl2_alloc_segment(This, MSFT_SEG_TYPEDESC, 8, 0);
	typedata = (void *)&This->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	typedata[0] = (0x7ffe << 16) | VT_CARRAY;
	typedata[1] = arrayoffset;

	*encoded_tdesc = typeoffset;
	*width = *width * elements;
	*decoded_size = sizeof(ARRAYDESC) + (num_dims - 1) * sizeof(SAFEARRAYBOUND);

	break;
      }
    case VT_USERDEFINED:
	TRACE("USERDEFINED.\n");
	for (typeoffset = 0; typeoffset < This->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&This->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if ((typedata[0] == ((0x7fff << 16) | VT_USERDEFINED)) && (typedata[1] == tdesc->u.hreftype)) break;
	}

	if (typeoffset == This->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    typeoffset = ctl2_alloc_segment(This, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&This->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (0x7fff << 16) | VT_USERDEFINED;
	    typedata[1] = tdesc->u.hreftype;
	}

	*encoded_tdesc = typeoffset;
	*width = 0;
	*alignment = 1;
	break;

    default:
	FIXME("Unrecognized type %d.\n", tdesc->vt);
	*encoded_tdesc = default_tdesc;
	*width = 0;
	*alignment = 1;
	break;
    }

    return 0;
}

/****************************************************************************
 *	ctl2_find_nth_reference
 *
 *  Finds a reference by index into the linked list of reference records.
 *
 * RETURNS
 *
 *  Success: Offset of the desired reference record.
 *  Failure: -1.
 */
static int ctl2_find_nth_reference(
	ICreateTypeLib2Impl *This, /* [I] The type library in which to search. */
	int offset,                /* [I] The starting offset of the reference list. */
	int index)                 /* [I] The index of the reference to find. */
{
    MSFT_RefRecord *ref;

    for (; index && (offset != -1); index--) {
	ref = (MSFT_RefRecord *)&This->typelib_segment_data[MSFT_SEG_REFERENCES][offset];
	offset = ref->onext;
    }

    return offset;
}

/****************************************************************************
 *	ctl2_find_typeinfo_from_offset
 *
 *  Finds an ITypeInfo given an offset into the TYPEINFO segment.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: TYPE_E_ELEMENTNOTFOUND.
 */
static HRESULT ctl2_find_typeinfo_from_offset(
	ICreateTypeLib2Impl *This, /* [I] The typelib to find the typeinfo in. */
	int offset,                /* [I] The offset of the desired typeinfo. */
	ITypeInfo **ppTinfo)       /* [I] The typeinfo found. */
{
    void *typeinfodata;
    ICreateTypeInfo2Impl *typeinfo;

    typeinfodata = &This->typelib_segment_data[MSFT_SEG_TYPEINFO][offset];

    for (typeinfo = This->typeinfos; typeinfo; typeinfo = typeinfo->next_typeinfo) {
	if (typeinfo->typeinfo == typeinfodata) {
	    *ppTinfo = (ITypeInfo *)&typeinfo->lpVtblTypeInfo2;
	    ITypeInfo2_AddRef(*ppTinfo);
	    return S_OK;
	}
    }

    ERR("Failed to find typeinfo, invariant varied.\n");

    return TYPE_E_ELEMENTNOTFOUND;
}

/*================== ICreateTypeInfo2 Implementation ===================================*/

/******************************************************************************
 * ICreateTypeInfo2_QueryInterface {OLEAUT32}
 *
 *  See IUnknown_QueryInterface.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnQueryInterface(
	ICreateTypeInfo2 * iface,
	REFIID riid,
	VOID **ppvObject)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    TRACE("(%p)->(IID: %s)\n",This,debugstr_guid(riid));

    *ppvObject=NULL;
    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid,&IID_ICreateTypeInfo)||
       IsEqualIID(riid,&IID_ICreateTypeInfo2))
    {
        *ppvObject = This;
    } else if (IsEqualIID(riid, &IID_ITypeInfo) ||
	       IsEqualIID(riid, &IID_ITypeInfo2)) {
	*ppvObject = &This->lpVtblTypeInfo2;
    }

    if(*ppvObject)
    {
        ICreateTypeLib2_AddRef(iface);
        TRACE("-- Interface: (%p)->(%p)\n",ppvObject,*ppvObject);
        return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/******************************************************************************
 * ICreateTypeInfo2_AddRef {OLEAUT32}
 *
 *  See IUnknown_AddRef.
 */
static ULONG WINAPI ICreateTypeInfo2_fnAddRef(ICreateTypeInfo2 *iface)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->ref was %u\n",This, ref - 1);

    return ref;
}

/******************************************************************************
 * ICreateTypeInfo2_Release {OLEAUT32}
 *
 *  See IUnknown_Release.
 */
static ULONG WINAPI ICreateTypeInfo2_fnRelease(ICreateTypeInfo2 *iface)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n",This, ref);

    if (!ref) {
	if (This->typelib) {
	    ICreateTypeLib2_fnRelease((ICreateTypeLib2 *)This->typelib);
	    This->typelib = NULL;
	}

	/* ICreateTypeLib2 frees all ICreateTypeInfos when it releases. */
	/* HeapFree(GetProcessHeap(),0,This); */
	return 0;
    }

    return ref;
}


/******************************************************************************
 * ICreateTypeInfo2_SetGuid {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetGuid.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetGuid(ICreateTypeInfo2 *iface, REFGUID guid)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    MSFT_GuidEntry guidentry;
    int offset;

    TRACE("(%p,%s)\n", iface, debugstr_guid(guid));

    guidentry.guid = *guid;
    guidentry.hreftype = This->typelib->typelib_typeinfo_offsets[This->typeinfo->typekind >> 16];
    guidentry.next_hash = -1;

    offset = ctl2_alloc_guid(This->typelib, &guidentry);
    
    if (offset == -1) return E_OUTOFMEMORY;

    This->typeinfo->posguid = offset;

    if (IsEqualIID(guid, &IID_IDispatch)) {
	This->typelib->typelib_header.dispatchpos = This->typelib->typelib_typeinfo_offsets[This->typeinfo->typekind >> 16];
    }

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_SetTypeFlags {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetTypeFlags.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetTypeFlags(ICreateTypeInfo2 *iface, UINT uTypeFlags)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    TRACE("(%p,0x%x)\n", iface, uTypeFlags);

    This->typeinfo->flags = uTypeFlags;

    if (uTypeFlags & 0x1000) {
	MSFT_GuidEntry foo;
	int guidoffset;
	int fileoffset;
	MSFT_ImpInfo impinfo;
	static const WCHAR stdole2tlb[] = { 's','t','d','o','l','e','2','.','t','l','b',0 };

	foo.guid = IID_StdOle;
	foo.hreftype = 2;
	foo.next_hash = -1;
	guidoffset = ctl2_alloc_guid(This->typelib, &foo);
	if (guidoffset == -1) return E_OUTOFMEMORY;

	fileoffset =  ctl2_alloc_importfile(This->typelib, guidoffset, 2, 0, stdole2tlb);
	if (fileoffset == -1) return E_OUTOFMEMORY;

	foo.guid = IID_IDispatch;
	foo.hreftype = 1;
	foo.next_hash = -1;
	guidoffset = ctl2_alloc_guid(This->typelib, &foo);
	if (guidoffset == -1) return E_OUTOFMEMORY;

	impinfo.flags = TKIND_INTERFACE << 24 | MSFT_IMPINFO_OFFSET_IS_GUID;
	impinfo.oImpFile = fileoffset;
	impinfo.oGuid = guidoffset;
	ctl2_alloc_importinfo(This->typelib, &impinfo);

	This->typelib->typelib_header.dispatchpos = 1;

	This->typeinfo->typekind |= 0x10;
	This->typeinfo->typekind &= ~0x0f;
	This->typeinfo->typekind |= TKIND_DISPATCH;
    }

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_SetDocString {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetDocString.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetDocString(
        ICreateTypeInfo2* iface,
        LPOLESTR pStrDoc)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    int offset;

    TRACE("(%p,%s)\n", iface, debugstr_w(pStrDoc));

    offset = ctl2_alloc_string(This->typelib, pStrDoc);
    if (offset == -1) return E_OUTOFMEMORY;
    This->typeinfo->docstringoffs = offset;
    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_SetHelpContext {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetHelpContext.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetHelpContext(
        ICreateTypeInfo2* iface,
        DWORD dwHelpContext)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    TRACE("(%p,%d)\n", iface, dwHelpContext);

    This->typeinfo->helpcontext = dwHelpContext;

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_SetVersion {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetVersion.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetVersion(
        ICreateTypeInfo2* iface,
        WORD wMajorVerNum,
        WORD wMinorVerNum)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    TRACE("(%p,%d,%d)\n", iface, wMajorVerNum, wMinorVerNum);

    This->typeinfo->version = wMajorVerNum | (wMinorVerNum << 16);
    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_AddRefTypeInfo {OLEAUT32}
 *
 *  See ICreateTypeInfo_AddRefTypeInfo.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnAddRefTypeInfo(
        ICreateTypeInfo2* iface,
        ITypeInfo* pTInfo,
        HREFTYPE* phRefType)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    ITypeLib *container;
    UINT index;
    HRESULT res;

    TRACE("(%p,%p,%p)\n", iface, pTInfo, phRefType);

    /*
     * If this is one of ours, we set *phRefType to the TYPEINFO offset of
     * the referred TypeInfo. Otherwise, we presumably have more magic to do.
     *
     * Unfortunately, we can't rely on the passed-in TypeInfo even having the
     * same internal structure as one of ours. It could be from another
     * implementation of ITypeInfo. So we need to do the following...
     */
    res = ITypeInfo_GetContainingTypeLib(pTInfo, &container, &index);
    if (!SUCCEEDED(res)) {
	TRACE("failed to find containing typelib.\n");
	return res;
    }

    if (container == (ITypeLib *)&This->typelib->lpVtblTypeLib2) {
	*phRefType = This->typelib->typelib_typeinfo_offsets[index];
    } else {
	FIXME("(%p,%p,%p), pTInfo from different typelib.\n", iface, pTInfo, phRefType);
    }

    ITypeLib_Release(container);
    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_AddFuncDesc {OLEAUT32}
 *
 *  See ICreateTypeInfo_AddFuncDesc.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnAddFuncDesc(
        ICreateTypeInfo2* iface,
        UINT index,
        FUNCDESC* pFuncDesc)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    int offset;
    int *typedata;
    int i;
    int decoded_size;

    FIXME("(%p,%d,%p), stub!\n", iface, index, pFuncDesc);
    FIXME("{%d,%p,%p,%d,%d,%d,%d,%d,%d,%d,{%d},%d}\n", pFuncDesc->memid, pFuncDesc->lprgscode, pFuncDesc->lprgelemdescParam, pFuncDesc->funckind, pFuncDesc->invkind, pFuncDesc->callconv, pFuncDesc->cParams, pFuncDesc->cParamsOpt, pFuncDesc->oVft, pFuncDesc->cScodes, pFuncDesc->elemdescFunc.tdesc.vt, pFuncDesc->wFuncFlags);
/*     FIXME("{%d, %d}\n", pFuncDesc->lprgelemdescParam[0].tdesc.vt, pFuncDesc->lprgelemdescParam[1].tdesc.vt); */
/*     return E_OUTOFMEMORY; */
    
    if (!This->typedata) {
	This->typedata = HeapAlloc(GetProcessHeap(), 0, 0x2000);
	This->typedata[0] = 0;
    }

    /* allocate type data space for us */
    offset = This->typedata[0];
    This->typedata[0] += 0x18 + (pFuncDesc->cParams * 12);
    typedata = This->typedata + (offset >> 2) + 1;

    /* fill out the basic type information */
    typedata[0] = (0x18 + (pFuncDesc->cParams * 12)) | (index << 16);
    ctl2_encode_typedesc(This->typelib, &pFuncDesc->elemdescFunc.tdesc, &typedata[1], NULL, NULL, &decoded_size);
    typedata[2] = pFuncDesc->wFuncFlags;
    typedata[3] = ((sizeof(FUNCDESC) + decoded_size) << 16) | This->typeinfo->cbSizeVft;
    typedata[4] = (index << 16) | (pFuncDesc->callconv << 8) | 9;
    typedata[5] = pFuncDesc->cParams;

    /* NOTE: High word of typedata[3] is total size of FUNCDESC + size of all ELEMDESCs for params + TYPEDESCs for pointer params and return types. */
    /* That is, total memory allocation required to reconstitute the FUNCDESC in its entirety. */
    typedata[3] += (sizeof(ELEMDESC) * pFuncDesc->cParams) << 16;

    for (i = 0; i < pFuncDesc->cParams; i++) {
	ctl2_encode_typedesc(This->typelib, &pFuncDesc->lprgelemdescParam[i].tdesc, &typedata[6+(i*3)], NULL, NULL, &decoded_size);
	typedata[7+(i*3)] = -1;
	typedata[8+(i*3)] = pFuncDesc->lprgelemdescParam[i].u.paramdesc.wParamFlags;
	typedata[3] += decoded_size << 16;

#if 0
	/* FIXME: Doesn't work. Doesn't even come up with usable VTs for varDefaultValue. */
	if (pFuncDesc->lprgelemdescParam[i].u.paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT) {
	    ctl2_alloc_custdata(This->typelib, &pFuncDesc->lprgelemdescParam[i].u.paramdesc.pparamdescex->varDefaultValue);
	}
#endif
    }

    /* update the index data */
    This->indices[index] = ((0x6000 | This->typeinfo->cImplTypes) << 16) | index;
    This->names[index] = -1;
    This->offsets[index] = offset;

    /* ??? */
    if (!This->typeinfo->res2) This->typeinfo->res2 = 0x20;
    This->typeinfo->res2 <<= 1;

    /* ??? */
    if (This->typeinfo->res3 == -1) This->typeinfo->res3 = 0;
    This->typeinfo->res3 += 0x38;

    /* ??? */
    if (index < 2) This->typeinfo->res2 += pFuncDesc->cParams << 4;
    This->typeinfo->res3 += pFuncDesc->cParams << 4;

    /* adjust size of VTBL */
    This->typeinfo->cbSizeVft += 4;

    /* Increment the number of function elements */
    This->typeinfo->cElement += 1;

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_AddImplType {OLEAUT32}
 *
 *  See ICreateTypeInfo_AddImplType.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnAddImplType(
        ICreateTypeInfo2* iface,
        UINT index,
        HREFTYPE hRefType)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    TRACE("(%p,%d,%d)\n", iface, index, hRefType);

    if ((This->typeinfo->typekind & 15) == TKIND_COCLASS) {
	int offset;
	MSFT_RefRecord *ref;

	if (index == 0) {
	    if (This->typeinfo->datatype1 != -1) return TYPE_E_ELEMENTNOTFOUND;

	    offset = ctl2_alloc_segment(This->typelib, MSFT_SEG_REFERENCES, sizeof(MSFT_RefRecord), 0);
	    if (offset == -1) return E_OUTOFMEMORY;

	    This->typeinfo->datatype1 = offset;
	} else {
	    int lastoffset;

	    lastoffset = ctl2_find_nth_reference(This->typelib, This->typeinfo->datatype1, index - 1);
	    if (lastoffset == -1) return TYPE_E_ELEMENTNOTFOUND;

	    ref = (MSFT_RefRecord *)&This->typelib->typelib_segment_data[MSFT_SEG_REFERENCES][lastoffset];
	    if (ref->onext != -1) return TYPE_E_ELEMENTNOTFOUND;

	    offset = ctl2_alloc_segment(This->typelib, MSFT_SEG_REFERENCES, sizeof(MSFT_RefRecord), 0);
	    if (offset == -1) return E_OUTOFMEMORY;

	    ref->onext = offset;
	}

	ref = (MSFT_RefRecord *)&This->typelib->typelib_segment_data[MSFT_SEG_REFERENCES][offset];

	ref->reftype = hRefType;
	ref->flags = 0;
	ref->oCustData = -1;
	ref->onext = -1;
    } else if ((This->typeinfo->typekind & 15) == TKIND_DISPATCH) {
	FIXME("dispatch case unhandled.\n");
    } else if ((This->typeinfo->typekind & 15) == TKIND_INTERFACE) {
	if (This->typeinfo->cImplTypes) {
	    return (index == 1)? TYPE_E_BADMODULEKIND: TYPE_E_ELEMENTNOTFOUND;
	}

	if (index != 0)  return TYPE_E_ELEMENTNOTFOUND;

	This->typeinfo->cImplTypes++;

	/* hacked values for IDispatch only, and maybe only for stdole. */
	This->typeinfo->cbSizeVft += 0x0c; /* hack */
	This->typeinfo->datatype1 = hRefType;
	This->typeinfo->datatype2 = (3 << 16) | 1; /* ? */
    } else {
	FIXME("AddImplType unsupported on typekind %d\n", This->typeinfo->typekind & 15);
	return E_OUTOFMEMORY;
    }

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_SetImplTypeFlags {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetImplTypeFlags.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetImplTypeFlags(
        ICreateTypeInfo2* iface,
        UINT index,
        INT implTypeFlags)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;
    int offset;
    MSFT_RefRecord *ref;

    TRACE("(%p,%d,0x%x)\n", iface, index, implTypeFlags);

    if ((This->typeinfo->typekind & 15) != TKIND_COCLASS) {
	return TYPE_E_BADMODULEKIND;
    }

    offset = ctl2_find_nth_reference(This->typelib, This->typeinfo->datatype1, index);
    if (offset == -1) return TYPE_E_ELEMENTNOTFOUND;

    ref = (MSFT_RefRecord *)&This->typelib->typelib_segment_data[MSFT_SEG_REFERENCES][offset];
    ref->flags = implTypeFlags;

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_SetAlignment {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetAlignment.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetAlignment(
        ICreateTypeInfo2* iface,
        WORD cbAlignment)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    TRACE("(%p,%d)\n", iface, cbAlignment);

    if (!cbAlignment) return E_INVALIDARG;
    if (cbAlignment > 16) return E_INVALIDARG;

    This->typeinfo->typekind &= ~0xffc0;
    This->typeinfo->typekind |= cbAlignment << 6;

    /* FIXME: There's probably some way to simplify this. */
    switch (This->typeinfo->typekind & 15) {
    case TKIND_ALIAS:
    default:
	break;

    case TKIND_ENUM:
    case TKIND_INTERFACE:
    case TKIND_DISPATCH:
    case TKIND_COCLASS:
	if (cbAlignment > 4) cbAlignment = 4;
	break;

    case TKIND_RECORD:
    case TKIND_MODULE:
    case TKIND_UNION:
	cbAlignment = 1;
	break;
    }

    This->typeinfo->typekind |= cbAlignment << 11;

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_SetSchema {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetSchema.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetSchema(
        ICreateTypeInfo2* iface,
        LPOLESTR pStrSchema)
{
    FIXME("(%p,%s), stub!\n", iface, debugstr_w(pStrSchema));
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_AddVarDesc {OLEAUT32}
 *
 *  See ICreateTypeInfo_AddVarDesc.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnAddVarDesc(
        ICreateTypeInfo2* iface,
        UINT index,
        VARDESC* pVarDesc)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;
    int offset;
    INT *typedata;
    int var_datawidth;
    int var_alignment;
    int var_type_size;
    int alignment;

    TRACE("(%p,%d,%p), stub!\n", iface, index, pVarDesc);
    TRACE("%d, %p, %d, {{%x, %d}, {%p, %x}}, 0x%x, %d\n", pVarDesc->memid, pVarDesc->lpstrSchema, pVarDesc->u.oInst,
	  pVarDesc->elemdescVar.tdesc.u.hreftype, pVarDesc->elemdescVar.tdesc.vt,
	  pVarDesc->elemdescVar.u.paramdesc.pparamdescex, pVarDesc->elemdescVar.u.paramdesc.wParamFlags,
	  pVarDesc->wVarFlags, pVarDesc->varkind);

    if ((This->typeinfo->cElement >> 16) != index) {
	TRACE("Out-of-order element.\n");
	return TYPE_E_ELEMENTNOTFOUND;
    }

    if (!This->typedata) {
	This->typedata = HeapAlloc(GetProcessHeap(), 0, 0x2000);
	This->typedata[0] = 0;
    }

    /* allocate type data space for us */
    offset = This->typedata[0];
    This->typedata[0] += 0x14;
    typedata = This->typedata + (offset >> 2) + 1;

    /* fill out the basic type information */
    typedata[0] = 0x14 | (index << 16);
    typedata[2] = pVarDesc->wVarFlags;
    typedata[3] = (sizeof(VARDESC) << 16) | 0;

    /* update the index data */
    This->indices[index] = 0x40000000 + index;
    This->names[index] = -1;
    This->offsets[index] = offset;

    /* figure out type widths and whatnot */
    ctl2_encode_typedesc(This->typelib, &pVarDesc->elemdescVar.tdesc,
			 &typedata[1], &var_datawidth, &var_alignment,
			 &var_type_size);

    /* pad out starting position to data width */
    This->datawidth += var_alignment - 1;
    This->datawidth &= ~(var_alignment - 1);
    typedata[4] = This->datawidth;
    
    /* add the new variable to the total data width */
    This->datawidth += var_datawidth;

    /* add type description size to total required allocation */
    typedata[3] += var_type_size << 16;

    /* fix type alignment */
    alignment = (This->typeinfo->typekind >> 11) & 0x1f;
    if (alignment < var_alignment) {
	alignment = var_alignment;
	This->typeinfo->typekind &= ~0xf800;
	This->typeinfo->typekind |= alignment << 11;
    }

    /* ??? */
    if (!This->typeinfo->res2) This->typeinfo->res2 = 0x1a;
    if ((index == 0) || (index == 1) || (index == 2) || (index == 4) || (index == 9)) {
	This->typeinfo->res2 <<= 1;
    }

    /* ??? */
    if (This->typeinfo->res3 == -1) This->typeinfo->res3 = 0;
    This->typeinfo->res3 += 0x2c;

    /* increment the number of variable elements */
    This->typeinfo->cElement += 0x10000;

    /* pad data width to alignment */
    This->typeinfo->size = (This->datawidth + (alignment - 1)) & ~(alignment - 1);

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_SetFuncAndParamNames {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetFuncAndParamNames.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetFuncAndParamNames(
        ICreateTypeInfo2* iface,
        UINT index,
        LPOLESTR* rgszNames,
        UINT cNames)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    int i;
    int offset;
    char *namedata;

    FIXME("(%p,%d,%s,%d), stub!\n", iface, index, debugstr_w(*rgszNames), cNames);

    offset = ctl2_alloc_name(This->typelib, rgszNames[0]);
    This->names[index] = offset;

    namedata = This->typelib->typelib_segment_data[MSFT_SEG_NAME] + offset;
    namedata[9] &= ~0x10;
    if (*((INT *)namedata) == -1) {
	*((INT *)namedata) = This->typelib->typelib_typeinfo_offsets[This->typeinfo->typekind >> 16];
    }

    for (i = 1; i < cNames; i++) {
	/* FIXME: Almost certainly easy to break */
	int *paramdata = &This->typedata[This->offsets[index] >> 2];

	offset = ctl2_alloc_name(This->typelib, rgszNames[i]);
	paramdata[(i * 3) + 5] = offset;
    }

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_SetVarName {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetVarName.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetVarName(
        ICreateTypeInfo2* iface,
        UINT index,
        LPOLESTR szName)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;
    int offset;
    char *namedata;

    TRACE("(%p,%d,%s), stub!\n", iface, index, debugstr_w(szName));

    if ((This->typeinfo->cElement >> 16) <= index) {
	TRACE("Out-of-order element.\n");
	return TYPE_E_ELEMENTNOTFOUND;
    }

    offset = ctl2_alloc_name(This->typelib, szName);
    if (offset == -1) return E_OUTOFMEMORY;

    namedata = This->typelib->typelib_segment_data[MSFT_SEG_NAME] + offset;
    if (*((INT *)namedata) == -1) {
	*((INT *)namedata) = This->typelib->typelib_typeinfo_offsets[This->typeinfo->typekind >> 16];
	namedata[9] |= 0x10;
    }
    if ((This->typeinfo->typekind & 15) == TKIND_ENUM) {
	namedata[9] |= 0x20;
    }
    This->names[index] = offset;

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_SetTypeDescAlias {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetTypeDescAlias.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetTypeDescAlias(
        ICreateTypeInfo2* iface,
        TYPEDESC* pTDescAlias)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    int encoded_typedesc;
    int width;

    if ((This->typeinfo->typekind & 15) != TKIND_ALIAS) {
	return TYPE_E_WRONGTYPEKIND;
    }

    FIXME("(%p,%p), hack!\n", iface, pTDescAlias);

    if (ctl2_encode_typedesc(This->typelib, pTDescAlias, &encoded_typedesc, &width, NULL, NULL) == -1) {
	return E_OUTOFMEMORY;
    }

    This->typeinfo->size = width;
    This->typeinfo->datatype1 = encoded_typedesc;

    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_DefineFuncAsDllEntry {OLEAUT32}
 *
 *  See ICreateTypeInfo_DefineFuncAsDllEntry.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnDefineFuncAsDllEntry(
        ICreateTypeInfo2* iface,
        UINT index,
        LPOLESTR szDllName,
        LPOLESTR szProcName)
{
    FIXME("(%p,%d,%s,%s), stub!\n", iface, index, debugstr_w(szDllName), debugstr_w(szProcName));
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetFuncDocString {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetFuncDocString.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetFuncDocString(
        ICreateTypeInfo2* iface,
        UINT index,
        LPOLESTR szDocString)
{
    FIXME("(%p,%d,%s), stub!\n", iface, index, debugstr_w(szDocString));
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetVarDocString {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetVarDocString.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetVarDocString(
        ICreateTypeInfo2* iface,
        UINT index,
        LPOLESTR szDocString)
{
    ICreateTypeInfo2Impl *This = (ICreateTypeInfo2Impl *)iface;

    FIXME("(%p,%d,%s), stub!\n", iface, index, debugstr_w(szDocString));

    ctl2_alloc_string(This->typelib, szDocString);

    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetFuncHelpContext {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetFuncHelpContext.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetFuncHelpContext(
        ICreateTypeInfo2* iface,
        UINT index,
        DWORD dwHelpContext)
{
    FIXME("(%p,%d,%d), stub!\n", iface, index, dwHelpContext);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetVarHelpContext {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetVarHelpContext.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetVarHelpContext(
        ICreateTypeInfo2* iface,
        UINT index,
        DWORD dwHelpContext)
{
    FIXME("(%p,%d,%d), stub!\n", iface, index, dwHelpContext);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetMops {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetMops.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetMops(
        ICreateTypeInfo2* iface,
        UINT index,
        BSTR bstrMops)
{
    FIXME("(%p,%d,%p), stub!\n", iface, index, bstrMops);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetTypeIdldesc {OLEAUT32}
 *
 *  See ICreateTypeInfo_SetTypeIdldesc.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetTypeIdldesc(
        ICreateTypeInfo2* iface,
        IDLDESC* pIdlDesc)
{
    FIXME("(%p,%p), stub!\n", iface, pIdlDesc);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_LayOut {OLEAUT32}
 *
 *  See ICreateTypeInfo_LayOut.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnLayOut(
	ICreateTypeInfo2* iface)
{
    TRACE("(%p), stub!\n", iface);
/*     return E_OUTOFMEMORY; */
    return S_OK;
}

/******************************************************************************
 * ICreateTypeInfo2_DeleteFuncDesc {OLEAUT32}
 *
 *  Delete a function description from a type.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnDeleteFuncDesc(
        ICreateTypeInfo2* iface, /* [I] The typeinfo from which to delete a function. */
        UINT index)              /* [I] The index of the function to delete. */
{
    FIXME("(%p,%d), stub!\n", iface, index);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_DeleteFuncDescByMemId {OLEAUT32}
 *
 *  Delete a function description from a type.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnDeleteFuncDescByMemId(
        ICreateTypeInfo2* iface, /* [I] The typeinfo from which to delete a function. */
        MEMBERID memid,          /* [I] The member id of the function to delete. */
        INVOKEKIND invKind)      /* [I] The invocation type of the function to delete. (?) */
{
    FIXME("(%p,%d,%d), stub!\n", iface, memid, invKind);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_DeleteVarDesc {OLEAUT32}
 *
 *  Delete a variable description from a type.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY, E_INVALIDARG, TYPE_E_IOERROR,
 *  TYPE_E_INVDATAREAD, TYPE_E_UNSUPFORMAT or TYPE_E_INVALIDSTATE.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnDeleteVarDesc(
        ICreateTypeInfo2* iface, /* [I] The typeinfo from which to delete the variable description. */
        UINT index)              /* [I] The index of the variable description to delete. */
{
    FIXME("(%p,%d), stub!\n", iface, index);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_DeleteVarDescByMemId {OLEAUT32}
 *
 *  Delete a variable description from a type.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY, E_INVALIDARG, TYPE_E_IOERROR,
 *  TYPE_E_INVDATAREAD, TYPE_E_UNSUPFORMAT or TYPE_E_INVALIDSTATE.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnDeleteVarDescByMemId(
        ICreateTypeInfo2* iface, /* [I] The typeinfo from which to delete the variable description. */
        MEMBERID memid)          /* [I] The member id of the variable description to delete. */
{
    FIXME("(%p,%d), stub!\n", iface, memid);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_DeleteImplType {OLEAUT32}
 *
 *  Delete an interface implementation from a type. (?)
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnDeleteImplType(
        ICreateTypeInfo2* iface, /* [I] The typeinfo from which to delete. */
        UINT index)              /* [I] The index of the interface to delete. */
{
    FIXME("(%p,%d), stub!\n", iface, index);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetCustData {OLEAUT32}
 *
 *  Set the custom data for a type.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetCustData(
        ICreateTypeInfo2* iface, /* [I] The typeinfo in which to set the custom data. */
        REFGUID guid,            /* [I] The GUID used as a key to retrieve the custom data. */
        VARIANT* pVarVal)        /* [I] The custom data. */
{
    FIXME("(%p,%s,%p), stub!\n", iface, debugstr_guid(guid), pVarVal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetFuncCustData {OLEAUT32}
 *
 *  Set the custom data for a function.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetFuncCustData(
        ICreateTypeInfo2* iface, /* [I] The typeinfo in which to set the custom data. */
        UINT index,              /* [I] The index of the function for which to set the custom data. */
        REFGUID guid,            /* [I] The GUID used as a key to retrieve the custom data. */
        VARIANT* pVarVal)        /* [I] The custom data. */
{
    FIXME("(%p,%d,%s,%p), stub!\n", iface, index, debugstr_guid(guid), pVarVal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetParamCustData {OLEAUT32}
 *
 *  Set the custom data for a function parameter.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetParamCustData(
        ICreateTypeInfo2* iface, /* [I] The typeinfo in which to set the custom data. */
        UINT indexFunc,          /* [I] The index of the function on which the parameter resides. */
        UINT indexParam,         /* [I] The index of the parameter on which to set the custom data. */
        REFGUID guid,            /* [I] The GUID used as a key to retrieve the custom data. */
        VARIANT* pVarVal)        /* [I] The custom data. */
{
    FIXME("(%p,%d,%d,%s,%p), stub!\n", iface, indexFunc, indexParam, debugstr_guid(guid), pVarVal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetVarCustData {OLEAUT32}
 *
 *  Set the custom data for a variable.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetVarCustData(
        ICreateTypeInfo2* iface, /* [I] The typeinfo in which to set the custom data. */
        UINT index,              /* [I] The index of the variable on which to set the custom data. */
        REFGUID guid,            /* [I] The GUID used as a key to retrieve the custom data. */
        VARIANT* pVarVal)        /* [I] The custom data. */
{
    FIXME("(%p,%d,%s,%p), stub!\n", iface, index, debugstr_guid(guid), pVarVal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetImplTypeCustData {OLEAUT32}
 *
 *  Set the custom data for an implemented interface.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetImplTypeCustData(
        ICreateTypeInfo2* iface, /* [I] The typeinfo on which to set the custom data. */
        UINT index,              /* [I] The index of the implemented interface on which to set the custom data. */
        REFGUID guid,            /* [I] The GUID used as a key to retrieve the custom data. */
        VARIANT* pVarVal)        /* [I] The custom data. */
{
    FIXME("(%p,%d,%s,%p), stub!\n", iface, index, debugstr_guid(guid), pVarVal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetHelpStringContext {OLEAUT32}
 *
 *  Set the help string context for the typeinfo.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetHelpStringContext(
        ICreateTypeInfo2* iface,   /* [I] The typeinfo on which to set the help string context. */
        ULONG dwHelpStringContext) /* [I] The help string context. */
{
    FIXME("(%p,%d), stub!\n", iface, dwHelpStringContext);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetFuncHelpStringContext {OLEAUT32}
 *
 *  Set the help string context for a function.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetFuncHelpStringContext(
        ICreateTypeInfo2* iface,   /* [I] The typeinfo on which to set the help string context. */
        UINT index,                /* [I] The index for the function on which to set the help string context. */
        ULONG dwHelpStringContext) /* [I] The help string context. */
{
    FIXME("(%p,%d,%d), stub!\n", iface, index, dwHelpStringContext);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetVarHelpStringContext {OLEAUT32}
 *
 *  Set the help string context for a variable.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetVarHelpStringContext(
        ICreateTypeInfo2* iface,   /* [I] The typeinfo on which to set the help string context. */
        UINT index,                /* [I] The index of the variable on which to set the help string context. */
        ULONG dwHelpStringContext) /* [I] The help string context */
{
    FIXME("(%p,%d,%d), stub!\n", iface, index, dwHelpStringContext);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_Invalidate {OLEAUT32}
 *
 *  Undocumented function. (!)
 */
static HRESULT WINAPI ICreateTypeInfo2_fnInvalidate(
        ICreateTypeInfo2* iface)
{
    FIXME("(%p), stub!\n", iface);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeInfo2_SetName {OLEAUT32}
 *
 *  Set the name for a typeinfo.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of STG_E_INSUFFICIENTMEMORY, E_OUTOFMEMORY, E_INVALIDARG or TYPE_E_INVALIDSTATE.
 */
static HRESULT WINAPI ICreateTypeInfo2_fnSetName(
        ICreateTypeInfo2* iface,
        LPOLESTR szName)
{
    FIXME("(%p,%s), stub!\n", iface, debugstr_w(szName));
    return E_OUTOFMEMORY;
}

/*================== ITypeInfo2 Implementation ===================================*/

/******************************************************************************
 * ITypeInfo2_QueryInterface {OLEAUT32}
 *
 *  See IUnknown_QueryInterface.
 */
static HRESULT WINAPI ITypeInfo2_fnQueryInterface(ITypeInfo2 * iface, REFIID riid, LPVOID * ppv)
{
    ICreateTypeInfo2Impl *This = impl_from_ITypeInfo2(iface);

    return ICreateTypeInfo2_QueryInterface((ICreateTypeInfo2 *)This, riid, ppv);
}

/******************************************************************************
 * ITypeInfo2_AddRef {OLEAUT32}
 *
 *  See IUnknown_AddRef.
 */
static ULONG WINAPI ITypeInfo2_fnAddRef(ITypeInfo2 * iface)
{
    ICreateTypeInfo2Impl *This = impl_from_ITypeInfo2(iface);

    return ICreateTypeInfo2_AddRef((ICreateTypeInfo2 *)This);
}

/******************************************************************************
 * ITypeInfo2_Release {OLEAUT32}
 *
 *  See IUnknown_Release.
 */
static ULONG WINAPI ITypeInfo2_fnRelease(ITypeInfo2 * iface)
{
    ICreateTypeInfo2Impl *This = impl_from_ITypeInfo2(iface);

    return ICreateTypeInfo2_Release((ICreateTypeInfo2 *)This);
}

/******************************************************************************
 * ITypeInfo2_GetTypeAttr {OLEAUT32}
 *
 *  See ITypeInfo_GetTypeAttr.
 */
static HRESULT WINAPI ITypeInfo2_fnGetTypeAttr(
        ITypeInfo2* iface,
        TYPEATTR** ppTypeAttr)
{
    FIXME("(%p,%p), stub!\n", iface, ppTypeAttr);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetTypeComp {OLEAUT32}
 *
 *  See ITypeInfo_GetTypeComp.
 */
static HRESULT WINAPI ITypeInfo2_fnGetTypeComp(
        ITypeInfo2* iface,
        ITypeComp** ppTComp)
{
    FIXME("(%p,%p), stub!\n", iface, ppTComp);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetFuncDesc {OLEAUT32}
 *
 *  See ITypeInfo_GetFuncDesc.
 */
static HRESULT WINAPI ITypeInfo2_fnGetFuncDesc(
        ITypeInfo2* iface,
        UINT index,
        FUNCDESC** ppFuncDesc)
{
    FIXME("(%p,%d,%p), stub!\n", iface, index, ppFuncDesc);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetVarDesc {OLEAUT32}
 *
 *  See ITypeInfo_GetVarDesc.
 */
static HRESULT WINAPI ITypeInfo2_fnGetVarDesc(
        ITypeInfo2* iface,
        UINT index,
        VARDESC** ppVarDesc)
{
    FIXME("(%p,%d,%p), stub!\n", iface, index, ppVarDesc);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetNames {OLEAUT32}
 *
 *  See ITypeInfo_GetNames.
 */
static HRESULT WINAPI ITypeInfo2_fnGetNames(
        ITypeInfo2* iface,
        MEMBERID memid,
        BSTR* rgBstrNames,
        UINT cMaxNames,
        UINT* pcNames)
{
    FIXME("(%p,%d,%p,%d,%p), stub!\n", iface, memid, rgBstrNames, cMaxNames, pcNames);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetRefTypeOfImplType {OLEAUT32}
 *
 *  See ITypeInfo_GetRefTypeOfImplType.
 */
static HRESULT WINAPI ITypeInfo2_fnGetRefTypeOfImplType(
        ITypeInfo2* iface,
        UINT index,
        HREFTYPE* pRefType)
{
    FIXME("(%p,%d,%p), stub!\n", iface, index, pRefType);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetImplTypeFlags {OLEAUT32}
 *
 *  See ITypeInfo_GetImplTypeFlags.
 */
static HRESULT WINAPI ITypeInfo2_fnGetImplTypeFlags(
        ITypeInfo2* iface,
        UINT index,
        INT* pImplTypeFlags)
{
    FIXME("(%p,%d,%p), stub!\n", iface, index, pImplTypeFlags);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetIDsOfNames {OLEAUT32}
 *
 *  See ITypeInfo_GetIDsOfNames.
 */
static HRESULT WINAPI ITypeInfo2_fnGetIDsOfNames(
        ITypeInfo2* iface,
        LPOLESTR* rgszNames,
        UINT cNames,
        MEMBERID* pMemId)
{
    FIXME("(%p,%p,%d,%p), stub!\n", iface, rgszNames, cNames, pMemId);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_Invoke {OLEAUT32}
 *
 *  See ITypeInfo_Invoke.
 */
static HRESULT WINAPI ITypeInfo2_fnInvoke(
        ITypeInfo2* iface,
        PVOID pvInstance,
        MEMBERID memid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    FIXME("(%p,%p,%d,%x,%p,%p,%p,%p), stub!\n", iface, pvInstance, memid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetDocumentation {OLEAUT32}
 *
 *  See ITypeInfo_GetDocumentation.
 */
static HRESULT WINAPI ITypeInfo2_fnGetDocumentation(
        ITypeInfo2* iface,
        MEMBERID memid,
        BSTR* pBstrName,
        BSTR* pBstrDocString,
        DWORD* pdwHelpContext,
        BSTR* pBstrHelpFile)
{
    FIXME("(%p,%d,%p,%p,%p,%p), stub!\n", iface, memid, pBstrName, pBstrDocString, pdwHelpContext, pBstrHelpFile);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetDllEntry {OLEAUT32}
 *
 *  See ITypeInfo_GetDllEntry.
 */
static HRESULT WINAPI ITypeInfo2_fnGetDllEntry(
        ITypeInfo2* iface,
        MEMBERID memid,
        INVOKEKIND invKind,
        BSTR* pBstrDllName,
        BSTR* pBstrName,
        WORD* pwOrdinal)
{
    FIXME("(%p,%d,%d,%p,%p,%p), stub!\n", iface, memid, invKind, pBstrDllName, pBstrName, pwOrdinal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetRefTypeInfo {OLEAUT32}
 *
 *  See ITypeInfo_GetRefTypeInfo.
 */
static HRESULT WINAPI ITypeInfo2_fnGetRefTypeInfo(
        ITypeInfo2* iface,
        HREFTYPE hRefType,
        ITypeInfo** ppTInfo)
{
    FIXME("(%p,%d,%p), stub!\n", iface, hRefType, ppTInfo);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_AddressOfMember {OLEAUT32}
 *
 *  See ITypeInfo_AddressOfMember.
 */
static HRESULT WINAPI ITypeInfo2_fnAddressOfMember(
        ITypeInfo2* iface,
        MEMBERID memid,
        INVOKEKIND invKind,
        PVOID* ppv)
{
    FIXME("(%p,%d,%d,%p), stub!\n", iface, memid, invKind, ppv);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_CreateInstance {OLEAUT32}
 *
 *  See ITypeInfo_CreateInstance.
 */
static HRESULT WINAPI ITypeInfo2_fnCreateInstance(
        ITypeInfo2* iface,
        IUnknown* pUnkOuter,
        REFIID riid,
        PVOID* ppvObj)
{
    FIXME("(%p,%p,%s,%p), stub!\n", iface, pUnkOuter, debugstr_guid(riid), ppvObj);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetMops {OLEAUT32}
 *
 *  See ITypeInfo_GetMops.
 */
static HRESULT WINAPI ITypeInfo2_fnGetMops(
        ITypeInfo2* iface,
        MEMBERID memid,
        BSTR* pBstrMops)
{
    FIXME("(%p,%d,%p), stub!\n", iface, memid, pBstrMops);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetContainingTypeLib {OLEAUT32}
 *
 *  See ITypeInfo_GetContainingTypeLib.
 */
static HRESULT WINAPI ITypeInfo2_fnGetContainingTypeLib(
        ITypeInfo2* iface,
        ITypeLib** ppTLib,
        UINT* pIndex)
{
    ICreateTypeInfo2Impl *This = impl_from_ITypeInfo2(iface);

    TRACE("(%p,%p,%p)\n", iface, ppTLib, pIndex);
    
    *ppTLib = (ITypeLib *)&This->typelib->lpVtblTypeLib2;
    This->typelib->ref++;
    *pIndex = This->typeinfo->typekind >> 16;

    return S_OK;
}

/******************************************************************************
 * ITypeInfo2_ReleaseTypeAttr {OLEAUT32}
 *
 *  See ITypeInfo_ReleaseTypeAttr.
 */
static void WINAPI ITypeInfo2_fnReleaseTypeAttr(
        ITypeInfo2* iface,
        TYPEATTR* pTypeAttr)
{
    FIXME("(%p,%p), stub!\n", iface, pTypeAttr);
}

/******************************************************************************
 * ITypeInfo2_ReleaseFuncDesc {OLEAUT32}
 *
 *  See ITypeInfo_ReleaseFuncDesc.
 */
static void WINAPI ITypeInfo2_fnReleaseFuncDesc(
        ITypeInfo2* iface,
        FUNCDESC* pFuncDesc)
{
    FIXME("(%p,%p), stub!\n", iface, pFuncDesc);
}

/******************************************************************************
 * ITypeInfo2_ReleaseVarDesc {OLEAUT32}
 *
 *  See ITypeInfo_ReleaseVarDesc.
 */
static void WINAPI ITypeInfo2_fnReleaseVarDesc(
        ITypeInfo2* iface,
        VARDESC* pVarDesc)
{
    FIXME("(%p,%p), stub!\n", iface, pVarDesc);
}

/******************************************************************************
 * ITypeInfo2_GetTypeKind {OLEAUT32}
 *
 *  Get the TYPEKIND value for a TypeInfo.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetTypeKind(
        ITypeInfo2* iface,   /* [I] The TypeInfo to obtain the typekind for. */
        TYPEKIND* pTypeKind) /* [O] The typekind for this TypeInfo. */
{
    FIXME("(%p,%p), stub!\n", iface, pTypeKind);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetTypeFlags {OLEAUT32}
 *
 *  Get the Type Flags for a TypeInfo.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetTypeFlags(
        ITypeInfo2* iface, /* [I] The TypeInfo to obtain the typeflags for. */
        ULONG* pTypeFlags) /* [O] The type flags for this TypeInfo. */
{
    FIXME("(%p,%p), stub!\n", iface, pTypeFlags);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetFuncIndexOfMemId {OLEAUT32}
 *
 *  Gets the index of a function given its member id.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetFuncIndexOfMemId(
        ITypeInfo2* iface,  /* [I] The TypeInfo in which to find the function. */
        MEMBERID memid,     /* [I] The member id for the function. */
        INVOKEKIND invKind, /* [I] The invocation kind for the function. */
        UINT* pFuncIndex)   /* [O] The index of the function. */
{
    FIXME("(%p,%d,%d,%p), stub!\n", iface, memid, invKind, pFuncIndex);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetVarIndexOfMemId {OLEAUT32}
 *
 *  Gets the index of a variable given its member id.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetVarIndexOfMemId(
        ITypeInfo2* iface, /* [I] The TypeInfo in which to find the variable. */
        MEMBERID memid,    /* [I] The member id for the variable. */
        UINT* pVarIndex)   /* [O] The index of the variable. */
{
    FIXME("(%p,%d,%p), stub!\n", iface, memid, pVarIndex);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetCustData {OLEAUT32}
 *
 *  Gets a custom data element from a TypeInfo.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetCustData(
        ITypeInfo2* iface, /* [I] The TypeInfo in which to find the custom data. */
        REFGUID guid,      /* [I] The GUID under which the custom data is stored. */
        VARIANT* pVarVal)  /* [O] The custom data. */
{
    FIXME("(%p,%s,%p), stub!\n", iface, debugstr_guid(guid), pVarVal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetFuncCustData {OLEAUT32}
 *
 *  Gets a custom data element from a function.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetFuncCustData(
        ITypeInfo2* iface, /* [I] The TypeInfo in which to find the custom data. */
        UINT index,        /* [I] The index of the function for which to retrieve the custom data. */
        REFGUID guid,      /* [I] The GUID under which the custom data is stored. */
        VARIANT* pVarVal)  /* [O] The custom data. */
{
    FIXME("(%p,%d,%s,%p), stub!\n", iface, index, debugstr_guid(guid), pVarVal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetParamCustData {OLEAUT32}
 *
 *  Gets a custom data element from a parameter.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetParamCustData(
        ITypeInfo2* iface, /* [I] The TypeInfo in which to find the custom data. */
        UINT indexFunc,    /* [I] The index of the function for which to retrieve the custom data. */
        UINT indexParam,   /* [I] The index of the parameter for which to retrieve the custom data. */
        REFGUID guid,      /* [I] The GUID under which the custom data is stored. */
        VARIANT* pVarVal)  /* [O] The custom data. */
{
    FIXME("(%p,%d,%d,%s,%p), stub!\n", iface, indexFunc, indexParam, debugstr_guid(guid), pVarVal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetVarCustData {OLEAUT32}
 *
 *  Gets a custom data element from a variable.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetVarCustData(
        ITypeInfo2* iface, /* [I] The TypeInfo in which to find the custom data. */
        UINT index,        /* [I] The index of the variable for which to retrieve the custom data. */
        REFGUID guid,      /* [I] The GUID under which the custom data is stored. */
        VARIANT* pVarVal)  /* [O] The custom data. */
{
    FIXME("(%p,%d,%s,%p), stub!\n", iface, index, debugstr_guid(guid), pVarVal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetImplTypeCustData {OLEAUT32}
 *
 *  Gets a custom data element from an implemented type of a TypeInfo.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetImplTypeCustData(
        ITypeInfo2* iface, /* [I] The TypeInfo in which to find the custom data. */
        UINT index,        /* [I] The index of the implemented type for which to retrieve the custom data. */
        REFGUID guid,      /* [I] The GUID under which the custom data is stored. */
        VARIANT* pVarVal)  /* [O] The custom data. */
{
    FIXME("(%p,%d,%s,%p), stub!\n", iface, index, debugstr_guid(guid), pVarVal);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetDocumentation2 {OLEAUT32}
 *
 *  Gets some documentation from a TypeInfo in a locale-aware fashion.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of STG_E_INSUFFICIENTMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetDocumentation2(
        ITypeInfo2* iface,           /* [I] The TypeInfo to retrieve the documentation from. */
        MEMBERID memid,              /* [I] The member id (why?). */
        LCID lcid,                   /* [I] The locale (why?). */
        BSTR* pbstrHelpString,       /* [O] The help string. */
        DWORD* pdwHelpStringContext, /* [O] The help string context. */
        BSTR* pbstrHelpStringDll)    /* [O] The help file name. */
{
    FIXME("(%p,%d,%d,%p,%p,%p), stub!\n", iface, memid, lcid, pbstrHelpString, pdwHelpStringContext, pbstrHelpStringDll);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetAllCustData {OLEAUT32}
 *
 *  Gets all of the custom data associated with a TypeInfo.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllCustData(
        ITypeInfo2* iface,   /* [I] The TypeInfo in which to find the custom data. */
        CUSTDATA* pCustData) /* [O] A pointer to the custom data. */
{
    FIXME("(%p,%p), stub!\n", iface, pCustData);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetAllFuncCustData {OLEAUT32}
 *
 *  Gets all of the custom data associated with a function.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllFuncCustData(
        ITypeInfo2* iface,   /* [I] The TypeInfo in which to find the custom data. */
        UINT index,          /* [I] The index of the function for which to retrieve the custom data. */
        CUSTDATA* pCustData) /* [O] A pointer to the custom data. */
{
    FIXME("(%p,%d,%p), stub!\n", iface, index, pCustData);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetAllParamCustData {OLEAUT32}
 *
 *  Gets all of the custom data associated with a parameter.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllParamCustData(
        ITypeInfo2* iface,   /* [I] The TypeInfo in which to find the custom data. */
        UINT indexFunc,      /* [I] The index of the function for which to retrieve the custom data. */
        UINT indexParam,     /* [I] The index of the parameter for which to retrieve the custom data. */
        CUSTDATA* pCustData) /* [O] A pointer to the custom data. */
{
    FIXME("(%p,%d,%d,%p), stub!\n", iface, indexFunc, indexParam, pCustData);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetAllVarCustData {OLEAUT32}
 *
 *  Gets all of the custom data associated with a variable.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllVarCustData(
        ITypeInfo2* iface,   /* [I] The TypeInfo in which to find the custom data. */
        UINT index,          /* [I] The index of the variable for which to retrieve the custom data. */
        CUSTDATA* pCustData) /* [O] A pointer to the custom data. */
{
    FIXME("(%p,%d,%p), stub!\n", iface, index, pCustData);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeInfo2_GetAllImplTypeCustData {OLEAUT32}
 *
 *  Gets all of the custom data associated with an implemented type.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: One of E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllImplTypeCustData(
        ITypeInfo2* iface,   /* [I] The TypeInfo in which to find the custom data. */
        UINT index,          /* [I] The index of the implemented type for which to retrieve the custom data. */
        CUSTDATA* pCustData) /* [O] A pointer to the custom data. */
{
    FIXME("(%p,%d,%p), stub!\n", iface, index, pCustData);
    return E_OUTOFMEMORY;
}


/*================== ICreateTypeInfo2 & ITypeInfo2 VTABLEs And Creation ===================================*/

static const ICreateTypeInfo2Vtbl ctypeinfo2vt =
{

    ICreateTypeInfo2_fnQueryInterface,
    ICreateTypeInfo2_fnAddRef,
    ICreateTypeInfo2_fnRelease,

    ICreateTypeInfo2_fnSetGuid,
    ICreateTypeInfo2_fnSetTypeFlags,
    ICreateTypeInfo2_fnSetDocString,
    ICreateTypeInfo2_fnSetHelpContext,
    ICreateTypeInfo2_fnSetVersion,
    ICreateTypeInfo2_fnAddRefTypeInfo,
    ICreateTypeInfo2_fnAddFuncDesc,
    ICreateTypeInfo2_fnAddImplType,
    ICreateTypeInfo2_fnSetImplTypeFlags,
    ICreateTypeInfo2_fnSetAlignment,
    ICreateTypeInfo2_fnSetSchema,
    ICreateTypeInfo2_fnAddVarDesc,
    ICreateTypeInfo2_fnSetFuncAndParamNames,
    ICreateTypeInfo2_fnSetVarName,
    ICreateTypeInfo2_fnSetTypeDescAlias,
    ICreateTypeInfo2_fnDefineFuncAsDllEntry,
    ICreateTypeInfo2_fnSetFuncDocString,
    ICreateTypeInfo2_fnSetVarDocString,
    ICreateTypeInfo2_fnSetFuncHelpContext,
    ICreateTypeInfo2_fnSetVarHelpContext,
    ICreateTypeInfo2_fnSetMops,
    ICreateTypeInfo2_fnSetTypeIdldesc,
    ICreateTypeInfo2_fnLayOut,

    ICreateTypeInfo2_fnDeleteFuncDesc,
    ICreateTypeInfo2_fnDeleteFuncDescByMemId,
    ICreateTypeInfo2_fnDeleteVarDesc,
    ICreateTypeInfo2_fnDeleteVarDescByMemId,
    ICreateTypeInfo2_fnDeleteImplType,
    ICreateTypeInfo2_fnSetCustData,
    ICreateTypeInfo2_fnSetFuncCustData,
    ICreateTypeInfo2_fnSetParamCustData,
    ICreateTypeInfo2_fnSetVarCustData,
    ICreateTypeInfo2_fnSetImplTypeCustData,
    ICreateTypeInfo2_fnSetHelpStringContext,
    ICreateTypeInfo2_fnSetFuncHelpStringContext,
    ICreateTypeInfo2_fnSetVarHelpStringContext,
    ICreateTypeInfo2_fnInvalidate,
    ICreateTypeInfo2_fnSetName
};

static const ITypeInfo2Vtbl typeinfo2vt =
{

    ITypeInfo2_fnQueryInterface,
    ITypeInfo2_fnAddRef,
    ITypeInfo2_fnRelease,

    ITypeInfo2_fnGetTypeAttr,
    ITypeInfo2_fnGetTypeComp,
    ITypeInfo2_fnGetFuncDesc,
    ITypeInfo2_fnGetVarDesc,
    ITypeInfo2_fnGetNames,
    ITypeInfo2_fnGetRefTypeOfImplType,
    ITypeInfo2_fnGetImplTypeFlags,
    ITypeInfo2_fnGetIDsOfNames,
    ITypeInfo2_fnInvoke,
    ITypeInfo2_fnGetDocumentation,
    ITypeInfo2_fnGetDllEntry,
    ITypeInfo2_fnGetRefTypeInfo,
    ITypeInfo2_fnAddressOfMember,
    ITypeInfo2_fnCreateInstance,
    ITypeInfo2_fnGetMops,
    ITypeInfo2_fnGetContainingTypeLib,
    ITypeInfo2_fnReleaseTypeAttr,
    ITypeInfo2_fnReleaseFuncDesc,
    ITypeInfo2_fnReleaseVarDesc,

    ITypeInfo2_fnGetTypeKind,
    ITypeInfo2_fnGetTypeFlags,
    ITypeInfo2_fnGetFuncIndexOfMemId,
    ITypeInfo2_fnGetVarIndexOfMemId,
    ITypeInfo2_fnGetCustData,
    ITypeInfo2_fnGetFuncCustData,
    ITypeInfo2_fnGetParamCustData,
    ITypeInfo2_fnGetVarCustData,
    ITypeInfo2_fnGetImplTypeCustData,
    ITypeInfo2_fnGetDocumentation2,
    ITypeInfo2_fnGetAllCustData,
    ITypeInfo2_fnGetAllFuncCustData,
    ITypeInfo2_fnGetAllParamCustData,
    ITypeInfo2_fnGetAllVarCustData,
    ITypeInfo2_fnGetAllImplTypeCustData,
};

static ICreateTypeInfo2 *ICreateTypeInfo2_Constructor(ICreateTypeLib2Impl *typelib, WCHAR *szName, TYPEKIND tkind)
{
    ICreateTypeInfo2Impl *pCreateTypeInfo2Impl;

    int nameoffset;
    int typeinfo_offset;
    MSFT_TypeInfoBase *typeinfo;

    TRACE("Constructing ICreateTypeInfo2 for %s with tkind %d\n", debugstr_w(szName), tkind);

    pCreateTypeInfo2Impl = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ICreateTypeInfo2Impl));
    if (!pCreateTypeInfo2Impl) return NULL;

    pCreateTypeInfo2Impl->lpVtbl = &ctypeinfo2vt;
    pCreateTypeInfo2Impl->lpVtblTypeInfo2 = &typeinfo2vt;
    pCreateTypeInfo2Impl->ref = 1;

    pCreateTypeInfo2Impl->typelib = typelib;
    typelib->ref++;

    nameoffset = ctl2_alloc_name(typelib, szName);
    typeinfo_offset = ctl2_alloc_typeinfo(typelib, nameoffset);
    typeinfo = (MSFT_TypeInfoBase *)&typelib->typelib_segment_data[MSFT_SEG_TYPEINFO][typeinfo_offset];

    typelib->typelib_segment_data[MSFT_SEG_NAME][nameoffset + 9] = 0x38;
    *((int *)&typelib->typelib_segment_data[MSFT_SEG_NAME][nameoffset]) = typeinfo_offset;

    pCreateTypeInfo2Impl->typeinfo = typeinfo;

    typeinfo->typekind |= tkind | 0x20;
    ICreateTypeInfo2_SetAlignment((ICreateTypeInfo2 *)pCreateTypeInfo2Impl, 4);

    switch (tkind) {
    case TKIND_ENUM:
    case TKIND_INTERFACE:
    case TKIND_DISPATCH:
    case TKIND_COCLASS:
	typeinfo->size = 4;
	break;

    case TKIND_RECORD:
    case TKIND_UNION:
	typeinfo->size = 0;
	break;

    case TKIND_MODULE:
	typeinfo->size = 2;
	break;

    case TKIND_ALIAS:
	typeinfo->size = -0x75;
	break;

    default:
	FIXME("(%s,%d), unrecognized typekind %d\n", debugstr_w(szName), tkind, tkind);
	typeinfo->size = 0xdeadbeef;
	break;
    }

    if (typelib->last_typeinfo) typelib->last_typeinfo->next_typeinfo = pCreateTypeInfo2Impl;
    typelib->last_typeinfo = pCreateTypeInfo2Impl;
    if (!typelib->typeinfos) typelib->typeinfos = pCreateTypeInfo2Impl;

    TRACE(" -- %p\n", pCreateTypeInfo2Impl);

    return (ICreateTypeInfo2 *)pCreateTypeInfo2Impl;
}


/*================== ICreateTypeLib2 Implementation ===================================*/

/******************************************************************************
 * ICreateTypeLib2_QueryInterface {OLEAUT32}
 *
 *  See IUnknown_QueryInterface.
 */
static HRESULT WINAPI ICreateTypeLib2_fnQueryInterface(
	ICreateTypeLib2 * iface,
	REFIID riid,
	VOID **ppvObject)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    TRACE("(%p)->(IID: %s)\n",This,debugstr_guid(riid));

    *ppvObject=NULL;
    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid,&IID_ICreateTypeLib)||
       IsEqualIID(riid,&IID_ICreateTypeLib2))
    {
        *ppvObject = This;
    } else if (IsEqualIID(riid, &IID_ITypeLib) ||
	       IsEqualIID(riid, &IID_ITypeLib2)) {
	*ppvObject = &This->lpVtblTypeLib2;
    }

    if(*ppvObject)
    {
        ICreateTypeLib2_AddRef(iface);
        TRACE("-- Interface: (%p)->(%p)\n",ppvObject,*ppvObject);
        return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/******************************************************************************
 * ICreateTypeLib2_AddRef {OLEAUT32}
 *
 *  See IUnknown_AddRef.
 */
static ULONG WINAPI ICreateTypeLib2_fnAddRef(ICreateTypeLib2 *iface)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->ref was %u\n",This, ref - 1);

    return ref;
}

/******************************************************************************
 * ICreateTypeLib2_Release {OLEAUT32}
 *
 *  See IUnknown_Release.
 */
static ULONG WINAPI ICreateTypeLib2_fnRelease(ICreateTypeLib2 *iface)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n",This, ref);

    if (!ref) {
	int i;

	for (i = 0; i < MSFT_SEG_MAX; i++) {
            HeapFree(GetProcessHeap(), 0, This->typelib_segment_data[i]);
            This->typelib_segment_data[i] = NULL;
	}

        HeapFree(GetProcessHeap(), 0, This->filename);
        This->filename = NULL;

	while (This->typeinfos) {
	    ICreateTypeInfo2Impl *typeinfo = This->typeinfos;
	    This->typeinfos = typeinfo->next_typeinfo;
            HeapFree(GetProcessHeap(), 0, typeinfo->typedata);
	    HeapFree(GetProcessHeap(), 0, typeinfo);
	}

	HeapFree(GetProcessHeap(),0,This);
	return 0;
    }

    return ref;
}


/******************************************************************************
 * ICreateTypeLib2_CreateTypeInfo {OLEAUT32}
 *
 *  See ICreateTypeLib_CreateTypeInfo.
 */
static HRESULT WINAPI ICreateTypeLib2_fnCreateTypeInfo(
	ICreateTypeLib2 * iface,
	LPOLESTR szName,
	TYPEKIND tkind,
	ICreateTypeInfo **ppCTInfo)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    TRACE("(%p,%s,%d,%p)\n", iface, debugstr_w(szName), tkind, ppCTInfo);

    *ppCTInfo = (ICreateTypeInfo *)ICreateTypeInfo2_Constructor(This, szName, tkind);

    if (!*ppCTInfo) return E_OUTOFMEMORY;

    return S_OK;
}

/******************************************************************************
 * ICreateTypeLib2_SetName {OLEAUT32}
 *
 *  See ICreateTypeLib_SetName.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetName(
	ICreateTypeLib2 * iface,
	LPOLESTR szName)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    int offset;

    TRACE("(%p,%s)\n", iface, debugstr_w(szName));

    offset = ctl2_alloc_name(This, szName);
    if (offset == -1) return E_OUTOFMEMORY;
    This->typelib_header.NameOffset = offset;
    return S_OK;
}

/******************************************************************************
 * ICreateTypeLib2_SetVersion {OLEAUT32}
 *
 *  See ICreateTypeLib_SetVersion.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetVersion(ICreateTypeLib2 * iface, WORD wMajorVerNum, WORD wMinorVerNum)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    TRACE("(%p,%d,%d)\n", iface, wMajorVerNum, wMinorVerNum);

    This->typelib_header.version = wMajorVerNum | (wMinorVerNum << 16);
    return S_OK;
}

/******************************************************************************
 * ICreateTypeLib2_SetGuid {OLEAUT32}
 *
 *  See ICreateTypeLib_SetGuid.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetGuid(ICreateTypeLib2 * iface, REFGUID guid)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    MSFT_GuidEntry guidentry;
    int offset;

    TRACE("(%p,%s)\n", iface, debugstr_guid(guid));

    guidentry.guid = *guid;
    guidentry.hreftype = -2;
    guidentry.next_hash = -1;

    offset = ctl2_alloc_guid(This, &guidentry);
    
    if (offset == -1) return E_OUTOFMEMORY;

    This->typelib_header.posguid = offset;

    return S_OK;
}

/******************************************************************************
 * ICreateTypeLib2_SetDocString {OLEAUT32}
 *
 *  See ICreateTypeLib_SetDocString.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetDocString(ICreateTypeLib2 * iface, LPOLESTR szDoc)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    int offset;

    TRACE("(%p,%s)\n", iface, debugstr_w(szDoc));

    offset = ctl2_alloc_string(This, szDoc);
    if (offset == -1) return E_OUTOFMEMORY;
    This->typelib_header.helpstring = offset;
    return S_OK;
}

/******************************************************************************
 * ICreateTypeLib2_SetHelpFileName {OLEAUT32}
 *
 *  See ICreateTypeLib_SetHelpFileName.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetHelpFileName(ICreateTypeLib2 * iface, LPOLESTR szHelpFileName)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    int offset;

    TRACE("(%p,%s)\n", iface, debugstr_w(szHelpFileName));

    offset = ctl2_alloc_string(This, szHelpFileName);
    if (offset == -1) return E_OUTOFMEMORY;
    This->typelib_header.helpfile = offset;
    This->typelib_header.varflags |= 0x10;
    return S_OK;
}

/******************************************************************************
 * ICreateTypeLib2_SetHelpContext {OLEAUT32}
 *
 *  See ICreateTypeLib_SetHelpContext.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetHelpContext(ICreateTypeLib2 * iface, DWORD dwHelpContext)
{
    FIXME("(%p,%d), stub!\n", iface, dwHelpContext);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeLib2_SetLcid {OLEAUT32}
 *
 *  See ICreateTypeLib_SetLcid.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetLcid(ICreateTypeLib2 * iface, LCID lcid)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    TRACE("(%p,%d)\n", iface, lcid);

    This->typelib_header.lcid2 = lcid;

    return S_OK;
}

/******************************************************************************
 * ICreateTypeLib2_SetLibFlags {OLEAUT32}
 *
 *  See ICreateTypeLib_SetLibFlags.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetLibFlags(ICreateTypeLib2 * iface, UINT uLibFlags)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    TRACE("(%p,0x%x)\n", iface, uLibFlags);

    This->typelib_header.flags = uLibFlags;

    return S_OK;
}

static int ctl2_write_chunk(HANDLE hFile, void *segment, int length)
{
    DWORD dwWritten;
    if (!WriteFile(hFile, segment, length, &dwWritten, 0)) {
        CloseHandle(hFile);
        return 0;
    }
    return -1;
}

static int ctl2_write_segment(ICreateTypeLib2Impl *This, HANDLE hFile, int segment)
{
    DWORD dwWritten;
    if (!WriteFile(hFile, This->typelib_segment_data[segment],
		   This->typelib_segdir[segment].length, &dwWritten, 0)) {
	CloseHandle(hFile);
	return 0;
    }

    return -1;
}

static void ctl2_finalize_typeinfos(ICreateTypeLib2Impl *This, int filesize)
{
    ICreateTypeInfo2Impl *typeinfo;

    for (typeinfo = This->typeinfos; typeinfo; typeinfo = typeinfo->next_typeinfo) {
	typeinfo->typeinfo->memoffset = filesize;
	if (typeinfo->typedata) {
	    ICreateTypeInfo2_fnLayOut((ICreateTypeInfo2 *)typeinfo);
	    filesize += typeinfo->typedata[0] + ((typeinfo->typeinfo->cElement >> 16) * 12) + ((typeinfo->typeinfo->cElement & 0xffff) * 12) + 4;
	}
    }
}

static int ctl2_finalize_segment(ICreateTypeLib2Impl *This, int filepos, int segment)
{
    if (This->typelib_segdir[segment].length) {
	This->typelib_segdir[segment].offset = filepos;
    } else {
	This->typelib_segdir[segment].offset = -1;
    }

    return This->typelib_segdir[segment].length;
}

static void ctl2_write_typeinfos(ICreateTypeLib2Impl *This, HANDLE hFile)
{
    ICreateTypeInfo2Impl *typeinfo;

    for (typeinfo = This->typeinfos; typeinfo; typeinfo = typeinfo->next_typeinfo) {
	if (!typeinfo->typedata) continue;

	ctl2_write_chunk(hFile, typeinfo->typedata, typeinfo->typedata[0] + 4);
	ctl2_write_chunk(hFile, typeinfo->indices, ((typeinfo->typeinfo->cElement & 0xffff) + (typeinfo->typeinfo->cElement >> 16)) * 4);
	ctl2_write_chunk(hFile, typeinfo->names, ((typeinfo->typeinfo->cElement & 0xffff) + (typeinfo->typeinfo->cElement >> 16)) * 4);
	ctl2_write_chunk(hFile, typeinfo->offsets, ((typeinfo->typeinfo->cElement & 0xffff) + (typeinfo->typeinfo->cElement >> 16)) * 4);
    }
}

/******************************************************************************
 * ICreateTypeLib2_SaveAllChanges {OLEAUT32}
 *
 *  See ICreateTypeLib_SaveAllChanges.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSaveAllChanges(ICreateTypeLib2 * iface)
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    int retval;
    int filepos;
    HANDLE hFile;

    TRACE("(%p)\n", iface);

    retval = TYPE_E_IOERROR;

    hFile = CreateFileW(This->filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE) return retval;

    filepos = sizeof(MSFT_Header) + sizeof(MSFT_SegDir);
    filepos += This->typelib_header.nrtypeinfos * 4;

    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_TYPEINFO);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_GUIDHASH);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_GUID);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_IMPORTINFO);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_IMPORTFILES);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_REFERENCES);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_NAMEHASH);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_NAME);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_STRING);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_TYPEDESC);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_ARRAYDESC);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_CUSTDATA);
    filepos += ctl2_finalize_segment(This, filepos, MSFT_SEG_CUSTDATAGUID);

    ctl2_finalize_typeinfos(This, filepos);

    if (!ctl2_write_chunk(hFile, &This->typelib_header, sizeof(This->typelib_header))) return retval;
    if (!ctl2_write_chunk(hFile, This->typelib_typeinfo_offsets, This->typelib_header.nrtypeinfos * 4)) return retval;
    if (!ctl2_write_chunk(hFile, &This->typelib_segdir, sizeof(This->typelib_segdir))) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_TYPEINFO    )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_GUIDHASH    )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_GUID        )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_IMPORTINFO  )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_IMPORTFILES )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_REFERENCES  )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_NAMEHASH    )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_NAME        )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_STRING      )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_TYPEDESC    )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_ARRAYDESC   )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_CUSTDATA    )) return retval;
    if (!ctl2_write_segment(This, hFile, MSFT_SEG_CUSTDATAGUID)) return retval;

    ctl2_write_typeinfos(This, hFile);

    if (!CloseHandle(hFile)) return retval;

    retval = S_OK;
    return retval;
}


/******************************************************************************
 * ICreateTypeLib2_DeleteTypeInfo {OLEAUT32}
 *
 *  Deletes a named TypeInfo from a type library.
 *
 * RETURNS
 *
 *  Success: S_OK
 *  Failure: E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeLib2_fnDeleteTypeInfo(
	ICreateTypeLib2 * iface, /* [I] The type library to delete from. */
	LPOLESTR szName)         /* [I] The name of the typeinfo to delete. */
{
    FIXME("(%p,%s), stub!\n", iface, debugstr_w(szName));
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeLib2_SetCustData {OLEAUT32}
 *
 *  Sets custom data for a type library.
 *
 * RETURNS
 *
 *  Success: S_OK
 *  Failure: E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetCustData(
	ICreateTypeLib2 * iface, /* [I] The type library to store the custom data in. */
	REFGUID guid,            /* [I] The GUID used as a key to retrieve the custom data. */
	VARIANT *pVarVal)        /* [I] The custom data itself. */
{
    ICreateTypeLib2Impl *This = (ICreateTypeLib2Impl *)iface;

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(guid), pVarVal);

    return ctl2_set_custdata(This, guid, pVarVal, &This->typelib_header.CustomDataOffset);
}

/******************************************************************************
 * ICreateTypeLib2_SetHelpStringContext {OLEAUT32}
 *
 *  Sets a context number for the library help string.
 *
 * RETURNS
 *
 *  Success: S_OK
 *  Failure: E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetHelpStringContext(
	ICreateTypeLib2 * iface,   /* [I] The type library to set the help string context for. */
	ULONG dwHelpStringContext) /* [I] The help string context. */
{
    FIXME("(%p,%d), stub!\n", iface, dwHelpStringContext);
    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeLib2_SetHelpStringDll {OLEAUT32}
 *
 *  Sets the DLL used to look up localized help strings.
 *
 * RETURNS
 *
 *  Success: S_OK
 *  Failure: E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ICreateTypeLib2_fnSetHelpStringDll(
	ICreateTypeLib2 * iface, /* [I] The type library to set the help DLL for. */
	LPOLESTR szFileName)     /* [I] The name of the help DLL. */
{
    FIXME("(%p,%s), stub!\n", iface, debugstr_w(szFileName));
    return E_OUTOFMEMORY;
}

/*================== ITypeLib2 Implementation ===================================*/

/******************************************************************************
 * ITypeLib2_QueryInterface {OLEAUT32}
 *
 *  See IUnknown_QueryInterface.
 */
static HRESULT WINAPI ITypeLib2_fnQueryInterface(ITypeLib2 * iface, REFIID riid, LPVOID * ppv)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    return ICreateTypeLib2_QueryInterface((ICreateTypeLib2 *)This, riid, ppv);
}

/******************************************************************************
 * ITypeLib2_AddRef {OLEAUT32}
 *
 *  See IUnknown_AddRef.
 */
static ULONG WINAPI ITypeLib2_fnAddRef(ITypeLib2 * iface)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    return ICreateTypeLib2_AddRef((ICreateTypeLib2 *)This);
}

/******************************************************************************
 * ITypeLib2_Release {OLEAUT32}
 *
 *  See IUnknown_Release.
 */
static ULONG WINAPI ITypeLib2_fnRelease(ITypeLib2 * iface)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    return ICreateTypeLib2_Release((ICreateTypeLib2 *)This);
}

/******************************************************************************
 * ITypeLib2_GetTypeInfoCount {OLEAUT32}
 *
 *  See ITypeLib_GetTypeInfoCount.
 */
static UINT WINAPI ITypeLib2_fnGetTypeInfoCount(
        ITypeLib2 * iface)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    TRACE("(%p)\n", iface);

    return This->typelib_header.nrtypeinfos;
}

/******************************************************************************
 * ITypeLib2_GetTypeInfo {OLEAUT32}
 *
 *  See ITypeLib_GetTypeInfo.
 */
static HRESULT WINAPI ITypeLib2_fnGetTypeInfo(
        ITypeLib2 * iface,
        UINT index,
        ITypeInfo** ppTInfo)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    TRACE("(%p,%d,%p)\n", iface, index, ppTInfo);

    if ((index < 0) || (index >= This->typelib_header.nrtypeinfos)) {
	return TYPE_E_ELEMENTNOTFOUND;
    }

    return ctl2_find_typeinfo_from_offset(This, This->typelib_typeinfo_offsets[index], ppTInfo);
}

/******************************************************************************
 * ITypeLib2_GetTypeInfoType {OLEAUT32}
 *
 *  See ITypeLib_GetTypeInfoType.
 */
static HRESULT WINAPI ITypeLib2_fnGetTypeInfoType(
        ITypeLib2 * iface,
        UINT index,
        TYPEKIND* pTKind)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    TRACE("(%p,%d,%p)\n", iface, index, pTKind);

    if ((index < 0) || (index >= This->typelib_header.nrtypeinfos)) {
	return TYPE_E_ELEMENTNOTFOUND;
    }

    *pTKind = (This->typelib_segment_data[MSFT_SEG_TYPEINFO][This->typelib_typeinfo_offsets[index]]) & 15;

    return S_OK;
}

/******************************************************************************
 * ITypeLib2_GetTypeInfoOfGuid {OLEAUT32}
 *
 *  See ITypeLib_GetTypeInfoOfGuid.
 */
static HRESULT WINAPI ITypeLib2_fnGetTypeInfoOfGuid(
        ITypeLib2 * iface,
        REFGUID guid,
        ITypeInfo** ppTinfo)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    int guidoffset;
    int typeinfo;

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(guid), ppTinfo);

    guidoffset = ctl2_find_guid(This, ctl2_hash_guid(guid), guid);
    if (guidoffset == -1) return TYPE_E_ELEMENTNOTFOUND;

    typeinfo = ((MSFT_GuidEntry *)&This->typelib_segment_data[MSFT_SEG_GUID][guidoffset])->hreftype;
    if (typeinfo < 0) return TYPE_E_ELEMENTNOTFOUND;

    return ctl2_find_typeinfo_from_offset(This, typeinfo, ppTinfo);
}

/******************************************************************************
 * ITypeLib2_GetLibAttr {OLEAUT32}
 *
 *  See ITypeLib_GetLibAttr.
 */
static HRESULT WINAPI ITypeLib2_fnGetLibAttr(
        ITypeLib2 * iface,
        TLIBATTR** ppTLibAttr)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    FIXME("(%p,%p), stub!\n", This, ppTLibAttr);

    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeLib2_GetTypeComp {OLEAUT32}
 *
 *  See ITypeLib_GetTypeComp.
 */
static HRESULT WINAPI ITypeLib2_fnGetTypeComp(
        ITypeLib2 * iface,
        ITypeComp** ppTComp)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    FIXME("(%p,%p), stub!\n", This, ppTComp);

    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeLib2_GetDocumentation {OLEAUT32}
 *
 *  See ITypeLib_GetDocumentation.
 */
static HRESULT WINAPI ITypeLib2_fnGetDocumentation(
        ITypeLib2 * iface,
        INT index,
        BSTR* pBstrName,
        BSTR* pBstrDocString,
        DWORD* pdwHelpContext,
        BSTR* pBstrHelpFile)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    FIXME("(%p,%d,%p,%p,%p,%p), stub!\n", This, index, pBstrName, pBstrDocString, pdwHelpContext, pBstrHelpFile);

    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeLib2_IsName {OLEAUT32}
 *
 *  See ITypeLib_IsName.
 */
static HRESULT WINAPI ITypeLib2_fnIsName(
        ITypeLib2 * iface,
        LPOLESTR szNameBuf,
        ULONG lHashVal,
        BOOL* pfName)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    char *encoded_name;
    int nameoffset;
    MSFT_NameIntro *nameintro;

    TRACE("(%p,%s,%x,%p)\n", iface, debugstr_w(szNameBuf), lHashVal, pfName);

    ctl2_encode_name(This, szNameBuf, &encoded_name);
    nameoffset = ctl2_find_name(This, encoded_name);

    *pfName = 0;

    if (nameoffset == -1) return S_OK;

    nameintro = (MSFT_NameIntro *)(&This->typelib_segment_data[MSFT_SEG_NAME][nameoffset]);
    if (nameintro->hreftype == -1) return S_OK;

    *pfName = 1;

    FIXME("Should be decoding our copy of the name over szNameBuf.\n");

    return S_OK;
}

/******************************************************************************
 * ITypeLib2_FindName {OLEAUT32}
 *
 *  See ITypeLib_FindName.
 */
static HRESULT WINAPI ITypeLib2_fnFindName(
        ITypeLib2 * iface,
        LPOLESTR szNameBuf,
        ULONG lHashVal,
        ITypeInfo** ppTInfo,
        MEMBERID* rgMemId,
        USHORT* pcFound)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    FIXME("(%p,%s,%x,%p,%p,%p), stub!\n", This, debugstr_w(szNameBuf), lHashVal, ppTInfo, rgMemId, pcFound);

    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ITypeLib2_ReleaseTLibAttr {OLEAUT32}
 *
 *  See ITypeLib_ReleaseTLibAttr.
 */
static void WINAPI ITypeLib2_fnReleaseTLibAttr(
        ITypeLib2 * iface,
        TLIBATTR* pTLibAttr)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    FIXME("(%p,%p), stub!\n", This, pTLibAttr);
}

/******************************************************************************
 * ICreateTypeLib2_GetCustData {OLEAUT32}
 *
 *  Retrieves a custom data value stored on a type library.
 *
 * RETURNS
 *
 *  Success: S_OK
 *  Failure: E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeLib2_fnGetCustData(
        ITypeLib2 * iface, /* [I] The type library in which to find the custom data. */
        REFGUID guid,      /* [I] The GUID under which the custom data is stored. */
        VARIANT* pVarVal)  /* [O] The custom data. */
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    FIXME("(%p,%s,%p), stub!\n", This, debugstr_guid(guid), pVarVal);

    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeLib2_GetLibStatistics {OLEAUT32}
 *
 *  Retrieves some statistics about names in a type library, supposedly for
 *  hash table optimization purposes.
 *
 * RETURNS
 *
 *  Success: S_OK
 *  Failure: E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeLib2_fnGetLibStatistics(
        ITypeLib2 * iface,      /* [I] The type library to get statistics about. */
        ULONG* pcUniqueNames,   /* [O] The number of unique names in the type library. */
        ULONG* pcchUniqueNames) /* [O] The number of changed (?) characters in names in the type library. */
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    FIXME("(%p,%p,%p), stub!\n", This, pcUniqueNames, pcchUniqueNames);

    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeLib2_GetDocumentation2 {OLEAUT32}
 *
 *  Obtain locale-aware help string information.
 *
 * RETURNS
 *
 *  Success: S_OK
 *  Failure: STG_E_INSUFFICIENTMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeLib2_fnGetDocumentation2(
        ITypeLib2 * iface,
        INT index,
        LCID lcid,
        BSTR* pbstrHelpString,
        DWORD* pdwHelpStringContext,
        BSTR* pbstrHelpStringDll)
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    FIXME("(%p,%d,%d,%p,%p,%p), stub!\n", This, index, lcid, pbstrHelpString, pdwHelpStringContext, pbstrHelpStringDll);

    return E_OUTOFMEMORY;
}

/******************************************************************************
 * ICreateTypeLib2_GetAllCustData {OLEAUT32}
 *
 *  Retrieve all of the custom data for a type library.
 *
 * RETURNS
 *
 *  Success: S_OK
 *  Failure: E_OUTOFMEMORY or E_INVALIDARG.
 */
static HRESULT WINAPI ITypeLib2_fnGetAllCustData(
        ITypeLib2 * iface,   /* [I] The type library in which to find the custom data. */
        CUSTDATA* pCustData) /* [O] The structure in which to place the custom data. */
{
    ICreateTypeLib2Impl *This = impl_from_ITypeLib2(iface);

    FIXME("(%p,%p), stub!\n", This, pCustData);

    return E_OUTOFMEMORY;
}


/*================== ICreateTypeLib2 & ITypeLib2 VTABLEs And Creation ===================================*/

static const ICreateTypeLib2Vtbl ctypelib2vt =
{

    ICreateTypeLib2_fnQueryInterface,
    ICreateTypeLib2_fnAddRef,
    ICreateTypeLib2_fnRelease,

    ICreateTypeLib2_fnCreateTypeInfo,
    ICreateTypeLib2_fnSetName,
    ICreateTypeLib2_fnSetVersion,
    ICreateTypeLib2_fnSetGuid,
    ICreateTypeLib2_fnSetDocString,
    ICreateTypeLib2_fnSetHelpFileName,
    ICreateTypeLib2_fnSetHelpContext,
    ICreateTypeLib2_fnSetLcid,
    ICreateTypeLib2_fnSetLibFlags,
    ICreateTypeLib2_fnSaveAllChanges,

    ICreateTypeLib2_fnDeleteTypeInfo,
    ICreateTypeLib2_fnSetCustData,
    ICreateTypeLib2_fnSetHelpStringContext,
    ICreateTypeLib2_fnSetHelpStringDll
};

static const ITypeLib2Vtbl typelib2vt =
{

    ITypeLib2_fnQueryInterface,
    ITypeLib2_fnAddRef,
    ITypeLib2_fnRelease,

    ITypeLib2_fnGetTypeInfoCount,
    ITypeLib2_fnGetTypeInfo,
    ITypeLib2_fnGetTypeInfoType,
    ITypeLib2_fnGetTypeInfoOfGuid,
    ITypeLib2_fnGetLibAttr,
    ITypeLib2_fnGetTypeComp,
    ITypeLib2_fnGetDocumentation,
    ITypeLib2_fnIsName,
    ITypeLib2_fnFindName,
    ITypeLib2_fnReleaseTLibAttr,

    ITypeLib2_fnGetCustData,
    ITypeLib2_fnGetLibStatistics,
    ITypeLib2_fnGetDocumentation2,
    ITypeLib2_fnGetAllCustData,
};

static ICreateTypeLib2 *ICreateTypeLib2_Constructor(SYSKIND syskind, LPCOLESTR szFile)
{
    ICreateTypeLib2Impl *pCreateTypeLib2Impl;
    int failed = 0;

    TRACE("Constructing ICreateTypeLib2 (%d, %s)\n", syskind, debugstr_w(szFile));

    pCreateTypeLib2Impl = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ICreateTypeLib2Impl));
    if (!pCreateTypeLib2Impl) return NULL;

    pCreateTypeLib2Impl->filename = HeapAlloc(GetProcessHeap(), 0, (strlenW(szFile) + 1) * sizeof(WCHAR));
    if (!pCreateTypeLib2Impl->filename) {
	HeapFree(GetProcessHeap(), 0, pCreateTypeLib2Impl);
	return NULL;
    }
    strcpyW(pCreateTypeLib2Impl->filename, szFile);

    ctl2_init_header(pCreateTypeLib2Impl);
    ctl2_init_segdir(pCreateTypeLib2Impl);

    pCreateTypeLib2Impl->typelib_header.varflags |= syskind;

    /*
     * The following two calls return an offset or -1 if out of memory. We
     * specifically need an offset of 0, however, so...
     */
    if (ctl2_alloc_segment(pCreateTypeLib2Impl, MSFT_SEG_GUIDHASH, 0x80, 0x80)) { failed = 1; }
    if (ctl2_alloc_segment(pCreateTypeLib2Impl, MSFT_SEG_NAMEHASH, 0x200, 0x200)) { failed = 1; }

    pCreateTypeLib2Impl->typelib_guidhash_segment = (int *)pCreateTypeLib2Impl->typelib_segment_data[MSFT_SEG_GUIDHASH];
    pCreateTypeLib2Impl->typelib_namehash_segment = (int *)pCreateTypeLib2Impl->typelib_segment_data[MSFT_SEG_NAMEHASH];

    memset(pCreateTypeLib2Impl->typelib_guidhash_segment, 0xff, 0x80);
    memset(pCreateTypeLib2Impl->typelib_namehash_segment, 0xff, 0x200);

    pCreateTypeLib2Impl->lpVtbl = &ctypelib2vt;
    pCreateTypeLib2Impl->lpVtblTypeLib2 = &typelib2vt;
    pCreateTypeLib2Impl->ref = 1;

    if (failed) {
	ICreateTypeLib2_fnRelease((ICreateTypeLib2 *)pCreateTypeLib2Impl);
	return 0;
    }

    return (ICreateTypeLib2 *)pCreateTypeLib2Impl;
}

/******************************************************************************
 * CreateTypeLib2 [OLEAUT32.180]
 *
 *  Obtains an ICreateTypeLib2 object for creating a new-style (MSFT) type
 *  library.
 *
 * NOTES
 *
 *  See also CreateTypeLib.
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI CreateTypeLib2(
	SYSKIND syskind,           /* [I] System type library is for */
	LPCOLESTR szFile,          /* [I] Type library file name */
	ICreateTypeLib2** ppctlib) /* [O] Storage for object returned */
{
    TRACE("(%d,%s,%p)\n", syskind, debugstr_w(szFile), ppctlib);

    if (!szFile) return E_INVALIDARG;
    *ppctlib = ICreateTypeLib2_Constructor(syskind, szFile);
    return (*ppctlib)? S_OK: E_OUTOFMEMORY;
}

/******************************************************************************
 * ClearCustData (OLEAUT32.171)
 *
 * Clear a custom data types' data.
 *
 * PARAMS
 *  lpCust [I] The custom data type instance
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI ClearCustData(LPCUSTDATA lpCust)
{
    if (lpCust && lpCust->cCustData)
    {
        if (lpCust->prgCustData)
        {
            DWORD i;

            for (i = 0; i < lpCust->cCustData; i++)
                VariantClear(&lpCust->prgCustData[i].varValue);

            /* FIXME - Should be using a per-thread IMalloc */
            HeapFree(GetProcessHeap(), 0, lpCust->prgCustData);
            lpCust->prgCustData = NULL;
        }
        lpCust->cCustData = 0;
    }
}
