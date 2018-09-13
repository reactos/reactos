/*
 *  Title
 *
 *  exe386.h
 *  (C) Copyright Microsoft Corp 1988-1990
 *
 *  Description
 *
 *  Data structure definitions for the OS/2
 *  executable file format (flat model).
 *
 *  Modification History
 *
 *  91/12/18    Wieslaw Kalkus  Windows NT version
 *  90/07/30    Wieslaw Kalkus  Modified linear-executable
 *  88/08/05    Wieslaw Kalkus  Initial version
 */



    /*_________________________________________________________________*
     |                                                                 |
     |                                                                 |
     |  OS/2 .EXE FILE HEADER DEFINITION - 386 version 0:32        |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */

#define BITPERBYTE  8       /* Should never change         */
#define BITPERWORD  16      /* I'm not sure about this one         */
#define OBJPAGELEN  4096        /* Memory page size in bytes       */
#define E32RESBYTES1    8       /* First bytes reserved        */
#define E32RESBYTES2    2       /* Second bytes reserved           */
#define E32RESBYTES3    12      /* Third bytes reserved        */
#define E32RESBYTES4    8       /* Fourth bytes reserved           */
#define E32RESBYTES5    4       /* Fifth bytes reserved        */
#define E32RESBYTES6    4       /* Sixth bytes reserved        */
#define STD_EXTRA   9       /* Standard number of extra information*/
                    /* units placed in the header; this    */
                    /* includes the following tables:      */
                    /*  - export, import, resource,    */
                    /*    exception, security, fixup,      */
                    /*    debug, image description,    */
                    /*    machine specific tables      */
#define EXP     0       /* Export table position           */
#define IMP     1       /* Import table position           */
#define RES     2       /* Resource table position         */
#define EXC     3       /* Exception table position        */
#define SEC     4       /* Security table position         */
#define FIX     5       /* Fixup table position        */
#define DEB     6       /* Debug table position        */
#define IMD     7       /* Image description table position    */
#define MSP     8       /* Machine specific table position     */

struct info             /* Extra information header block      */
{
    unsigned long   rva;        /* Virtual relative address of info    */
    unsigned long   size;       /* Size of information block       */
};


struct e32_exe              /* PE 32-bit .EXE header           */
{
    unsigned char   e32_magic[4];   /* Magic number E32_MAGIC          */
    unsigned short  e32_cpu;        /* The CPU type            */
    unsigned short  e32_objcnt;     /* Number of memory objects        */
    unsigned long   e32_timestamp;  /* Time EXE file was created/modified  */
    unsigned char   e32_res1[E32RESBYTES1]; /* Reserved bytes - must be 0  */
    unsigned short  e32_opthdrsize; /* Optional header size        */
    unsigned short  e32_imageflags; /* Image flags             */
    unsigned char   e32_res2[E32RESBYTES2]; /* Reserved bytes - must be 0  */
    unsigned char   e32_linkmajor;  /* The linker major version number     */
    unsigned char   e32_linkminor;  /* The linker minor version number     */
    unsigned char   e32_res3[E32RESBYTES3]; /* Reserved bytes - must be 0  */
    unsigned long   e32_entryrva;   /* Relative virt. addr. of entry point */
    unsigned char   e32_res4[E32RESBYTES4]; /* Reserved bytes - must be 0  */
    unsigned long   e32_vbase;      /* Virtual base address of module      */
    unsigned long   e32_objalign;   /* Object Virtual Address align. factor*/
    unsigned long   e32_filealign;  /* Image page alignment/truncate factor*/
    unsigned short  e32_osmajor;    /* The operating system major ver. no. */
    unsigned short  e32_osminor;    /* The operating system minor ver. no. */
    unsigned short  e32_usermajor;  /* The user major version number       */
    unsigned short  e32_userminor;  /* The user minor version number       */
    unsigned short  e32_subsysmajor;/* The subsystem major version number  */
    unsigned short  e32_subsysminor;/* The subsystem minor version number  */
    unsigned char   e32_res5[E32RESBYTES5]; /* Reserved bytes - must be 0  */
    unsigned long   e32_vsize;      /* Virtual size of the entire image    */
    unsigned long   e32_hdrsize;    /* Header information size         */
    unsigned long   e32_filechksum; /* Checksum for entire file        */
    unsigned short  e32_subsys;     /* The subsystem type          */
    unsigned short  e32_dllflags;   /* DLL flags               */
    unsigned long   e32_stackmax;   /* Maximum stack size          */
    unsigned long   e32_stackinit;  /* Initial committed stack size    */
    unsigned long   e32_heapmax;    /* Maximum heap size           */
    unsigned long   e32_heapinit;   /* Initial committed heap size     */
    unsigned char   e32_res6[E32RESBYTES6]; /* Reserved bytes - must be 0  */
    unsigned long   e32_hdrextra;   /* Number of extra info units in header*/
    struct info     e32_unit[STD_EXTRA]; /* Array of extra info units      */
};

