/* File: C:\WACKER\xfer\krm.hh (Created: 28-Jan-1994)
 * created from HAWIN source file
 * krm.hh  --  Internal definitions for KERMIT file transfer
 *					protocol routines.
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#define	Dbg(a,b)
#define	DbgW(a,b,c)
#define	DbgWW(a,b,c,d)
#define	DbgWWW(a,b,c,d,e)
#define	DbgL(a,b,c)

#if defined(ERROR)
#undef	ERROR
#endif
#define ERROR		  (-1)

#if !defined(MAXLINE)
#define	MAXLINE	130
#endif
#if !defined(DEL)
#define	DEL	0177
#endif

#define MAXPCKT 100 		/* maximum length for packet data */
#define K_QCTL '#'
#define K_QBIN '&'
#define K_REPT '~'
#define K_CHK1 1
#define K_CHK2 2
#define K_CHK3 3
#define CAPMASK_ATTR 0x08

#define tochar(ch) ((ch) + ' ')
#define unchar(ch) ((ch) - ' ')
#define ctl(ch) ((ch) ^ 0x40)

/* valid packet types include 'Y', 'N', 'S, 'I', 'F', 'A', 'D', 'Z',
 *	'B', 'E', 'R', 'C', 'K', 'T', 'G'
 *	plus special types returned by rec_packet():
 */
#define BAD_PACKET	'\030'

/* Kermit receive states */
#define KREC_INIT 0
#define KREC_FILE 1
#define KREC_DATA 2
#define KREC_COMPLETE 3
#define KREC_ABORT 4

/* Kermit packet error codes */
#define KE_NOERROR 0
#define KE_TIMEOUT 1
#define KE_NAK 2
#define KE_BAD_PACKET 3
#define KE_RMTERR 4
#define KE_WRONG 5
#define KE_REPEAT 6
#define KE_SEQUENCE 7
#define KE_FATAL 8

/* Kermit abort codes */
#define KA_OK			 0
#define KA_LABORT1		 1
#define KA_RABORT1		 2
#define KA_LABORTALL	 3
#define KA_RABORTALL	 4
#define KA_IMMEDIATE	 5
#define KA_RMTERR		 6
#define KA_LOST_CARRIER  7
#define KA_ERRLIMIT 	 8
#define KA_OUT_OF_SEQ	 9
#define KA_BAD_FORMAT	10
#define KA_TOO_MANY 	11
#define KA_CANT_OPEN	12
#define KA_DISK_FULL	13
#define KA_DISK_ERROR	14
#define KA_OLDER_FILE	15
#define KA_NO_FILETIME	16
#define KA_WONT_CANCEL	17
#define KA_VIRUS_DETECT 18
#define	KA_USER_REFUSED 19

/* progress display codes */
#define FILE_DONE		1
#define TRANSFER_DONE	2

#define KPCKT struct _kpckt
KPCKT
	{
	unsigned char pmark;
	unsigned char plen;
	unsigned char pseq;
	unsigned char ptype;
	unsigned char pdata[MAXPCKT];
	int datalen;
	};

struct s_krm_rcv_control
	{
	int 	  files_received;
	int 	  files_aborted;
	int 	  oldtries;
	int 	  dsptries;
	int 	  total_tries;
	long	  bytes_expected;
	int 	  size_known;
	int 	  lasterr;
	unsigned char  uabort_code;
	int 	  data_packet_rcvd;
	int 	  store_later;

	// struct	  s_filetime compare_time;
	unsigned long ul_compare_time;
	// struct	  s_filetime filetime;

	unsigned long ul_filetime;
	int 	  next_rtype;
	int 	  next_plen;
	int 	  next_rseq;
	unsigned char  next_packet[MAXPCKT];
	KPCKT	  resp_pckt;
	};

typedef struct s_krm_rcv_control ST_R_KRM;

typedef struct s_krm_control ST_KRM;

