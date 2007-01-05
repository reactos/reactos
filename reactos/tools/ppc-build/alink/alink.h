#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;

#define typeid x_typeid
#define class x_class
#define stricmp strcasecmp

#define TRUE  (1==1)
#define FALSE (1==0)

#define SWITCHCHAR '-'
#define PATH_CHAR '\\'
#define DEFAULT_EXTENSION ".obj"

#define ERR_EXTRA_DATA 1
#define ERR_NO_HEADER 2
#define ERR_NO_RECDATA 3
#define ERR_NO_MEM 4
#define ERR_INV_DATA 5
#define ERR_INV_SEG 6
#define ERR_BAD_FIXUP 7
#define ERR_BAD_SEGDEF 8
#define ERR_ABS_SEG 9
#define ERR_DUP_PUBLIC 10
#define ERR_NO_MODEND 11
#define ERR_EXTRA_HEADER 12
#define ERR_UNKNOWN_RECTYPE 13
#define ERR_SEG_TOO_LARGE 14
#define ERR_MULTIPLE_STARTS 15
#define ERR_BAD_GRPDEF 16
#define ERR_OVERWRITE 17
#define ERR_INVALID_COMENT 18
#define ERR_ILLEGAL_IMPORTS 19

#define PREV_LE 1
#define PREV_LI 2
#define PREV_LI32 3

#define THEADR 0x80
#define LHEADR 0x82
#define COMENT 0x88
#define MODEND 0x8a
#define MODEND32 0x8b
#define EXTDEF 0x8c
#define TYPDEF 0x8e
#define PUBDEF 0x90
#define PUBDEF32 0x91
#define LINNUM 0x94
#define LINNUM32 0x95
#define LNAMES 0x96
#define SEGDEF 0x98
#define SEGDEF32 0x99
#define GRPDEF 0x9a
#define FIXUPP 0x9c
#define FIXUPP32 0x9d
#define LEDATA 0xa0
#define LEDATA32 0xa1
#define LIDATA 0xa2
#define LIDATA32 0xa3
#define COMDEF 0xb0
#define BAKPAT 0xb2
#define BAKPAT32 0xb3
#define LEXTDEF 0xb4
#define LEXTDEF32 0xb5
#define LPUBDEF 0xb6
#define LPUBDEF32 0xb7
#define LCOMDEF 0xb8
#define CEXTDEF 0xbc
#define COMDAT 0xc2
#define COMDAT32 0xc3
#define LINSYM 0xc4
#define LINSYM32 0xc5
#define ALIAS 0xc6
#define NBKPAT 0xc8
#define NBKPAT32 0xc9
#define LLNAMES 0xca
#define LIBHDR 0xf0
#define LIBEND 0xf1

#define COMENT_TRANSLATOR 0x00
#define COMENT_INTEL_COPYRIGHT 0x01
#define COMENT_LIB_SPEC 0x81
#define COMENT_MSDOS_VER 0x9c
#define COMENT_MEMMODEL 0x9d
#define COMENT_DOSSEG 0x9e
#define COMENT_DEFLIB 0x9f
#define COMENT_OMFEXT 0xa0
#define COMENT_NEWOMF 0xa1
#define COMENT_LINKPASS 0xa2
#define COMENT_LIBMOD 0xa3 
#define COMENT_EXESTR 0xa4
#define COMENT_INCERR 0xa6
#define COMENT_NOPAD 0xa7
#define COMENT_WKEXT 0xa8
#define COMENT_LZEXT 0xa9
#define COMENT_PHARLAP 0xaa
#define COMENT_IBM386 0xb0
#define COMENT_RECORDER 0xb1
#define COMENT_COMMENT 0xda
#define COMENT_COMPILER 0xdb
#define COMENT_DATE 0xdc
#define COMENT_TIME 0xdd
#define COMENT_USER 0xdf
#define COMENT_DEPFILE 0xe9
#define COMENT_COMMANDLINE 0xff
#define COMENT_PUBTYPE 0xe1
#define COMENT_COMPARAM 0xea
#define COMENT_TYPDEF 0xe3
#define COMENT_STRUCTMEM 0xe2
#define COMENT_OPENSCOPE 0xe5
#define COMENT_LOCAL 0xe6
#define COMENT_ENDSCOPE 0xe7
#define COMENT_SOURCEFILE 0xe8

