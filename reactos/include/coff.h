/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_coff_h_
#define __dj_include_coff_h_

#ifdef __cplusplus
extern "C" {
#endif

//#ifndef __dj_ENFORCE_ANSI_FREESTANDING

//#ifndef __STRICT_ANSI__

//#ifndef _POSIX_SOURCE

/*** coff information for Intel 386/486.  */

/********************** FILE HEADER **********************/

struct external_filehdr {
	unsigned short f_magic;		/* magic number			*/
	unsigned short f_nscns;		/* number of sections		*/
	unsigned long f_timdat;	/* time & date stamp		*/
	unsigned long f_symptr;	/* file pointer to symtab	*/
	unsigned long f_nsyms;		/* number of symtab entries	*/
	unsigned short f_opthdr;	/* sizeof(optional hdr)		*/
	unsigned short f_flags;		/* flags			*/
};


/* Bits for f_flags:
 *	F_RELFLG	relocation info stripped from file
 *	F_EXEC		file is executable (no unresolved external references)
 *	F_LNNO		line numbers stripped from file
 *	F_LSYMS		local symbols stripped from file
 *	F_AR32WR	file has byte ordering of an AR32WR machine (e.g. vax)
 */

#define F_RELFLG	(0x0001)
#define F_EXEC		(0x0002)
#define F_LNNO		(0x0004)
#define F_LSYMS		(0x0008)



#define	I386MAGIC	0x14c
#define I386AIXMAGIC	0x175
#define I386BADMAG(x) (((x).f_magic!=I386MAGIC) && (x).f_magic!=I386AIXMAGIC)


#define	FILHDR	struct external_filehdr
#define	FILHSZ	sizeof(FILHDR)


/********************** AOUT "OPTIONAL HEADER" **********************/


typedef struct 
{
  unsigned short 	magic;		/* type of file				*/
  unsigned short	vstamp;		/* version stamp			*/
  unsigned long	tsize;		/* text size in bytes, padded to FW bdry*/
  unsigned long	dsize;		/* initialized data "  "		*/
  unsigned long	bsize;		/* uninitialized data "   "		*/
  unsigned long	entry;		/* entry pt.				*/
  unsigned long 	text_start;	/* base of text used for this file */
  unsigned long 	data_start;	/* base of data used for this file */
}
AOUTHDR;


typedef struct gnu_aout {
	unsigned long info;
	unsigned long tsize;
	unsigned long dsize;
	unsigned long bsize;
	unsigned long symsize;
	unsigned long entry;
	unsigned long txrel;
	unsigned long dtrel;
	} GNU_AOUT;

#define AOUTSZ (sizeof(AOUTHDR))

#define OMAGIC          0404    /* object files, eg as output */
#define ZMAGIC          0413    /* demand load format, eg normal ld output */
#define STMAGIC		0401	/* target shlib */
#define SHMAGIC		0443	/* host   shlib */


/********************** SECTION HEADER **********************/


struct external_scnhdr {
	char		s_name[8];	/* section name			*/
	unsigned long		s_paddr;	/* physical address, aliased s_nlib */
	unsigned long		s_vaddr;	/* virtual address		*/
	unsigned long		s_size;		/* section size			*/
	unsigned long		s_scnptr;	/* file ptr to raw data for section */
	unsigned long		s_relptr;	/* file ptr to relocation	*/
	unsigned long		s_lnnoptr;	/* file ptr to line numbers	*/
	unsigned short		s_nreloc;	/* number of relocation entries	*/
	unsigned short		s_nlnno;	/* number of line number entries*/
	unsigned long		s_flags;	/* flags			*/
};

#define	SCNHDR	struct external_scnhdr
#define	SCNHSZ	sizeof(SCNHDR)

/*
 * names of "special" sections
 */
#define _TEXT	".text"
#define _DATA	".data"
#define _BSS	".bss"
#define _COMMENT ".comment"
#define _LIB ".lib"

/*
 * s_flags "type"
 */
#define STYP_TEXT	 (0x0020)	/* section contains text only */
#define STYP_DATA	 (0x0040)	/* section contains data only */
#define STYP_BSS	 (0x0080)	/* section contains bss only */

/********************** LINE NUMBERS **********************/

/* 1 line number entry for every "breakpointable" source line in a section.
 * Line numbers are grouped on a per function basis; first entry in a function
 * grouping will have l_lnno = 0 and in place of physical address will be the
 * symbol table index of the function name.
 */
struct external_lineno {
	union {
		unsigned long l_symndx __attribute__((packed));	/* function name symbol index, iff l_lnno == 0 */
		unsigned long l_paddr __attribute__((packed));		/* (physical) address of line number */
	} l_addr;
	unsigned short l_lnno;						/* line number */
};


#define	LINENO	struct external_lineno
#define	LINESZ	sizeof(LINENO)


/********************** SYMBOLS **********************/

#define E_SYMNMLEN	8	/* # characters in a symbol name	*/
#define E_FILNMLEN	14	/* # characters in a file name		*/
#define E_DIMNUM	4	/* # array dimensions in auxiliary entry */

struct external_syment 
{
  union {
    char e_name[E_SYMNMLEN];
    struct {
      unsigned long e_zeroes __attribute__((packed));
      unsigned long e_offset __attribute__((packed));
    } e;
  } e;
  unsigned long e_value __attribute__((packed));
  short e_scnum;
  unsigned short e_type;
  unsigned char e_sclass;
  unsigned char e_numaux;
};

#define N_BTMASK	(0xf)
#define N_TMASK		(0x30)
#define N_BTSHFT	(4)
#define N_TSHIFT	(2)
  
union external_auxent {
	struct {
		unsigned long x_tagndx __attribute__((packed));		/* str, un, or enum tag indx */
		union {
			struct {
			    unsigned short  x_lnno;				/* declaration line number */
			    unsigned short  x_size; 				/* str/union/array size */
			} x_lnsz;
			unsigned long x_fsize __attribute__((packed));		/* size of function */
		} x_misc;
		union {
			struct {					/* if ISFCN, tag, or .bb */
			    unsigned long x_lnnoptr __attribute__((packed));	/* ptr to fcn line # */
			    unsigned long x_endndx __attribute__((packed));	/* entry ndx past block end */
			} x_fcn;
			struct {					/* if ISARY, up to 4 dimen. */
			    unsigned short x_dimen[E_DIMNUM];
			} x_ary;
		} x_fcnary;
		unsigned short x_tvndx;						/* tv index */
	} x_sym;

	union {
		char x_fname[E_FILNMLEN];
		struct {
			unsigned long x_zeroes __attribute__((packed));
			unsigned long x_offset __attribute__((packed));
		} x_n;
	} x_file;

	struct {
		unsigned long x_scnlen __attribute__((packed));		/* section length */
		unsigned short x_nreloc;					/* # relocation entries */
		unsigned short x_nlinno;					/* # line numbers */
	} x_scn;

        struct {
		unsigned long x_tvfill __attribute__((packed));		/* tv fill value */
		unsigned short x_tvlen;						/* length of .tv */
		unsigned short x_tvran[2];					/* tv range */
	} x_tv;		/* info about .tv section (in auxent of symbol .tv)) */


};

#define	SYMENT	struct external_syment
#define	SYMESZ	sizeof(SYMENT)
#define	AUXENT	union external_auxent
#define	AUXESZ	sizeof(AUXENT)


#	define _ETEXT	"etext"


/* Relocatable symbols have number of the section in which they are defined,
   or one of the following: */

#define N_UNDEF	((short)0)	/* undefined symbol */
#define N_ABS	((short)-1)	/* value of symbol is absolute */
#define N_DEBUG	((short)-2)	/* debugging symbol -- value is meaningless */
#define N_TV	((short)-3)	/* indicates symbol needs preload transfer vector */
#define P_TV	((short)-4)	/* indicates symbol needs postload transfer vector*/

/*
 * Type of a symbol, in low N bits of the word
 */
#define T_NULL		0
#define T_VOID		1	/* function argument (only used by compiler) */
#define T_CHAR		2	/* character		*/
#define T_SHORT		3	/* short integer	*/
#define T_INT		4	/* integer		*/
#define T_LONG		5	/* long integer		*/
#define T_FLOAT		6	/* floating point	*/
#define T_DOUBLE	7	/* double word		*/
#define T_STRUCT	8	/* structure 		*/
#define T_UNION		9	/* union 		*/
#define T_ENUM		10	/* enumeration 		*/
#define T_MOE		11	/* member of enumeration*/
#define T_UCHAR		12	/* unsigned character	*/
#define T_USHORT	13	/* unsigned short	*/
#define T_UINT		14	/* unsigned integer	*/
#define T_ULONG		15	/* unsigned long	*/
#define T_LNGDBL	16	/* long double		*/

/*
 * derived types, in n_type
*/
#define DT_NON		(0)	/* no derived type */
#define DT_PTR		(1)	/* pointer */
#define DT_FCN		(2)	/* function */
#define DT_ARY		(3)	/* array */

#define BTYPE(x)	((x) & N_BTMASK)

#define ISPTR(x)	(((x) & N_TMASK) == (DT_PTR << N_BTSHFT))
#define ISFCN(x)	(((x) & N_TMASK) == (DT_FCN << N_BTSHFT))
#define ISARY(x)	(((x) & N_TMASK) == (DT_ARY << N_BTSHFT))
#define ISTAG(x)	((x)==C_STRTAG||(x)==C_UNTAG||(x)==C_ENTAG)
#define DECREF(x) ((((x)>>N_TSHIFT)&~N_BTMASK)|((x)&N_BTMASK))

/********************** STORAGE CLASSES **********************/

/* This used to be defined as -1, but now n_sclass is unsigned.  */
#define C_EFCN		0xff	/* physical end of function	*/
#define C_NULL		0
#define C_AUTO		1	/* automatic variable		*/
#define C_EXT		2	/* external symbol		*/
#define C_STAT		3	/* static			*/
#define C_REG		4	/* register variable		*/
#define C_EXTDEF	5	/* external definition		*/
#define C_LABEL		6	/* label			*/
#define C_ULABEL	7	/* undefined label		*/
#define C_MOS		8	/* member of structure		*/
#define C_ARG		9	/* function argument		*/
#define C_STRTAG	10	/* structure tag		*/
#define C_MOU		11	/* member of union		*/
#define C_UNTAG		12	/* union tag			*/
#define C_TPDEF		13	/* type definition		*/
#define C_USTATIC	14	/* undefined static		*/
#define C_ENTAG		15	/* enumeration tag		*/
#define C_MOE		16	/* member of enumeration	*/
#define C_REGPARM	17	/* register parameter		*/
#define C_FIELD		18	/* bit field			*/
#define C_AUTOARG	19	/* auto argument		*/
#define C_LASTENT	20	/* dummy entry (end of block)	*/
#define C_BLOCK		100	/* ".bb" or ".eb"		*/
#define C_FCN		101	/* ".bf" or ".ef"		*/
#define C_EOS		102	/* end of structure		*/
#define C_FILE		103	/* file name			*/
#define C_LINE		104	/* line # reformatted as symbol table entry */
#define C_ALIAS	 	105	/* duplicate tag		*/
#define C_HIDDEN	106	/* ext symbol in dmert public lib */

/********************** RELOCATION DIRECTIVES **********************/



struct external_reloc {
  unsigned long r_vaddr __attribute__((packed));
  unsigned long r_symndx __attribute__((packed));
  unsigned short r_type;
};


#define RELOC struct external_reloc
#define RELSZ sizeof(RELOC)

#define RELOC_REL32	20	/* 32-bit PC-relative address */
#define RELOC_ADDR32	6	/* 32-bit absolute address */

#define DEFAULT_DATA_SECTION_ALIGNMENT 4
#define DEFAULT_BSS_SECTION_ALIGNMENT 4
#define DEFAULT_TEXT_SECTION_ALIGNMENT 4
/* For new sections we havn't heard of before */
#define DEFAULT_SECTION_ALIGNMENT 4

//#endif /* !_POSIX_SOURCE */
//#endif /* !__STRICT_ANSI__ */
//#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_coff_h_ */
