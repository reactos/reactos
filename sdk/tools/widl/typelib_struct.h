/*
 * typelib_struct.h  internal wine data structures
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
#ifndef _WIDL_TYPELIB_STRUCT_H
#define _WIDL_TYPELIB_STRUCT_H

#define HELPDLLFLAG (0x0100)
#define DO_NOT_SEEK (-1)

#define MSFT_HREFTYPE_INTHISFILE(href) (!((href) & 3))
#define MSFT_HREFTYPE_INDEX(href) ((href) /sizeof(MSFT_TypeInfoBase))

/*-------------------------FILE STRUCTURES-----------------------------------*/

/* There are two known file formats, those created with ICreateTypeLib
 * have the signature "SLTG" as their first four bytes, while those created
 * with ICreateTypeLib2 have "MSFT".
 */

#define MSFT_MAGIC 0x5446534d

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
typedef struct tagMSFT_Header {
/*0x00*/int   magic1;       /* 0x5446534D "MSFT" */
        int   magic2;       /* 0x00010002 version nr? */
        int   posguid;      /* position of libid in guid table  */
                            /* (should be,  else -1) */
        int   lcid;         /* locale id */
/*0x10*/int   lcid2;
        int   varflags;     /* (largely) unknown flags */
                            /* the lower nibble is syskind */
                            /* 0x40 always seems to be set */
                            /* 0x10 set with a helpfile defined */
                            /* 0x100 set with a helpstringdll defined - in this
                                  case the offset to the name in the stringtable
                                  appears right after this struct, before the
                                  typeinfo offsets */
        int   version;      /* set with SetVersion() */
        int   flags;        /* set with SetFlags() */
/*0x20*/int   nrtypeinfos;  /* number of typeinfo's (till so far) */
        int   helpstring;   /* position of help string in stringtable */
        int   helpstringcontext;
        int   helpcontext;
/*0x30*/int   nametablecount;   /* number of names in name table */
        int   nametablechars;   /* nr of characters in name table */
        int   NameOffset;       /* offset of name in name table */
        int   helpfile;         /* position of helpfile in stringtable */
/*0x40*/int   CustomDataOffset; /* if -1 no custom data, else it is offset */
                                /* in customer data/guid offset table */
        int   res44;            /* unknown always: 0x20 (guid hash size?) */
        int   res48;            /* unknown always: 0x80 (name hash size?) */
        int   dispatchpos;      /* HREFTYPE to IDispatch, or -1 if no IDispatch */
/*0x50*/int   nimpinfos;        /* number of impinfos */
} MSFT_Header;

/* segments in the type lib file have a structure like this: */
typedef struct tagMSFT_pSeg {
        int   offset;       /* absolute offset in file */
        int   length;       /* length of segment */
        int   res08;        /* unknown always -1 */
        int   res0c;        /* unknown always 0x0f in the header */
                            /* 0x03 in the typeinfo_data */
} MSFT_pSeg;

/* layout of the main segment directory */
typedef struct tagMSFT_SegDir {
/*1*/MSFT_pSeg pTypeInfoTab; /* each typeinfo gets an entry of 0x64 bytes */
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
/*000*/ int   typekind;             /*  it is the TKIND_xxx */
                                    /* some byte alignment stuff */
        int     memoffset;          /* points past the file, if no elements */
        int     res2;               /* zero if no element, N*0x40 */
        int     res3;               /* -1 if no element, (N-1)*0x38 */
/*010*/ int     res4;               /* always? 3 */
        int     res5;               /* always? zero */
        int     cElement;           /* counts elements, HI=cVars, LO=cFuncs */
        int     res7;               /* always? zero */
/*020*/ int     res8;               /* always? zero */
        int     res9;               /* always? zero */
        int     resA;               /* always? zero */
        int     posguid;            /* position in guid table */
/*030*/ int     flags;              /* Typeflags */
        int     NameOffset;         /* offset in name table */
        int     version;            /* element version */
        int     docstringoffs;      /* offset of docstring in string tab */
/*040*/ int     helpstringcontext;  /*  */
        int     helpcontext;    /* */
        int     oCustData;          /* offset in customer data table */
        short   cImplTypes;     /* nr of implemented interfaces */
        short   cbSizeVft;      /* virtual table size, including inherits */
/*050*/ int     size;           /* size in bytes, at least for structures */
        /* FIXME: name of this field */
        int     datatype1;      /* position in type description table */
                                /* or in base interfaces */
                                /* if coclass: offset in reftable */
                                /* if interface: reference to inherited if */
        int     datatype2;      /* for interfaces: hiword is num of inherited funcs */
                                /*                 loword is num of inherited interfaces */
        int     res18;          /* always? 0 */
/*060*/ int     res19;          /* always? -1 */
} MSFT_TypeInfoBase;

