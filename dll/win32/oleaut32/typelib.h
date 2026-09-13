/*
 * typelib.h  internal wine data structures
 * used to decode typelib's
 *
 * Copyright 1999 Rein KLazes
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
#ifndef _WINE_TYPELIB_H
#define _WINE_TYPELIB_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "oleauto.h"

#define HELPDLLFLAG (0x0100)
#define DO_NOT_SEEK (-1)

#define MSFT_HREFTYPE_INTHISFILE(href) (!((href) & 3))
#define MSFT_HREFTYPE_INDEX(href) ((href) /sizeof(MSFT_TypeInfoBase))

/*-------------------------FILE STRUCTURES-----------------------------------*/

/* There are two known file formats, those created with ICreateTypeLib
 * have the signature "SLTG" as their first four bytes, while those created
 * with ICreateTypeLib2 have "MSFT".
 */

/*****************************************************
 *                MSFT typelibs
 *
 * These are TypeLibs created with ICreateTypeLib2
 *
 */

/*
 * structure of the typelib type2 header
 * it is at the beginning of a type lib file
 *
 */

#define MSFT_SIGNATURE 0x5446534D /* "MSFT" */

typedef struct tagMSFT_Header {
/*0x00*/INT   magic1;       /* 0x5446534D "MSFT" */
        INT   magic2;       /* 0x00010002 version nr? */
        INT   posguid;      /* position of libid in guid table  */
                            /* (should be,  else -1) */
        INT   lcid;         /* locale id */
/*0x10*/INT   lcid2;
        INT   varflags;     /* (largely) unknown flags ,seems to be always 41 */
                            /* becomes 0x51 with a helpfile defined */
                            /* if help dll defined it's 0x151 */
                            /* update : the lower nibble is syskind */
        INT   version;      /* set with SetVersion() */
        INT   flags;        /* set with SetFlags() */
/*0x20*/INT   nrtypeinfos;  /* number of typeinfo's (till so far) */
        INT   helpstring;   /* position of help string in stringtable */
        INT   helpstringcontext;
        INT   helpcontext;
/*0x30*/INT   nametablecount;   /* number of names in name table */
        INT   nametablechars;   /* nr of characters in name table */
        INT   NameOffset;       /* offset of name in name table */
        INT   helpfile;         /* position of helpfile in stringtable */
/*0x40*/INT   CustomDataOffset; /* if -1 no custom data, else it is offset */
                                /* in customer data/guid offset table */
        INT   res44;            /* unknown always: 0x20 (guid hash size?) */
        INT   res48;            /* unknown always: 0x80 (name hash size?) */
        INT   dispatchpos;      /* HREFTYPE to IDispatch, or -1 if no IDispatch */
/*0x50*/INT   nimpinfos;        /* number of impinfos */
} MSFT_Header;

/* segments in the type lib file have a structure like this: */
typedef struct tagMSFT_pSeg {
        INT   offset;       /* absolute offset in file */
        INT   length;       /* length of segment */
        INT   res08;        /* unknown always -1 */
        INT   res0c;        /* unknown always 0x0f in the header */
                            /* 0x03 in the typeinfo_data */
} MSFT_pSeg;

/* layout of the main segment directory */
typedef struct tagMSFT_SegDir {
/*1*/MSFT_pSeg pTypeInfoTab; /* each type info get an entry of 0x64 bytes */
                             /* (25 ints) */
/*2*/MSFT_pSeg pImpInfo;     /* table with info for imported types */
/*3*/MSFT_pSeg pImpFiles;    /* import libraries */
/*4*/MSFT_pSeg pRefTab;      /* References table */
/*5*/MSFT_pSeg pGuidHashTab; /* always exists, always same size (0x80) */
                             /* hash table with offsets to guid */
/*6*/MSFT_pSeg pGuidTab;     /* all guids are stored here together with  */
                             /* offset in some table???? */
/*7*/MSFT_pSeg pNameHashTab; /* always created, always same size (0x200) */
                             /* hash table with offsets to names */
/*8*/MSFT_pSeg pNametab;     /* name tables */
/*9*/MSFT_pSeg pStringtab;   /* string table */
/*A*/MSFT_pSeg pTypdescTab;  /* table with type descriptors */
/*B*/MSFT_pSeg pArrayDescriptions;
/*C*/MSFT_pSeg pCustData;    /* data table, used for custom data and default */
                             /* parameter values */
/*D*/MSFT_pSeg pCDGuids;     /* table with offsets for the guids and into */
                             /* the customer data table */
/*E*/MSFT_pSeg res0e;        /* unknown */
/*F*/MSFT_pSeg res0f;        /* unknown  */
} MSFT_SegDir;