#define E32HDR_SIZE     sizeof(struct e32_exe)

#define E32_MAGIC(x)        ((unsigned short)((x).e32_magic[0]<<BITPERBYTE)|(x).e32_magic[1])
#define E32_MAGIC1(x)       (x).e32_magic[0]
#define E32_MAGIC2(x)       (x).e32_magic[1]
#define E32_CPU(x)      (x).e32_cpu
#define E32_OBJCNT(x)       (x).e32_objcnt
#define E32_TIMESTAMP(x)    (x).e32_timestamp
#define E32_IMAGEFLAGS(x)   (x).e32_imageflags
#define E32_LINKMAJOR(x)    (x).e32_linkmajor
#define E32_LINKMINOR(x)    (x).e32_linkminor
#define E32_ENTRYRVA(x)     (x).e32_entryrva
#define E32_VBASE(x)        (x).e32_vbase
#define E32_OBJALIGN(x)     (x).e32_objalign
#define E32_FILEALIGN(x)    (x).e32_filealign
#define E32_OSMAJOR(x)      (x).e32_osmajor
#define E32_OSMINOR(x)      (x).e32_osminor
#define E32_USERMAJOR(x)    (x).e32_usermajor
#define E32_USERMINOR(x)    (x).e32_userminor
#define E32_SUBSYSMAJOR(x)  (x).e32_subsysmajor
#define E32_SUBSYSMINOR(x)  (x).e32_subsysminor
#define E32_VSIZE(x)        (x).e32_vsize
#define E32_HDRSIZE(x)      (x).e32_hdrsize
#define E32_FILECHKSUM(x)   (x).e32_filechksum
#define E32_SUBSYS(x)       (x).e32_subsys
#define E32_DLLFLAGS(x)     (x).e32_dllflags
#define E32_STACKMAX(x)     (x).e32_stackmax
#define E32_STACKINIT(x)    (x).e32_stackinit
#define E32_HEAPMAX(x)      (x).e32_heapmax
#define E32_HEAPINIT(x)     (x).e32_heapinit
#define E32_HDREXTRA(x)     (x).e32_hdrextra
#define E32_EXPTAB(x)       (x).e32_unit[EXP].rva
#define E32_EXPSIZ(x)       (x).e32_unit[EXP].size
#define E32_IMPTAB(x)       (x).e32_unit[IMP].rva
#define E32_IMPSIZ(x)       (x).e32_unit[IMP].size
#define E32_RESTAB(x)       (x).e32_unit[RES].rva
#define E32_RESSIZ(x)       (x).e32_unit[RES].size
#define E32_EXCTAB(x)       (x).e32_unit[EXC].rva
#define E32_EXCSIZ(x)       (x).e32_unit[EXC].size
#define E32_SECTAB(x)       (x).e32_unit[SEC].rva
#define E32_SECSIZ(x)       (x).e32_unit[SEC].size
#define E32_FIXTAB(x)       (x).e32_unit[FIX].rva
#define E32_FIXSIZ(x)       (x).e32_unit[FIX].size
#define E32_DEBTAB(x)       (x).e32_unit[DEB].rva
#define E32_DEBSIZ(x)       (x).e32_unit[DEB].size
#define E32_IMDTAB(x)       (x).e32_unit[IMD].rva
#define E32_IMDSIZ(x)       (x).e32_unit[IMD].size
#define E32_MSPTAB(x)       (x).e32_unit[MSP].rva
#define E32_MSPSIZ(x)       (x).e32_unit[MSP].size