#define EXT_IMPDEF 0x01
#define EXT_EXPDEF 0x02

#define SEG_ALIGN 0x3e0
#define SEG_COMBINE 0x1c
#define SEG_BIG 0x02
#define SEG_USE32 0x01

#define SEG_ABS 0x00
#define SEG_BYTE 0x20
#define SEG_WORD 0x40
#define SEG_PARA 0x60
#define SEG_PAGE 0x80
#define SEG_DWORD 0xa0
#define SEG_MEMPAGE 0xc0
#define SEG_BADALIGN 0xe0
#define SEG_8BYTE 0x100
#define SEG_32BYTE 0x200
#define SEG_64BYTE 0x300

#define SEG_PRIVATE 0x00
#define SEG_PUBLIC 0x08
#define SEG_PUBLIC2 0x10
#define SEG_STACK 0x14
#define SEG_COMMON 0x18
#define SEG_PUBLIC3 0x1c

typedef enum segdisp_t {
//#define REL_SEGDISP 0x80
    REL_SEGDISP = 0x80,
//#define REL_GRPDISP 0x81
    REL_GRPDISP = 0x81,
//#define REL_EXTDISP 0x82
    REL_EXTDISP = 0x82,
//#define REL_EXPFRAME 0x83
    REL_EXPFRAME = 0x83,
//#define REL_SEGONLY 0x84
    REL_SEGONLY = 0x84,
//#define REL_GRPONLY 0x85
    REL_GRPONLY = 0x85,
//#define REL_EXTONLY 0x86
    REL_EXTONLY = 0x86
} segdisp_t;

typedef enum segframe_t {
//#define REL_SEGFRAME 0
    REL_SEGFRAME,
//#define REL_GRPFRAME 1
    REL_GRPFRAME,
//#define REL_EXTFRAME 2
    REL_EXTFRAME,
//#define REL_EXPFFRAME 3
    REL_EXPFFRAME,
//#define REL_LILEFRAME 4
    REL_LILEFRAME,
//#define REL_TARGETFRAME 5
    REL_TARGETFRAME
};

#define FIX_SELFREL 0x10
#define FIX_MASK (0x0f+FIX_SELFREL)

#define FIX_THRED 0x08
#define THRED_MASK 0x07

#define FIX_LBYTE 0
#define FIX_OFS16 1
#define FIX_BASE 2
#define FIX_PTR1616 3
#define FIX_HBYTE 4
#define FIX_OFS16_2 5
#define FIX_OFS32 9
#define FIX_PTR1632 11
#define FIX_OFS32_2 13

#define FIX_PPC_ADDR24   259
#define FIX_PPC_REL24    260
#define FIX_PPC_RVA16LO  261
#define FIX_PPC_RVA16HA  262
#define FIX_PPC_SECTOFF  263

/* RVA32 fixups are not supported by OMF, so has an out-of-range number */
#define FIX_RVA32 256
#define FIX_REL32 264
#define FIX_ADDR32 265

#define FIX_SELF_LBYTE (FIX_LBYTE+FIX_SELFREL)
#define FIX_SELF_OFS16 (FIX_OFS16+FIX_SELFREL)
#define FIX_SELF_OFS16_2 (FIX_OFS16_2+FIX_SELFREL)
#define FIX_SELF_OFS32 (FIX_OFS32+FIX_SELFREL)
#define FIX_SELF_OFS32_2 (FIX_OFS32_2+FIX_SELFREL)

#define LIBF_CASESENSITIVE 1

#define EXT_NOMATCH       0
#define EXT_MATCHEDPUBLIC 1
#define EXT_MATCHEDIMPORT 2