/* base type info data */
typedef struct tagMSFT_TypeInfoBase {
/*000*/ INT   typekind;             /*  it is the TKIND_xxx */
                                    /* some byte alignment stuff */
        INT     memoffset;          /* points past the file, if no elements */
        INT     res2;               /* zero if no element, N*0x40 */
        INT     res3;               /* -1 if no element, (N-1)*0x38 */
/*010*/ INT     res4;               /* always? 3 */
        INT     res5;               /* always? zero */
        INT     cElement;           /* counts elements, HI=cVars, LO=cFuncs */
        INT     res7;               /* always? zero */
/*020*/ INT     res8;               /* always? zero */
        INT     res9;               /* always? zero */
        INT     resA;               /* always? zero */
        INT     posguid;            /* position in guid table */
/*030*/ INT     flags;              /* Typeflags */
        INT     NameOffset;         /* offset in name table */
        INT     version;            /* element version */
        INT     docstringoffs;      /* offset of docstring in string tab */
/*040*/ INT     helpstringcontext;  /*  */
        INT     helpcontext;    /* */
        INT     oCustData;          /* offset in customer data table */
#ifdef WORDS_BIGENDIAN
        INT16   cbSizeVft;      /* virtual table size, including inherits */
        INT16   cImplTypes;     /* nr of implemented interfaces */
#else
        INT16   cImplTypes;     /* nr of implemented interfaces */
        INT16   cbSizeVft;      /* virtual table size, including inherits */
#endif
/*050*/ INT     size;           /* size in bytes, at least for structures */
        /* FIXME: name of this field */
        INT     datatype1;      /* position in type description table */
                                /* or in base interfaces */
                                /* if coclass: offset in reftable */
                                /* if interface: reference to inherited if */
                                /* if module: offset to dllname in name table */
        INT     datatype2;      /* for interfaces: hiword is num of inherited funcs */
                                /*                 loword is num of inherited interfaces */
        INT     res18;          /* always? 0 */
/*060*/ INT     res19;          /* always? -1 */
} MSFT_TypeInfoBase;

/* layout of an entry with information on imported types */
typedef struct tagMSFT_ImpInfo {
    INT     flags;          /* bits 0 - 15:  count */
                            /* bit  16:      if set oGuid is an offset to Guid */
                            /*               if clear oGuid is a typeinfo index in the specified typelib */
                            /* bits 24 - 31: TKIND of reference */
    INT     oImpFile;       /* offset in the Import File table */
    INT     oGuid;          /* offset in Guid table or typeinfo index (see bit 16 of res0) */
} MSFT_ImpInfo;

#define MSFT_IMPINFO_OFFSET_IS_GUID 0x00010000

/* function description data */
typedef struct {
    INT   Info;         /* record size including some extra stuff */
    INT   DataType;     /* data type of the member, eg return of function */
    INT   Flags;        /* something to do with attribute flags (LOWORD) */
#ifdef WORDS_BIGENDIAN
    INT16 funcdescsize; /* size of reconstituted FUNCDESC and related structs */
    INT16 VtableOffset; /* offset in vtable */
#else
    INT16 VtableOffset; /* offset in vtable */
    INT16 funcdescsize; /* size of reconstituted FUNCDESC and related structs */
#endif
    INT   FKCCIC;       /* bit string with the following  */
                        /* meaning (bit 0 is the msb): */
                        /* bit 2 indicates that oEntry is numeric */
                        /* bit 3 that parameter has default values */
                        /* calling convention (bits 4-7 ) */
                        /* bit 8 indicates that custom data is present */
                        /* Invocation kind (bits 9-12 ) */
                        /* function kind (eg virtual), bits 13-15  */
#ifdef WORDS_BIGENDIAN
    INT16 nroargs;      /* nr of optional arguments */
    INT16 nrargs;       /* number of arguments (including optional ????) */
#else
    INT16 nrargs;       /* number of arguments (including optional ????) */
    INT16 nroargs;      /* nr of optional arguments */
#endif

    /* optional attribute fields, the number of them is variable */
    INT   HelpContext;
    INT   oHelpString;
    INT   oEntry;       /* either offset in string table or numeric as it is */
    INT   res9;         /* unknown (-1) */
    INT   resA;         /* unknown (-1) */
    INT   HelpStringContext;
    /* these are controlled by a bit set in the FKCCIC field  */
    INT   oCustData;        /* custom data for function */
    INT   oArgCustData[1];  /* custom data per argument */
} MSFT_FuncRecord;

