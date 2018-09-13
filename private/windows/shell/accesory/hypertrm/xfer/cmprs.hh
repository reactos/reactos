/* File: C:\WACKER\xfer\cmprs.hh (Created: 20-Jan-1994)
 * created from HAWIN sources
 * cmprs.hh -- Internal definitions for compression routines
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
// Set to 1 or 0 to enable/disable descriptive output
// #define SHOW 0

// Set to 1 or 0 to enable/disable use of assembly language module
#define USE_ASM 0


#define MAXNODES 4096	/* number of nodes in lookup tables */
#define MAXCODEBITS 12	/* largest code size in bits */
#define CLEARCODE 256	/* special code from compressor to decompressor to
							signal it to clear the table and start anew */
#define STOPCODE 257	/* special code from compressor to decompressor to
							signal it that following data will
							not be compressed */
#define FIRSTFREE 258	/* first code available to code pattern */


/* These variables are shared by the compression and decompression routines */

extern void *compress_tblspace;

struct s_cmprs_node
	{
	struct s_cmprs_node *first;
	struct s_cmprs_node *next;
	BYTE cchar;
	};


extern unsigned long  ulHoldReg;
extern int            sBitsLeft;
extern int            sCodeBits;
extern unsigned int   usMaxCode;
extern unsigned int   usFreeCode;
extern unsigned int   usxCmprsStatus;
extern int            fxLastBuildGood;
extern int            fFlushable;		// True if compression stream can

/* function prototypes: */

/* from cmprs1.c */
extern void   cmprs_inittbl(void);
extern int    cmprs_shutdown(void *);

/* from cmpgetc.asm */
extern int    cmprs_getc(void *);

/* from cmprs2.c */
extern int    dcmp_abort(void);
extern int    dcmp_start(void *pX, int c);
extern int    dcmp_putc(void *pX, int c);

/* from dcmplook.asm */
extern int    dcmp_lookup(unsigned int code);
extern void   dcmp_inittbl(void);


/* end of cmprs.hh */