#define PE_SIGNATURE      0x00
#define PE_MACHINEID      0x04
#define PE_NUMOBJECTS     0x06
#define PE_DATESTAMP      0x08
#define PE_SYMBOLPTR      0x0c
#define PE_NUMSYMBOLS     0x10
#define PE_HDRSIZE        0x14
#define PE_FLAGS          0x16
#define PE_MAGIC          0x18
#define PE_LMAJOR         0x1a
#define PE_LMINOR         0x1b
#define PE_CODESIZE       0x1c
#define PE_INITDATASIZE   0x20
#define PE_UNINITDATASIZE 0x24
#define PE_ENTRYPOINT     0x28
#define PE_CODEBASE       0x2c
#define PE_DATABASE       0x30
#define PE_IMAGEBASE      0x34
#define PE_OBJECTALIGN    0x38
#define PE_FILEALIGN      0x3c
#define PE_OSMAJOR        0x40
#define PE_OSMINOR        0x42
#define PE_USERMAJOR      0x44
#define PE_USERMINOR      0x46
#define PE_SUBSYSMAJOR    0x48
#define PE_SUBSYSMINOR    0x4a
#define PE_IMAGESIZE      0x50
#define PE_HEADERSIZE     0x54
#define PE_CHECKSUM       0x58
#define PE_SUBSYSTEM      0x5c
#define PE_DLLFLAGS       0x5e
#define PE_STACKSIZE      0x60
#define PE_STACKCOMMSIZE  0x64
#define PE_HEAPSIZE       0x68
#define PE_HEAPCOMMSIZE   0x6c
#define PE_LOADERFLAGS    0x70
#define PE_NUMRVAS        0x74
#define PE_EXPORTRVA      0x78
#define PE_EXPORTSIZE     0x7c
#define PE_IMPORTRVA      0x80
#define PE_IMPORTSIZE     0x84
#define PE_RESOURCERVA    0x88
#define PE_RESOURCESIZE   0x8c
#define PE_EXCEPTIONRVA   0x90
#define PE_EXCEPTIONSIZE  0x94
#define PE_SECURITYRVA    0x98
#define PE_SECURITYSIZE   0x9c
#define PE_FIXUPRVA       0xa0
#define PE_FIXUPSIZE      0xa4
#define PE_DEBUGRVA       0xa8
#define PE_DEBUGSIZE      0xac
#define PE_DESCRVA        0xb0
#define PE_DESCSIZE       0xb4
#define PE_MSPECRVA       0xb8
#define PE_MSPECSIZE      0xbc
#define PE_TLSRVA         0xc0
#define PE_TLSSIZE        0xc4
#define PE_LOADCONFIGRVA  0xc8
#define PE_LOADCONFIGSIZE 0xcc
#define PE_BOUNDIMPRVA    0xd0
#define PE_BOUNDIMPSIZE   0xd4
#define PE_IATRVA         0xd8
#define PE_IATSIZE        0xdc

#define PE_OBJECT_NAME     0x00
#define PE_OBJECT_VIRTSIZE 0x08
#define PE_OBJECT_VIRTADDR 0x0c
#define PE_OBJECT_RAWSIZE  0x10
#define PE_OBJECT_RAWPTR   0x14
#define PE_OBJECT_RELPTR   0x18
#define PE_OBJECT_LINEPTR  0x1c
#define PE_OBJECT_NUMREL   0x20
#define PE_OBJECT_NUMLINE  0x22
#define PE_OBJECT_FLAGS    0x24

#define PE_BASE_HEADER_SIZE     0x18
#define PE_OPTIONAL_HEADER_SIZE 0xe0
#define PE_OBJECTENTRY_SIZE     0x28
#define PE_HEADBUF_SIZE         (PE_BASE_HEADER_SIZE+PE_OPTIONAL_HEADER_SIZE)
#define PE_IMPORTDIRENTRY_SIZE  0x14
#define PE_NUM_VAS              0x10
#define PE_EXPORTHEADER_SIZE    0x28
#define PE_RESENTRY_SIZE        0x08
#define PE_RESDIR_SIZE          0x10
#define PE_RESDATAENTRY_SIZE    0x10
#define PE_SYMBOL_SIZE          0x12
#define PE_RELOC_SIZE           0x0a

