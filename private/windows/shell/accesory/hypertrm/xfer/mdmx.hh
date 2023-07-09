/* File: C:\WACKER\xfer\mdmx.hh (Created: 17-Jan-1994)
 * created from HAWIN source file
 * mdmx.hh -- Internal definitions for xmodem file transfer routines
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */


/* Various constants */
#define SMALL_PACKET 128
#define LARGE_PACKET 1024
#define SMALL_WAIT	50
#define MEDIUM_WAIT 200
#define LARGE_WAIT	400

/* Block results */
#define UNDEFINED	   0
#define GOOD_PCKT	   1
#define ALT_PCKT	   2
#define NOBATCH_PCKT   3
#define END_PCKT	   4
#define REPEAT_PCKT    5
#define WRONG_PCKT	   6
#define SHORT_PCKT	   7
#define BAD_FORMAT	   8
#define BAD_CHECK	   9
#define NO_PCKT 	   10
#define BLK_ABORTED    11
#define CARRIER_LOST   12
#define CANNED		   13

/* Responses include any character plus the following: */
#define NO_RESPONSE 	-1
#define ABORTED 		-2	/* also used as BlockReceive status */
#define CARR_LOST		-3

/* transfer status codes */
#define XS_OK			  0
#define XS_ABORTED		  1
#define XS_CANCELLED	  2
#define XS_NO_RESPONSE	  3
#define XS_COMMERR		  4
#define XS_FILEERR		  5
#define XS_SYNCHERR 	  6
#define XS_NOSPACE		  7
#define XS_CANTOPEN 	  8
#define XS_COMPLETE 	  9
#define XS_SHORTFILE	  10
#define XS_BATCH_EXPECTED 11

/* Status codes for mdmx_progress */
#define FILE_DONE	  1
#define TRANSFER_DONE 2

/* Control structure, allocated during transfers to hold control variables */
typedef	struct s_mdmx_cntrl ST_MDMX;
struct s_mdmx_cntrl
	{
	HSESSION hSession;
	HCOM     hCom;					/* derived from the above */

	int      nProto;				/* The current protocol */

	long	 file_bytes;
	long	 total_bytes;
	void *	 flagkey;
	ST_IOBUF *fh;
	long	 basesize;
	long	 xfertimer;
	long	 xfertime;
	unsigned nfiles;
	unsigned filen;
	long	 filesize;
	long	 nbytes;
	long     mdmx_byte_cnt;		  /* count of bytes transferred */
	long     displayed_time;      /*  = -1L; */

	struct s_mdmx_pckt *next_pckt;
	unsigned this_pckt;

	int      check_type;
	int      batch;
	int      streaming;

	unsigned short *p_crc_tbl;    /* pointer to the CRC-16 table */

	int      mdmx_chkt;
	int      mdmx_tries;
	int      mdmx_chartime;
	int      mdmx_pckttime;

	stFB	 stP;				/* Used in ComSend functions */

	int      (*p_getc)(ST_MDMX *);
	int      (*p_putc)(ST_MDMX *, int);
	};

// typedef	struct s_mdmx_cntrl ST_MDMX;

/* Global control variables */
// extern struct s_mdmx_cntrl *xc;
// extern metachar (NEAR *p_getc)(VOID); // for snd
// extern metachar (NEAR *p_putc)(metachar); // for rcv


/* Packet structure: allocated during transfers to hold packet contents */
struct s_mdmx_pckt
	{
	int    result;
	int    expected;
	int    pcktsize;
	long   byte_count;
	BYTE   start_char;
	BYTE   pcktnum;
	BYTE   npcktnum;
	BYTE   bdata[1];
	};

/* function prototypes : */
extern void	 mdmx_progress(ST_MDMX *pX, int status);

// extern unsigned calc_crc(unsigned startval, BYTE *pntr, int count);

extern unsigned short calc_crc(ST_MDMX *pX,
								unsigned short startval,
								LPSTR pntr,
								int cnt);

int load_pckt(ST_MDMX *pX,
				 struct s_mdmx_pckt *p,
				 unsigned pcktnum,
				 int kpckt,
				 int chktype);

int xm_getc(ST_MDMX *pX);

int xs_unload(ST_MDMX *pX, BYTE *cp, int size);

int xm_putc(ST_MDMX *pX, int c);


/*
 * the following were added for HA5G
 */

#define ComSendInit(h,p)		fooComSendClear(h,p)
#define ComSendWait(h,p)		fooComSendPush(h,p)
#define ComSendPush(h,p)		fooComSendPush(h,p)
#define ComSendChar(h,p,c)		fooComSendChar(h,p,c)
#define ComSendCharNow(h,p,c)	fooComSendCharNow(h,p,c)

// #define RemoteGet(h)		mComRcvChar(mGetComHdl(h))
// #define RemoteClear(h)		mComRcvBufrClear(mGetComHdl(h))

#define SOH 1
#define STX 2
#define EOT 4
#define ACK 6
#define	CAN	0x18
#define NAK 025
#define	ESC 0x1B

#define CPMEOF 032

extern void mdmxXferInit(ST_MDMX *pX, int method);

/*
 * The following are display functions, some of which might be removed and
 * replaced with MACROS.
 */

extern void mdmxdspFilecnt(ST_MDMX *pX, int cnt);

extern void mdmxdspErrorcnt(ST_MDMX *pX, int cnt);

extern void mdmxdspPacketErrorcnt(ST_MDMX *pX, int cnt);

extern void mdmxdspTotalsize(ST_MDMX *pX, long bytes);

extern void mdmxdspFilesize(ST_MDMX *pX, long fsize);

extern void mdmxdspNewfile(ST_MDMX *pX,
						   int filen,
						   LPSTR theirname,
						   LPTSTR ourname);

extern void mdmxdspProgress(ST_MDMX *pX,
							long stime,
							long ttime,
							long cps,
							long file_so_far,
							long total_so_far);

extern void mdmxdspChecktype(ST_MDMX *pX, int ctype);

extern void mdmxdspPacketnumber(ST_MDMX *pX, long number);

extern void mdmxdspLastError(ST_MDMX *pX, int errcode);

extern void mdmxdspCloseDisplay(ST_MDMX *pX);

/* end of mdmx.hh */
