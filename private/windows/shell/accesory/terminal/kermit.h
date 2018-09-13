/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

BOOL  bKermitTaskRemote;                     /* slc nova 110 */
INT   gKermitTaskCMD;                        /* slc nova 110 */
LPSTR glpKermitTaskParm1;                    /* slc nova 110 */
LPSTR glpKermitTaskParm2;                    /* slc nova 110 */

/* basic file transfer packet types */
#define KER_ACK            'Y'
#define KER_NACK           'N'
#define KER_SEND           'S'
#define KER_INIT           'I'
#define KER_FILE           'F'
#define KER_TEXT           'X'
#define KER_ATTRIB         'A'
#define KER_DATA           'D'
#define KER_EOF            'Z'
#define KER_BREAK          'B'
#define KER_ERROR          'E'
#define KER_RCV            'R'
#define KER_COMMAND        'C'
#define KER_KERMIT         'K'
#define KER_TIMEOUT        'T'
#define KER_ERRCHK         'Q'

#define KER_CMPLT          'C'
#define KER_ABORT          'A'

#define KER_GENERIC        'G'

/* generic packet types */
#define KER_LOGIN          'I'
#define KER_CHDIR          'C'
#define KER_LOGOUT         'L'
#define KER_FINISH         'F'
#define KER_DIR            'D'
#define KER_FREE           'U'
#define KER_DEL            'E'
#define KER_TYPE           'T'
#define KER_REN            'R'
#define KER_COPY           'K'
#define KER_WHO            'W'
#define KER_MESSAGE        'M'
#define KER_HELP           'H'
#define KER_STATUS         'Q'
#define KER_PROGRAM        'P'
#define KER_JOURNAL        'J'
#define KER_VAR            'V'

#define KER_NOPARITY       '\0'
#define KER_ODDPARITY      'o'
#define KER_EVENPARITY     'e'
#define KER_MARKPARITY     'm'
#define KER_SPACEPARITY    's'

/* initialization packet subscripts and values */
#define KER_INIT_MAXL      0
#define KER_INIT_TIME      1
#define KER_INIT_NPAD      2
#define KER_INIT_PADC      3
#define KER_INIT_EOL       4
#define KER_INIT_QCTL      5
#define KER_INIT_QBIN      6
#define KER_INIT_CHKT      7
#define KER_INIT_REPT      8

#define KER_MAXPACKSIZE    94                /* Maximum packet size */
#define KER_RCVTIMEOUT     13                /* Seconds for receive time out */
#define KER_SNDTIMEOUT      8                /* Seconds for send time out */
#define KER_MAXTIMEOUT     60                /* Maximum timeout interval */
#define KER_MINTIMEOUT      2                /* Minumum timeout interval */
#define KER_NPAD            0                /* Number of padding characters I will need */
#define KER_PADCHAR         0                /* Padding character I need (NULL) */
#define KER_EOL            0x0D              /* End-Of-Packet character */
#define KER_QUOTE          '#'               /* Control prefix character */
#define KER_8BITPREFIX     '&'               /* Eighth bit prefix character */
#define KER_BLOCKCHK1      '1'               /* Block check type (1 byte) */
#define KER_BLOCKCHK2      '2'               /* Block check type (2 byte) */
#define KER_BLOCKCHK3      '3'               /* Block check type (3 byte) */

/* other kermit session constants */
#define KER_MAXRETRY       5                 /* Times to retry a packet */

/*  myh swat: set the buffer size for the number of bytes read in from file at a time */
#define BUFFSIZE           512

INT KER_bytetran;          /* myh swat: number of bytes transfered from the buffer */
INT KER_buffsiz;           /* myh swat: the buffer size read from the file; usually is */
                           /*     BUFFSIZE, but the last buffer size can be smaller    */
BYTE KER_buffer[BUFFSIZE]; /* myh swat: the buffer storing read in chars from file */
INT KER_debug;
BYTE KER_rem8bit;

typedef
   struct   {
      INT   KER_spsiz;        /* Maximum send packet size */
      INT   KER_timint;       /* Timeout for foreign host on sends */
      INT   KER_pad;          /* How much padding to send */
      BYTE  KER_padchar;      /* Padding character to send */
      BYTE  KER_eol;          /* End-Of-Line character to send */
      BYTE  KER_quote;        /* Quote character in incoming data */
      BYTE  KER_select8;      /* 8th bit quote character to send either 'Y' or '&'*/
            } KERPACKETSTRUCT;