#define PE_ORDINAL_FLAG    0x80000000
#define PE_INTEL386        0x014c
#define PE_POWERPC         0x01f0
#define PE_MAGICNUM        0x010b
#define PE_FILE_EXECUTABLE 0x0002
#define PE_FILE_32BIT      0x0100
#define PE_FILE_LIBRARY    0x2000

#define PE_REL_LOW16 0x2000
#define PE_REL_OFS32 0x3000

#define PE_SUBSYS_NATIVE  1
#define PE_SUBSYS_WINDOWS 2
#define PE_SUBSYS_CONSOLE 3
#define PE_SUBSYS_POSIX   7

//seglist[0]->nameindex=0 o=-1 l=38 a=140 f=00100a00
//seglist[1]->nameindex=1 o=-1 l=8  a=178 f=60400020
//seglist[2]->nameindex=2 o=-1 l=20 a=186 f=40300040

#define WINF_UNDEFINED   0x00000000
#define WINF_CODE        0x00000020
#define WINF_INITDATA    0x00000040
#define WINF_UNINITDATA  0x00000080
#define WINF_DISCARDABLE 0x02000000
#define WINF_NOPAGE      0x08000000
#define WINF_SHARED      0x10000000
#define WINF_EXECUTE     0x20000000
#define WINF_READABLE    0x40000000
#define WINF_WRITEABLE   0x80000000
#define WINF_ALIGN_NOPAD 0x00000008
#define WINF_ALIGN_BYTE  0x00100000
#define WINF_ALIGN_WORD  0x00200000
#define WINF_ALIGN_DWORD 0x00300000
#define WINF_ALIGN_8     0x00400000
#define WINF_ALIGN_PARA  0x00500000
#define WINF_ALIGN_32    0x00600000
#define WINF_ALIGN_64    0x00700000
#define WINF_ALIGN       (WINF_ALIGN_64)
#define WINF_COMMENT     0x00000200
#define WINF_REMOVE      0x00000800
#define WINF_COMDAT      0x00001000
#define WINF_NEG_FLAGS   (WINF_DISCARDABLE | WINF_NOPAGE)
#define WINF_IMAGE_FLAGS 0xfa0008e0

#define COFF_SYM_EXTERNAL 2
#define COFF_SYM_STATIC   3
#define COFF_SYM_LABEL    6
#define COFF_SYM_FUNCTION 101
#define COFF_SYM_FILE     103
#define COFF_SYM_SECTION  104

#define COFF_FIX_DIR32    6
#define COFF_FIX_RVA32    7
#define COFF_FIX_REL32    0x14

#define R_PPC_ADDR32      1
#define R_PPC_ADDR24      2
#define R_PPC_ADDR16      3
#define R_PPC_ADDR16_LO   4
#define R_PPC_ADDR16_HI   5
#define R_PPC_ADDR16_HA   6
#define R_PPC_REL24       10
#define R_PPC_UADDR32     24
#define R_PPC_REL32       26
#define R_PPC_SECTOFF     33

#define IMAGE_REL_PPC_ABSOLUTE 0x00
#define IMAGE_REL_PPC_ADDR64   0x01
#define IMAGE_REL_PPC_ADDR32   0x02
#define IMAGE_REL_PPC_ADDR24   0x03
#define IMAGE_REL_PPC_ADDR16   0x04
#define IMAGE_REL_PPC_ADDR14   0x05
#define IMAGE_REL_PPC_REL24    0x06
#define IMAGE_REL_PPC_REL14    0x07
#define IMAGE_REL_PPC_ADDR32NB 0x0a
#define IMAGE_REL_PPC_SECREL   0x0b
#define IMAGE_REL_PPC_SECTION  0x0c
#define IMAGE_REL_PPC_SECREL16 0x0f
#define IMAGE_REL_PPC_REFHI    0x10
#define IMAGE_REL_PPC_REFLO    0x11
#define IMAGE_REL_PPC_PAIR     0x12
#define IMAGE_REL_PPC_SECRELLO 0x13
#define IMAGE_REL_PPC_SECRELHI 0x14
#define IMAGE_REL_PPC_GPREL    0x15

#define OUTPUT_COM 1
#define OUTPUT_EXE 2
#define OUTPUT_PE  3

