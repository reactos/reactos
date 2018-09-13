#ifndef WINAPI
#ifdef BUILDDLL                                 /* ;Internal */
#define WINAPI              _loadds far pascal  /* ;Internal */
#define CALLBACK	    _loadds far pascal	/* ;Internal */
#else						/* ;Internal */
#define WINAPI              far pascal
#define CALLBACK	    far pascal
#endif                                          /* ;Internal */
#endif

#define LWORD(x)        ((int)((x)&0xFFFF))


/* spooler error code */
#define SP_ERROR            (-1)    /* general error - mostly used when spooler isn't loaded */
#define SP_APPABORT         (-2)    /* app aborted the job through the driver */
#define SP_USERABORT        (-3)    /* user aborted the job through spooler's front end */
#define SP_OUTOFDISK        (-4)    /* simply no disk to spool */
#define SP_OUTOFMEMORY      (-5)
#define SP_RETRY            (-6)    /* retry sending to the port again  */
#define SP_NOTREPORTED      0x4000  /* set if GDI did not report error */

/* subfunctions of the Spooler support function, GetSpoolJob()
 *  CP_* are used by the control panel for modifying the printer setup/
 */
#define SP_PRINTERNAME      20
#define SP_REGISTER         21
#define SP_CONNECTEDPORTCNT 25
#define SP_QUERYDISKUSAGE   26
#define SP_DISKFREED        27
#define SP_INIT             28
#define SP_LISTEDPORTCNT    29
// #define CP_ISPORTFREE    30
#define SP_QUERYVALIDJOB    30
#define CP_REINIT	    31
#define SP_TXTIMEOUT	    32
#define SP_DNSTIMEOUT	    33
#define CP_CHECKSPOOLER     34
#define CP_SET_TT_ONLY      35
#define CP_SETSPOOLER       36
#define CP_SETDOSPRINT      37


#define SP_DISK_BUFFER      (20000) /* wait for about 20 K of disk space to free
                                       free up before attempting to write to disk */

/* messages posted or sent to the spooler window */
// change these to WM_SPOOLER_ messages
#define SP_NEWJOB           0x1001
#define SP_DELETEJOB        0x1002
#define SP_DISKNEEDED       0x1003
#define SP_QUERYDISKAVAIL   0x1004
#define SP_ISPORTFREE       0x1005
#define SP_CHANGEPORT       0x1006

/* in /windows/oem/printer.h */


// JCB.type status flag bits 

// job is done printing (no more pages will be spooled)
#define JB_ENDDOC           0x0001  

// spooler canceled the job (user deleted it) either while it
// is being spooled (spool routines care about this) or after
// it is done spooling (no one really cares about this).
#define JB_CANCELED_JOB     0x0002

// the spool data is a metafile (with the devmode stuck on the end)
#define JB_METAFILE	    0x0080

// don't spool, send data straight to the desired port (gdi
// does all comm IO, deals with timeouts etc).
#define JB_DIRECT_OUTPUT    0x8000

// printing to a file ("FILE:" or file name).  use dos
// writes when outputting data.  also used for non spooled net jobs
// (net data goes through dos)
#define JB_PRINT_TO_FILE    0x4000  

// this is a spooled job (going to be sent to printman eventually)
#define JB_SPOOLED_JOB      0x2000

// when the spooler is notified we mark the job with this, be careful
// because the user can close down printman after notification
#define JB_NOTIFIED_SPOOLER 0x1000

// we are in an out of disk condition and we are waiting for printman
// to finish outputting some spool data to get more disk space
#define JB_WAITFORDISK      0x0800

// tell printman to "print through dos" instead of using comm routines
#define JB_DOS_WRITES	    0x0200

// use WNetCloseJob() instead of _lclose(), this is a net job
#define JB_NET_JOB	    0x0100

// no deletion of file after spool (USED?)
// #define JB_DEL_FILE         0x0400  

#define NAME_LEN        32
#define BUF_SIZE        128
#define MAX_PROFILE     80
#define JCBBUF_LEN      256

/* comm driver buffer sizes (used by print manager and gdi when opening ports) */
#define COMM_INQUE          0x010
#define COMM_OUTQUE         0x400

#define COMM_ERR_BIT        0x8000
#define TXTIMEOUT           45000               /* milliseconds */
#define DNSTIMEOUT          15000               /* milliseconds */

#define BAUDRATE            0
#define PARITY              1
#define BYTESIZE            2
#define STOPBITS            3
#define REPEAT              4

#define MAXPORTLIST 	20  	/* max # ports listed in win.ini [ports] */
#define MAXPORT     	MAXPORTLIST
#define MAXSPOOL    	100	/* max # jobs spooled per port */
#define MAXMAP      	18
#define PORTINDENT   	2
#define JOBINDENT    	3
#define MAXPAGE		7	/* allow 7 pages at first */
#define INC_PAGE    	8     	/* increase by 8 pages at a time */

typedef struct {
    ATOM  aPortName;
    ATOM  aPrinterName;
    ATOM  aDriverName;
    long txtimeout;
    long dnstimeout;
} JCBQ;

typedef struct jcb {
    unsigned        type;
    int             pagecnt;
    int             maxpage;
    int             portnum;
    HDC             hDC;
    int             chBuf;
    long	    timeSpooled;
    char            buffer[JCBBUF_LEN];
    unsigned long   size;
    unsigned long   iLastPage;
    WORD	    psp;		// the PSP of the app that started printing
//    PORT	    pPort;		// use this instead of overloading page[1]
    char            jobName[NAME_LEN];
    int             page[MAXPAGE];
} JCB, FAR *LPJCB;


// DIALOGMARK.type values (NOT USED)
// #define SP_TEXT         0   /* text type                                */
// #define SP_NOTTEXT      1   /* not text type                            */
// #define SP_DIALOG       2   /* dialog type data                         */
// #define SP_CALLBACK     3   /* call back type function                  */

// the PAGE structure containes an array of these, one for every dialog 
typedef struct {
    int    size;       	// size of the dialog data
    long   data_offset;	// offset into the spool file of this data
} DIALOGMARK, FAR *LPDIALOGMARK;

// change these to 2 (these are rare things) and rename
#define SP_DLGINC       8	// this many dlg msgs per page (initially)
#define SP_DLGINIT      8	// grow DIALOGMARK.dialog array by this amount

typedef struct page {
    int      filenum;		// file handle
    int	     maxdlg;            // max number of dialog
    int      dlg_index;         // index into dialog[] of next dialog
    long     spoolsize;		// size of this page spool file
    OFSTRUCT fileBuf;		// OpenFile() buffer
    DIALOGMARK  dialog[SP_DLGINIT];
} PAGE, FAR *LPPAGE;

// #define SP_COMM_PORT    	0
// #define SP_FILE_PORT		1
// #define SP_REMOTE_QUEUE 	2


// GDI uses this (not printman)
typedef struct {
    int   type;
    int   fn;
    long  retry;            /* system timer on first error  */
} PORT;


/* exported routines */


// these should be in windows.h or printers.h, remove from here
int   WINAPI WriteDialog(HANDLE hJCB, LPSTR str, int n);
int   WINAPI WriteSpool(HANDLE hJCB, LPSTR str, int n);


LONG  WINAPI GetSpoolJob(int, long);	// printman and control panel call here

char  WINAPI GetSpoolTempDrive(void);	// no one should call this
BOOL  WINAPI QueryJob(HANDLE, int);	// no one should call this
