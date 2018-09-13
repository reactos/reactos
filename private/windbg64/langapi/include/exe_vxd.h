/*
 *  Title
 *
 *	exe386.h
 *	Wieslaw Kalkus
 *	(C) Copyright Microsoft Corp 1988-1992
 *	5 August 1988
 *
 *  Description
 *
 *	Data structure definitions for the OS/2
 *	executable file format (flat model).
 *
 *  Modification History
 *
 *	88/08/05	Wieslaw Kalkus	Initial version
 */


#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif


    /*_________________________________________________________________*
     |                                                                 |
     |                                                                 |
     |	OS/2 .EXE FILE HEADER DEFINITION - 386 version 0:32	       |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */


#define BITPERWORD	16
#define BITPERBYTE	8
#define OBJPAGELEN	4096
#define E32MAGIC1	'L'		/* New magic number  "LE" */
#define E32MAGIC2	'E'		/* New magic number  "LE" */
#define E32MAGIC        0x454c          /* New magic number  "LE" */
#define E32RESBYTES1    0               /* First bytes reserved */
#define E32RESBYTES2	0		/* Second bytes reserved */
#define E32RESBYTES3	24		/* Third bytes reserved */
#define E32LEBO 	0x00		/* Little Endian Byte Order */
#define E32BEBO 	0x01		/* Big Endian Byte Order */
#define E32LEWO 	0x00		/* Little Endian Word Order */
#define E32BEWO 	0x01		/* Big Endian Word Order */
#define E32LEVEL	0L		/* 32-bit EXE format level */
#define E32CPU286	0x001		/* Intel 80286 or upwardly compatibile */
#define E32CPU386	0x002		/* Intel 80386 or upwardly compatibile */
#define E32CPU486	0x003		/* Intel 80486 or upwardly compatibile */



struct e32_exe {			/* New 32-bit .EXE header */
    unsigned char	e32_magic[2];	/* Magic number E32_MAGIC */
    unsigned char	e32_border;	/* The byte ordering for the .EXE */
    unsigned char	e32_worder;	/* The word ordering for the .EXE */
    unsigned long	e32_level;	/* The EXE format level for now = 0 */
    unsigned short	e32_cpu;	/* The CPU type */
    unsigned short	e32_os; 	/* The OS type */
    unsigned long	e32_ver;	/* Module version */
    unsigned long	e32_mflags;	/* Module flags */
    unsigned long	e32_mpages;	/* Module # pages */
    unsigned long	e32_startobj;	/* Object # for instruction pointer */
    unsigned long	e32_eip;	/* Extended instruction pointer */
    unsigned long	e32_stackobj;	/* Object # for stack pointer */
    unsigned long	e32_esp;	/* Extended stack pointer */
    unsigned long	e32_pagesize;	/* .EXE page size */
    unsigned long       e32_lastpagesize;/* Last page size in .EXE */
    unsigned long	e32_fixupsize;	/* Fixup section size */
    unsigned long	e32_fixupsum;	/* Fixup section checksum */
    unsigned long	e32_ldrsize;	/* Loader section size */
    unsigned long	e32_ldrsum;	/* Loader section checksum */
    unsigned long	e32_objtab;	/* Object table offset */
    unsigned long	e32_objcnt;	/* Number of objects in module */
    unsigned long	e32_objmap;	/* Object page map offset */
    unsigned long	e32_itermap;	/* Object iterated data map offset */
    unsigned long	e32_rsrctab;	/* Offset of Resource Table */
    unsigned long	e32_rsrccnt;	/* Number of resource entries */
    unsigned long	e32_restab;	/* Offset of resident name table */
    unsigned long	e32_enttab;	/* Offset of Entry Table */
    unsigned long	e32_dirtab;	/* Offset of Module Directive Table */
    unsigned long	e32_dircnt;	/* Number of module directives */
    unsigned long	e32_fpagetab;	/* Offset of Fixup Page Table */
    unsigned long	e32_frectab;	/* Offset of Fixup Record Table */
    unsigned long	e32_impmod;	/* Offset of Import Module Name Table */
    unsigned long	e32_impmodcnt;	/* Number of entries in Import Module Name Table */
    unsigned long	e32_impproc;	/* Offset of Import Procedure Name Table */
    unsigned long	e32_pagesum;	/* Offset of Per-Page Checksum Table */
    unsigned long	e32_datapage;	/* Offset of Enumerated Data Pages */
    unsigned long	e32_preload;	/* Number of preload pages */
    unsigned long	e32_nrestab;	/* Offset of Non-resident Names Table */
    unsigned long	e32_cbnrestab;	/* Size of Non-resident Name Table */
    unsigned long	e32_nressum;	/* Non-resident Name Table Checksum */
    unsigned long	e32_autodata;	/* Object # for automatic data object */
    unsigned long	e32_debuginfo;	/* Offset of the debugging information */
    unsigned long	e32_debuglen;	/* The length of the debugging info. in bytes */
    unsigned long	e32_instpreload;/* Number of instance pages in preload section of .EXE file */
    unsigned long	e32_instdemand; /* Number of instance pages in demand load section of .EXE file */
    unsigned long	e32_heapsize;	/* Size of heap - for 16-bit apps */
    unsigned char	e32_res3[E32RESBYTES3 - 4 - 8];
					/* Pad structure to 192 bytes */
    unsigned long	e32_winresoff ;
    unsigned long	e32_winreslen ;
    unsigned short	Dev386_Device_ID;
					/* Device ID for VxD */
    unsigned short	Dev386_DDK_Version;
					/* DDK version for VxD */
};