/*
 *  Valid linear-executable signature:
 */

#define E32MAGIC1   'P'     /* New magic number  "PE" */
#define E32MAGIC2   'E'     /* New magic number  "PE" */
#define E32MAGIC    0x5045      /* New magic number  "PE" */


/*
 *  Valid CPU types:
 */

#define E32CPUUNKNOWN   0x0000      /* Unknown CPU                 */
#define E32CPU386   0x014c      /* Intel 80386 or upwardly compatibile */
#define E32CPU486   0x014d      /* Intel 80486 or upwardly compatibile */
#define E32CPU586   0x014e      /* Intel 80586 or upwardly compatibile */
#define E32CPUMIPS_I    0x0162      /* MIPS Mark I (R2000, R3000)          */
#define E32CPUMIPS_II   0x0163      /* MIPS Mark II (R6000)            */
#define E32CPUMIPS_III  0x0166      /* MIPS Mark II (R4000)            */


/*
 *  Image Flags:
 */

#define E32_MODEXE  0x0000      /* Program module              */
#define E32_LOADABLE    0x0002      /* Module Loadable (no error or ilink) */
#define E32_NOINTFIX    0x0200      /* Resolved fixups have been removed   */
#define E32_MODDLL  0x2000      /* Library Module - used as NENOTP     */

#define IsLOADABLE(x)     ((x) & E32_LOADABLE)
#define IsNOTRELOC(x)     ((x) & E32_NOINTFIX)
#define IsDLL(x)      ((x) & E32_MODDLL)
#define IsAPLIPROG(x)     (!IsDLL(x))

#define SetAPLIPROG(x)    ((x) &= ~E32_MODDLL)
#define SetLOADABLE(x)    ((x) |= E32_LOADABLE)
#define SetNOTLOADABLE(x) ((x) &= ~E32_LOADABLE)
#define SetNOTRELOC(x)    ((x) |= E32_NOINTFIX)
#define SetDLL(x)     ((x) |= E32_MODDLL)
#define SetNOFIXUPS(x)    SetNOTRELOC(x)


/*
 *  Target subsystem required to run module:
 */

#define E32_SSUNKNOWN   0x0000      /* Unknown subsystem        */
#define E32_SSNATIVE    0x0001      /* NT native subsystem      */
#define E32_SSWINGUI    0x0002      /* Windows GUI subsystem    */
#define E32_SSWINCHAR   0x0003      /* Windows character subsystem  */
#define E32_SSOS2CHAR   0x0005      /* OS/2 character subsystem */
#define E32_SSPOSIXCHAR 0x0007      /* POSIX character subsystem    */

#define IsNOTGUI(x)   ((x) == E32_SSWINCHAR)
#define IsGUICOMPAT(x)    ((x) == E32_SSWINCHAR)
#define IsGUI(x)      ((x) == E32_SSWINGUI)

#define SetNOTGUI(x)      ((x) = E32_SSWINCHAR)
#define SetGUICOMPAT(x)   ((x) = E32_SSWINCHAR)
#define SetGUI(x)     ((x) = E32_SSWINGUI)


/*
 *  DLL Flags:
 */

#define E32_PROCINIT    0x0001      /* Per-Process Library Initialization */
#define E32_PROCTERM    0x0002      /* Per-Process Library Termination    */
#define E32_THREADINIT  0x0004      /* Per-Thread Library Initialization  */
#define E32_THREADTERM  0x0008      /* Per-Thread Library Termination     */

#define IsINSTINIT(x)     ((x) & E32_PROCINIT)
#define IsINSTTERM(x)     ((x) & E32_PROCTERM)
#define SetINSTINIT(x)    ((x) |= E32_PROCINIT)
#define SetINSTTERM(x)    ((x) |= E32_PROCTERM)


/*
 *  OBJECT TABLE
 */

#define E32OBJNAMEBYTES 8       /* Name bytes               */
#define E32OBJRESBYTES1 12      /* First bytes reserved         */

