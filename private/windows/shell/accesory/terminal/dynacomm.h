#include "fsemacro.h"         // rjs - macros for dynacomm
#include <io.h>       /* for open,read,write,close file crt -sdj*/
#include <string.h>   /* for strncpy -sdj */
#include <stdio.h>    /* for sprintf sscanf -sdj */
#include <fcntl.h>  /*added cause CRT's need defines*/
#include <sys\types.h>
#include <sys\stat.h>
#include "asm2c_32.h"

/*****************************************************************************/
/* Compilation Switches                                                      */
/*****************************************************************************/

#ifdef DEBUG
#define DEBOUT(a,b)         DbgPrint("WIN32DEBUG: " a, (b));
#else
#define DEBOUT(a,b)
#endif

#define DEBUG_FLAG   FALSE
#define DEBUG_FLAG1  (DEBUG_FLAG & 0x0002)

#define NULL_PORT    TRUE                                             /* enables DynaComm to function */
                                                                                                                        /* without a valid COMM PORT    */

/*****************************************************************************/
/* Type Definitions                                                          */
/*****************************************************************************/

typedef BYTE            STRING;              /* denotes PASCAL-type string */
typedef BYTE *          Ptr;
typedef BYTE HUGE_T *   HPSTR;

typedef VOID            (NEAR *procPtr)();
typedef HWND            ControlHandle;


/*****************************************************************************/

typedef enum {KBD_LOCK, KBD_UNLOCK, KBD_ECHO, KBD_BUFFER, KBD_WAIT} KBD_STATE;

typedef enum {ICS_NONE, ICS_BRITISH, ICS_DANISH, ICS_FINISH,
              ICS_FRENCH, ICS_CANADIAN, ICS_GERMAN, ICS_ITALIAN,
              ICS_SPANISH, ICS_SWEDISH, ICS_SWISS, ICS_MAXTYPE} ICS_TYPE;

typedef enum {XFRNORMAL, XFRCHAR, XFRLINE} TTXTTYPE;

typedef enum {XFRCHRDELAY, XFRCHRWAIT} TCHRTYPE;

typedef enum {XFRLINDELAY, XFRLINWAIT} TLINTYPE;

typedef enum {BNEITHER, BPAUSE, BWHEN, BBOTH} BREAKCOND;

typedef enum {EXNONE, EXVPOS, EXHPOS, EXSCS, EXSETMODE, EXRESETMODE} ESCEXTEND;

typedef enum {TBLBEGINLINE, TBLNONSPACE, TBLONESPACE, TBLSPACES} TBLPOS;

typedef enum {XFRNONE, XFRSND, XFRRCV, XFRTYP, XFRBSND, XFRBRCV} XFERFLAG;

typedef enum {GRNONE, GRSEMI, GRHIGH, GRMEDM} VIDGRAPHICS;

typedef enum {XFRDYNACOMM, XFRXMODEM, XFRYMODEM, XFRKERMIT, XFRYTERM} tBinType;

typedef enum {XFRBOTH, XFRDATA, XFRRESOURCE} tBinFork;


/*****************************************************************************/
/* Constant Definitions                                                      */
/*****************************************************************************/

#define PATHLEN                     80
#define FILENAMELEN		    32 // -sdj 12 is really bad if you have ntfs names 12

#define ESCSKIPNDX                  32       /* used: resetEmul (itutil1.c)  */

#define MAXTIMERNUM                 3        /* mbbx 2.00: multiple timers */


/*****************************************************************************/
/*                        STANDARD CHARACTER CONSTANTS                       */

#define CR                  13               /* Carriage return              */
#define LF                  10               /* Line Feed                    */
#define SP                  32               /* Space                        */
#define TAB                 9                /* Tab                          */
#define FF                  12               /* Form Feed                    */
#define BS                  8                /* BackSpace                    */
#define ESC                 27               /* Escape                       */
#define BELL                7                /* Bell                         */
#define CHFILL              32               /* buffer fill char.            */
#define XOFF                19               /* XOFF                         */
#define XON                 17               /* XON                          */
#define CNTRLZ              26               /* end of file ^Z               */
#define CNTRLC               3               /* ^C same song second CHETX    */

#define CHSOH                1
#define CHSTX                2
#define CHETX                3
#define CHEOT                4
#define CHACK                6
#define CHNAK               21
#define CHCAN               24
#define CHTIMEOUT           -1

#define CHQUOTE            '"'               /* mbbx 1.04: REZ... */
#define CHSQUOTE            '\''


/*                              SYSTEM CONSTANTS                             */

#define KILOBYTES                  * 1024
#define KILOBYTESL                 * 1024L
#define MEGABYTES                  * 1024 KILOBYTES
#define MEGABYTESL                 * 1024 KILOBYTESL
#define MAXUNSIGNEDINT             64 KILOBYTES - 1
#define MAXUNSIGNEDINTL            64 KILOBYTESL - 1
#define MAXINT                     32 KILOBYTES - 1
#define MININT                     -1 * MAXINT
#define MAXINTL                    32 KILOBYTESL -1
#define MAXLONGINT                 2048 MEGABYTESL
#define MINLONGINT                 -1 * MAXLONGINT
#define MAXUNSIGNEDCHAR            255
#define TERM_MAXCHAR               127 /* changed from MAXCHAR to avoid conflict*/
                                       /* MAXCHAR was not used in any files anyway-sdj*/
#define MILLISECONDS               * 1
#define SECONDS                    * 1000 MILLISECONDS

#define versSettings         4

#define STR255               256             /* Faking Pascal str255 type    */
#define DONTCARE             0xff            /* General purpose don't care   */
#define MAXROWCOL            MAXLONGINT      /* Max. value for type ROWCOL   */
#define STANDARDKEY          0x00
#define KEYPADKEY            0x01
#define TERMINALFKEY         0x02
#define SCROLLKEY            0x04
#define SYSTEMFKEY           0xF0
#define SHORTBREAK           0xFE            /* Defined for serial short brk */
#define LONGBREAK            0xFF            /* Defined for serial long brk  */
#define DAYS_IN_A_YEAR       31+28+31+30+31+30+31+31+30+31+30+31
#define SECS_IN_A_DAY        24l*60l*60l
#define MAXSCREENLINE        23
#define MINPOINTSIZE          6
#define MAXPOINTSIZE          15             /* Maximum point size for the   */
                                             /* terminal screen font         */
#define STATUSLINE           24
#define UPDATETICKS           3
#define YIELDCHARS           48  /* *** optimize this */

/*                         ARRAY DECLARATOR CONSTANTS                        */

#define LOCALMODEMBUFSZ       513            /* mbbx: previously 1024 */
#define NINQUEUE              1024           /* mbbx 1.04: was 3072 */
#define NOUTQUEUE             256            /* mbbx 1.04: was 512 */
#define KEYSEQLEN             5
#define SIZEOFEMULKEYINFO     50*KEYSEQLEN   /* mbbx 2.00: was 52,44 */

/* #define FKEYLEN               64  jtf 3.12         Max. sizeof funct. key text  */
#define DCS_FKEYLEVELS         4             /* Number of level keys         */
#define DCS_NUMFKEYS           8             /* Number of function keys      */
#define DCS_FKEYTITLESZ       20             /* Length of fkey title rkhx 2.00 */
#define DCS_FKEYTEXTSZ        44             /* Length of fkey text rkhx 2.00 */
#define DCS_NUMSYSVARS        10             /* rkhx 2.00 */

#define MDMRESLEN             20
#define TMPNSTR               255             /* Gen. purpose temp. string len*/
#define MINRESSTR             32
#define STATUSRECTBORDER      4


#define SPACE              32                /* Space (yterm)                */
#define SERINBUFFSIZE      LOCALMODEMBUFSZ   /* mbbx: yterm */
#define YTERMTIMEOUT       600


/* VIRTUAL KEY STATE */

#define VKS_SHIFT                   0x0100   /* mbbx 1.04: keymap ... */
#define VKS_CTRL                    0x0200
#define VKS_ALT                     0x0400


#define FKB_UPDATE_BKGD             0x0001   /* mbbx 2.00: fkey button options... */
#define FKB_UPDATE_TIMER            0x0002
#define FKB_DISABLE_CTRL            0x8000


/* CHARACTER ATTRIBUTES */