#define E32_MAGIC1(x)	    (x).e32_magic[0]
#define E32_MAGIC2(x)	    (x).e32_magic[1]
#define E32_BORDER(x)	    (x).e32_border
#define E32_WORDER(x)	    (x).e32_worder
#define E32_LEVEL(x)	    (x).e32_level
#define E32_CPU(x)	    (x).e32_cpu
#define E32_OS(x)	    (x).e32_os
#define E32_VER(x)	    (x).e32_ver
#define E32_MFLAGS(x)	    (x).e32_mflags
#define E32_MPAGES(x)	    (x).e32_mpages
#define E32_STARTOBJ(x)     (x).e32_startobj
#define E32_EIP(x)	    (x).e32_eip
#define E32_STACKOBJ(x)     (x).e32_stackobj
#define E32_ESP(x)	    (x).e32_esp
#define E32_PAGESIZE(x)     (x).e32_pagesize
#define E32_LASTPAGESIZE(x) (x).e32_lastpagesize
#define E32_FIXUPSIZE(x)    (x).e32_fixupsize
#define E32_FIXUPSUM(x)     (x).e32_fixupsum
#define E32_LDRSIZE(x)	    (x).e32_ldrsize
#define E32_LDRSUM(x)	    (x).e32_ldrsum
#define E32_OBJTAB(x)	    (x).e32_objtab
#define E32_OBJCNT(x)	    (x).e32_objcnt
#define E32_OBJMAP(x)	    (x).e32_objmap
#define E32_ITERMAP(x)	    (x).e32_itermap
#define E32_RSRCTAB(x)	    (x).e32_rsrctab
#define E32_RSRCCNT(x)	    (x).e32_rsrccnt
#define E32_RESTAB(x)	    (x).e32_restab
#define E32_ENTTAB(x)	    (x).e32_enttab
#define E32_DIRTAB(x)	    (x).e32_dirtab
#define E32_DIRCNT(x)	    (x).e32_dircnt
#define E32_FPAGETAB(x)     (x).e32_fpagetab
#define E32_FRECTAB(x)	    (x).e32_frectab
#define E32_IMPMOD(x)	    (x).e32_impmod
#define E32_IMPMODCNT(x)    (x).e32_impmodcnt
#define E32_IMPPROC(x)	    (x).e32_impproc
#define E32_PAGESUM(x)	    (x).e32_pagesum
#define E32_DATAPAGE(x)     (x).e32_datapage
#define E32_PRELOAD(x)	    (x).e32_preload
#define E32_NRESTAB(x)	    (x).e32_nrestab
#define E32_CBNRESTAB(x)    (x).e32_cbnrestab
#define E32_NRESSUM(x)	    (x).e32_nressum
#define E32_AUTODATA(x)     (x).e32_autodata
#define E32_DEBUGINFO(x)    (x).e32_debuginfo
#define E32_DEBUGLEN(x)     (x).e32_debuglen
#define E32_INSTPRELOAD(x)  (x).e32_instpreload
#define E32_INSTDEMAND(x)   (x).e32_instdemand
#define E32_HEAPSIZE(x)     (x).e32_heapsize