/* layout of an entry with information on imported types */
typedef struct tagMSFT_ImpInfo {
    int     flags;          /* bits 0 - 15:  count */
                            /* bit  16:      if set oGuid is an offset to Guid */
                            /*               if clear oGuid is a typeinfo index in the specified typelib */
                            /* bits 24 - 31: TKIND of reference */
    int     oImpFile;       /* offset in the Import File table */
    int     oGuid;          /* offset in Guid table or typeinfo index (see bit 16 of flags) */
} MSFT_ImpInfo;

#define MSFT_IMPINFO_OFFSET_IS_GUID 0x00010000

/* function description data */
typedef struct {
/*  int   recsize;       record size including some extra stuff */
    int   DataType;     /* data type of the member, eg return of function */
    int   Flags;        /* something to do with attribute flags (LOWORD) */
    short VtableOffset; /* offset in vtable */
    short funcdescsize; /* size of reconstituted FUNCDESC and related structs */
    int   FKCCIC;       /* bit string with the following  */
                        /* meaning (bit 0 is the lsb): */
                        /* bits 0 - 2: FUNCKIND */
                        /* bits 3 - 6: INVOKEKIND */
                        /* bit  7: custom data present */
                        /* bits 8 - 11: CALLCONV */
                        /* bit  12: parameters have default values */
                        /* bit  13: oEntry is numeric */
                        /* bit  14: has retval param */
                        /* bits 16 - 31: index of next function with same id */
    short nrargs;       /* number of arguments (including optional ????) */
    short nroargs;      /* nr of optional arguments */
    /* optional attribute fields, the number of them is variable */
    int   OptAttr[1];
/*
0*  int   helpcontext;
1*  int   oHelpString;
2*  int   oEntry;       // either offset in string table or numeric as it is (see bit 13 of FKCCIC) //
3*  int   res9;         // unknown (-1) //
4*  int   resA;         // unknown (-1) //
5*  int   HelpStringContext;
    // these are controlled by a bit set in the FKCCIC field  //
6*  int   oCustData;        // custom data for function //
7*  int   oArgCustData[1];  // custom data per argument //
*/
} MSFT_FuncRecord;

/* after this may follow an array with default value pointers if the
 * appropriate bit in the FKCCIC field has been set:
 * int   oDefaultValue[nrargs];
 */

    /* Parameter info one per argument*/
typedef struct {
        int   DataType;
        int   oName;
        int   Flags;
    } MSFT_ParameterInfo;

/* Variable description data */
typedef struct {
/*  int   recsize;      // record size including some extra stuff */
    int   DataType;     /* data type of the variable */
    int   Flags;        /* VarFlags (LOWORD) */
    short VarKind;      /* VarKind */
    short vardescsize;  /* size of reconstituted VARDESC and related structs */
    int   OffsValue;    /* value of the variable or the offset  */
                        /* in the data structure */
    /* optional attribute fields, the number of them is variable */
    /* controlled by record length */
    int   HelpContext;
    int   oHelpString;
    int   res9;         /* unknown (-1) */
    int   oCustData;        /* custom data for variable */
    int   HelpStringContext;

} MSFT_VarRecord;