#define ANORMAL         0x00                 /* mbbx 1.03 ... */
#define ABOLD           0x01
#define AREVERSE        0x02
#define ABLINK          0x04
#define AUNDERLINE      0x08
#define APROTECT        0x10
#define AGRAPHICS       0x20
#define ADIRTY          0x40
#define ACLEAR          0x80
#define AMASK           (ABOLD | AREVERSE | ABLINK | AUNDERLINE | APROTECT)
#define ANOTGRAPHICS    ~AGRAPHICS


/* LINE ATTRIBUTES */

#define LNORMAL      0
#define LHIGHTOP     1
#define LHIGHBOTTOM  2
#define LWIDE        3
#define LATTRIB      132
#define LFLAGS       133
#define LDIRTY       0x01
#define LCLEAR       0x02
#define ATTRROWLEN   134


/* FILE ERROR CONSTANTS */

#define NOERR                 FALSE          /* No file error flag value     */
#define EOFERR                262
#define FERR_FILENOTFOUND     2
#define FERR_PATHNOTFOUND     3
#define FERR_TOOMANYOPENFILES 4
#define FERR_ACCESSDENIED     5
#define FERR_INVALIDACCESS    12
#define FERR_INVALIDFILENAME  257
#define FERR_OPEN             258
#define FERR_READ             259
#define FERR_FILELENGTH       260
#define FERR_CLOSE            261


/* rkhx 2.00 ... */

#if OLD_CODE                                 /* mbbx 2.00: using bit fields... */
/* commFlags */
#define DCS_CF_RESETDEVICE       0x8000      /* select new comm device */
#endif

/* termFlags */
#define DCS_TF_SCROLLBARS        0x0001      /* show terminal scroll bars */

/* modemFlags */
#define DCS_MF_USEDEFAULT        0x0001      /* use default modem settings */

/* communication flags */   /* rjs bug2 */
#define DCS_CF_NETNAMEPADDING    0x0001      /* if set, then use blank padding in NetBios names, else null */

/* environmentFlags */
#define DCS_EVF_FKEYSSHOW        0x0001      /* show fkeys */
#define DCS_EVF_FKEYSARRANGE     0x0002      /* auto arrange when shown */

/* parentFlags */
#define DCS_PF_                  0x0001      /* */

#define DCS_FILE_ID              0x20534344  /* mbbx 2.00 ... */
/* #define DCS_HEADERSZ          4 */
#define DCS_VERSIONSZ         6
#define DCS_DESCRIPTIONSZ    53
#define DCS_PASSWORDSZ       16

#define DCS_ANSWERBACKSZ     44
#define DCS_FONTFACESZ       32

#define DCS_XLINSTRSZ        24

#define DCS_MODEMCMDSZ       32

#define DCS_FKEYNEXTSZ       20
#define DCS_SYSVARSZ         44

#define DCS_DCINITAGSZ       16

#if OLD_CODE

/*                     FILE DIALOG BOX ATTRIBUTE CONSTANTS                   */
#define GETFILE              0x0001
#define PUTFILE              0x0002
#define VIEWTXTFILE          0x0004
#define SENDTXTFILE          0x0008
#define MACFILETYPE          0x0010
#define APPENDTXTFILE        0x0020
#define SENDFILE             0x0040
#define FORCEXTENSION        0x0080
#define RECEIVEFILE          0x0100
#define EXECUTEFILE          0x0200
#define PRINTFILE            0x0400
#define REMOTEFILE           0x0800

#endif


/* strings constants: */

#define DC_WNDCLASS                 "DC_Term"      /* mbbx 1.04: REZ... */

#define HEX_STR                     "0123456789ABCDEF"   /* mbbx 2.00 */

#define NULL_STR                    "\0"     /* mbbx 1.00: 00 00 */
#define VOID_STR                    "\1\0"   /* mbbx 1.00: 01 00 00 */
#define PRMPT_STR                   "\1?"
#define LABEL_STR                   "\1*"
#define SPACE_STR                   "\1 "
#define SEMI_STR                    "\1;"

#define TIME_STR                    "\800:00:00"
#define OK_STR                      "\2OK"
#define VIDEO_STR                   "\5VIDEO"
#define CRLF_STR                    "\2\015\012"
#define DBG_FNL_STR                 "["
#define DBG_FNR_STR                 "]\r\n"
#define MSDOS_STR                   "MSDOS"
#define VT100_STR                   "VT-100"

#define PAR2_STR                    "PAR2"
#define TEXT_STR                    "TEXT"
#define PARM_STR                    "PARM"
#define CCL_STR                     "CCL "
#define YT_WSA_STR                  "\3WSA"
#define YT_W43_STR                  "\3W43"
#define YT_RSA_STR                  "\3RSA"
#define YT_R43_STR                  "\3R43"
#define YT_CRC_STR                  "\02##"
#define YT_RESP_STR                 "012345"


/*****************************************************************************/
/* Data File Definitions                                                     */
/*****************************************************************************/

typedef enum {FILE_NDX_DATA, FILE_NDX_SETTINGS, FILE_NDX_TASK,
              FILE_NDX_SCRIPT, FILE_NDX_MEMO, MAX_FILE_NDX} FILEDOCTYPE;


#define DATA_FILE_TYPE        "\\*.*"
#define SETTINGS_FILE_TYPE    "\\*.TRM"
#define TASK_FILE_TYPE        "\\*.TXT"
#define SCRIPT_FILE_TYPE      "\\*.TXT"
#define MEMO_FILE_TYPE        "\\*.TXT"
#define ANY_FILE_TYPE         "\\*.*"
#define NO_FILE_TYPE          "\\*."         /* mbbx 2.00 */

#define INI_FILE_TYPE         "\\*.INI"
#define EXE_FILE_TYPE         "\\*.EXE"
#define DRIVER_FILE_TYPE      "\\*.DRV"      /* mbbx 2.00 ... */
/* #define TERMINAL_FILE_TYPE    "\\*.TRM"   mbbx 2.00: no forced extents */


#define FILEDOCDATA                 struct tagFileDocData

struct tagFileDocData
{
   BYTE     filePath[PATHLEN];
   BYTE     fileName[16];
   BYTE     fileExt[16];
   BYTE     title[PATHLEN];
} fileDocData[MAX_FILE_NDX];

/*
struct tagFileDocData
{
   BYTE     filePath[FO_MAXPATHLENGTH];
   BYTE     fileName[FO_MAXFILELENGTH];
   BYTE     fileExt[FO_MAXEXTLENGTH];
   BYTE     title[FO_MAXPATHLENGTH];
} fileDocData[MAX_FILE_NDX];
*/

/* #define ATTRDIRLIST           0x4010 */


INT   saveFileType;                          /* mbbx 2.00: save prev file type... */


/*****************************************************************************/
/* Structure Definitions                                                     */
/*****************************************************************************/


/* rkhx 2.00 */
/* obsolete
typedef
   struct {
      BYTE  title[TITLELEN];
      BYTE  xtra[XTRALEN];
   } TITLEREC;
*/


// -sdj this is the portable way to pack the structures 1 byte aligned.
// win3.0 sources are compiled with -Zp option so that the structures are
// byte aligned. This pragma would work for MIPS and X86 MS compilers

#ifdef ORGCODE
#else
#pragma pack(1)
#endif