struct s_krm_control
	{
	HSESSION hSession;
	HCOM	 hCom;
	ST_IOBUF *fhdl;
	long   basesize;
	int    ksequence;
	int    packetnum;
	int    tries;
	int    abort_code;
	void * flagkey_hdl;
	long   xfertime;
	char   xtra_err[MAXLINE];
	char   their_fname[MAXPCKT];
	char   our_fname[FNAME_LEN];
	long   total_dsp;
	long   total_thru;
	long   nbytes;
	int    file_cnt;
	int    files_done;
	int    its_maxl;
	int    its_timeout;
	int    its_npad;
	unsigned char  its_padc;
	unsigned char  its_eol;
	unsigned char  its_chkt;
	unsigned char  its_qctl;
	unsigned char  its_qbin;
	unsigned char  its_rept;
	int    its_capat;
	int    fname_width;

	/* These used to be globals */
	int           k_useattr; 			/* send 'normalized' file names ? */
	int	          k_maxl;				/* maximum packet length we'll take */
	int	          k_timeout; 			/* time they should wait for us */
	unsigned char k_chkt;				/* check type we want to use */
	int	          k_retries; 			/* no. of retries */
	unsigned char k_markchar;			/* first char of each packet */
	unsigned char k_eol; 				/* end of line character for packets */
	int           k_npad;		  		/* no. of pad chars. to send us */
	unsigned char k_padc;				/* pad char. we want */
	 unsigned     total_retries;
	long          displayed_time;

	ST_R_KRM	kr;						/* receive control structure */

	/* variables for sending and receiving */
	KPCKT		*this_kpckt;
	KPCKT		*next_kpckt;
	long		kbytes_sent;
	int			(*p_kputc)(ST_KRM *p, int c);
	long		kbytes_received;
	int			(*p_kgetc)(ST_KRM *p);
	void		(*KrmProgress)(ST_KRM *p, int n);
	};

// extern int	 krm_dbg;	/* used for real-time debugging using dbg.c */

// extern struct s_krm_control FAR *kc;
// extern void (NEARF *KrmProgress)(HSESSION, bits);

// variables for receiving
// extern struct s_krm_rcv_control FAR *krc;
// extern metachar (NEAR *p_kputc)(metachar);
// extern long kbytes_received;

// variables for sending
// extern metachar (NEAR *p_kgetc)(void);
// extern long kbytes_sent;


extern unsigned  ke_msg[];			 /* packet error messages */
extern int		 kresult_code[];	 /* result codes */


/* function prototypes: */
extern void krm_box(int p_toprow,
					int p_leftcol,
					int p_botmrow,
					int p_rightcol);

extern void krm_box_remove(void);

extern void krmGetParameters(ST_KRM *kc);

extern void	ksend_packet(ST_KRM *hK,
							unsigned char type,
							unsigned dlength,
							int seq,
							KPCKT FAR *pckt);

extern int krec_packet(ST_KRM *hK,
						int *len,
						int *seq,
						unsigned char *data);

extern int buildparams(ST_KRM *hK,
						int initiating,
						unsigned char *bufr);

extern void getparams(ST_KRM *hK,
						int initiating,
						unsigned char *bufr);

extern void ks_progress(ST_KRM *hK, int status);
extern int krec_init(ST_KRM *hK);
extern int krec_file(ST_KRM *hK);
extern int krec_data(ST_KRM *hK);
extern void kr_progress(ST_KRM *hK, int status);

extern int kunload_packet(ST_KRM *hK,
						int len,
						unsigned char *bufr);

extern int kr_putc(ST_KRM *kc, int c);

extern void krm_rcheck(ST_KRM *hK, int before);
extern void krm_check_input(int suspend);

extern void kunload_attributes(ST_KRM *hK,
								unsigned char *data,
								KPCKT FAR *rsp_pckt);

extern void krm_settime(unsigned char *data, unsigned long *pl);

extern int kload_packet(ST_KRM *hK,
						unsigned char *bufr);

extern int ks_getc(ST_KRM *kc);

/************************** end of krm.hh *****************************/