struct o32_obj              /* .EXE memory object table entry   */
{
    unsigned char   o32_name[E32OBJNAMEBYTES];/* Object name            */
    unsigned long   o32_vsize;  /* Virtual memory size          */
    unsigned long   o32_rva;    /* Object relative virtual address  */
    unsigned long   o32_psize;  /* Physical file size of init. data */
    unsigned long   o32_pages;  /* Image pages offset           */
    unsigned char   o32_reserved[E32OBJRESBYTES1];/* Reserved, must be 0*/
    unsigned long   o32_flags;  /* Attribute flags for the object   */
};

#define O32_OBJSIZE sizeof(struct o32_obj)

#define O32_VSIZE(x)    (x).o32_vsize
#define O32_RVA(x)  (x).o32_rva
#define O32_PSIZE(x)    (x).o32_psize
#define O32_PAGES(x)    (x).o32_pages
#define O32_FLAGS(x)    (x).o32_flags


/*
 *  Format of O32_FLAGS(x)
 *
 * 31       25 24   16 15       8   7   0
 *  #### #### | #### #### | #### #### | #### #### - bit no
 *  |||| ||||   |||| ||||   |||| ||||   |||| ||||
 *  |||| ||||   |||| ||||   |||| ||||   |||+-++++-- Reserved - must be zero
 *  |||| ||||   |||| ||||   |||| ||||   ||+-------- Code Object
 *  |||| ||||   |||| ||||   |||| ||||   |+--------- Initialized data object
 *  |||| ||||   |||| ||||   |||| ||||   +---------- Uninitialized data object
 *  |||| ||||   |||| ||||   |||| ||||
 *  |||| ||++---++++-++++---++++-++++-------------- Reserved - must be zero
 *  |||| |+---------------------------------------- Object must not be cached
 *  |||| +----------------------------------------- Object must not be paged
 *  |||| 
 *  |||+------------------------------------------- Shared object
 *  ||+-------------------------------------------- Executable object
 *  |+--------------------------------------------- Readable object
 *  +---------------------------------------------- Writable object
 */

#define OBJ_CODE    0x00000020L     /* Code Object           */
#define OBJ_INITDATA    0x00000040L     /* Initialized Data Object   */
#define OBJ_UNINITDATA  0x00000080L     /* Uninitialized Data Object */
#define OBJ_NOCACHE 0x04000000L     /* Object Not Cacheable      */
#define OBJ_RESIDENT    0x08000000L     /* Object Not Pageable       */
#define OBJ_SHARED  0x10000000L     /* Object is Shared      */
#define OBJ_EXEC    0x20000000L     /* Executable Object         */
#define OBJ_READ    0x40000000L     /* Readable Object       */
#define OBJ_WRITE   0x80000000L     /* Writeable Object      */


#define IsCODEOBJ(x)        ((x) & OBJ_CODE)
#define IsINITIALIZED(x)    ((x) & OBJ_INITDATA)
#define IsUNINITIALIZED(x)  ((x) & OBJ_UNINITDATA)
#define IsCACHED(x)     (((x) & OBJ_NOCACHE) == 0)
#define IsRESIDENT(x)       ((x) & OBJ_RESIDENT)
#define IsSHARED(x)     ((x) & OBJ_SHARED)
#define IsEXECUTABLE(x)     ((x) & OBJ_EXEC)
#define IsREADABLE(x)       ((x) & OBJ_READ)
#define IsWRITEABLE(x)      ((x) & OBJ_WRITE)

#define SetCODEOBJ(x)       ((x) |= OBJ_CODE)
#define SetCACHED(x)        ((x) &= ~OBJ_NOCACHE)
#define SetINITIALIZED(x)   ((x) |= OBJ_INITDATA)
#define SetUNINITIALIZED(x) ((x) |= OBJ_UNINITDATA)
#define SetRESIDENT(x)      ((x) |= OBJ_RESIDENT)
#define SetSHARED(x)        ((x) |= OBJ_SHARED)
#define SetEXECUTABLE(x)    ((x) |= OBJ_EXEC)
#define SetREADABLE(x)      ((x) |= OBJ_READ)
#define SetWRITABLE(x)      ((x) |= OBJ_WRITE)


/*
 *  EXPORT ADDRESS TABLE - Previously called entry table
 */

