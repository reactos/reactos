/* File: C:\WACKER\xfer\hpr.hh (Created: 24-Jan-1994)
 * created from HAWIN source file:
 * hpr.hh  --  Header file for HyperProtocol routines.
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */


/* The NOTIMEOUTS constant can be defined to prevent HyperProtocol from
 *	timing out while waiting for the other end of the connection. This is
 *	useful while debugging when events may be drawn out by debugging activities
 */
/* #define NOTIMEOUTS */


/* Constants for HyperProtocol */

#define H_MSGCHAR		0x01
#define MAXRETRIES		5
#define MAX_START_TRIES 25
#define RESYNCH_TIME	50
#define RESYNCH_INC 	50
// #define RESYNCH_TIME    300
// #define RESYNCH_INC	   300
#define FINAL_ACK_WAIT	1800L
#define BURSTSIZE		5
#define TIMEOUT_LIMIT	3
#define H_START_WAIT	100 	/* no. of seconds to wait for startup */
#define H_MINBLOCK		128
#define H_MINDEADMAN	10
#define H_CHARTIME		100L

#define tochar(c) ((BYTE)((c) + 0x20))
#define unchar(c) ((BYTE)((c) - 0x20))
#define h_crc_calc(x, cc)\
	{\
	unsigned q;\
	q = (x->h_crc ^ (cc)) & 0x0F;\
	x->h_crc = (x->h_crc >> 4) ^ (q * 0x1081);\
	q = (x->h_crc ^ ((cc) >> 4)) & 0x0F;\
	x->h_crc = (x->h_crc >> 4) ^ (q * 0x1081);\
	}

/* * * * * * * * * * * * * * * * * * * * * * *
 * This section pertains to the sender only  *
 * * * * * * * * * * * * * * * * * * * * * * */

/* status display codes passed to hsdsp_status() */
#define HSS_WAITSTART	0
#define HSS_SENDING 	1
#define HSS_WAITACK 	2
#define HSS_WAITRESUME	3
#define HSS_CANCELLED	4
#define HSS_COMPLETE	5
#define HSS_MAX 		6

/* event display codes passed to hsdsp_event() */
#define HSE_GOTSTART	0
#define HSE_GOTRETRY	1
#define HSE_GOTRESUME	2
#define HSE_GOTACK		3
#define HSE_USRABORT	4
#define HSE_RMTABORT	5
#define HSE_USRCANCEL	6
#define HSE_RMTCANCEL	7
#define HSE_FILEERR 	8
#define HSE_NORESP		9
#define HSE_FULL		10
#define HSE_DONE		11
#define HSE_ILLEGAL 	12
#define HSE_ERRLIMIT	13
#define HSE_MAX 		14


/* structure for the file status table. This table holds details about files
 *	during and after the time they are sent until they are acknowledged by
 *	the receiver.
 */
struct s_ftbl
	{
	int filen;				/* file number (1st file sent is 1, etc.		 */
	int cntrl;				/* control bits FTC_CONFIRMED and FTC_COMPRESSED */
	int status;				/* transfer status, normal, aborted, error, etc. */
	long flength;			/* length of this file in bytes 				 */
	long dsp_bytes; 		/* length of all files prior to this file		 */
	long thru_bytes;		/* amt. of data actually sent prior to this file */
	TCHAR fname[FNAME_LEN]; /* file name */
	};

/* file table control bits */
#define FTC_CONFIRMED	0x01
#define FTC_COMPRESSED	0x02
#define FTC_DONT_CMPRS	0x04

/* file table status values */
#define FTS_NORMAL		0
#define FTS_USRABORT	1
#define FTS_RMTABORT	2
#define FTS_FILEERR 	3
#define FTS_COMMERR 	4