/* after this may follow an array with default value pointers if the
 * appropriate bit in the FKCCIC field has been set:
 * INT   oDefaultValue[nrargs];
 */

    /* Parameter info one per argument*/
typedef struct {
        INT   DataType;
        INT   oName;
        INT   Flags;
    } MSFT_ParameterInfo;

/* Variable description data */
typedef struct {
    INT   Info;         /* record size including some extra stuff */
    INT   DataType;     /* data type of the variable */
    INT   Flags;        /* VarFlags (LOWORD) */
#ifdef WORDS_BIGENDIAN
    INT16 vardescsize;  /* size of reconstituted VARDESC and related structs */
    INT16 VarKind;      /* VarKind */
#else
    INT16 VarKind;      /* VarKind */
    INT16 vardescsize;  /* size of reconstituted VARDESC and related structs */
#endif
    INT   OffsValue;    /* value of the variable or the offset  */
                        /* in the data structure */
    /* optional attribute fields, the number of them is variable */
    /* controlled by record length */
    INT   HelpContext;
    INT   HelpString;
    INT   res9;         /* unknown (-1) */
    INT   oCustData;        /* custom data for variable */
    INT   HelpStringContext;
} MSFT_VarRecord;

/* Structure of the reference data  */
typedef struct {
    INT   reftype;  /* either offset in type info table, then it's */
                    /* a multiple of 64 */
                    /* or offset in the external reference table */
                    /* with an offset of 1 */
    INT   flags;
    INT   oCustData;    /* custom data */
    INT   onext;    /* next offset, -1 if last */
} MSFT_RefRecord;

/* this is how a guid is stored */
typedef struct {
    GUID guid;
    INT   hreftype;     /* -2 for the typelib guid, typeinfo offset
			   for typeinfo guid, low two bits are 01 if
			   this is an imported typeinfo, low two bits
			   are 10 if this is an imported typelib (used
			   by imported typeinfos) */
    INT   next_hash;    /* offset to next guid in the hash bucket */
} MSFT_GuidEntry;
/* some data preceding entries in the name table */
typedef struct {
    INT   hreftype;     /* is -1 if name is for neither a typeinfo,
			   a variable, or a function (that is, name
			   is for a typelib or a function parameter).
			   otherwise is the offset of the first
			   typeinfo that this name refers to (either
			   to the typeinfo itself or to a member of
			   the typeinfo */
    INT   next_hash;    /* offset to next name in the hash bucket */
    INT   namelen;      /* only lower 8 bits are valid */
                        /* 0x1000 if name is only used once as a variable name */
                        /* 0x2000 if name is a variable in an enumeration */
                        /* 0x3800 if name is typeinfo name */
} MSFT_NameIntro;
/* the custom data table directory has entries like this */
typedef struct {
    INT   GuidOffset;
    INT   DataOffset;
    INT   next;     /* next offset in the table, -1 if it's the last */
} MSFT_CDGuid;


/***********************************************************
 *
 *                SLTG typelibs.
 *
 * These are created with ICreateTypeLib
 *
 */

#include "pshpack1.h"

typedef struct {
/*00*/	DWORD SLTG_magic;	/* 0x47544c53  == "SLTG" */
/*04*/	WORD nrOfFileBlks;	/* no of SLTG_BlkEntry's + 1 */
/*06*/  WORD res06;		/* ?? always 9 */
/*08*/  WORD res08;             /* some kind of len/offset ?? */
/*0a*/	WORD first_blk;		/* 1 based index into blk entries that
				   corresponds to first block in file */
/*0c*/	GUID guid;		/* always 000204ff-0000-0000-c000-000000000046 */
/*1c*/	DWORD res1c;		/* always 0x00000044 */
/*20*/	DWORD res20;		/* always 0xffff0000 */
} SLTG_Header;

/* This gets followed by a list of block entries */
typedef struct {
/*00*/  DWORD len;
/*04*/	WORD index_string; /* offs from start of SLTG_Magic to index string */
/*06*/  WORD next;
} SLTG_BlkEntry;

/* The order of the blocks in the file is given by starting at Block
   entry first_blk and stepping through using the next pointer */