/*
 *  Format of E32_MFLAGS(x):
 *
 *  Low word has the following format:
 *
 *  15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0  - bit no
 *   |	   |	    | | |     | |   |
 *   |	   |	    | | |     | |   +------- Per-Process Library Initialization
 *   |	   |	    | | |     | +----------- No Internal Fixups for Module in .EXE
 *   |	   |	    | | |     +------------- No External Fixups for Module in .EXE
 *   |	   |	    | | +------------------- Incompatible with PM Windowing
 *   |	   |	    | +--------------------- Compatible with PM Windowing
 *   |	   |	    +----------------------- Uses PM Windowing API
 *   |	   +-------------------------------- Module not Loadable
 *   +-------------------------------------- Library Module
 */


#define E32NOTP 	 0x8000L	/* Library Module - used as NENOTP */
#define E32NOLOAD	 0x2000L	/* Module not Loadable */
#define E32PMAPI	 0x0300L	/* Uses PM Windowing API */
#define E32PMW		 0x0200L	/* Compatible with PM Windowing */
#define E32NOPMW	 0x0100L	/* Incompatible with PM Windowing */
#define E32NOEXTFIX	 0x0020L	/* NO External Fixups in .EXE */
#define E32NOINTFIX	 0x0010L	/* NO Internal Fixups in .EXE */
#define E32LIBINIT	 0x0004L	/* Per-Process Library Initialization */
#define E32APPMASK	 0x0700L	/* Aplication Type Mask */


/*
 *  Format of E32_MFLAGS(x):
 *
 *  High word has the following format:
 *
 *  15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0  - bit no
 *				      | |
 *				      | +--- Protected memory library module
 *				      +----- Device driver
 */

#define E32PROTDLL	 0x10000L	// Protected memory library module
#define E32DEVICE	 0x20000L	// Device driver
#define E32MODEXE	 0x00000L	// .EXE module
#define E32MODDLL	 0x08000L	// .DLL module
#define E32MODPROTDLL	 0x18000L	// Protected memory library module
#define E32MODPDEV	 0x20000L	// Physical device driver
#define E32MODVDEV	 0x28000L	// Virtual device driver
#define E32MODVDEVDYN	 0x38000L	// Virtual device driver (dynamic)
#define E32MODMASK	 0x38000L	// Module type mask

/*
 *  RELOCATION DEFINITIONS - RUN-TIME FIXUPS
 */


#pragma pack(1) 			/* This data must be packed */


typedef union _offset
{
    unsigned short offset16;
    unsigned long  offset32;
}
    offset;				/* 16-bit or 32-bit offset */


/***ET+	r32_rlc - Relocation item */

struct r32_rlc {			/* Relocation item */
    unsigned char nr_stype;		/* Source type - shared with new_rlc */
    unsigned char nr_flags;		/* Flag byte - shared with new_rlc */
    short r32_soff;			/* Source page offset */
    unsigned short r32_objmod;		/* Target obj. no. or Module ordinal */