typedef
   struct {
/* House keeping:    86 +   42 =  128 bytes */
   LONG fileID;                              /* mbbx 2.00 ... */
/* BYTE header[DCS_HEADERSZ];                header ('DCS ') */
   BYTE version[DCS_VERSIONSZ];           /* version (2.00: ) */
   BYTE description[DCS_DESCRIPTIONSZ];   /* description */
   BYTE controlZ;                         /* fake eof for typing text */
   WORD fileSize;                         /* file size */
   WORD crc;                              /* crc check */
   WORD groupSave;                        /* global save params flag DCS_GS_... */
   BYTE password[DCS_PASSWORDSZ];         /* password */
   BYTE headerXtraRoom[42];               /* extra room for header section */

/* Communications:   (7 + 5) + (7 + 5) + (98 + 6) =  128 bytes */
   WORD fParity: 1;                       /* mbbx 2.00: bit fields... */
   WORD fCarrier: 1;
   WORD commFlags: 13;                    /* communication settings DCS_CF_... */
   WORD fResetDevice: 1;

   BYTE newDevRef;                        /* new comm device */
   BYTE comDevRef;                        /* comm device */
   BYTE comExtRef;                        /* comm extension */
   BYTE comPortRef;                       /* comm port */
   BYTE comModeRef;
   BYTE commXtraRoom1[5];                 /* extra room for comm section */

   WORD speed;                            /* mbbx 2.00: allow ANY baud rate */
   BYTE dataBits;
   BYTE parity;
   BYTE stopBits;
   BYTE flowControl;
   BYTE priority;                         /* com priority (as per spooler) */
   BYTE commXtraRoom2[5];                 /* extra room for comm section */

   BYTE localName[16];                    /* used for deviceName */
   BYTE remoteName[64];
   BYTE deviceName[14];                   /* used for deviceName */
   WORD netRcvTimeOut;
   WORD netSndTimeOut;
   BYTE commXtraRoom3[6];                 /* extra room for comm section */

/* Terminal:         94 +   34 =  128 bytes */
   WORD termFlags: 12;
   WORD fInpLFCR: 1;
   WORD fBSKey: 1;
   WORD fHideTermHSB: 1;
   WORD fHideTermVSB: 1;
   BYTE emulate;                          /* terminal emulation */
   BYTE fCtrlBits;                        /* mbbx 1.10: VT220 8BIT */
   BYTE answerBack[DCS_ANSWERBACKSZ];     /* answer back string */
   BYTE lineWrap;
   BYTE localEcho;
   BYTE sound;
   BYTE inpCRLF;
   BYTE outCRLF;
   BYTE columns;
   BYTE termCursor;
   BYTE cursorBlink;
   BYTE fontFace[DCS_FONTFACESZ];         /* font face */
   WORD fontSize;                         /* font point size */
   WORD language;                         /* mbbx 1.04 */
   WORD bufferLines;                      /* terminal scroll buffer size */
/**** nov25,91 win31 added 2 bytes here! -sdj ********************/
/**** to compensate the xtra room was reduced by 2 bytes 34->32 -sdj *********/
   BYTE setIBMXANSI;
   BYTE useWinCtrl;

   BYTE termXtraRoom[32];                 /* extra room for terminal section */






/* Binary Transfers:  9 +  119 =  128 bytes */
   WORD binXferFlags;                     /* binary transfer state DCS_BXF_... */
   BYTE xBinType;
   WORD rcvBlSz;
   WORD sendBlSz;
   BYTE retryCt;
   BYTE psChar;                           /* mbbx 1.04: xferPSChar */
   BYTE binXferXtraRoom[119];             /* extra room for bin xfer section */

/* Text Transfers:   33 +   95 =  128 bytes */
   WORD txtXferFlags;                     /* text transfer state DCS_TXF_... */
   BYTE xTxtType;
   BYTE xChrType;
   BYTE xChrDelay;
   BYTE xLinType;
   BYTE xLinDelay;
   BYTE xLinStr[DCS_XLINSTRSZ];
   BYTE xWordWrap;
   BYTE xWrapCol;
   BYTE txtXferXtraRoom[95];              /* extra room for txt xfer section */

/* Phone:            38 +   90 =  128 bytes */
   WORD phoneFlags;                       /* phone state DCS_PHF_... */
   BYTE phone[DCS_MODEMCMDSZ];
   BYTE dlyRetry;
   BYTE cntRetry;
   BYTE flgRetry;
   BYTE flgSignal;
   BYTE phoneXtraRoom[90];                /* extra room for phone section */

/* Modem:           387 +  253 =  640 bytes */
   WORD modemFlags;                       /* modem state DCS_MF_... */
   BYTE xMdmType;
   BYTE dialPrefix[DCS_MODEMCMDSZ];
   BYTE dialSuffix[DCS_MODEMCMDSZ];
   BYTE hangPrefix[DCS_MODEMCMDSZ];
   BYTE hangSuffix[DCS_MODEMCMDSZ];
   BYTE binTXPrefix[DCS_MODEMCMDSZ];
   BYTE binTXSuffix[DCS_MODEMCMDSZ];
   BYTE binRXPrefix[DCS_MODEMCMDSZ];
   BYTE binRXSuffix[DCS_MODEMCMDSZ];
   BYTE answer[DCS_MODEMCMDSZ];
   BYTE originate[DCS_MODEMCMDSZ];
   BYTE fastInq[DCS_MODEMCMDSZ];             /* mbbx 2.00: not used */
   BYTE fastRsp[DCS_MODEMCMDSZ];             /* mbbx 2.00: not used */
   BYTE modemXtraRoom[221];                  /* extra room for modem section */
   BYTE phone2[DCS_MODEMCMDSZ];             /* mbbx 2.00: not used */

/* Environment:    2510 +   50 = 2560 bytes */
   WORD environmentFlags;                 /* environment state DCS_EVF_... */
   BYTE fKeyNext[DCS_FKEYNEXTSZ];
   BYTE fKeyTitle[DCS_FKEYLEVELS][DCS_NUMFKEYS][DCS_FKEYTITLESZ]; /*  4 *  8 * 20 =  640 */
   BYTE fKeyText[DCS_FKEYLEVELS][DCS_NUMFKEYS][DCS_FKEYTEXTSZ];   /*  4 *  8 * 44 = 1408 */
   BYTE systemVariables[DCS_NUMSYSVARS][DCS_SYSVARSZ];            /*      10 * 44 =  440 */
   BYTE environmentXtraRoom[50];          /* extra room for environment section */

/* Parent:           42 +   86 =  128 bytes */
   WORD parentFlags;                      /* arrange/stack flags DCS_PF_... */
   SHORT dummy1;
   SHORT dummy2;
   SHORT dummy3;
   SHORT dummy4;
   BYTE keyMapTag[DCS_DCINITAGSZ];        /* key remapping tag (.ini file) */
   BYTE videoAttrTag[DCS_DCINITAGSZ];     /* video tag (.ini file) */
   BYTE szConnectorName[DCS_DCINITAGSZ];  /* slc nova 031 bjw nova 001 Connector DLL file name */
   BYTE connectorConfigData[32];          /* slc nova 028 */
   BYTE parentXtraRoom[86 - DCS_DCINITAGSZ - 32 ];               /* extra room for parent section */
   } recTrmParams;

typedef recTrmParams FAR *LPSETTINGS;

#ifdef ORGCODE
#else
#pragma pack()
#endif

typedef
   struct {
             LONG row;
             LONG col;
          }  ROWCOL;


typedef
   struct {
             GLOBALHANDLE  hText;
             RECT          viewRect;
             LONG          selStart;
             LONG          selEnd;
             BOOL          active;
             FARPROC       clikLoop;
             HFONT         hFont;
          } tEHandle;


#define TF_CHANGED                  0x8000
#define TF_NOCLOSE                  0x4000
#define TF_DIM                      0x2000
#define TF_HIDE                     0x1000
#define TF_DEFTITLE                 0x0010

typedef
   struct
   {
      BYTE      filePath[PATHLEN+1];
      BYTE      fileName[FILENAMELEN+1];
      BYTE      fileExt[FILENAMELEN+1];
      BYTE      title[PATHLEN+1];
      WORD      flags;
   } curDataRec;


typedef
   struct {
             BOOL good;
             BYTE vRefNum[PATHLEN+1];
             BYTE fName[PATHLEN+1];
          }  FSReply;

typedef
   struct {
             WORD  hour;
             WORD  minute;
             WORD  second;
             WORD  dayOfWeek;                 /* 0 = Sunday ... 6 = Saturday  */
             WORD  mm;                        /* Month (1 - 12)               */
             WORD  dd;                        /* Day   (1 - 31)               */
             WORD  yy;                        /* Year  (1980 - 2099)          */
          }  DOSTIME;


typedef
   struct {
            BYTE  fdType[4];
            BYTE  fdCreator[4];
            INT   fdFlags;
            DWORD ioFlLgLen;
            DWORD ioFlRLgLen;
            LONG  ioFlCrDat;
            LONG  ioFlMdDat;
          } PARAMBLOCKREC;


typedef
   struct {
            BYTE  reserved[21];
            BYTE  attribute;
            WORD  time;
            WORD  date;
            LONG  size;
            BYTE  filename[14];
          } DTA;

typedef DTA FAR *LPDTA;


/*****************************************************************************/
/* Variable Definitions                                                      */
/*****************************************************************************/

/* WORD  winVersion; */

INT  itemHit;