#define WIN32_DEFAULT_BASE              0x00400000
#define WIN32_DEFAULT_FILEALIGN         0x00000200
#define WIN32_DEFAULT_OBJECTALIGN       0x00001000
#define WIN32_DEFAULT_STACKSIZE         0x00100000
#define WIN32_DEFAULT_STACKCOMMITSIZE   0x00001000
#define WIN32_DEFAULT_HEAPSIZE          0x00100000
#define WIN32_DEFAULT_HEAPCOMMITSIZE    0x00001000
#define WIN32_DEFAULT_SUBSYS            PE_SUBSYS_WINDOWS
#define WIN32_DEFAULT_SUBSYSMAJOR       4
#define WIN32_DEFAULT_SUBSYSMINOR       0
#define WIN32_DEFAULT_OSMAJOR           1
#define WIN32_DEFAULT_OSMINOR           0

#define EXP_ORD 0x80

typedef char *PCHAR,**PPCHAR;
typedef unsigned char *PUCHAR;
typedef unsigned long UINT;

extern UINT imageBase;
extern UINT fileAlign;
extern UINT objectAlign;
extern UINT stackSize;
extern UINT stackCommitSize;
extern UINT heapSize;
extern UINT heapCommitSize;

template <typename T>
int get32(T &vect, int offset)
{
    return vect[offset] | 
	(vect[offset+1] << 8) | 
	(vect[offset+2] << 16) | 
	(vect[offset+3] << 24);
}

template <typename T>
void put32(T &vect, int offset, int val)
{
    vect[offset] = val;
    vect[offset+1] = val >> 8;
    vect[offset+2] = val >> 16;
    vect[offset+3] = val >> 24;
}

template <typename T>
int get16(T &vect, int offset)
{
    return vect[offset] | (vect[offset+1] << 8);
}

template <typename T>
void put16(T &vect, int offset, int val)
{
    vect[offset] = val; vect[offset+1] = val >> 8;
}

template <typename T>
void ClearNbit(T &mask,long i)
{
    mask[i/8]&=0xff-(1<<(i%8));
}

template <typename T>
void SetNbit(T &mask,long i)
{
    mask[i/8]|=(1<<(i%8));
}

template <typename T>
char GetNbit(T &mask,long i)
{
    return (mask[i/8]>>(i%8))&1;
}

template <typename T>
long GetIndex(T &buf,long *index)
{
    long i;
    if(buf[*index]&0x80)
    {
	i=((buf[*index]&0x7f)*256)+buf[(*index)+1];
	(*index)+=2;
	return i;
    }
    else
    {
	return buf[(*index)++];
    }
}

static int roundup(int x, int align)
{
    x += align-1;
    return x & ~(align-1);
}

typedef struct __int_to_name {
    int id; 
    const char *name;
} INT_TO_NAME, *PINT_TO_NAME;

static const char *findName(const INT_TO_NAME *names, int id)
{
    for( int i = 0; names[i].name; i++ ) {
	if(names[i].id == id) return names[i].name;
    }
    return "<unknown>";
}

typedef struct __seg {
private:
    UINT base;
public:
    std::string name;
    long classindex;
    long overlayindex;
    long orderindex;
    UINT length;
    UINT virtualSize;
    UINT absframe;
    UINT absofs;
    UINT winFlags;
    UINT fileBase;
    unsigned short attr;
    std::vector<unsigned char> data;
    std::vector<unsigned char> datmask;
    
    __seg() {
	classindex = overlayindex = orderindex = -1;
	length = virtualSize = absframe = absofs = base = 0;
	winFlags = WINF_READABLE | WINF_WRITEABLE | WINF_NEG_FLAGS;
	attr = SEG_PRIVATE | SEG_PARA;
    }
    
    UINT getbase() { return base; }
    void setbase(UINT base) { 
	this->base = base;
    }
} SEG, *PSEG, **PPSEG;

typedef struct __datablock {
    long count;
    long blocks;
    long dataofs;
    std::vector<UCHAR> data;
    __datablock() { count = blocks = dataofs = 0; }
} DATABLOCK, *PDATABLOCK, **PPDATABLOCK;