/* Structure of the reference data  */
typedef struct {
    int   reftype;  /* either offset in type info table, then it's */
                    /* a multiple of 64 */
                    /* or offset in the external reference table */
                    /* with an offset of 1 */
    int   flags;
    int   oCustData;    /* custom data */
    int   onext;    /* next offset, -1 if last */
} MSFT_RefRecord;

/* this is how a guid is stored */
typedef struct {
    struct uuid guid;
    int   hreftype;     /* -2 for the typelib guid, typeinfo offset
			   for typeinfo guid, low two bits are 01 if
			   this is an imported typeinfo, low two bits
			   are 10 if this is an imported typelib (used
			   by imported typeinfos) */
    int   next_hash;    /* offset to next guid in the hash bucket */
} MSFT_GuidEntry;
/* some data preceding entries in the name table */
typedef struct {
    int   hreftype;     /* is -1 if name is for neither a typeinfo,
			   a variable, or a function (that is, name
			   is for a typelib or a function parameter).
			   otherwise is the offset of the first
			   typeinfo that this name refers to (either
			   to the typeinfo itself or to a member of
			   the typeinfo */
    int   next_hash;    /* offset to next name in the hash bucket */
    int   namelen;      /* only lower 8 bits are valid */
                        /* 0x1000 if name is only used once as a variable name */
                        /* 0x2000 if name is a variable in an enumeration */
                        /* 0x3800 if name is typeinfo name */
			/* upper 16 bits are hash code */
} MSFT_NameIntro;
/* the custom data table directory has entries like this */
typedef struct {
    int   GuidOffset;
    int   DataOffset;
    int   next;     /* next offset in the table, -1 if it's the last */
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
/*00*/	unsigned int   SLTG_magic;	/* 0x47544c53  == "SLTG" */
/*04*/	unsigned short nrOfFileBlks;	/* no of SLTG_BlkEntry's + 1 */
/*06*/  unsigned short res06;		/* ?? always 9 */
/*08*/  unsigned short res08;           /* some kind of len/offset ?? */
/*0a*/	unsigned short first_blk;	/* 1 based index into blk entries that
					   corresponds to first block in file */
/*0c*/	unsigned int   res0c;		/* always 0x000204ff */
/*10*/  unsigned int   res10;		/* always 0x00000000 */
/*14*/	unsigned int   res14;		/* always 0x000000c0 */
/*18*/	unsigned int   res18;		/* always 0x46000000 */
/*1c*/	unsigned int   res1c;		/* always 0x00000044 */
/*20*/	unsigned int   res20;		/* always 0xffff0000 */
} SLTG_Header;

/* This gets followed by a list of block entries */
typedef struct {
/*00*/  unsigned int len;
/*04*/	unsigned short index_string; /* offs from start of SLTG_Magic to index string */
/*06*/  unsigned short next;
} SLTG_BlkEntry;

/* The order of the blocks in the file is given by starting at Block
   entry first_blk and stepping through using the next pointer */

/* These then get followed by this magic */
typedef struct {
/*00*/ unsigned char res00;		/* always 0x01 */
/*01*/ char          CompObj_magic[8];	/* always "CompObj" */
/*09*/ char          dir_magic[4];	/* always "dir" */
} SLTG_Magic;

#define SLTG_COMPOBJ_MAGIC "CompObj"
#define SLTG_DIR_MAGIC "dir"

/* Next we have SLTG_Header.nrOfFileBlks - 2 of Index strings.  These
are presumably unique to within the file and look something like
"AAAAAAAAAA" with the first character incremented from 'A' to ensure
uniqueness.  I guess successive chars increment when we need to wrap
the first one. */

typedef struct {
/*00*/ char string[11];
} SLTG_Index;