BOOL doneFlag;                               /* End of DynaComm - Flag       */
BOOL cancelAbort;                            /* Abort spooling process ?     */
DWORD dwWriteFileTimeout;                    /* used for wait after writefile*/
BYTE  MaxComPortNumberInMenu;		     /* N in max ComN, just now 4    */
BYTE  bPortDisconnected;		     /* this flag is used to indicate that
						the specified port is no longer accesible
						user should select some other port. Typically
						this will happen when user presses cntl-c or
						quit at the telnet prompt, and after this the
						right thing to do is to close this port and
						prompt user to select other port */

int   WindowXPosition;			     // x cord for createwindow
int   WindowYPosition;			     // y cord for createwindow
int   WindowWidth;			     // width  for createwindow
int   WindowHeight;			     // height for createwindow
CHAR  szCurrentPortName[TMPNSTR+1];	     // this variable will contain the name
					     // of port which terminal attempted an open on


BOOL  bPortIsGood;                           /* to indicate that the port is good*/

typedef struct _ComNumAndName {
    BYTE Index;
    CHAR PortName[TMPNSTR+1];
    } COMMNUMANDNAME, *PCOMMNUMANDNAME;

COMMNUMANDNAME arComNumAndName[20];

BOOL activTerm;                              /* Active terminal flag         */

POINT nScrollRange;
POINT nScrollPos;                                                        /* port macro added MPOINT*/

BOOL noSelect;                               /* rjs bugs 020 */
BOOL activSelect;
BOOL answerMode;                             /* Answer mode flag             */
BOOL mdmValid;
BOOL cursorValid;
BOOL debugFlg;                               /* Monitor mode (dis)enabled ?  */
BOOL prtFlag;
BOOL transPrintFlag;                         /* rjs bug2 */
BOOL prtPause;
BOOL escSeq;                                 /* Incoming are part of escSeq  */
BOOL ansi;
BOOL escAnsi;
BOOL escGraphics;
BOOL escCursor;
BOOL escVideo;
BOOL statusLine;                             /* Status line enabled ?        */
BOOL chInsMode;
BOOL vidInverse;
BOOL cursorKeyMode;
BOOL keyPadAppMode;
BOOL originMode;
BOOL grChrMode;
BOOL cursorOn;
BOOL dialing;
BOOL cursBlinkOn;
BOOL scrapSeq;
BOOL outBufSeq;
BOOL copiedTable;
BOOL useScrap;
BOOL mdmOnLine;
BOOL timerActiv;
BOOL useMacFileName;
BOOL vScrollShowing;
BOOL later;
BOOL bufferFull;
BOOL fKeysShown;
BOOL protectMode;                            /* mbbx: emulation */

BYTE  szAppClass[20];                        /* rjs bug2 */
BYTE  szAppName_private[20];                 /* Windows registered name      */
BYTE  szAppName[20];                         /* Windows name      */
BYTE  szMessage[80];                         /* Tiled window caption         */

BYTE  serBytes[LOCALMODEMBUFSZ];             /* Our IT local modem buffer    */
BYTE  attrib[25][134];                       /* Screen attribute map         */
BYTE  curAttrib;                             /* Current character attribute  */
BYTE  ch;                                    /* Modem character              */
BYTE  theChar;                               /* Modem character - all 7 bits */
BYTE  the8Char;                              /* Modem character - all 8 bits */
BYTE  tabs[132];
BYTE  fileVolume[80];
BYTE  macFileName[PATHLEN+1];
BYTE  charSet[2];
BYTE  chAscii;
BYTE  emulInfo[128];
BYTE  icsXlateTable[256];                    /* mbbx 1.04: ics */
BYTE  ansiXlateTable[256];                   /* mbbx 1.06A: ics new xlate */
WORD  keyMapState;                           /* mbbx 1.04: keymap */

STRING mdmResult[MDMRESLEN];                 /* Non-displayed modem chars.   */
STRING line25[132];                          /* Status line 25 characters    */
STRING strAnsi[STR255];
STRING outBuf[134];
STRING fKeyStr[STR255];
STRING keyPadString[5];

/*---------------------------------------------------------------------------*/

XFERFLAG    xferFlag;
BOOL        xferStopped;
INT         xferErrors;
LONG        xferLength;
HANDLE      xferBufferHandle;    /* rjs bugs 016 */
WORD        xferBufferCount;     /* rjs bugs 016 */

INT         xferRefNo;
STRING      xferVRefNum[PATHLEN];
STRING      xferFname[PATHLEN];
STRING      rdCH[256];
int         ioCnt;    /* flagged by port macro*/

WORD  xferMenuAdds;

INT   xferSndLF;                             /* mbbx: (-1,0,1)=>(NOLF,CR,CRLF) */
WORD  xferViewPause;                         /* mbbx: auto line count */
WORD  xferViewLine;

INT   xferBlkNumber;
INT   xferPct;
INT   xferLinDelay;
INT   xferChrDelay;

BOOL xferPaused;
BOOL xferBreak;                              /* mbbx 2.00: xfer ctrls */
BOOL xferSaveCtlChr;
BOOL xferTableSave;
BOOL xferWaitEcho;
BOOL xferAppend;
BOOL xferFast;

BYTE  xferCharEcho;
BYTE  xferPSChar;                            /* mbbx 1.02: packet switching */

STRING xferLinStr[DCS_XLINSTRSZ];            /* rkhx 2.00 */
STRING strRXBytes[32];
STRING strRXErrors[32];
STRING strRXFname[32];
STRING strRXFork[4];

TTXTTYPE      xferTxtType;
TCHRTYPE      xferChrType;
TLINTYPE      xferLinType;
tBinType      xferBinType;
tBinFork      xferBinFork;

LONG  xferEndTimer;
LONG  xferOrig;
LONG  xferBlkSize;
LONG  xferRLgLen;
LONG  xferLgLen;
LONG  xferBytes;
LPBYTE xferBuffer;
CHAR  NoMemStr[41];

/*---------------------------------------------------------------------------*/

INT            serNdx;                                /* Index into local modem buff. */
INT            serCount;                              /* Index comm. buffer           */
INT            curCol;
INT            curLin;
INT            maxChars;
INT            maxLines;
INT            escChar;
INT            escSkip;
BOOL           termDirty;                             /* mbbx: used to be termLine */
INT            savTopLine;
INT            curTopLine;
INT            savLeftCol;
INT            chrWidth,  stdChrWidth;
INT            chrHeight, stdChrHeight;
INT            scrRgnBeg;
INT            scrRgnEnd;
INT            escCol;
INT            escLin;
INT            curLeftCol;
INT            maxScreenLine;                         /* Windows only                 */
INT            visScreenLine;                         /* Windows only                 */
INT            curLevel;
INT            vidBG;
INT            textIndex;
INT            outBufCol;
INT            fKeyNdx;
INT            keyPadIndex;
HANDLE         sPort;                                 /* Serial port id win32         */
INT            portLocks;                             /* number of unreleased getPorts*/
BOOL           sPortErr;

INT            lineFeeds;
INT            seqTableNdx;
INT            progress;
INT            decScs;
INT            shiftCharSet;
INT            activCursor;
INT            scrollBegin;
INT            scrollEnd;
INT            nScroll;
INT            fKeysHeight;                           /* (mbbx) */
INT            ctrlsHeight;                           /* mbbx 1.04 */

ESCEXTEND      escExtend;
TBLPOS         tblPos;
VIDGRAPHICS    vidGraphics;
KBD_STATE      kbdLock;

LONG           gIdleTimer;                            /* rjs bug2 001 */

LONG           lastChTick;
LONG           timPointer;
LONG           cursorTick;
LONG           dialStart;

LONG           textLength;

/*                            \\\ Long Pointers ///                          */

LPBYTE         textPtr;

HWND           hItWnd;                        /* Application Window Handle    */
HWND           hTermWnd;                      /* Window handle to terminal    */
HWND           hdbmyControls;                 /* Window handle to fkey dlg.   */
HWND           hdbXferCtrls;                  /* mbbx 1.04: term ctrls */
HWND           hEdit;
HWND           fKeyHdl;

HWND           hwndThread;                      // rjs thread
DWORD          dwTimerRes;                      // rjs thread
HWND           commThread;                      // rjs thread
BOOL           CommThreadExit;                  // sdj thread
BOOL           gotCommEvent;                    // rjs thread
HANDLE         overlapEvent;                    // rjs thread
BOOL           gbThreadDoneFlag;                // rjs thread
HANDLE         hMutex;                          // rjs thread
BOOL           gbXferActive;                    // rjs thread
BOOL           bgOutStandingWrite;   /* slc swat */

