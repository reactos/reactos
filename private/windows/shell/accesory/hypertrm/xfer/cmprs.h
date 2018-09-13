/* File: C:\WACKER\xfer\cmprs.h (Created: 20-Jan-1994)
 * created from HAWIN sources
 * CMPRS.H -- Exported definitions for HyperACCESS compression routines
 *
 *	Copyright 1989,1991,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#if !defined(EOF)
#define	EOF	(-1)
#endif

#define CMPRS_MINSIZE 4000L /* min. size of file to use compression on */

/* return codes from compress_status() */
#define COMPRESS_IDLE	  0
#define COMPRESS_ACTIVE   1
#define COMPRESS_SHUTDOWN 2
#define COMPRESS_ALL_DONE (-1)

#define DCMP_UNFINISHED (-2)
#define DCMP_FLUSH		(-4)
#define DCMP_RESET		(-5)

#define decompress_status compress_status

extern int   	compress_enable(void);
extern void 	compress_disable(void);
extern unsigned int	compress_status(void);
extern int  	compress_start(int (**getfunc)(void *),
								void *p,
								long *loadcnt,
								int fPauses);
extern void 	compress_stop(void);
extern int  	decompress_start(int (**put_func)(void *, int),
									  void *pP,
									  int fPauses);
extern void 	decompress_stop(void);
extern int 	    decompress_error(void);
extern int      decompress_continue(void);

/* from cmprsrle.c */
#if defined(DOS_HOST)
extern void 	 CmprsRLECompressBufrInit(BYTE *fpuchDataBufr,
					 int sDataCnt);
extern void 	 CmprsRLEDecompressInit(BYTE *fpuchDataBufr,
					 unsigned int usBufrSize,
					 int (*PutCodes)(int mch),
					 unsigned int *pusExpandedCnt);

#else
extern void 	 CmprsRLECompressBufrInit(BYTE FAR *fpuchDataBufr,
					 int sDataCnt);
extern void 	 CmprsRLEDecompressInit(BYTE FAR *fpuchDataBufr,
					 unsigned int usBufrSize,
					 int (*PutCodes)(int mch),
					 unsigned int *pusExpandedCnt);

#endif

extern void CmprsRLECompressBufrFini(void);
extern int  CmprsRLECompress(void);
extern int  CmprsRLEDecompress(int mch);

/***************************** end of cmprs.h **************************/