/* These then get followed by this magic */
typedef struct {
/*00*/ CHAR CompObj_magic[9];	/* always "\1CompObj" */
/*09*/ CHAR dir_magic[4];	/* always "dir" */
} SLTG_Magic;

#define SLTG_COMPOBJ_MAGIC "\1CompObj"
#define SLTG_DIR_MAGIC "dir"

/* Next we have SLTG_Header.nrOfFileBlks - 2 of Index strings.  These
are presumably unique to within the file and look something like
"AAAAAAAAAA" with the first character incremented from 'A' to ensure
uniqueness.  I guess successive chars increment when we need to wrap
the first one. */

typedef struct {
/*00*/ CHAR string[11];
} SLTG_Index;


/* This is followed by SLTG_pad9 */
typedef struct {
/*00*/ CHAR pad[9];	/* 9 '\0's */
} SLTG_Pad9;


/* Now we have the noOfFileBlks - 1 worth of blocks. The length of
each block is given by its entry in SLTG_BlkEntry. */

/* type SLTG_NAME in rather like a BSTR except that the length in
bytes is given by the first WORD and the string contains 8bit chars */

typedef WORD SLTG_Name;

/* The main library block looks like this.  This one seems to come last */

typedef struct {
/*00*/	WORD magic;		/* 0x51cc */
/*02*/  WORD res02;		/* 0x0003, 0x0004 */
/*04*/  WORD name;              /* offset to name in name table */
/*06*/  SLTG_Name res06;	/* maybe this is just WORD == 0xffff */
	SLTG_Name helpstring;
	SLTG_Name helpfile;
	DWORD helpcontext;
	WORD syskind;		/* == 1 for win32, 0 for win16 */
	WORD lcid;		/* == 0x409, 0x809 etc */
	DWORD res12;		/* == 0 */
 	WORD libflags;		/* LIBFLAG_* */
	WORD maj_vers;
	WORD min_vers;
	GUID uuid;
} SLTG_LibBlk;

#define SLTG_LIBBLK_MAGIC 0x51cc

/* we then get 0x40 bytes worth of 0xffff or small numbers followed by
   nrOfFileBlks - 2 of these */
typedef struct {
	WORD small_no;
	SLTG_Name index_name; /* This refers to a name in the directory */
	SLTG_Name other_name; /* Another one of these weird names */
	WORD res1a;	      /* 0xffff */
	WORD name_offs;	      /* offset to name in name table */
	WORD more_bytes;      /* if this is non-zero we get this many
				 bytes before the next element, which seem
				 to reference the docstring of the type ? */
	WORD res20;	      /* 0xffff */
	DWORD helpcontext;
	WORD res26;	      /* 0xffff */
        GUID uuid;
} SLTG_OtherTypeInfo;

/* Next we get WORD 0x0003 followed by a DWORD which if we add to
0x216 gives the offset to the name table from the start of the LibBlk
struct */

typedef struct {
/*00*/	WORD magic;		/* 0x0501 */
/*02*/	DWORD href_table;	/* if not 0xffffffff, then byte offset from
				   beginning of struct to href table */
/*06*/	DWORD res06;		/* 0xffffffff */
/*0a*/	DWORD elem_table;	/* offset to members */
/*0e*/	DWORD res0e;		/* 0xffffffff */
/*12*/	WORD major_version;	/* major version number */
/*14*/  WORD minor_version;	/* minor version number */
/*16*/	DWORD res16;	/* 0xfffe0000 or 0xfffc0000 (on dual interfaces?) */
/*1a*/	BYTE typeflags1;/* 0x02 | top 5 bits hold l5sbs of TYPEFLAGS */
/*1b*/	BYTE typeflags2;/* TYPEFLAGS >> 5 */
/*1c*/	BYTE typeflags3;/* 0x02*/
/*1d*/	BYTE typekind;	/* 0x03 == TKIND_INTERFACE etc. */
/*1e*/  DWORD res1e;	/* 0x00000000 or 0xffffffff */
} SLTG_TypeInfoHeader;

#define SLTG_TIHEADER_MAGIC 0x0501