/* control structure used by hyperprotocol sender */
struct s_hsc
	{
	int 	ft_current; 		/* index to current file in file table */
	int 	ft_open;			/* index to currently opened file */
	int 	ft_top; 			/* index of highest used entry in file table */
	int 	ft_limit;			/* number of entries available in file table */
	jmp_buf jb_bailout;			/* jump buffer used to bail out of xfer */
	jmp_buf jb_restart;			/* jump buffer used during restarts */
	int		rmt_compress;		/* TRUE if remote has indicated it is able to
										handle compression */
	int 	nfiles; 			/* no. of files to be transferred */
	long	nbytes; 			/* no. of bytes to be transferred */
	long	bytes_sent; 		/* no. of bytes actually passed to xmitter */

	int 	rmtchkt;			/* Type of error checking requested by remote
										session. H_CRC by default. */
	long	last_response;		/* time last message received */
	int 	lastmsgn;			/* last message number */
	int		rmtcancel;			/* TRUE if remote has cancelled xfer */
	int		started;			/* TRUE after 1st restart message is received
										from remote */
	int 	lasterr_filenum;	/* file number of last restart request */
	long	lasterr_offset; 	/* file offset of last restart request */
	int 	sameplace;			/* no. of consecutive restarts to same spot*/
	int		receiver_timedout;	/* true if receiver sent time out msg */

	struct s_ftbl *hs_ftbl;		/* pointer to dynamically allocated file
									progress table */
	int		(*hs_ptrgetc)(void *);	/* pointer to function to get
										characters to send */
	};


/* globals used by sender */
// extern struct s_hsc FAR *hsc;		/* contains transfer control variables */
// extern struct s_ftbl FAR *hs_ftbl;	/* pointer to dynamically allocated file
//										progress table */
// extern metachar (NEAR *hs_ptrgetc)(void);/* pointer to function to get
//											characters to send */

/* macros used to send characters using the double buffering scheme */
#define HSXB_SIZE	1024
#define HSXB_CYCLE	75

#define hs_xmit_(p,c) (--p->hsxb.cnt > 0 ? (*p->hsxb.bptr++ = (c)) : hs_xmit_switch(p,c))
#define HS_XMIT(p,c) {if ((c) == H_MSGCHAR) hs_xmit_(p,c); hs_xmit_(p,c);}

struct s_hsxb
	{
	char *curbufr;		/* buffer being filled */
	char *altbufr;		/* other buffer, possibly being transmitted */
	char *bptr; 		/* place to put next character to be sent */
	unsigned bufrsize;	/* size of each buffer in bytes */
	unsigned total; 	/* number of bytes still free in the current buffer
						   not counting those in the current fill cycle */
	unsigned cnt;		/* number of bytes left to load in this fill cycle */
	};


// extern unsigned hse_text[];
// extern unsigned hss_text[];
// extern struct s_hsxb hsxb;		/* control structure for double buffered
//										transmission scheme */

#define HS_XMIT_CLEAR hs_xbclear
#define HS_XMIT_FLUSH hs_xbswitch

struct s_hprsd
	{
	int 	 k_received;
	unsigned int hld_options;
	};

// extern struct s_hprsd FAR *hsd;

/* * * * * * * * * * * * * * * * * * * * * * * *
 * This section pertains to the receiver only  *
 * * * * * * * * * * * * * * * * * * * * * * * */

/* Result codes for overall transfer */
#define H_OK			0
#define H_ERRLIMIT		1
#define H_BADMSGFMT 	2
#define H_FILEERR		3
#define H_NORESYNCH 	4
#define H_USERABORT 	5
#define H_TIMEOUT		6
#define H_COMPLETE		7
#define H_NOSTART		8
#define H_RMTABORT		9

/* Result codes for data collection routines */
#define HR_UNDECIDED	0
#define HR_COMPLETE 	1
#define HR_TIMEOUT		2
#define HR_MESSAGE		3
#define HR_KBDINT		4
#define HR_FILEERR		5
#define HR_NOCARRIER	6
#define HR_BADCHECK 	7
#define HR_BADMSG		8
#define HR_LOSTDATA 	9
#define HR_DCMPERR		10
#define HR_LOST_CARR	11
#define	HR_VIRUS_FOUND	12