typedef
   struct   {
      INT   KER_size;         /* Size of present data */
      INT   KER_rpsiz;        /* Maximum receive packet size */
      INT   KER_spsiz;        /* Maximum send packet size */
      INT   KER_pad;          /* How much padding to send */
      INT   KER_timint;       /* Timeout for foreign host on sends */
      INT   KER_n;            /* Packet number */
      INT   KER_numtry;       /* Times this packet retried */
      INT   KER_oldtry;       /* Times previous packet retried */
      INT   ttyfd;            /* File descriptor of tty for I/O, 0 if remote */
      INT   KER_remote;       /* -1 means we're a remote kermit */
      INT   KER_image;        /* -1 means 8-bit mode */
      INT   KER_parflg;       /* TRUE means use parity specified */
      INT   KER_turn;         /* TRUE means look for turnaround char (XON) */
      INT   KER_lecho;        /* TRUE for locally echo chars in connect mode */
      INT   KER_8flag;        /* TRUE means 8th bit quoting is done */
      INT   KER_pktdeb;       /* TRUE means log all packet to a file */
      INT   KER_filnamcnv;    /* -1 means do file name case conversions */
      INT   KER_filecount;    /* Number of files left to send */
      INT   KER_timeout;      /* TRUE means a timeout has occurred. */
      BYTE  KER_state;        /* Present state of the automaton */
      BYTE  KER_cchksum;      /* Our (computed) checksum */
      BYTE  KER_padchar;      /* Padding character to send */
      BYTE  KER_eol;          /* End-Of-Line character to send */
      BYTE  KER_escchr;       /* Connect command escape character */
      BYTE  KER_quote;        /* Quote character in incoming data */
      BYTE  KER_select8;      /* 8th bit quote character to send either 'Y' or '&'*/
      BYTE  KER_firstfile;
      BYTE  KER_getflag;
      BYTE  **KER_filelist;         /* List of files to be sent */
      BYTE  *KER_filnam;            /* Current file name */
      BYTE  recpkt[KER_MAXPACKSIZE];     /* Receive packet buffer */
      BYTE  packet[KER_MAXPACKSIZE];     /* Packet buffer */
      BYTE  KER_buff[KER_MAXPACKSIZE];   /* buffer for translations */
      BYTE  outstr[80];             /*output string for debugging and translations */
      BYTE  KERRCVFLAG;             /* direct recieved buffers to screen, file or buffer*/

      LONG  KER_bytes;              /* number of bytes received */
            } KERMITSTRUCT;

BOOL FAR  KER_Receive(BOOL);
BYTE NEAR KER_ReceiveInit();
BYTE NEAR KER_ReceiveFile();
BYTE NEAR KER_ReceiveData();

BOOL FAR  KER_Send();
BYTE NEAR KER_SendInit();
BYTE NEAR KER_SendFile();
BYTE NEAR KER_SendData();
BYTE NEAR KER_SendGeneric(BYTE lKER_state);

VOID NEAR KER_Init(BYTE state);
INT  NEAR KER_GetParity();
INT  NEAR KER_GetTurnAroundTime();
INT  NEAR KER_GetLocalEcho();
BYTE NEAR KER_Abort(WORD msgID);
BYTE NEAR KER_DoParity (BYTE ch);
VOID NEAR KER_SndPacket(BYTE type, INT num, INT len, BYTE *data);
BYTE NEAR KER_RcvPacket(INT*, INT*, BYTE*);
BYTE NEAR KER_CInChar();
BYTE NEAR KER_InChar();
INT  NEAR KER_ModemWait();
INT  NEAR KER_BufferFill(BYTE*);
VOID NEAR KER_BufferEmpty(BYTE buffer[],INT len,BYTE flag);
BOOL NEAR KER_GetNextFile();
BYTE NEAR KER_PrintErrPacket(BYTE*);
VOID NEAR KER_RcvPacketInit(BYTE*);
VOID NEAR KER_SndPacketInit(BYTE*);
VOID FAR  KER_Answer();
VOID FAR  KER_Server(BYTE, INT, INT);
VOID NEAR KER_DoGenericPack(BYTE*, INT, INT);
BOOL NEAR KER_DoDir(BYTE*);
BOOL   APIENTRY dbKerRemote(HWND, WORD, WPARAM, LONG);
BOOL NEAR KER_RemoteParamsOK(HWND, INT);
VOID NEAR KER_DoRemoteShow(HWND, INT);
VOID NEAR KER_Remote(HWND, INT);
BYTE NEAR KER_Tinit();
INT  NEAR KER_Encode(BYTE*, BYTE*, INT);
VOID NEAR KER_Pack(HWND, BYTE, INT, INT);
VOID NEAR KER_HandleTrans();
VOID NEAR KER_PutScreenStr(BYTE*);
VOID NEAR KER_bSetUp(BYTE state);
BYTE NEAR KER_AutoPar();                        /* tge gold 01 */
VOID NEAR KER_ResetFromRemote();                /* tge gold 01 */