struct ExpHdr               /* Export directory table      */
{
    unsigned long   exp_flags;  /* Export table flags, must be 0   */
    unsigned long   exp_timestamp;  /* Time export data created        */
    unsigned short  exp_vermajor;   /* Major version stamp         */
    unsigned short  exp_verminor;   /* Minor version stamp         */
    unsigned long   exp_dllname;    /* Offset to the DLL name      */
    unsigned long   exp_ordbase;    /* First valid ordinal         */
    unsigned long   exp_eatcnt; /* Number of EAT entries       */
    unsigned long   exp_namecnt;    /* Number of exported names    */
    unsigned long   exp_eat;    /* Export Address Table offset     */
    unsigned long   exp_name;   /* Export name pointers table off  */
    unsigned long   exp_ordinal;    /* Export ordinals table offset    */
};

#define EXPHDR_SIZE sizeof(struct ExpHdr)

#define EXP_FLAGS(x)     (x).exp_flags
#define EXP_TIMESTAMP(x) (x).exp_timestamp
#define EXP_VERMAJOR(x)  (x).exp_vermajor
#define EXP_VERMINOR(x)  (x).exp_verminor
#define EXP_DLLNAME(x)   (x).exp_dllname
#define EXP_ORDBASE(x)   (x).exp_ordbase
#define EXP_EATCNT(x)    (x).exp_eatcnt
#define EXP_NAMECNT(x)   (x).exp_namecnt
#define EXP_EAT(x)   (x).exp_eat
#define EXP_NAME(x)  (x).exp_name
#define EXP_ORDINAL(x)   (x).exp_ordinal


/*
 *  IMPORT MODULE DESCRIPTOR TABLE
 *
 *  Import Directory Table consists of an array of ImpHdr structures (one
 *  for each DLL imported), and is terminated by a zeroed ImpHdr structure.
 */

struct ImpHdr               /* Import directory table      */
{
    unsigned long   imp_flags;  /* Import table flags, must be 0   */
    unsigned long   imp_timestamp;  /* Time import data created        */
    unsigned short  imp_vermajor;   /* Major version stamp         */
    unsigned short  imp_verminor;   /* Minor version stamp         */
    unsigned long   imp_dllname;    /* Offset to the DLL name      */
    unsigned long   imp_address;    /* Import address table offset     */
};

#define IMPHDR_SIZE sizeof(struct ImpHdr)

#define IMP_FLAGS(x)     (x).imp_flags
#define IMP_TIMESTAMP(x) (x).imp_timestamp
#define IMP_VERMAJOR(x)  (x).imp_vermajor
#define IMP_VERMINOR(x)  (x).imp_verminor
#define IMP_DLLNAME(x)   (x).imp_dllname
#define IMP_ADDRESS(x)   (x).imp_address


/*
 *  IMPORT PROCEDURE NAME TABLE
 */

struct ImpProc
{
    unsigned short  ip_hint;        /* Hint value           */
    char        ip_name[1];     /* Zero terminated importe name */
};

#define IP_HINT(x)  (x).ip_hint;

/*
 *  IMPORT ADDRESS TABLE
 */

#define IMPORD_MASK 0x7fffffffL /* Ordinal number mask          */
#define IMPOFF_MASK 0x7fffffffL /* Import data offset mask      */
#define ORD_BIT     0x80000000L /* Import by ordinal bit        */

#define IsIMPBYORD(x)   ((x)&ORD_BIT)


/*
 *  RESOURCE TABLE
 */

struct ResDir               /* Resource Directory */
{
    unsigned long   dir_flags;      /* Resource table flags, must be 0 */
    unsigned long   dir_timestamp;  /* Time resource data created      */
    unsigned short  dir_vermajor;   /* Major version stamp         */
    unsigned short  dir_verminor;   /* Minor version stamp         */
    unsigned short  dir_namecnt;    /* Number of name entries      */
    unsigned short  dir_idcnt;      /* Number of integer ID entries    */
};


struct ResDirEntry          /* Resource Directory Entry */
{
    unsigned long   dir_name;       /* Resource name RVA/Integer ID    */
    unsigned long   dir_data;       /* Resource data/Subdirectory RVA  */
};

#define RES_SUBDIR  0x80000000L     /* Subdirectory bit */


struct ResDirStrEntry           /* Resource String Entry */
{
    unsigned short  str_len;        /* String length */
    unsigned short  str_wcs[1];     /* Unicode string */
};