    union targetid {			/* BEGIN UNION */
	unsigned long intref;		/* Internal fixup offset */
	unsigned long proc;		/* Procedure name offset */
	unsigned long ord;	 	/* Procedure ordinal */
    } r32_target;			/* END UNION */
    unsigned long addval;		/* Value added to the address */
    unsigned short r32_srccount;	/* Number of chained fixup records */
    unsigned short r32_chain;		/* Chain head */
};


#pragma pack()				/* Stop packing */


/*
 *  In 32-bit .EXE file run-time relocations are written as varying size
 *  records, so we need many size definitions.
 */

#define RINTSIZE16	8
#define RINTSIZE32	10
#define RORDSIZE	8
#define RNAMSIZE16	8
#define RNAMSIZE32	10
#define RADDSIZE16	10
#define RADDSIZE32	12



#if FALSE
/*
 *  Access macros defined in NEWEXE.H !!!
 */
#define NR_STYPE(x)	 (x).nr_stype
#define NR_FLAGS(x)	 (x).nr_flags
#endif

#define R32_SOFF(x)	 (x).r32_soff
#define R32_OBJNO(x)	 (x).r32_objmod
#define R32_MODORD(x)	 (x).r32_objmod
#define R32_OFFSET(x)    (x).r32_target.intref
#define R32_PROCOFF(x)   (x).r32_target.proc
#define R32_PROCORD(x)	 (x).r32_target.ord
#define R32_ADDVAL(x)    (x).addval
#define R32_SRCCNT(x)	 (x).r32_srccount
#define R32_CHAIN(x)	 (x).r32_chain



/*
 *  Format of NR_STYPE(x)
 *
 *	 7 6 5 4 3 2 1 0  - bit no
 *	     | | | | | |
 *	     | | +-+-+-+--- Source type
 *	     | +----------- Fixup to 16:16 alias
 *	     +------------- List of source offset follows fixup record
 */

#if FALSE

	    /* DEFINED in newexe.h !!! */

#define NRSTYP		0x0f		/* Source type mask */
#define NRSBYT		0x00		/* lo byte (8-bits)*/
#define NRSSEG		0x02		/* 16-bit segment (16-bits) */
#define NRSPTR		0x03		/* 16:16 pointer (32-bits) */
#define NRSOFF		0x05		/* 16-bit offset (16-bits) */
#define NRPTR48 	0x06		/* 16:32 pointer (48-bits) */
#define NROFF32 	0x07		/* 32-bit offset (32-bits) */
#define NRSOFF32	0x08		/* 32-bit self-relative offset (32-bits) */
#endif


#define NRSRCMASK	0x0f		/* Source type mask */
#define NRALIAS 	0x10		/* Fixup to alias */
#define NRCHAIN 	0x20		/* List of source offset follows */
					/* fixup record, source offset field */
					/* in fixup record contains number */
					/* of elements in list */

/*
 *  Format of NR_FLAGS(x) and R32_FLAGS(x):
 *
 *	 7 6 5 4 3 2 1 0  - bit no
 *	 | | | |   | | |
 *	 | | | |   | +-+--- Reference type
 *	 | | | |   +------- Additive fixup
 *	 | | | +----------- 32-bit Target Offset Flag (1 - 32-bit; 0 - 16-bit)
 *	 | | +------------- 32-bit Additive Flag (1 - 32-bit; 0 - 16-bit)
 *	 | +--------------- 16-bit Object/Module ordinal (1 - 16-bit; 0 - 8-bit)
 *	 +----------------- 8-bit import ordinal (1 - 8-bit;
 *						  0 - NR32BITOFF toggles
 *						      between 16 and 32 bit
 *						      ordinal)
 */

#if FALSE

	    /* DEFINED in newexe.h !!! */

#define NRADD		0x04		/* Additive fixup */
#define NRRTYP		0x03		/* Reference type mask */
#define NRRINT		0x00		/* Internal reference */
#define NRRORD		0x01		/* Import by ordinal */
#define NRRNAM		0x02		/* Import by name */
#endif

#define NRRENT		0x03		/* Internal entry table fixup */