typedef struct {
/*00*/  WORD cFuncs;
/*02*/  WORD cVars;
/*04*/  WORD cImplTypes;
/*06*/  WORD res06; /* always 0000 */
/*08*/  WORD funcs_off; /* offset to functions (starting from the member header) */
/*0a*/  WORD vars_off; /* offset to vars (starting from the member header) */
/*0c*/  WORD impls_off; /* offset to implemented types (starting from the member header) */
/*0e*/  WORD funcs_bytes; /* bytes used by function data */
/*10*/  WORD vars_bytes; /* bytes used by var data */
/*12*/  WORD impls_bytes; /* bytes used by implemented type data */
/*14*/  WORD tdescalias_vt; /* for TKIND_ALIAS */
/*16*/  WORD res16; /* always ffff */
/*18*/  WORD res18; /* always 0000 */
/*1a*/  WORD res1a; /* always 0000 */
/*1c*/  WORD simple_alias; /* tdescalias_vt is a vt rather than an offset? */
/*1e*/  WORD res1e; /* always 0000 */
/*20*/  WORD cbSizeInstance;
/*22*/  WORD cbAlignment;
/*24*/  WORD res24;
/*26*/  WORD res26;
/*28*/  WORD cbSizeVft;
/*2a*/  WORD res2a; /* always ffff */
/*2c*/  WORD res2c; /* always ffff */
/*2e*/  WORD res2e; /* always ffff */
/*30*/  WORD res30; /* always ffff */
/*32*/  WORD res32;
/*34*/  WORD res34;
} SLTG_TypeInfoTail;

typedef struct {
/*00*/ WORD res00; /* 0x0001 sometimes 0x0003 ?? */
/*02*/ WORD res02; /* 0xffff */
/*04*/ BYTE res04; /* 0x01 */
/*05*/ DWORD cbExtra; /* No of bytes that follow */
} SLTG_MemberHeader;

typedef struct {
/*00*/	WORD magic;	/* 0x120a */
/*02*/	WORD next;	/* offset in bytes to next block from start of block
                           group, 0xffff if last item */
/*04*/	WORD name;	/* offset to name within name table */
/*06*/	WORD value;	/* offset to value from start of block group */
/*08*/	WORD res08;	/* 0x56 */
/*0a*/	DWORD memid;	/* memid */
/*0e*/  WORD helpcontext;/* 0xfffe == no context, 0x0001 == stored in EnumInfo struct, else offset
			    to value from start of block group */
/*10*/	WORD helpstring;/* offset from start of block group to string offset */
} SLTG_EnumItem;

#define SLTG_ENUMITEM_MAGIC 0x120a

typedef struct {
	BYTE magic;	/* 0x4c, 0xcb or 0x8b with optional SLTG_FUNCTION_FLAGS_PRESENT flag */
	BYTE inv;	/* high nibble is INVOKE_KIND, low nibble = 2 */
	WORD next;	/* byte offset from beginning of group to next fn */
	WORD name;	/* Offset within name table to name */
	DWORD dispid;	/* dispid */
	WORD helpcontext; /* helpcontext (again 1 is special) */
	WORD helpstring;/* helpstring offset to offset */
	WORD arg_off;	/* offset to args from start of block */
	BYTE nacc;	/* lowest 3bits are CALLCONV, rest are no of args */
        BYTE retnextopt;/* if 0x80 bit set ret type follows else next WORD
			   is offset to ret type. No of optional args is
			   middle 6 bits */
	WORD rettype;	/* return type VT_?? or offset to ret type */
	WORD vtblpos;	/* position in vtbl? */
	WORD funcflags; /* present if magic & 0x20 */
/* Param list starts, repeat next two as required */
#if 0
	WORD  name;	/* offset to 2nd letter of name */
	WORD+ type;	/* VT_ of param */
#endif
} SLTG_Function;

#define SLTG_FUNCTION_FLAGS_PRESENT 0x20
#define SLTG_FUNCTION_MAGIC 0x4c
#define SLTG_DISPATCH_FUNCTION_MAGIC 0xcb
#define SLTG_STATIC_FUNCTION_MAGIC 0x8b