/* Hyper receive status codes for hrdsp_status() */
#define HRS_REQSTART	0
#define HRS_REQRESYNCH	1
#define HRS_REC 		2
#define HRS_CANCELLED	3
#define HRS_COMPLETE	4
#define HRS_MAX 		5

/* event codes for hrdsp_event() */
#define HRE_NONE			0
#define HRE_DATAERR 		1
#define HRE_LOSTDATA		2
#define HRE_NORESP			3
#define HRE_RETRYERR		4
#define HRE_ILLEGAL 		5
#define HRE_ERRFIXED		6
#define HRE_RMTABORT		7
#define HRE_USRCANCEL		8
#define HRE_TIMEOUT 		9
#define HRE_DCMPERR 		10
#define HRE_LOST_CARR		11
#define HRE_TOO_MANY		12
#define HRE_DISK_FULL		13
#define HRE_CANT_OPEN		14
#define HRE_DISK_ERR		15
#define HRE_OLDER_FILE		16
#define HRE_NO_FILETIME 	17
#define HRE_VIRUS_DET		18
#define	HRE_USER_SKIP		19
#define HRE_USER_REFUSED 	20
#define HRE_MAX 			21


/* Structure containing control variables for the receiver */
struct s_hrc
	{
	long  bytes_expected;		/* total number of bytes expected for xfer */
	long  checkpoint;			/* number of bytes known to be good */
	long  basesize; 			/* original size of file being appended to */
	int   expected_msg; 		/* number of next expected message */
								/* a timestamp of 0 means unused */
	unsigned long ul_lstw;		/* timestamp of file being received, if any */
	unsigned long ul_cmp;		/* time to compare to if /N option used */
								/* a timestamp of -1 means rejected by time */
	long  filesize; 			/* size of file being received */
	int   files_expected;		/* number of files to be rcvd. during xfer */
	BYTE  rmtfname[FNAME_LEN];  /* name remote system used for file */
	TCHAR ourfname[FNAME_LEN]; /* name we will use for file */
	int   cancel_reason;		/* event code associated with cancellation */
	int   using_compression;	/* TRUE if we're using compression */
	BYTE rmsg_bufr[256];		/* buffer to build outgoing messages */
	int  single_file;			/* TRUE if user named single file as dest */
	//SSHDLMCH ssmchVscan;		/* handle for virus scanning */
	int virus_detected;			/* set TRUE upon virus detection */
	//FARPROC pfVirusCheck;		/* pointer to function for virus check */

	int (*hr_ptr_putc)(void *, int);
	};

// VOID _export PASCAL hr_virus_detect(VOID FAR *h, USHORT usMatchId);

/* globals for receiver */
// extern struct s_hrc FAR *hrc;/* pointer to receiver's control structure */
// extern metachar (NEAR *hr_ptr_putc)(metachar);

extern int hr_result_codes[];  /* maps HyperProtocol event codes to
									transfer status codes */

/******************************************************/
/* control structure used by both sender and receiver */
/******************************************************/