/* This is followed by SLTG_pad9 */
typedef struct {
/*00*/ char pad[9];	/* 9 '\0's */
} SLTG_Pad9;


/* Now we have the noOfFileBlks - 1 worth of blocks. The length of
each block is given by its entry in SLTG_BlkEntry. */

/* type SLTG_NAME in rather like a BSTR except that the length in
bytes is given by the first WORD and the string contains 8bit chars */

typedef unsigned short SLTG_Name;

/* The main library block looks like this.  This one seems to come last */

typedef struct {
/*00*/	unsigned short magic;		/* 0x51cc */
/*02*/  unsigned short res02;		/* 0x0003, 0x0004 */
/*04*/  unsigned short name;            /* offset to name in name table */
/*06*/  SLTG_Name      res06;		/* maybe this is just WORD == 0xffff */
	SLTG_Name      helpstring;
	SLTG_Name      helpfile;
	unsigned int   helpcontext;
	unsigned short syskind;		/* == 1 for win32, 0 for win16 */
	unsigned short lcid;		/* == 0x409, 0x809 etc */
	unsigned int   res12;		/* == 0 */
	unsigned short libflags;	/* LIBFLAG_* */
	unsigned short maj_vers;
	unsigned short min_vers;
	struct uuid uuid;
} SLTG_LibBlk;

#define SLTG_LIBBLK_MAGIC 0x51cc

/* we then get 0x40 bytes worth of 0xffff or small numbers followed by
   nrOfFileBlks - 2 of these */
typedef struct {
    unsigned short small_no;
    SLTG_Name      index_name; /* This refers to a name in the directory */
    SLTG_Name      other_name; /* Another one of these weird names */
    unsigned short res1a;	   /* 0xffff */
    unsigned short name_offs;  /* offset to name in name table */
    unsigned short more_bytes; /* if this is non-zero we get this many
                                  bytes before the next element, which seem
                                  to reference the docstring of the type ? */
    unsigned short res20;      /* 0xffff */
    unsigned int   helpcontext;
    unsigned short res26;      /* 0xffff */
    struct uuid uuid;
} SLTG_OtherTypeInfo;

/* Next we get WORD 0x0003 followed by a DWORD which if we add to
0x216 gives the offset to the name table from the start of the LibBlk
struct */

typedef struct {
/*00*/	unsigned short magic;		/* 0x0501 */
/*02*/	unsigned int   href_table;	/* if not 0xffffffff, then byte offset from
					   beginning of struct to href table */
/*06*/	unsigned int   res06;		/* 0xffffffff */
/*0a*/	unsigned int   elem_table;	/* offset to members */
/*0e*/	unsigned int   res0e;		/* 0xffffffff */
/*12*/	unsigned short major_version;	/* major version number */
/*14*/  unsigned short minor_version;	/* minor version number */
/*16*/	unsigned int   res16;	/* 0xfffe0000 */
/*1a*/	unsigned char typeflags1;/* 0x02 | top 5 bits hold l5sbs of TYPEFLAGS */
/*1b*/	unsigned char typeflags2;/* TYPEFLAGS >> 5 */
/*1c*/	unsigned char typeflags3;/* 0x02*/
/*1d*/	unsigned char typekind;	/* 0x03 == TKIND_INTERFACE etc. */
/*1e*/  unsigned int   res1e;	/* 0x00000000 or 0xffffffff */
} SLTG_TypeInfoHeader;

#define SLTG_TIHEADER_MAGIC 0x0501