struct ResData              /* Resource Data */
{
    unsigned long   res_data;       /* Resource data RVA     */
    unsigned long   res_size;       /* Size of resource data */
    unsigned long   res_codepage;   /* Codepage      */
    unsigned long   res_reserved;   /* Reserved, must be 0   */
};


/*
 *  RELOCATION DEFINITIONS - RUN-TIME FIXUPS
 */

struct r32_rlc              /* Fixup block header           */
{
    unsigned long   r32_rvasrc;     /* RVA of the page to be fixed up   */
    unsigned long   r32_size;       /* Size of the fixup block in bytes */
};


#define R32_RVASRC(x)   (x).r32_rvasrc
#define R32_SIZE(x) (x).r32_size
#define R32_FIXUPHDR    sizeof(struct r32_rlc)


/*
 *  The fixup block header is followed by the array of type/offset values.
 *
 *  Format of relocation type/offset value
 *
 *  15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0  - bit no
 *   |  |  |  |  |  | | | | | | | | | | |
 *   |  |  |  |  +--+-+-+-+-+-+-+-+-+-+-+--- Offset within page
 *   +--+--+--+----------------------------- Fixup type
 */

/*
 *  Valid fixup types:
 */

#define R32_ABSOLUTE    0x0000      /* Absolute    - NOP; skipped by loader     */
#define R32_HIGHWORD    0x1000      /* High word   - high word of 32 bit offset */
#define R32_LOWWORD 0x2000      /* Low word    - low word of 32 bit offset  */
#define R32_OFF32   0x3000      /* Offset      - 32-bit offset          */
#define R32_HIGHADJ 0x4000      /* High adjust - next offset is low word    */
#define R32_MIPSJMP 0x5000      /* MIPSJMPADDR                  */
#define R32_TYPMASK 0xf000      /* Fixup type mask              */
#define R32_OFFMASK 0x0fff      /* Fixup page offset mask           */

#define IsABS(x)    (((x) & R32_SRCMASK) == R32_ABSOLUTE)
#define IsHIWORD(x) (((x) & R32_SRCMASK) == R32_HIGHWORD)
#define IsLOWORD(x) (((x) & R32_SRCMASK) == R32_LOWORD)
#define IsOFF32(x)  (((x) & R32_SRCMASK) == R32_OFF32)
#define IsHIADJ(x)  (((x) & R32_SRCMASK) == R32_HIGHADJ)
#define IsMIPSJMP(x)    (((x) & R32_SRCMASK) == R32_MIPSJMP)


/*
 *  DEBUG INFORMATION
 */

struct  DbgDir
{
    unsigned long   dbg_flags;      /* Debug flags: must be zero     */
    unsigned long   dbg_timestamp;  /* Time debug data created       */
    unsigned short  dbg_vermajor;   /* Major version stamp           */
    unsigned short  dbg_verminor;   /* Minor version stamp           */
    unsigned long   dbg_type;       /* Format type           */
    unsigned long   dbg_size;       /* Size of debug data w/o DbgDir */
    unsigned long   dbg_rva;        /* RVA of debug data when mapped */
    unsigned long   dbg_seek;       /* seek offset of debug data     */
};

#define DBG_FLAGS(x)     (x).dbg_flags
#define DBG_TIMESTAMP(x) (x).dbg_timestamp
#define DBG_VERMAJOR(x)  (x).dbg_vermajor
#define DBG_VERMINOR(x)  (x).dbg_verminor
#define DBG_TYPE(x)  (x).dbg_type
#define DBG_SIZE(x)  (x).dbg_size
#define DBG_RVA(x)   (x).dbg_rva
#define DBG_SEEK(x)      (x).dbg_seek
#define DBGDIR_SIZE      sizeof(struct DbgDir)


/*
 *  Format of DBG_TYPE - debug information format
 *
 *  15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0  - bit no
 *   |  |  |  |  |  | | | | | | | | | | |
 *   |  |  |  |  |  | | | | | | | | | | +--- CV 4.00 format
 *   +--+--+--+--+--+-+-+-+-+-+-+-+-+-+----- Reserved
 */

#define DBG_NTCOFF  1L
#define DBG_CV4     2L