#define NR32BITOFF	0x10		/* 32-bit Target Offset */
#define NR32BITADD	0x20		/* 32-bit Additive fixup */
#define NR16OBJMOD	0x40		/* 16-bit Object/Module ordinal */
#define NR8BITORD	0x80		/* 8-bit import ordinal */
/*end*/

/*
 *  Data structures for storing run-time fixups in linker virtual memory.
 *
 *  Each object has a list of Object Page Directories which specify
 *  fixups for given page. Each page has its own hash table which is
 *  used to detect fixups to the same target.
 */

#define PAGEPERDIR	62
#define LG2DIR		7


typedef struct _OBJPAGEDIR
{
    DWORD   next;			/* Virtual pointer to next dir on list */
    WORD    ht[PAGEPERDIR];		/* Pointers to individual hash tables */
}
    OBJPAGEDIR;



/*
 *  OBJECT TABLE
 */

/***ET+	o32_obj Object Table Entry */

struct o32_obj				/* Flat .EXE object table entry */
{
    unsigned long	o32_size;	/* Object virtual size */
    unsigned long	o32_base;	/* Object base virtual address */
    unsigned long	o32_flags;	/* Attribute flags */
    unsigned long	o32_pagemap;	/* Object page map index */
    unsigned long	o32_mapsize;	/* Number of entries in object page map */
    unsigned long	o32_reserved;	/* Reserved */
};


#define O32_SIZE(x)	(x).o32_size
#define O32_BASE(x)	(x).o32_base
#define O32_FLAGS(x)	(x).o32_flags
#define O32_PAGEMAP(x)	(x).o32_pagemap
#define O32_MAPSIZE(x)	(x).o32_mapsize
#define O32_RESERVED(x) (x).o32_reserved



/*
 *  Format of O32_FLAGS(x)
 *
 *  High word of dword flag field is not used for now.
 *  Low word has the following format:
 *
 *  15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0  - bit no
 *   |	|  |  |     | | | | | | | | | | |
 *   |	|  |  |     | | | | | | | | | | +--- Readable Object
 *   |	|  |  |     | | | | | | | | | +----- Writeable Object
 *   |	|  |  |     | | | | | | | | +------- Executable Object
 *   |	|  |  |     | | | | | | | +--------- Resource Object
 *   |	|  |  |     | | | | | | +----------- Object is Discardable
 *   |	|  |  |     | | | | | +------------- Object is Shared
 *   |	|  |  |     | | | | +--------------- Object has preload pages
 *   |	|  |  |     | | | +----------------- Object has invalid pages
 *   |	|  |  |     | | +------------------- Object is permanent and swappable
 *   |	|  |  |     | +--------------------- Object is permanent and resident
 *   |	|  |  |     +----------------------- Object is permanent and long lockable
 *   |	|  |  +----------------------------- 16:16 alias required (80x86 specific)
 *   |	|  +-------------------------------- Big/Default bit setting (80x86 specific)
 *   |	+----------------------------------- Object is conforming for code (80x86 specific)
 *   +-------------------------------------- Object I/O privilege level (80x86 specific)
 *
 */

#define OBJREAD 	0x0001L 	    /* Readable Object	 */
#define OBJWRITE	0x0002L 	    /* Writeable Object  */
#define OBJRSRC 	0x0008L 	    /* Resource Object	 */
#define OBJINVALID	0x0080L 	    /* Object has invalid pages  */
#define OBJNONPERM	0x0600L 	    /* Object is nonpermanent - should be */
					    /* zero in the .EXE but internally we use 6 */
#define OBJPERM 	0x0100L 	    /* Object is permanent and swappable */
#define OBJRESIDENT	0x0200L 	    /* Object is permanent and resident */
#define OBJCONTIG	0x0300L 	    /* Object is resident and contiguous */
#define OBJDYNAMIC	0x0400L 	    /* Object is permanent and long locable */
#define OBJTYPEMASK	0x0700L 	    /* Object type mask */
#define OBJALIAS16	0x1000L 	    /* 16:16 alias required (80x86 specific)	       */
#define OBJBIGDEF	0x2000L 	    /* Big/Default bit setting (80x86 specific)        */
#define OBJIOPL 	0x8000L 	    /* Object I/O privilege level (80x86 specific)     */