ControlHandle  xferCtlStop;
ControlHandle  xferCtlPause;
ControlHandle  xferCtlScale;
ControlHandle  fKeyHandles[DCS_NUMFKEYS+1];

HDC            hPrintDC;
HDC            thePort;
HBRUSH         theBrush;
HBRUSH         blackBrush;

HMENU          hMenu;
WORD           sysAppMenu[16];                 /* mbbx 2.00.03: control app menu */
HMENU          hEditMenu;                      /* Edit popup for Terminal WND  */
HMENU          hLastEdit;

HANDLE         hInst;
HANDLE         hIconLib;                       /* mbbx 1.10: DCICONS.LIB */
HANDLE         hemulKeyInfo;
HANDLE         tEScrapHandle;

HANDLE        hDevNames;
HANDLE        hDevMode;

HBITMAP        hCloseBox;                      /* mbbx: mdi */

HANDLE         hDCCLib;                         /* mbbx 2.00: compile */
HWND           hDCCDlg;                         /* mbbx 2.00: compile */

MSG            msg;                            /* Application queue messages   */
RECT           cursorRect;                     /* Rectangle of the current curs*/
RECT           timerRect;
RECT           statusRect;                      /* CCL commands show up in here */
POINT         lastPoint;                       /* last hTermWnd client point   */
DOSTIME        startTimer[MAXTIMERNUM];         /* mbbx 2.00: multiple timers */
DOSTIME        lastTime;
recTrmParams   trmParams;                      /* Contains global 'settings'   */
tEHandle       hTE;                            /* Main text edit window struct */
curDataRec     termData;                        /* Current data associated w/   */
                                             /* terminal window              */
PARAMBLOCKREC  xferParams;

struct icontype
{
   HICON  hIcon;
   WORD   last;
   BOOL   flash;
   INT    dy;
   INT    dx;
}  icon;

struct                                       /* mbbx 2.00: intl date/time... */
{
   INT   iDate;                              /* 0=MDY, 1=DMY, 2=YMD */
   BYTE  sDate[2];                           /* date separator */
   INT   iTime;                              /* 0=12HR, 1=24HR */
   BYTE  sTime[2];                           /* time separator */
   BYTE  s1159[4];                           /* AM trailing string */
   BYTE  s2359[4];                           /* PM trailing string */
}  intlData;


/*                       \\\ Functions & Procedures ///                      */

/* FARPROC lpdbPortInit;                     mbbx: now local */

WNDPROC lpdbmyControls;
WNDPROC lpdbDialing;
WNDPROC lpdbKerRemote;

/*                                           mbbx 1.04: obsolete...
FARPROC lpitWndProc;
FARPROC lptrmWndProc;
FARPROC lpSizeBoxProc;
FARPROC lpdbStdFN;
FARPROC lpdbGetPutFN;
FARPROC lpdbTypTFile;
FARPROC lpdbRcvTFile;
FARPROC lpdbSendFile;
FARPROC lpdbPrompt;
FARPROC lpdbSelSavApp;
FARPROC lpAbortDlg;
FARPROC lpabortDlgProc;
FARPROC lpdbPhon;
FARPROC lpdbEmul;
FARPROC lpdbTerm;
FARPROC lpdbFkey;
FARPROC lpdbTxtX;
FARPROC lpdbBinX;
FARPROC lpdbComm;
FARPROC lpdbModem;
FARPROC lpEditProc;
*/

procPtr termState;
procPtr escHandler;
procPtr pEscTable[128];
procPtr aEscTable[128];
procPtr pProcTable[72];                      /* mbbx 2.00 ... */
procPtr aProcTable[72];
procPtr ansiParseTable[16];


/* KERMIT STUFF */

#define DEL         127                /* Delete (rubout) */
/* crt also defines EOF so dont define this twice -sdj*/
#ifndef EOF
#define EOF         -1
#endif

#define DEFESC      '^'     /* Default escape character for CONNECT */
#define DEFIM       TRUE    /* Default image mode */

#define DEFFNC      FALSE

#define KERFILE     1       /* bufemp goes to a file */
#define KERBUFF     2       /* bufemp goes to KER_buff */
#define KERSCREEN   4       /* bufemp goes to the screen */

#define tochar(ch)  ((ch) + ' ')
#define unchar(ch)  ((ch) - ' ')
#define ctl(ch)     ((ch) ^ 64 )
#define unpar(ch)   ((ch) & 127)

INT
   KER_size,          /* Size of present data */
   KER_rpsiz,         /* Maximum receive packet size */
   KER_spsiz,         /* Maximum send packet size */
   KER_pad,           /* How much padding to send */
   KER_timint,        /* Timeout for foreign host on sends */
   KER_n,             /* Packet number */
   KER_numtry,        /* Times this packet retried */
   KER_oldtry,        /* Times previous packet retried */
   ttyfd,             /* File descriptor of tty for I/O, 0 if remote */
   KER_remote,        /* -1 means we're a remote kermit */
   KER_image,         /* -1 means 8-bit mode */
   KER_parflg,        /* TRUE means use parity specified */
   KER_turn,          /* TRUE means look for turnaround char (XON) */
   KER_lecho,         /* TRUE for locally echo chars in connect mode */
   KER_8flag,         /* TRUE means 8th bit quoting is done */
   KER_initState,     /* jtf 3.20 Used for retries durring send/receive */
   KER_parMask,       /* tge Used for auto parity selection */
   KER_mask,          /* tge Used for auto parity checsum masking */
   KER_pktdeb,        /* TRUE means log all packet to a file */
   KER_filnamcnv,     /* -1 means do file name case conversions */
   KER_filecount,     /* Number of files left to send */
   KER_timeout;       /* TRUE means a timeout has occurred. */

BYTE
   KER_state,         /* Present state of the automaton */
   KER_cchksum,       /* Our (computed) checksum */
   KER_padchar,       /* Padding character to send */
   KER_eol,           /* End-Of-Line character to send */
   KER_escchr,        /* Connect command escape character */
   KER_quote,         /* Quote character in incoming data */
   KER_select8,       /* 8th bit quote character to send either 'Y' or '&'*/
   KER_firstfile,
   KER_getflag,
   **KER_filelist,    /* List of files to be sent */
   *KER_filnam,       /* Current file name */
   recpkt[94],        /* Receive packet buffer */
   packet[94],        /* Packet buffer */
   KER_buff[94],      /* buffer for translations */
   outstr[80],        /*output string for debugging and translations */
   KERRCVFLAG;        /* direct recieved buffers to screen, file or buffer*/

LONG
   KER_bytes;          /* number of bytes received */

/*
   19 *  2 byte   =   38 bytes

    9 *  1 byte
    2 *  4 bytes
    3 * 94 bytes
    1 * 80 bytes
    1 *  1 byte   =  380 bytes

    1 *  4 bytes  =    4 bytes

                     422 bytes total + room for filenames */

/*****************************************************************************/
/* Macro Definitions                                                         */
/*****************************************************************************/

#define proc                     void near
#define getResId                 MAKEINTRESOURCE
#define eraseRect(rectangle)     FillRect(getPort(),(LPRECT) &rectangle, theBrush); releasePort()
#define invalRect(rectangle)     InvalidateRect(hTermWnd, (LPRECT) &rectangle)
#define validRect(rectangle)     ValidateRect(hTermWnd, (LPRECT) &rectangle)
#define invertRect(rectanlge)    InvertRect(getPort(), (LPRECT) &rectanlge); releasePort()
#define tickCount()              GetCurrentTime() * 60/1000
#define TEDelete(h)              SendMessage(h, WM_CLEAR, 0, 0L)
#define sysBeep()                MessageBeep(0)
#define nullTerminate(str)       str[*str+1] = 0
#define strEquals(dst,src)       memcpy(dst, src, (WORD) src[0] + 1), nullTerminate(dst)
#define blockMove(src, dst, len) lmovmem(src, dst, (DWORD) len)
#define equalString(str1, str2)  !strcmpi(str1, str2)
#define repeat                   do
#define until(cond)              while(!(cond))
#define stringToNum(str,num)     sscanf(&str[1], "%ld", num)
#define c2p(str1, str2)          lstrcpy((LPBYTE) &str1[1], (LPBYTE) str2), *str1 = lstrlen((LPBYTE) &str1[1])
#define p2c(str1, str2)          lmovmem((LPBYTE) &str2[1], (LPBYTE) str1, (unsigned) *str2), str1[*str2] = NULL
#define SWAPBYTES(i)             (((WORD) i >> 8) | (i << 8))
#define yield(lpmsg, hWnd)       PeekMessage(lpmsg, hWnd, 0, 0, FALSE)