struct s_hc
	{
	HSESSION  hSession;

	int 	  blocksize;		/* current size of data blocks */
	int 	  current_filen;	/* current file number */
	int 	  datacnt;			/* keeps track of data sent or received */
	long	  deadmantime;		/* time between dead man notices */
	unsigned  total_tries;		/* number of restarts */
	long	  total_thru;		/* total number of bytes transferred */
	long	  total_dsp;		/* total number of bytes for vu display */
	int		  ucancel;			/* set true if user hits cancel key */
	int		  usecrc;			/* set true if CRC is being used */
	long	  xfertimer;		/* interval timer value */
	long	  xfertime; 		/* transfer time */
	BYTE	  omsg_bufr[256];	/* outgoing message buffer */
	BYTE	 *omsg; 			/* pointer into outgoing message buffer */
	int 	  omsgn;			/* next outgoing message number */
	int		  omsg_printable;	/* true of outgoing message characters
									should be confined to printable chrs */
	int		  omsg_embed;		/* true if outgoing messages should be embedded
									in other outgoing data */
	long	  last_omsg;		/* time last outgoing message was sent */

	unsigned  h_checksum;		/* used to calculate data checksum */
	unsigned  h_crc;			/* used to calculate data crcs */
	long	  h_filebytes;		/* counts bytes transferred */

								/* External parameters set by the user */
	int       h_useattr;		/* use received attributes when available */
	int       h_trycompress;	/* try to use compression when possible */
	int       h_chkt;			/* 1 == checksum, 2 == CRC */
	int       h_resynctime;		/* new resync time value */
	int       h_suspenddsk;		/* old feature, no longer used */

	long      displayed_time;	/* used in display timing */

	BYTE     *storageptr;		/* temporary use */

	BYTE      msgdata[96];		/* more static storate stuff */
	BYTE     *dptr;
	int       rmcnt;

	struct s_hrc rc;			/* data used for receiving */

	struct s_hsc sc;			/* data used for sending */

	struct s_hsxb hsxb;			/* control structure for double buffered */

	struct s_hprsd sd;			/* display stuff */

	ST_IOBUF *fhdl;
	};


/* Globals used by sender and receiver.
 * These items are defined as globals rather than being placed int one
 *	of the control structures because they are accessed frequently in
 *	speed-sensitive portions of the code.
 */
// extern struct s_hc FAR *hc; 			/* pointer to control structure */
// extern unsigned    h_checksum;		/* used to calculate data checksum */
// extern unsigned    h_crc;			/* used to calculate data crcs */
// extern long 	   h_filebytes; 	/* counts bytes transferred */

/*************************/
/* function prototypes : */
/*************************/

/* used by both sender and receiver -- */
extern void 	omsg_init(struct s_hc *hc, int fPrintable, int fEmbedMsg);
extern void 	omsg_new(struct s_hc *hc, BYTE type);
extern int  	omsg_add(struct s_hc *hc, BYTE *newfield);
extern int		omsg_setnum(struct s_hc *hc, int n);
extern int		omsg_send(struct s_hc *hc,
						int burstcnt,
						int usecrc,
						int backspace);
extern long 	omsg_last(struct s_hc *hc);
extern int		omsg_number(struct s_hc *hc);

// extern void	   h_crc_calc(uchar);

/* used by receiver -- */
extern int		hr_collect_msg(struct s_hc *, int *, BYTE **, long);
extern int      hr_storedata(void *, int);
extern void 	hr_still_alive(struct s_hc *, int, int);
extern void 	hr_kbdint(struct s_hc *);
extern int		hr_decode_msg(struct s_hc *, BYTE *);
extern int		hr_reject_file(struct s_hc *, int);
extern int  	hr_closefile(struct s_hc *, int);
extern int		hr_collect_data(struct s_hc *, int *, int, long);
extern int  	hr_cancel(struct s_hc *, int);
extern int  	hr_restart(struct s_hc *, int);
extern int  	hr_resynch(struct s_hc *, int);
extern int      hr_putc(void *, int);
extern int      hr_putc_vir(void *, int);
extern int      hr_toss(void *, int);

extern void  hr_suspend_input(VOID *hS, int suspend);
extern void  hr_check_input(VOID *hS, int suspend);

/* used by sender -- */
extern int  	hyper_sendx(struct s_hc *, int);
extern int  	hs_datasend(struct s_hc *);
extern void 	hs_dorestart(struct s_hc *, int, long, int, int);
extern int		hs_reteof(struct s_hc *);
extern int		hs_getc(struct s_hc *);
extern void 	hs_background(struct s_hc *);
extern void 	hs_rcvmsg(struct s_hc *);
extern void 	hs_filebreak(struct s_hc *hc, int nfiles, long nbytes);
extern void 	hs_waitack(struct s_hc *);
extern void 	hs_decode_rmsg(struct s_hc *, BYTE *);
extern void 	hs_fileack(struct s_hc *, int);
extern void 	hs_logx(struct s_hc *, int);

/******************* end of hpr.hh *************************/
