/*
 * xmodem.hh -- Include file for ZMODEM private stuff
 *
 * Copyright 1989 by Hilgraeve Inc. -- Monroe, MI
 * All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

// These constants are used to turn on various types of debug display
// #define DEBUG_DUMPPACKET    // Log bad packet contents to a file

#if defined(ERROR)
#undef	ERROR
#endif
#if defined(CAN)
#undef	CAN
#endif

#define OK				0
#define FALSE			0
#define TRUE			1
#if defined(ERROR)
#undef	ERROR
#endif
#define ERROR		  (-1)
#define FILE_DONE		1
#define TRANSFER_DONE	2

/*
 * Max value for HOWMANY is 255.
 *   A larger value reduces system overhead but may evoke kernel bugs.
 *   133 corresponds to an XMODEM/CRC sector
 */
#ifndef HOWMANY
#define HOWMANY 133
#endif

/* Ward Christensen / CP/M parameters - Don't change these! */
#define ENQ 005
#define CAN ('X'&037)
#define XOFF ('s'&037)
#define XON ('q'&037)
#define SOH 1
#define STX 2
#define EOT 4
#define ACK 6
#define NAK 025
#define CPMEOF 032
#define WANTCRC 0103	/* send C not NAK to get crc not checksum */
#define TIMEOUT (-2)
#define RCDO (-3)
#define ZCARRIER_LOST	 (-4)
#define ERRORMAX 5
#define RETRYMAX 5
#define WCEOT (-10)
#define PATHLEN 257	/* ready for 4.2 bsd ? */
#define UNIXFILE 0xF000	/* The S_IFMT file mask bit for stat */

/* Parameters for ZSINIT frame */

#define ZATTNLEN 32	/* Max length of attention string */

/* Control structure, allocated during transfers to hold control variables */
struct z_mdm_cntrl
	{
	HSESSION hSession;			/* in case we actually need this */
	HCOM     hCom;				/* derived from above */

	int		 nMethod;			/* Zmodem, or Zmodem wiht crash recovery? */
	int		 fSavePartial;		/* Save partially received files? */
	unsigned long	ulOverride;	/* Override "file exists" overwrite options? */

	long	 real_bytes;		/* real number of bytes, not virtual number */
	long	 file_bytes;		/* number of bytes processed in current file */
	long	 total_bytes;		/* number of bytes total in previous files */
	long	 actual_bytes;		/* previous value includes skipped files */
	int		 nSkip;				/* true if skipping a file */
	void *	 flagkey;			/* used for detecting the user abort key */
	jmp_buf  flagkey_buf;		/* used for long jump after user abort */
	jmp_buf  intrjmp;
	ST_IOBUF *fh;				/* handle for the current file */
	long	 basesize;			/* used when appending to a file */
	long	 xfertimer; 		/* used for timing the transfers */
	long	 xfertime;			/* ditto */
	int      nfiles;			/* total number of files to transfer */
	int      filen; 			/* number of the current file */
	long	 filesize;			/* size of the current file */
	long	 nbytes;			/* total number of bytes to transfer */
	int      Rxtimeout;			/* Tenths of a second to wait for something */
	long     *z_cr3tab; 		/* pointer to 32 bit checksum table */
	unsigned short *z_crctab;	/* pointer to 16 bit checksum table */
	int      Rxframeind; 	/* ZBIN ZBIN32, or ZHEX type of frame received */
	int      Rxtype; 			/* Type of header received */
	int      Rxcount;			/* Count of data bytes received */
	char     Rxhdr[4];			/* Received header */
	char     Txhdr[4];			/* Transmitted header */
	long     Rxpos; 			/* Received file position */
	long     Txpos; 			/* Transmitted file position */
	int      Txfcs32; 		/* TRUE means send binary frames with 32 bit FCS */
	int      Crc32t; 		/* Display flag indicating 32 bit CRC being sent */
	int      Crc32;		/* Display flag indicating 32 bit CRC being received */
	char     Attn[ZATTNLEN+1];	/* Attention string rx sends to tx on err */
	int      lastsent;			/* Last char we sent */
	int      Not8bit; 			/* Seven bits seen on header */
	long     displayed_time;

	int      Zctlesc;
	int      Zrwindow;
	int      Eofseen;
	int      tryzhdrtype;
	int	     Thisbinary;
	int	     Filemode;
	long     Modtime;
	int      do_init;
	TCHAR    zconv; 		/* ZMODEM file conversion request */
	TCHAR    zmanag;		/* ZMODEM file management request */
	TCHAR    ztrans;		/* ZMODEM file transport request */
	TCHAR   *secbuf;
	TCHAR   *fname;
	TCHAR   *our_fname;

	stFB	 stP;			/* used in foo com functions */

	TCHAR   *txbuf;
	int	     Filesleft;
	long     Totalleft;
	int	     blklen;
	int	     blkopt; 			 /* do we override zmodem block length */
	int	     Beenhereb4;
	TCHAR	 Wantfcs32;			/* Do we want 32 bit crc			   */
	int	     Rxflags;
	unsigned Rxbuflen;			/* receiver maximum buffer length */
	unsigned Txwindow;
	unsigned Txwcnt;			/* counter used to space ack requests */
	unsigned Txwspac;			/* space between ZCRCQ requests */
	TCHAR    Myattn[1];
	int      errors;
	int      s_error;
	int      pstatus;
	int      last_event;

	int      Lskipnocor;
	long     Lastsync;			/* last offset to which we got a ZRPOS */
	long     Lrxpos;			/* receivers last reported offset */
	};