/*****************************************************************************/
/* Forward Procedure Definitions                                             */
/*****************************************************************************/

/*** INITCODE.C ***/
//WORD MMain(HANDLE, HANDLE, LPSTR, INT);  /* causing compiler to puke*/
BOOL initWndClass();                         /* mbbx 1.04: was registerIt(); */
VOID initPort();
VOID initIcon();
BOOL createWindows(INT);
/* VOID hidemyControls();                    mbbx 2.00: obsolete... */
VOID setDefaultFonts();
VOID sizeWindows();
HBITMAP NEAR extractCloseBitmap();
BOOL initWindows(HANDLE, HANDLE, INT);
VOID initDialogs();
BOOL  APIENTRY dbPortInit(HWND, UINT, WPARAM, LONG);
BOOL NEAR setProfileExtent(BYTE *, BYTE *);  /* mbbx 2.00: default paths... */
BOOL NEAR initFileDocData(FILEDOCTYPE fileType, WORD strResID,BYTE  *fileExt,BYTE *szSection);
VOID initProfileData();
BOOL setup();
/* VOID initMemPort();                       mbbx 2.00: old, dumb shit */
VOID NEAR initIconLib();
BOOL NEAR initTermFile(BYTE *);
BOOL NEAR initTaskExec(BYTE *);
BOOL NEAR initEditFile(INT, BYTE *);
VOID NEAR readCmdLine(LPSTR);
VOID freeItResources();

/*** WINMAIN.C ***/
VOID FAR mainProcess();
VOID FAR mainEventLoop();
BOOL NEAR checkInputBuffer(MSG *);
DWORD APIENTRY idleProcess(VOID);
VOID FAR updateTimer();
BOOL updateFKeyButton(WPARAM wParam ,LONG lParam,WORD  status);   /* JAP fix adding wParam*/
VOID getTimeString(BYTE *, DOSTIME *);       /* mbbx 2.00: intl date/time */
VOID FAR cursorAdjust();
VOID NEAR blinkCursor();
BOOL NEAR taskProcess();

/*** WNDPROC.C ***/                          /* mbbx 1.04 ... */
VOID selectTopWindow();
/* changed WORD -> UINT -sdj */
LONG  APIENTRY DC_WndProc(HWND, UINT, WPARAM, LONG);
VOID termKeyProc(HWND hWnd, WORD message, WPARAM wParam,LONG lParam);
/* changed WORD -> UINT -sdj */
LONG  APIENTRY TF_WndProc(HWND, UINT, WPARAM, LONG);

/*** CHRPAINT.C ***/
VOID eraseColorRect(HDC hDC,LPRECT rect,BYTE cAttr);
VOID reDrawTermScreen(INT, INT, INT);        /* mbbx 2.00.06: jtf disp2 */
VOID reDrawTermLine(INT, INT, INT);
VOID updateLine(INT);
VOID drawTermLine(LPBYTE txtPtr,INT len, INT lin, INT col,BYTE  lAttr,BYTE cAttr);
VOID setAttrib(BYTE cAttr);

/*** DOFILE.C ***/
VOID getFileDocData(FILEDOCTYPE, BYTE *, BYTE *, BYTE *, BYTE *);
VOID setFileDocData(FILEDOCTYPE, BYTE *, BYTE *, BYTE *, BYTE *);    /* mbbx 2.00: no forced extents... */
VOID getDataPath(FILEDOCTYPE, BYTE *, BYTE *);
BOOL setDataPath(FILEDOCTYPE, BYTE *, BOOL);
BOOL EditGetDocData(BYTE *, BYTE *, BYTE *);
BOOL EditSetDocData(BYTE *, BYTE *, BYTE *, WORD, BOOL);

BOOL  APIENTRY dbCopySpecial(HWND, WORD, WORD, LONG);   /* mbbx 2.00 */

/*** MODEMINP.C ***/
VOID cleanRect(INT, INT);
VOID updateLine(INT);
proc scrollBuffer();
proc doScroll();
VOID trackCursor();
VOID termCleanUp();
proc putChar(BYTE ch);
VOID checkSelect();
VOID clrAttrib(INT, INT, INT, INT);
proc clrLines(INT, INT);
proc clrChars(INT, INT, INT);
VOID getUnprot(INT, INT, INT *, INT *);
INT getProtCol();
proc pCursToggle();
proc pCursOn();
proc pCursOff();
proc pCursRC();
proc pSetStatusLine();                       /* mbbx 1.03: TV925 ... */
proc pCursHome();
proc pVideo(BYTE attr);
proc pCursRelative(INT, INT);
proc pCursUp();
proc pCursDn();
proc pCursRt();
proc pCursLt();
proc pVPosAbsolute();
proc pHPosAbsolute();
proc pClrScr();
proc pClrBol();
proc pClrBop();
proc pClrEol();
proc pClrEop();
proc pClrLine();
proc scrollAttrib(INT, INT, INT, BOOL);
proc pLF();
proc pInsLin(INT, INT);
proc pDelLin(INT, INT, INT);
proc pDelChar(INT);
VOID begGraphics();
VOID endGraphics();
proc pGrSemi();
proc pGrDoIt(INT, HBRUSH);
proc pGrFill();
proc pGrChar();
proc pSetGrMode();
proc pSetMode();
proc pDecScs();
proc getParms();
proc pInquire();
proc pTab();
proc pClearAllTabs();
proc pSetTab();
proc pClearTab();
proc pCmpSrvResponse();
proc pSndCursor();
proc pIndex();
proc pRevIndex();
proc pSetLineAttrib();
proc pInsChar();
proc pSaveCursorPos();
proc pRestoreCursorPos();
proc pEscSkip();
proc pNullState();
proc pCursorState();
proc pVPosState();
proc pHPosState();
proc pLAttrState();
proc pAnsi();
proc pAnsiState();
proc pGrState();
proc pSkipState();
proc pReverseOff();
proc pReverseOn();
proc pProtOff();
proc pProtOn();
proc pBegProtect();                          /* mbbx 1.03: TV925 */
proc pEndProtect();                          /* mbbx 1.03: TV925 */
proc pBegGraphics();                         /* mbbx 1.03: TV925 */
proc pEndGraphics();                         /* mbbx 1.03: TV925 */
proc pLinDel();
proc pCharDel();
proc pLinIns();
proc pNextLine();
proc pClrAll();
proc pPrintOn();
proc pPrintOff();
proc pTransPrint();                          /* rjs bug2 */
VOID NEAR checkTransPrint(BYTE);             /* rjs swat */
proc pVideoAttrib();
proc pVideoAttribState();
proc pCursorOnOff();
proc pCursorOnOffState();
proc pAnswerBack();
proc pEchoOff();
proc pEchoOn();
proc pCR();
proc pBackSpace();
proc pBeep();
proc pEscSequence();
VOID NEAR aSetCompLevel();                   /* mbbx 1.10: VT220 8BIT */
VOID NEAR pSetCtrlBits();                    /* mbbx 2.00: VT220 8BIT */
proc aCursor();
proc aClrEol();
proc aClrEop();
proc aCursUp();
proc aCursDn();
proc aCursRt();
proc aCursLt();
proc aClearTabs();
proc aVideo();
proc aSetMode();
proc aReport();
proc aSetScrRgn();
proc aDelLin();
proc aInsLin();
proc aDelChar();
proc pVT100H();
proc pVT100D();
proc pVT100M();
proc pVT100c();
proc pVT100P();                              /* mbbx: new routine */
proc pDCS();                                 /* mbbx: yterm */
proc pDCSTerminate();
proc ansiArgument();
proc ansiDelimiter();
proc ansiHeathPrivate();
proc ansiDecPrivate();
proc testPause(BYTE);
BOOL NEAR writeRcvChar(BYTE theChar);                /* mbbx 1.10 */
VOID NEAR putRcvChar(BYTE theChar);                  /* mbbx 1.10 */
VOID putDebugChar(BYTE, BOOL);               /* mbbx 2.00: FAR, bRcvChar... */
VOID modemInp(INT, BOOL);                    /* mbbx 1.10 */

/*** PHONE.C ***/
BOOL termSendCmd(BYTE *str, INT nBytes,WORD  wFlags);