#define OBJDISCARD	 0x0010L	    /* Object is Discardable */
#define OBJSHARED	 0x0020L	    /* Object is Shared */
#define OBJPRELOAD	 0x0040L	    /* Object has preload pages  */
#define OBJEXEC 	 0x0004L	    /* Executable Object */
#define OBJCONFORM	 0x4000L	    /* Object is conforming for code (80x86 specific)  */

/*
 *  Life will be easier, if we keep the same names for the following flags:
 */
#define NSDISCARD	OBJDISCARD 	    /* Object is Discardable */
#define NSMOVE		NSDISCARD	    /* Moveable object is for sure Discardable */
#define NSSHARED	OBJSHARED 	    /* Object is Shared */
#define NSPRELOAD	OBJPRELOAD 	    /* Object has preload pages  */
#define NSEXRD		OBJEXEC 	    /* Executable Object */
#define NSCONFORM	OBJCONFORM 	    /* Object is conforming for code (80x86 specific)  */
/*end*/

/***ET+	o32_map - Object Page Map entry */

struct o32_map				    /* Object Page Map entry */
{
    unsigned char   o32_pageidx[3];	    /* 24-bit page # in .EXE file */
    unsigned char   o32_pageflags;	    /* Per-Page attributes */
};


#define GETPAGEIDX(x)	((((unsigned long)((x).o32_pageidx[0])) << BITPERWORD) + \
			 (((x).o32_pageidx[1]) << BITPERBYTE) + \
			   (x).o32_pageidx[2])

#define PUTPAGEIDX(x,i) ((x).o32_pageidx[0] = (unsigned char) ((unsigned long)(i) >> BITPERWORD), \
			 (x).o32_pageidx[1] = (unsigned char) ((i) >> BITPERBYTE), \
			 (x).o32_pageidx[2] = (unsigned char) ((i) &  0xff))

#define PAGEFLAGS(x)	(x).o32_pageflags


#define VALID		0x00		    /* Valid Physical Page in .EXE */
#define ITERDATA	0x01		    /* Iterated Data Page */
#define INVALID 	0x02		    /* Invalid Page */
#define ZEROED		0x03		    /* Zero Filled Page */
#define RANGE		0x04		    /* Range of pages */
/*end*/

/*
 *  RESOURCE TABLE
 */

/***ET+	rsrc32 - Resource Table Entry */

struct rsrc32				    /* Resource Table Entry */
{
    unsigned short	type;		    /* Resource type */
    unsigned short	name;		    /* Resource name */
    unsigned long	cb;		    /* Resource size */
    unsigned short	obj;		    /* Object number */
    unsigned long	offset; 	    /* Offset within object */
};
/*end*/


#pragma pack(1) 			/* This data must be packed */

/*
 *  ENTRY TABLE DEFINITIONS
 */

/***ET+	b32_bundle - Entry Table */

struct b32_bundle
{
    unsigned char	b32_cnt;	/* Number of entries in this bundle */
    unsigned char	b32_type;	/* Bundle type */
    unsigned short	b32_obj;	/* Object number */
};					/* Follows entry types */

struct e32_entry
{
    unsigned char	e32_flags;	/* Entry point flags */
    union entrykind
    {
	offset		e32_offset;	/* 16-bit/32-bit offset entry */
	struct
	{
	    unsigned short offset;	/* Offset in segment */
	    unsigned short callgate;	/* Callgate selector */
	}
			e32_callgate;	/* 286 (16-bit) call gate */
	struct
	{
	    unsigned short  modord;	/* Module ordinal number */
	    unsigned long   value;	/* Proc name offset or ordinal */
	}
			e32_fwd;	/* Forwarder */
    }
			e32_variant;	/* Entry variant */
};

#pragma pack()				/* Stop packing */