typedef struct __pubdef {
    PSEG seg;
    long grpnum;
    long typenum;
    UINT ofs;
    UINT modnum;
    std::string name;
    std::string aliasName;
    __pubdef() {
	seg = 0; grpnum = typenum = ofs = modnum = 0;
    }
} PUBLIC, *PPUBLIC,**PPPUBLIC;

typedef struct __extdef {
    std::string name;
    long typenum;
    PPUBLIC pubdef;
    long impnum;
    long flags;
    UINT modnum;
    __extdef(std::string name = "") : name(name), typenum(-1), pubdef(0), impnum(0), flags(EXT_NOMATCH), modnum(0) { }
} EXTREC, *PEXTREC,**PPEXTREC;

typedef struct __imprec {
    std::string int_name, mod_name, imp_name;
    unsigned short ordinal;
    char flags;
    PSEG seg;
    UINT abs, ofs;
    __imprec() {
	abs = ordinal = flags = 0; seg = 0; ofs = 0;
    }
} IMPREC, *PIMPREC, **PPIMPREC;

typedef struct __exprec {
    std::string int_name, exp_name;
    UINT ordinal;
    char flags;
    PPUBLIC pubdef;
    UINT modnum;
    __exprec() {
	ordinal = flags = modnum = 0;
	pubdef = 0;
    }
} EXPREC, *PEXPREC, **PPEXPREC;

typedef struct __comdef {
    std::string name;
    UINT length;
    int isFar;
    UINT modnum;
    __comdef() { length = isFar = modnum; }
} COMREC, *PCOMREC, **PPCOMREC;

typedef struct __elf32sym
{
    ULONG  st_name;
    ULONG  st_value;
    ULONG  st_size;
    UCHAR  st_info;
    UCHAR  st_other;
    USHORT st_shndx;
} ELF32SYM, *PELF32SYM;

typedef struct __elf32rela
{
    ULONG r_offset;
    ULONG r_info;
    ULONG r_address;
} ELF32RELA, *PELF32RELA;

typedef struct __elf32relx
{
    ULONG r_offset;
    ULONG r_info;
    ULONG r_address;
    ULONG r_seg; /* Implied by section order */
} ELF32RELX, *PELF32RELX;

typedef struct __reloc {
    UINT ofs;
    segframe_t ftype;
    segdisp_t  ttype;
    unsigned short rtype;
    long target;
    UINT disp;
    UINT outputPos;
    PSEG seg, targseg;
    UINT frame;
    ELF32RELX orig;
    __reloc() { 
	frame = 0; seg = 0; targseg = 0;
	ftype = REL_SEGFRAME; 
	ttype = REL_SEGDISP;
	rtype = target = disp = outputPos = 0; 
    }
} RELOC, *PRELOC,**PPRELOC;

typedef struct __grp {
    std::string name;
    long numsegs;
    PSEG segindex[256];
    PSEG seg;
    __grp() { numsegs = 0; seg = 0; memset(segindex, 0, sizeof(segindex)); }
} GRP, *PGRP, **PPGRP;

typedef std::map<std::string, std::vector<void *> > SORTLIST;
typedef struct __libfile {
    std::string filename;
    unsigned short blocksize;
    unsigned short numdicpages;
    UINT dicstart;
    char flags;
    char libtype;
    int modsloaded;
    std::vector<UINT> modlist;
    std::vector<UCHAR> longnames;
    SORTLIST symbols;
} LIBFILE, *PLIBFILE, **PPLIBFILE;

typedef struct __libentry {
    UINT libfile;
    UINT modpage; 
} LIBENTRY, *PLIBENTRY, **PPLIBENTRY;

typedef struct __resource {
    std::vector<unsigned char> xtypename;
    std::vector<unsigned char> name;
    std::vector<unsigned char> data;
    UINT length;
    unsigned short typeid;
    unsigned short id;
    unsigned short languageid; 
} RESOURCE, *PRESOURCE;