/*** RDMODEM.C ***/
BOOL FAR testWhenEnabled();
BOOL FAR testWhenActive();
BOOL NEAR brake();
VOID rdModem(BOOL);

/*** SERIAL.C ***/
VOID resetSerial(recTrmParams *trmParams, BOOL bLoad, BOOL  bInit, BYTE byFlowFlag);   /* slc swat */
BOOL PASCAL NEAR resetSerialError0(recTrmParams *trmParams, WORD count);
BOOL PASCAL NEAR resetSerialError1(recTrmParams *trmParams, WORD count);
VOID checkCommEvent();

/*** SETTINGS.C ***/
BOOL doSettings(INT, FARPROC);
/* changed WORD -> UINT -sdj */
BOOL  APIENTRY dbPhon(HWND, UINT, WPARAM, LONG);
BOOL  APIENTRY dbEmul(HWND, UINT, WPARAM, LONG);
BOOL  APIENTRY dbTerm(HWND, UINT, WPARAM, LONG);
VOID NEAR setDlgFkeys(HWND, INT);
VOID NEAR getDlgFkeys(HWND, INT);
/* changed WORD -> UINT -sdj */
BOOL  APIENTRY dbFkey(HWND, UINT, WPARAM, LONG);
VOID NEAR enableChrItems(HWND, BOOL);
VOID NEAR enableLinItems(HWND, BOOL);
/* changed WORD -> UINT -sdj */
BOOL  APIENTRY dbTxtX(HWND, UINT, WPARAM, LONG);
BOOL  APIENTRY dbBinX(HWND, UINT, WPARAM, LONG);
BOOL  APIENTRY dbComm(HWND, UINT, WPARAM, LONG);
BOOL  APIENTRY dbComBios(HWND, WORD, WPARAM, LONG);    /* mbbx 2.00: network... */
BOOL  APIENTRY dbNetBios(HWND, WORD, WPARAM, LONG);    /* mbbx 2.00: network... */
BOOL  APIENTRY dbUBNetCI(HWND, WORD, WPARAM, LONG);    /* mbbx 2.00: network... */
BOOL  APIENTRY dbDevice(HWND, WORD, WPARAM, LONG);     /* mbbx 2.00: network... */
/* changed WORD -> UINT -sdj */
BOOL  APIENTRY dbModem(HWND, UINT, WPARAM, LONG);   /* mbbx 1.10: CUA... */
VOID chkGrpButton(HWND, INT, INT, INT);
BYTE whichGrpButton(HWND, INT, INT);
/*                                           mbbx 1.04: obsolete routines...
VOID initDlgPhon ();
VOID dlgCommandPhon ();
VOID initDlgEmul ();
VOID dlgCommandEmul ();
VOID initDlgTerm ();
VOID dlgCommandTerm ();
VOID initDlgFkey ();
VOID dlgCommandFkey ();
VOID setFkeys ();
VOID getFkeys ();
VOID enableButton ();
VOID initDlgTxtX ();
VOID dlgCommandTxtX ();
VOID initDlgBinX ();
VOID dlgCommandBinX ();
VOID initDlgComm ();
VOID dlgCommandComm ();
BOOL  APIENTRY dbModem();
*/
BOOL  APIENTRY dbKerRemote(HWND, WORD, WPARAM, LONG);
/*
VOID initDlgKerRemote();
VOID dlgCommandKerRemote();
*/

/*** MDMUTIL.C ***/
BOOL mdmConnect();
VOID modemReset();
VOID modemSendBreak(INT);
WORD modemBytes();
/* BOOL modemAvail(); */
BYTE getMdmChar(BOOL);                       /* mbbx 1.06A: ics new xlate */
BOOL getRcvChar(BYTE *, BYTE);
BOOL waitRcvChar(BYTE *, WORD, BYTE,BYTE, ...);
BOOL modemRd(BYTE *);                        /* obsolete !!! */
BOOL modemRdy();                             /* obsolete !!! */
BOOL modemWt(BYTE *);                        /* obsolete !!! */
VOID flushRBuff();
BOOL modemWrite(LPSTR, INT);
VOID modemWr(BYTE);
VOID termStr(STRING *, INT, BOOL);

/*** YTERM.C ***/
VOID yTermRcvBlock();
VOID yTermEnd();
VOID yTermCancel();
VOID yTermInit();
VOID blk43Decode();
VOID yTermPutStr();
INT yTermBlkEncode();
VOID ytSendCRC();
VOID yTermResponse();
VOID yTermBegin();
VOID yTermCheckSum();
VOID hostReady();
BYTE hostAck();

/*** DCUTIL1.C ***/
VOID setDefaults();
BOOL clearTermBuffer(WORD prevLines,WORD  bufLines,WORD  lineWidth);      /* mbbx 2.00.03 ... */
BOOL initTermBuffer(WORD bufLines, WORD lineWidth,BOOL bReset);
VOID resetTermBuffer(VOID);

/*** DCUTIL2.C ***/
VOID showTerminal(BOOL, BOOL);               /* mbbx 2.00.08: term init */
VOID showHidedbmyControls(BOOL, BOOL);       /* mbbx 2.00 */

/*** DCUTIL3.C ***/
VOID buildTermFont();
VOID clearFontCache();

// VOID lsetmem(LPSTR, BYTE, WORD);

VOID getFileDate(DOSTIME *, INT);

// VOID lmovmem(LPSTR, LPSTR, WORD);
/*** SCROLL.C ***/
VOID updateTermScrollBars(BOOL);             /* mbbx 2.00.06: jtf display... */
proc scrollTermWindow(INT, INT);
proc scrollTermLine(INT, INT, INT);
VOID scrollBits();
VOID scrollUp(INT, INT, INT);
VOID scrollDown(INT, INT, INT);
VOID pageScroll(INT);
VOID hPageScroll(INT);
VOID trackScroll(INT, INT);

/*** SHOWSTAT.C ***/                         /* mbbx 2.00: xfer ctrls... */
VOID setXferCtrlButton(WORD wCtrlID, WORD wResID);
INT NEAR placeXferCtrl(HWND, INT);
VOID showXferCtrls(WORD fShowCtrls);
HDC NEAR beginXferCtrlUpdate(HWND, RECT *, BOOL);
VOID NEAR setItemText(INT, BYTE *, BOOL);
VOID bSetUp(BYTE *);
VOID showScale();
VOID updateProgress(BOOL);
VOID showBBytes(LONG, BOOL);
VOID showRXFname(BYTE *, INT);
VOID showBErrors(INT);
VOID updateIndicators();




/* mbbx: yterm mods to SNDBFILE.C */
VOID setupFinderInfo();
VOID getFinderInfo();

proc scrollTopPart ();
proc scroll ();

VOID pageFeed ();
VOID paintTerm ();

VOID setFKeyTitles();                        /* mbbx 2.00 ... */
BOOL NEAR testFKeyLevel(INT);
INT NEAR nextFKeyLevel(INT);
VOID setFKeyLevel(INT, BOOL);

VOID doCommand ();
VOID resetEmul ();
VOID openAwindow ();
VOID clearBuffer ();
VOID termAnswer ();
VOID termSpecial ();
VOID clearModes ();
VOID onCursor ();
VOID offCursor ();

VOID putCursor ();

VOID reDrawTermLine ();
VOID toggleCursor ();
VOID doEditMenu ();
VOID scrollText ();
VOID xferFile ();
VOID termSpecial ();
VOID stripLeadingSpaces ();
VOID xferEnd ();
VOID trackScroll ();
VOID scrollBits ();
VOID scrollUp ();
VOID scrollDown ();
VOID pageScroll ();
VOID hPageScroll ();
VOID teScr ();
VOID delay (WORD units, LONG *endingCount);
VOID setDefaultFonts ();
/* VOID zoomTerm (); */
VOID zoomChild(HWND);
VOID freeItResources ();
VOID updateTimer ();
VOID readDateTime ();
VOID timerAction(BOOL, BOOL);                /* mbbx 1.03: VOID tmrAction(); */
VOID timerToggle(BOOL);                      /* mbbx 1.03: VOID tmrToggle(); */
VOID clipRect ();
//VOID setAttrib ();
VOID getMdmResult ();
VOID saveSelection ();
VOID frameTime ();
VOID loadKeyPadString ();
VOID rectCursor ();
VOID exitSerial ();
VOID errTest ();
VOID newFile ();
VOID clsFile ();
VOID savFile ();
VOID svsFile ();
VOID opnFile ();
VOID getWTitle ();
VOID addExtension();
VOID setWTitle ();
VOID execErr ();
VOID addParen ();
VOID rdErr ();
VOID stripFileExt ();
/* VOID forceExtension();                    mbbx 2.00: defined in FILEOPEN.H */
VOID sndTFile ();
VOID typTFile ();
VOID rcvTFile ();
VOID sndBFile ();
VOID rcvPutFile ();
VOID rcvBFile ();
VOID rcvErr ();
VOID rcvPre ();
VOID showRXFname ();
VOID showScale ();
VOID sndBPre ();
VOID sndBFileErr ();
VOID showBErrors ();
VOID rcvErr ();
VOID rcvBPre ();
VOID showBBytes ();
VOID rxEventLoop ();
VOID rcvAbort ();