#define B32_CNT(x)	(x).b32_cnt
#define B32_TYPE(x)	(x).b32_type
#define B32_OBJ(x)	(x).b32_obj

#define E32_EFLAGS(x)	(x).e32_flags
#define E32_OFFSET16(x) (x).e32_variant.e32_offset.offset16
#define E32_OFFSET32(x) (x).e32_variant.e32_offset.offset32
#define E32_GATEOFF(x)	(x).e32_variant.e32_callgate.offset
#define E32_GATE(x)	(x).e32_variant.e32_callgate.callgate
#define E32_MODORD(x)	(x).e32_variant.e32_fwd.modord
#define E32_VALUE(x)	(x).e32_variant.e32_fwd.value

#define FIXENT16	3
#define FIXENT32	5
#define GATEENT16	5
#define FWDENT		7

/*
 *  BUNDLE TYPES
 */

#define EMPTY	     0x00		/* Empty bundle */
#define ENTRY16      0x01		/* 16-bit offset entry point */
#define GATE16	     0x02		/* 286 call gate (16-bit IOPL) */
#define ENTRY32      0x03		/* 32-bit offset entry point */
#define ENTRYFWD     0x04		/* Forwarder entry point */
#define TYPEINFO     0x80		/* Typing information present flag */


/*
 *  Format for E32_EFLAGS(x)
 *
 *	 7 6 5 4 3 2 1 0  - bit no
 *	 | | | | | | | |
 *	 | | | | | | | +--- exported entry
 *	 | | | | | | +----- uses shared data
 *	 +-+-+-+-+-+------- parameter word count
 */

#define E32EXPORT	0x01		/* Exported entry */
#define E32SHARED	0x02		/* Uses shared data */
#define E32PARAMS	0xf8		/* Parameter word count mask */

/*
 *  Flags for forwarders only:
 */

#define FWD_ORDINAL	0x01		/* Imported by ordinal */
/*end*/


struct VxD_Desc_Block {
    ULONG DDB_Next;         /* VMM RESERVED FIELD */
    USHORT DDB_SDK_Version;     /* INIT <DDK_VERSION> RESERVED FIELD */
    USHORT DDB_Req_Device_Number;   /* INIT <UNDEFINED_DEVICE_ID> */
    UCHAR DDB_Dev_Major_Version;    /* INIT <0> Major device number */
    UCHAR DDB_Dev_Minor_Version;    /* INIT <0> Minor device number */
    USHORT DDB_Flags;           /* INIT <0> for init calls complete */
    UCHAR DDB_Name[8];          /* AINIT <"        "> Device name */
    ULONG DDB_Init_Order;       /* INIT <UNDEFINED_INIT_ORDER> */
    ULONG DDB_Control_Proc;     /* Offset of control procedure */
    ULONG DDB_V86_API_Proc;     /* INIT <0> Offset of API procedure */
    ULONG DDB_PM_API_Proc;      /* INIT <0> Offset of API procedure */
    ULONG DDB_V86_API_CSIP;     /* INIT <0> CS:IP of API entry point */
    ULONG DDB_PM_API_CSIP;      /* INIT <0> CS:IP of API entry point */
    ULONG DDB_Reference_Data;       /* Reference data from real mode */
    ULONG DDB_Service_Table_Ptr;    /* INIT <0> Pointer to service table */
    ULONG DDB_Service_Table_Size;   /* INIT <0> Number of services */
    ULONG DDB_Win32_Service_Table;  /* INIT <0> Pointer to Win32 services */
    ULONG DDB_Prev;         /* INIT <'Prev'> Ptr to prev 4.0 DDB */
    ULONG DDB_Reserved0;        /* INIT <0> Reserved */
    ULONG DDB_Reserved1;        /* INIT <'Rsv1'> Reserved */
    ULONG DDB_Reserved2;        /* INIT <'Rsv2'> Reserved */
    ULONG DDB_Reserved3;        /* INIT <'Rsv3'> Reserved */
};