typedef struct __coffsym {
    std::string name;
    UINT value;
    short section;
    unsigned short type;
    unsigned char coff_class;
    long extnum;
    UINT numAuxRecs;
    std::vector<UCHAR> auxRecs;
    int isComDat;
    int originalSym;
    PSEG segptr;

    __coffsym() {
	value = numAuxRecs = extnum = isComDat = originalSym = 0;
	section = 0; type = 0;
	coff_class = 0; segptr = 0;
    }
} COFFSYM, *PCOFFSYM;

typedef struct __comdatrec 
{
    UINT segnum;
    UINT combineType;
    UINT linkwith;
} COMDATREC, *PCOMDAT;

#define EI_NIDENT 16

typedef struct __elf32hdr
{
    UCHAR  e_ident[EI_NIDENT];
    USHORT e_type, e_machine;
    ULONG  e_version;
    ULONG  e_entry;
    ULONG  e_phoff;
    ULONG  e_shoff;
    ULONG  e_flags;
    USHORT e_ehsize, e_phentsize;
    USHORT e_phnum,  e_shentsize;
    USHORT e_shnum,  e_shtrndx;
} ELF32HDR, *PELF32HDR;

typedef struct __elf32shdr
{
    ULONG sh_name;
    ULONG sh_type;
    ULONG sh_flags;
    ULONG sh_addr;
    ULONG sh_offset;
    ULONG sh_size;
    ULONG sh_link;
    ULONG sh_info;
    ULONG sh_addralign;
    ULONG sh_entsize;
} ELF32SHDR, *PELF32SHDR;

#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOOS   0xfe00
#define ET_HIOS   0xfeff
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

#define EM_NONE        0
#define EM_M32         1
#define EM_SPARC       2
#define EM_386         3
#define EM_68K         4
#define EM_88K         5
/* #define EM_RESERVED 6 */
#define EM_860         7
#define EM_MIPS        8
/* #define EM_RESERVED 9 */
#define EM_MIPS_RS3_LE 10
/* #define EM_RESERVED 11-14 */
#define EM_PARISC      15
/* #define EM_RESERVED 16 */
#define EM_VPP500      17
#define EM_SPARC32PLUS 18
#define EM_960         19
#define EM_PPC         20
/* #define EM_RESERVED 21-35 */
#define EM_V800        36
#define EM_FR20        37
#define EM_RH32        38
#define EM_RCE         39
#define EM_ARM         40
#define EM_ALPHA       41
#define EM_SH          42
#define EM_SPARCV9     43
#define EM_TRICORE     44
#define EM_ARC         45
#define EM_H8_300      46
#define EM_H8_300H     47
#define EM_H8S         48
#define EM_H8_500      49
#define EM_IA_64       50
#define EM_MIPPS_X     51
#define EM_COLDFIRE    52
#define EM_68HC12      53

#define ELFMAG0        0x7f
#define ELFMAG1        'E'
#define ELFMAG2        'L'
#define ELFMAG3        'F'

#define SHT_NULL       0
#define SHT_PROGBITS   1
#define SHT_SYMTAB     2
#define SHT_STRTAB     3
#define SHT_RELA       4
#define SHT_HASH       5
#define SHT_DYNAMIC    6
#define SHT_NOTE       7
#define SHT_NOBITS     8
#define SHT_REL        9
#define SHT_SHLIB      10
#define SHT_DYNSYM     11
#define SHT_UNKNOWN12  12
#define SHT_UNKNOWN13  13
#define SHT_INIT_ARRAY 14
#define SHT_FINI_ARRAY 15
#define SHT_PREINIT_ARRAY 16
#define SHT_GROUP      17
#define SHT_SYMTAB_SHNDX 18
#define SHT_NUM        19

#define SHF_WRITE      1
#define SHF_ALLOC      2
#define SHF_EXECINSTR  4

#define STB_LOCAL      0
#define STB_GLOBAL     1
#define STB_WEAK       2
#define STB_LOOS       10
#define STB_HIOS       11
#define STB_LOPROC     13
#define STB_HIPROC     15

#define STT_FILE       4
#define STT_COMMON     5
#define SHN_COMMON     0xfff2

#define ELF32SYMSIZE   16