typedef struct {
/*00*/  unsigned short cFuncs;
/*02*/  unsigned short cVars;
/*04*/  unsigned short cImplTypes;
/*06*/  unsigned short res06;
/*08*/  unsigned short res08;
/*0a*/  unsigned short res0a;
/*0c*/  unsigned short res0c;
/*0e*/  unsigned short res0e;
/*10*/  unsigned short res10;
/*12*/  unsigned short res12;
/*14*/  unsigned short tdescalias_vt; /* for TKIND_ALIAS */
/*16*/  unsigned short res16;
/*18*/  unsigned short res18;
/*1a*/  unsigned short res1a;
/*1c*/  unsigned short res1c;
/*1e*/  unsigned short res1e;
/*20*/  unsigned short cbSizeInstance;
/*22*/  unsigned short cbAlignment;
/*24*/  unsigned short res24;
/*26*/  unsigned short res26;
/*28*/  unsigned short cbSizeVft;
/*2a*/  unsigned short res2a;
/*2c*/  unsigned short res2c;
/*2e*/  unsigned short res2e;
/*30*/  unsigned short res30;
/*32*/  unsigned short res32;
/*34*/  unsigned short res34;
} SLTG_TypeInfoTail;

typedef struct {
/*00*/ unsigned short res00; /* 0x0001 sometimes 0x0003 ?? */
/*02*/ unsigned short res02; /* 0xffff */
/*04*/ unsigned char res04; /* 0x01 */
/*05*/ unsigned int cbExtra; /* No of bytes that follow */
} SLTG_MemberHeader;

typedef struct {
/*00*/	unsigned short magic;	/* 0x120a */
/*02*/	unsigned short next;	/* offset in bytes to next block from start of block
	                           group, 0xffff if last item */
/*04*/	unsigned short name;	/* offset to name within name table */
/*06*/	unsigned short value;	/* offset to value from start of block group */
/*08*/	unsigned short res08;	/* 0x56 */
/*0a*/	unsigned int   memid;	/* memid */
/*0e*/  unsigned short helpcontext;/* 0xfffe == no context, 0x0001 == stored in EnumInfo struct, else offset
				    to value from start of block group */
/*10*/	unsigned short helpstring;/* offset from start of block group to string offset */
} SLTG_EnumItem;

#define SLTG_ENUMITEM_MAGIC 0x120a

typedef struct {
/*00*/	unsigned short vt;	/* vartype, 0xffff marks end. */
/*02*/	unsigned short res02;	/* ?, 0xffff marks end */
} SLTG_AliasItem;

#define SLTG_ALIASITEM_MAGIC 0x001d


typedef struct {
	unsigned char  magic;	/* 0x4c or 0x6c */
	unsigned char  inv;	/* high nibble is INVOKE_KIND, low nibble = 2 */
	unsigned short next;	/* byte offset from beginning of group to next fn */
	unsigned short name;	/* Offset within name table to name */
	unsigned int   dispid;	/* dispid */
	unsigned short helpcontext; /* helpcontext (again 1 is special) */
	unsigned short helpstring;/* helpstring offset to offset */
	unsigned short arg_off;	/* offset to args from start of block */
	unsigned char  nacc;	/* lowest 3bits are CALLCONV, rest are no of args */
        unsigned char  retnextopt;/* if 0x80 bit set ret type follows else next WORD
			   is offset to ret type. No of optional args is
			   middle 6 bits */
	unsigned short rettype;	/* return type VT_?? or offset to ret type */
	unsigned short vtblpos;	/* position in vtbl? */
	unsigned short funcflags; /* present if magic == 0x6c */
/* Param list starts, repeat next two as required */
#if 0
	unsigned short name;	/* offset to 2nd letter of name */
	unsigned short+ type;	/* VT_ of param */
#endif
} SLTG_Function;

#define SLTG_FUNCTION_MAGIC 0x4c
#define SLTG_FUNCTION_WITH_FLAGS_MAGIC 0x6c