typedef struct {
/*00*/	BYTE magic;		/* 0xdf */
/*01*/  BYTE res01;		/* 0x00 */
/*02*/	DWORD res02;		/* 0xffffffff */
/*06*/	DWORD res06;		/* 0xffffffff */
/*0a*/	DWORD res0a;		/* 0xffffffff */
/*0e*/	DWORD res0e;		/* 0xffffffff */
/*12*/	DWORD res12;		/* 0xffffffff */
/*16*/	DWORD res16;		/* 0xffffffff */
/*1a*/	DWORD res1a;		/* 0xffffffff */
/*1e*/	DWORD res1e;		/* 0xffffffff */
/*22*/	DWORD res22;		/* 0xffffffff */
/*26*/	DWORD res26;		/* 0xffffffff */
/*2a*/	DWORD res2a;		/* 0xffffffff */
/*2e*/	DWORD res2e;		/* 0xffffffff */
/*32*/	DWORD res32;		/* 0xffffffff */
/*36*/	DWORD res36;		/* 0xffffffff */
/*3a*/	DWORD res3a;		/* 0xffffffff */
/*3e*/	DWORD res3e;		/* 0xffffffff */
/*42*/	WORD  res42;		/* 0xffff */
/*44*/	DWORD number;		/* this is 8 times the number of refs */
/*48*/	/* Now we have number bytes (8 for each ref) of SLTG_UnknownRefInfo */

/*50*/	WORD res50;		/* 0xffff */
/*52*/	BYTE res52;		/* 0x01 */
/*53*/	DWORD res53;		/* 0x00000000 */
/*57*/  SLTG_Name names[1];
  /*    Now we have number/8 SLTG_Names (first WORD is no of bytes in the ascii
   *    string).  Strings look like "*\Rxxxx*#n".  If xxxx == ffff then the
   *    ref refers to the nth type listed in this library (0 based).  Else
   *    the xxxx (which maybe fewer than 4 digits) is the offset into the name
   *    table to a string "*\G{<guid>}#1.0#0#C:\WINNT\System32\stdole32.tlb#"
   *    The guid is the typelib guid; the ref again refers to the nth type of
   *    the imported typelib.
   */

/*xx*/ BYTE resxx;		/* 0xdf */

} SLTG_RefInfo;

#define SLTG_REF_MAGIC 0xdf

typedef struct {
	WORD res00;	/* 0x0001 */
	BYTE res02;	/* 0x02 */
	BYTE res03;	/* 0x40 if internal ref, 0x00 if external ? */
	WORD res04;	/* 0xffff */
	WORD res06;	/* 0x0000, 0x0013 or 0xffff ?? */
} SLTG_UnknownRefInfo;

typedef struct {
  WORD res00; /* 0x004a */
  WORD next;  /* byte offs to next interface */
  WORD res04; /* 0xffff */
  BYTE impltypeflags; /* IMPLTYPEFLAG_* */
  BYTE res07; /* 0x80 */
  WORD res08; /* 0x0012, 0x0028 ?? */
  WORD ref;   /* number in ref table ? */
  WORD res0c; /* 0x4000 */
  WORD res0e; /* 0xfffe */
  WORD res10; /* 0xffff */
  WORD res12; /* 0x001d */
  WORD pos_in_table; /* 0x0, 0x4, ? */
} SLTG_ImplInfo;

#define SLTG_IMPL_MAGIC 0x004a

typedef struct {
  BYTE magic; /* 0x0a */
  BYTE flags;
  WORD next;
  WORD name;
  WORD byte_offs; /* pos in struct, or offset to const type or const data (if flags & 0x08) */
  WORD type; /* if flags & 0x02 this is the type, else offset to type */
  DWORD memid;
  WORD helpcontext; /* ?? */
  WORD helpstring; /* ?? */
  WORD varflags; /* only present if magic & 0x02 */
} SLTG_Variable;

#define SLTG_VAR_MAGIC 0x0a
#define SLTG_VAR_WITH_FLAGS_MAGIC 0x2a


/* CARRAYs look like this
WORD type == VT_CARRAY
WORD offset from start of block to SAFEARRAY
WORD typeofarray
*/

#include "poppack.h"

/* The OLE Automation ProxyStub Interface Class (aka Typelib Marshaler) */
DEFINE_OLEGUID( CLSID_PSDispatch,    0x00020420, 0x0000, 0x0000 );
DEFINE_OLEGUID( CLSID_PSEnumVariant, 0x00020421, 0x0000, 0x0000 );
DEFINE_OLEGUID( CLSID_PSTypeInfo,    0x00020422, 0x0000, 0x0000 );
DEFINE_OLEGUID( CLSID_PSTypeLib,     0x00020423, 0x0000, 0x0000 );
DEFINE_OLEGUID( CLSID_PSOAInterface, 0x00020424, 0x0000, 0x0000 );
DEFINE_OLEGUID( CLSID_PSTypeComp,    0x00020425, 0x0000, 0x0000 );

/*---------------------------END--------------------------------------------*/
#endif