#define ELF32_ST_BIND(info) ((info) >> 4)
#define ELF32_ST_TYPE(info) ((info) & 0xf)
#define ELF32_ST_INFO(bind,type) (((bind)<<4) | ((type)&0xf))

#define ELF32_R_SYM(info) ((info) >> 8)
#define ELF32_R_TYPE(info) ((unsigned char)(info))
#define ELF32_R_INFO(sym, type) (((sym)<<8)+(unsigned char)(type))

#define T_NULL 0
#define T_VOID 1
#define T_CHAR 2
#define T_SHORT 3
#define T_INT 4
#define T_LONG 5
#define T_FLOAT 6
#define T_DOUBLE 7
#define T_STRUCT 8
#define T_UNION 9
#define T_ENUM 10
#define T_MOE 11
#define T_UCHAR 12
#define T_USHORT 13
#define T_UINT 14
#define T_ULONG 15
#define T_LNGDBL 16
#define DT_NON 0
#define DT_PTR 1
#define DT_FCN 2
#define DT_ARY 3

#define COFFTYPE(dt,t) (((dt)<<4)|t)

int sortCompare(const void *x1,const void *x2);
void processArgs(int argc,char *argv[]);
void combine_groups(long i,long j);
void combine_common(long i,long j);
void combine_segments(PSEG i,PSEG j);
void combineBlocks();
void OutputWin32file(const std::string &outname);
void OutputEXEfile(const std::string &outname);
void OutputCOMfile(const std::string &outname);
void GetFixupTarget(PRELOC r,PSEG *tseg,UINT *tofs,int isFlat);
void loadlibmod(UINT libnum,UINT modpage);
void loadlib(FILE *libfile,std::string libname);
void loadelf(FILE *libfile);
void loadCoffLib(FILE *libfile,std::string libname);
void loadcofflibmod(PLIBFILE p,FILE *libfile);
long loadmod(FILE *objfile);
void loadres(FILE *resfile);
void loadcoff(FILE *objfile);
void loadCoffImport(FILE *objfile);
void LoadFIXUP(PRELOC r,PUCHAR buf,long *p);
void RelocLIDATA(PDATABLOCK p,long *ofs,PRELOC r);
void EmitLiData(PDATABLOCK p,long segnum,long *ofs);
PDATABLOCK BuildLiData(long *bufofs);
void DestroyLIDATA(PDATABLOCK p);
void ReportError(long errnum);
int wstricmp(const char *s1,const char*s2);
int wstrlen(const char *s);
unsigned short wtoupper(unsigned short a);
int getBitCount(UINT a);
int _wstricmp(unsigned char *a, unsigned char *b);
int _wstrlen(unsigned char *a);
#define strdup _strdup

extern char case_sensitive;
extern char padsegments;
extern char mapfile;
extern std::string mapname;
extern unsigned short maxalloc;
extern int output_type;
extern std::string outname;

extern FILE *afile;
extern UINT filepos;
extern long reclength;
extern unsigned char rectype;
extern char li_le;
extern UINT prevofs;
extern long prevseg;
extern long gotstart;
extern RELOC startaddr;
extern unsigned char osMajor,osMinor;
extern unsigned char subsysMajor,subsysMinor;
extern unsigned int subSystem;

extern long errcount;

extern std::vector<unsigned char> buf;
extern PDATABLOCK lidata;

//extern std::vector<std::string> namelist;
extern std::vector<PSEG> seglist;
extern std::vector<PSEG> outlist;
extern std::vector<PGRP> grplist;
extern SORTLIST publics;
extern std::vector<EXTREC> externs;
extern std::vector<PCOMREC> comdefs;
extern std::vector<PRELOC> relocs;
extern std::vector<IMPREC> impdefs;
extern std::vector<EXPREC> expdefs;
extern std::vector<LIBFILE> libfiles;
extern std::vector<RESOURCE> resource;
extern std::vector<std::string> modname;
extern std::vector<std::string> filename;
extern SORTLIST comdats;
extern UINT namemin,
    pubmin,
    segmin,
    grpmin,
    extmin,
    commin,
    fixmin,
    impmin,impsreq,
    expmin;

extern int buildDll;
extern std::string stubName, entryPoint;
extern int cpuIdent;