typedef struct {
/*00*/	unsigned char  magic;		/* 0xdf */
/*01*/  unsigned char  res01;		/* 0x00 */
/*02*/	unsigned int   res02;		/* 0xffffffff */
/*06*/	unsigned int   res06;		/* 0xffffffff */
/*0a*/	unsigned int   res0a;		/* 0xffffffff */
/*0e*/	unsigned int   res0e;		/* 0xffffffff */
/*12*/	unsigned int   res12;		/* 0xffffffff */
/*16*/	unsigned int   res16;		/* 0xffffffff */
/*1a*/	unsigned int   res1a;		/* 0xffffffff */
/*1e*/	unsigned int   res1e;		/* 0xffffffff */
/*22*/	unsigned int   res22;		/* 0xffffffff */
/*26*/	unsigned int   res26;		/* 0xffffffff */
/*2a*/	unsigned int   res2a;		/* 0xffffffff */
/*2e*/	unsigned int   res2e;		/* 0xffffffff */
/*32*/	unsigned int   res32;		/* 0xffffffff */
/*36*/	unsigned int   res36;		/* 0xffffffff */
/*3a*/	unsigned int   res3a;		/* 0xffffffff */
/*3e*/	unsigned int   res3e;		/* 0xffffffff */
/*42*/	unsigned short res42;		/* 0xffff */
/*44*/	unsigned int   number;		/* this is 8 times the number of refs */
/*48*/	/* Now we have number bytes (8 for each ref) of SLTG_UnknownRefInfo */

/*50*/	unsigned short res50;		/* 0xffff */
/*52*/	unsigned char  res52;		/* 0x01 */
/*53*/	unsigned int   res53;		/* 0x00000000 */
/*57*/  SLTG_Name      names[1];
  /*    Now we have number/8 SLTG_Names (first WORD is no of bytes in the ascii
   *    string).  Strings look like "*\Rxxxx*#n".  If xxxx == ffff then the
   *    ref refers to the nth type listed in this library (0 based).  Else
   *    the xxxx (which maybe fewer than 4 digits) is the offset into the name
   *    table to a string "*\G{<guid>}#1.0#0#C:\WINNT\System32\stdole32.tlb#"
   *    The guid is the typelib guid; the ref again refers to the nth type of
   *    the imported typelib.
   */

/*xx*/  unsigned char  resxx;		/* 0xdf */

} SLTG_RefInfo;

#define SLTG_REF_MAGIC 0xdf

typedef struct {
	unsigned short res00;	/* 0x0001 */
	unsigned char  res02;	/* 0x02 */
	unsigned char  res03;	/* 0x40 if internal ref, 0x00 if external ? */
	unsigned short res04;	/* 0xffff */
	unsigned short res06;	/* 0x0000, 0x0013 or 0xffff ?? */
} SLTG_UnknownRefInfo;

typedef struct {
    unsigned short res00; /* 0x004a */
    unsigned short next;  /* byte offs to next interface */
    unsigned short res04; /* 0xffff */
    unsigned char  impltypeflags; /* IMPLTYPEFLAG_* */
    unsigned char  res07; /* 0x80 */
    unsigned short res08; /* 0x0012, 0x0028 ?? */
    unsigned short ref;   /* number in ref table ? */
    unsigned short res0c; /* 0x4000 */
    unsigned short res0e; /* 0xfffe */
    unsigned short res10; /* 0xffff */
    unsigned short res12; /* 0x001d */
    unsigned short pos_in_table; /* 0x0, 0x4, ? */
} SLTG_ImplInfo;

#define SLTG_IMPL_MAGIC 0x004a

typedef struct {
    unsigned char  magic; /* 0x0a */
    unsigned char  typepos;
    unsigned short next;
    unsigned short name;
    unsigned short byte_offs; /* pos in struct */
    unsigned short type; /* if typepos == 0x02 this is the type, else offset to type */
    unsigned int   memid;
    unsigned short helpcontext; /* ?? */
    unsigned short helpstring; /* ?? */
} SLTG_RecordItem;

#define SLTG_RECORD_MAGIC 0x0a


/* CARRAYs look like this
WORD type == VT_CARRAY
WORD offset from start of block to SAFEARRAY
WORD typeofarray
*/

#include "poppack.h"

/*---------------------------END--------------------------------------------*/
#endif