VOID getDateTime ();
VOID sec2date ();
VOID date2secs ();
VOID termActivate ();
VOID termDeactivate ();
VOID termSetSelect ();
VOID termClick ();
VOID releasePort ();
VOID dialPhone ();
VOID hangUpPhone ();
VOID waitCall ();
VOID printchar ();
VOID lineFeed ();
VOID endOfPrintJob ();
VOID prAction ();
VOID prSelection ();
VOID xShowLine ();
VOID ansInTalk ();
VOID ansKermit();
VOID selectTermFont ();
VOID  sizeTimerRect ();
VOID  hideTermCursor ();
VOID  showTermCursor ();

BOOL getScrCh ();
BOOL copyTable ();
BOOL myAbort();
BOOL keyPadSequence ();
BOOL getArg ();
BOOL  APIENTRY dbSendFile ();
BOOL  APIENTRY pasClikLoop ();
BOOL  APIENTRY dbSelSavApp ();
BOOL  APIENTRY dbAbortDlg ();
//BOOL  APIENTRY dbDialing ();
//BOOL  APIENTRY dbmyControls();
BOOL writeFile ();
BOOL myPutFile ();
BOOL getPutFN ();
BOOL newPath ();
BOOL searchFileSpec ();
BOOL checkFilename ();
BOOL isCharLegal ();
BOOL fnErr ();
BOOL fileExist ();
BOOL ldFile ();
BOOL readFile ();
BOOL rdFileErr ();
BOOL  APIENTRY dbStdFN ();
BOOL  APIENTRY dbGetPutFN ();
BOOL  APIENTRY dbTypTFile ();
BOOL  APIENTRY dbRcvTFile ();
BOOL  APIENTRY dbAbortDlg ();
BOOL  APIENTRY abortDlgProc ();
BOOL readMacSettings ();
BOOL writeMacSettings ();
BOOL readMacSettings ();
BOOL writeMacSettings ();
//BOOL  APIENTRY dbDialing ();

BOOL rcvPutBFile ();
BOOL getSndTFile ();
BOOL getSndBFile ();
BOOL getSndTFile ();
BOOL startOfPrintJob ();

BYTE *concat ();
// BYTE *getcwd ();
/* this is redefined here, now including the cruntime headers -sdj*/
/* BYTE *strchr (); */
BYTE whichGrpButton ();

INT  valIndex ();
INT  pos ();
INT classifyKey (WORD vrtKey);
INT availSerial ();
INT  testBox ();
INT  sysError ();
INT  wrErr ();

LONG fileSize();

HDC getPort ();

HANDLE macToDOSText ();

/* VOID newHTE ();                           mbbx 2.00.03: old code */
/* VOID crBuffer(int, int);                  mbbx 2.00.03: old code */

/*** DATESTUF.C ***/
VOID date2secs (DOSTIME *, LONG *);






/*were  not defined but called from initcode.c */

BOOL PrintFileInit();
int PrintFileShutDown ();
VOID sizeFkeys(LONG clientSize);
VOID initChildSize(RECT *pRect);
VOID setDefaultAttrib(BOOL bLoad);
VOID initDlgPos(HWND hDlg);
VOID initComDevSelect(HWND hDlg, WORD wListID, BOOL bInit);
BYTE getComDevSelect(HWND hDlg, WORD wListID, BYTE *newDevRef);
BOOL getFileType(BYTE *fileName, BYTE *fileExt);
VOID taskInit();
VOID keyMapInit();
BOOL termInitSetup(HANDLE hPrevInstance);
VOID forceExtension(BYTE *fileName, BYTE *fileExt, BOOL bReplace);
BOOL termFile(BYTE *filePath,BYTE *fileName,BYTE *fileExt,BYTE *title,WORD flags);
VOID sizeTerm(LONG termSize);
VOID keyMapCancel();

/*****************/

/* were not defined but called from winmain.c */

VOID xSndBFile();
VOID xRcvBFile();

/**************/

/* were not defined but called from winmain.c*/

int myDrawIcon(HDC hDC, BOOL bErase);
BOOL termCloseAll(VOID);
int flashIcon(BOOL bInitFlash, BOOL bEndProc);
WORD childZoomStatus(WORD wTest, WORD wSet);
VOID initMenuPopup(WORD menuIndex);
BOOL keyMapTranslate(WPARAM *wParam, LONG *lParam, STRING *mapStr);
BOOL fKeyStrBuffer(BYTE *str,WORD  len);
BOOL keyMapSysKey(HWND hWnd, WORD message , WPARAM *wParam, LONG lParam); //sdj: AltGr
VOID longToPoint(long sel, POINT *pt);

VOID keyMapKeyProc(HWND hWnd, WORD message, WPARAM wParam, LONG lParam); //sdj: AltGr
BOOL termCloseFile(VOID);
VOID hpageScroll(int which);



VOID PrintFileString(LPSTR lpchr,LONG  count, BOOL bCRtoLF);
BOOL PrintFileOn(HANDLE theInstance,HWND theWnd,
LPSTR thePrintName,LPSTR thePrintType,LPSTR thePrintDriver,
LPSTR thePrintPort,BOOL showDialog);
BOOL PrintFileOff();
int PrintFileLineFeed (BOOL nextLine);
int PrintFilePageFeed ();


BOOL termSaveFile(BOOL bGetName);


int testMsg(BYTE *str0, BYTE* str1, BYTE *str2);



VOID xferStopBreak(BOOL bStop);

VOID xferPauseResume(BOOL bPause, BOOL bResume);

INT selectFKey(WORD wIDFKey);

BOOL sendKeyInput(BYTE theByte);

VOID sndAbort  ();

int countChildWindows(BOOL bUnzoom);

VOID  stripBlanks (LPBYTE ptr, DWORD *len);

VOID  doFileNew();

VOID  doFileOpen();
VOID  doFileClose();
VOID  doFileSave();
VOID  doFileSaveAs();

VOID  stripControl(STRING *str);
int   TF_ErrProc(WORD errMess, WORD errType,WORD  errCode);


BOOL  XM_RcvFile(WORD rcvStatus);
BOOL  FAR KER_Receive(BOOL bRemoteServer);
VOID  listFontSizes(BYTE *faceName, BYTE *sizeList, int maxSize);

int   updateIcon();
BOOL  XM_SndFile(WORD);
BOOL  FAR KER_Send();
VOID  setAppTitle();


VOID  icsResetTable(WORD icsType);
VOID  rcvFileErr();


/* connector.c needs this fn from wndproc.c */
HWND  dlgGetFocus();

/* initcode.c needs this from connect.c */
BOOL  initConnectors(BOOL bInit);
/* defined in messages.c called from dcutil */
INT   testResMsg(WORD wResID);

/* two famous functions. dont know how this was working in win3.0 -sdj*/
/* these functions have some case problem, called with uppercase B and
   defined with lowercase b in term.c, not changed to Board */

VOID  keyBoardToMouse(INT partCode);

/* this one was called as bSetup and defined as bSetUp() in showstat.c */
VOID  bSetup(BYTE  *str);

LONG  APIENTRY    nextFlash(HWND hWnd, UINT message, DWORD nIDEvent, LONG sysTime);
LONG  APIENTRY    dbDialing(HWND hDlg,UINT message,WPARAM wParam,LONG lParam);
LONG  APIENTRY    dbmyControls(HWND hDlg,UINT message,WPARAM wParam,LONG lParam);

/* rjs - add prototype for the about dialog function */
long CALLBACK dbAbout(HWND hDlg, UINT message, DWORD wParam, LONG lParam);