typedef struct z_mdm_cntrl ZC;

#define updcrc(x,cp,crc) (x->z_crctab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp)
#define UPDC32(x,b,c) (x->z_cr3tab[((int)c ^ b) & 0xff] ^ ((c >> 8) & 0x00FFFFFF))

/* from zmdm.c */

void zsbhdr		(ZC *zc, int type, BYTE *hdr);
void zsbh32		(ZC *zc, BYTE *hdr, int type);
void zshhdr		(ZC *zc, int type, BYTE *hdr);
void zsdata		(ZC *zc, BYTE *buf, int length, int frameend);
void zsda32		(ZC *zc, BYTE *buf, int length, int frameend);

int  zrdata		(ZC *zc, BYTE *buf, int length);
int  zrdat32	(ZC *zc, BYTE *buf, int length);
int  zgethdr	(ZC *zc, BYTE *hdr, int eflag);
int  zrbhdr		(ZC *zc, BYTE *hdr, int eflag);
int  zrbhdr32 	(ZC *zc, BYTE *hdr, int eflag);
int  zrhhdr		(ZC *zc, BYTE *hdr, int eflag);

void zputhex	(ZC *zc, int c);
void zsendline	(ZC *zc, int c);
int  zgethex	(ZC *zc);
int  zgeth1		(ZC *zc);
int  zdlread	(ZC *zc);
int  noxrd7		(ZC *zc);
void stohdr		(ZC *zc, long pos);
long rclhdr		(BYTE *hdr);

int  zmdm_rl	(ZC *zc, int timeout);

/* Functions used to fiddle with the displays */

void zmdms_progress  (ZC *zc, int status);
void zmdms_newfile   (ZC *zc, int filen, TCHAR *fname, long flength);
void zmdms_update    (ZC *zc, int state);

void zmdmr_progress  (ZC *zc, int status);
void zmdmr_update    (ZC *zc, int status);
void zmdmr_filecnt   (ZC *zc, int cnt);
void zmdmr_totalsize (ZC *zc, long bytes);
void zmdmr_newfile   (ZC *zc, int filen, BYTE *theirname, TCHAR *ourname);
void zmdmr_filesize  (ZC *zc, long fsize);

unsigned int zmdm_error  (ZC *zc, int error);
		 int zmdm_retval (ZC *zc, int flag, int error);

/* from zmdm_rcv.c */

int  tryz 	(ZC *zc);
int  rzfiles	(ZC *zc);
int  rzfile	(ZC *zc);
void zmputs	(ZC *zc, char *s);
int  closeit	(ZC *zc);
void ackbibi	(ZC *zc);
/* void bttyout  (char c); */

/* from zmdm_snd.c */

int  wcs		  (ZC *zc, char *oname);
int  wctxpn	  (ZC *zc, char *name);
int  zfilbuf	  (ZC *zc);
void canit	  (ZC *zc);
int  getzrxinit (ZC *zc);
int  sendzsinit (ZC *zc);
int  zsendfile  (ZC *zc, char *buf, int blen);
int  zsendfdata (ZC *zc);
int  getinsync  (ZC *zc, int flag);
void saybibi	  (ZC *zc);

/* the following stuff is here as an attempt to match names into HA5 */

#define readline(h,x)	zmdm_rl(h,x)

#define xsendline(h,p,c)	fooComSendChar(h->hCom,p,c)
#define sendline(h,p,c)		fooComSendChar(h->hCom,p,c)
#define flushmo(h,p)		fooComSendPush(h->hCom,p)

