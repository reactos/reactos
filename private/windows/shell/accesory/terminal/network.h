/*****************************************************************************/
/* NETBIOS: definitions for netbios interface                                */
/*****************************************************************************/

/*****************************************************************************/
/* Constant Definitions                                                      */
/*****************************************************************************/

#define DOS_GETVECTOR            0x35

#define INT_NETBIOS              0x5C
#define INT_ATTEXT_NETBIOS       0x5B

#define NAMSZ           16          /* max length of a net names */
#define NAMSZLONG       80          /* max length of extended net names */
#define NAMSZISN        64          /* max length of ISN names */
#define NETBUFFERSIZE   (768-64)    /* now 3/4 K bytes for each NCB */
#if OLDCODE
/* tge gold 005 changed to a variable, used with checkbox in dialog */
#define NETNAMEPADDING  0x20        /* mbbx 2.01.28: was 0x20 */
#endif
#define MAXTASKNCBS     32          /* max number of outstanding NCB's
                                       that DC can keep track of */

#define NAB_MAX_LUS     32          /* NABIOS max LU's per PU */

#define WWID_WHEN_MASK     0xFF00
#define WWID_WAIT_MASK     0x00FF
#define WWID_WHEN_STD      0x0100
#define WWID_WAIT_STD      0x0001
#define WWID_WHEN_NABIOS   0x0200
#define WWID_WAIT_NABIOS   0x0002
#define WWID_WHEN_ATT      0x0300
#define WWID_WAIT_ATT      0x0003

#define NET_TASK_NETINT    0   /* flags for hScriptNCB->res */
#define NET_TASK_NETLANA   1   /* (secret place for script runtime flags) */
#define NET_TASK_NABINT    2
#define NET_TASK_NABLANA   3 
#define NET_TASK_ATTINT    4
#define NET_TASK_ATTLANA   5 

#define NETBIOS_NOTEXISTS -1  /* flags for dcIoStat.netbiosStatus */
#define NETBIOS_NOTCHECKED 0
#define NETBIOS_EXISTS     1

#define NABIOS_NOTEXISTS  -1  /* flags for dcIoStat.nabiosStatus */
#define NABIOS_NOTCHECKED  0
#define NABIOS_EXISTS      1

#define ATTBIOS_NOTEXISTS -1  /* flags for dcIoStat.attbiosStatus */
#define ATTBIOS_NOTCHECKED 0
#define ATTBIOS_EXISTS     1

#define FSE_LOCK     0        /* flags for memJugler() */
#define FSE_UNLOCK   1
#define FSE_FREE     2

#define NERR_LVL_IGNORE 0x01  /* netbios error levels */
#define NERR_LVL_WEIRD  0x02
#define NERR_LVL_ERROR  0x03

#define NET_COM_OK      0x00  /* NET_CommandWait() constants */
#define NET_COM_ABORT   0x01
#define NET_COM_FAIL    0x02
#define NET_COM_BUSY    0x03
#define NET_COM_DUP     0x04

#define COM_ADDN_NCB    0x00  /* NCB constants */
#define COM_CALL_NCB    0x00
#define COM_LISN_NCB    0x00
#define COM_HANG_NCB    0x00
#define COM_STAT_NCB    0x00
#define COM_BRAK_NCB    0x00  /* mbbx 2.00: ATT extension */
#define COM_SEND_NCB    0x01
#define COM_RECV_NCB    0x02
#define COM_CNCL_NCB    0x03
#define COM_DATA_NCB    0x04
#define LAST_COMM_NCB   0x04
#define MAXCOMMNCBS     5     /* NCB's for comm session */

#define NET_ASYNCH      0x80  /* int 5Ch calls */

#define NET_CALL_RESET     0x32     /******* General Commands *******/
#define NET_CALL_ASTATUS   0x33
#define NET_CALL_CANCEL    0x35
#define NET_CALL_UNLINK    0x70

#define NET_CALL_ADDNAME   0x30     /******* Name Support Commands *******/
#define NET_CALL_ADDGNAME  0x36
#define NET_CALL_DELNAME   0x31

#define NET_CALL_CALL         0x10  /******* Session Support Commands *******/
#define NET_CALL_LISTEN       0x11
#define NET_CALL_HANGUP       0x12
#define NET_CALL_SEND         0x14
#define NET_CALL_CHAINSEND    0x17
#define NET_CALL_RECEIVE      0x15
#define NET_CALL_RECEIVEANY   0x16
#define NET_CALL_SSTATUS      0x34

#define NET_CALL_DGSEND       0x20  /******* Datagram Support Commands *******/
#define NET_CALL_BDGSEND      0x22
#define NET_CALL_DGRECEIVE    0x21
#define NET_CALL_BDGRECEIVE   0x23

#define NET_CALL_CALLNIU      0x74  /******* UB Extensions *******/
#define NET_CALL_LISTENNIU    0x7B
#define NET_CALL_UBSNDPACKET  0x77           /* mbbx 2.00.06 ... */
#define NET_CALL_UBRCVPACKET  0x78

#define NET_CALL_CALLISN      0x10  /******* AT&T Extensions *******/
#define NET_CALL_BREAK        0x70  /* ATT via int 5Bh */

#define NET_CALL_ACTPU        0x30  /******* NABIOS Extensions *******/
#define NET_CALL_DACTPU       0x31

#define NET_CALL_ACTLU        0x10
#define NET_CALL_DACTLU       0x12

#define NET_CALL_NABSEND      0x14
#define NET_CALL_NABRECV      0x15

#define NET_CALL_NABASTAT     0x33
#define NET_CALL_NABSSTAT     0x34

#define NET_CALL_INVALID      0x7F  /* invalid command to test for Netbios */

#define NLANANETBIOS    0x00  /* lpNcb.lana constants */
#define NLANAEICONS     0xFF

/* ncb return codes */
#define NET_ERR_GOODRET 0x00  /* good return */
#define NET_ERR_BUFLEN  0x01  /* illegal buffer length */
#define NET_ERR_BFULL   0x02  /* buffers full, no receive issued */
#define NET_ERR_ILLCMD  0x03  /* illegal command */
#define NET_ERR_CMDTMO  0x05  /* command timed out */
#define NET_ERR_INCOMP  0x06  /* message incomplete, issue another command */
#define NET_ERR_BADDR   0x07  /* illegal buffer address */
#define NET_ERR_SNUMOUT 0x08  /* session number out of range */
#define NET_ERR_NORES   0x09  /* no resource available */
#define NET_ERR_SCLOSED 0x0a  /* session closed */
#define NET_ERR_CMDCAN  0x0b  /* command canceled */
#define NET_ERR_DMAFAIL 0x0c  /* PC DMA failed */
#define NET_ERR_DUPNAME 0x0d  /* duplicate name */
#define NET_ERR_NAMTFUL 0x0e  /* name table full */
#define NET_ERR_ACTSES  0x0f  /* no deletions, name has active sessions */
#define NET_ERR_INVALID 0x10  /* name not found or no valid name */
#define NET_ERR_LOCTFUL 0x11  /* local session table full */
#define NET_ERR_REMTFUL 0x12  /* remote session table full */
#define NET_ERR_ILLNN   0x13  /* illegal name number */
#define NET_ERR_NOCALL  0x14  /* no callname */
#define NET_ERR_NOWILD  0x15  /* cannot put * in NCB_NAME */
#define NET_ERR_INUSE   0x16  /* name in use on remote adapter */
#define NET_ERR_NAMERR  0x17  /* called name cannot == name nor name # */
#define NET_ERR_SABORT  0x18  /* session ended abnormally */
#define NET_ERR_NAMCONF 0x19  /* name conflict detected */
#define NET_ERR_REMTDEV 0x1A  /* Incompatible remote device */
#define NET_ERR_IFBUSY  0x21  /* interface busy, IRET before retrying */
#define NET_ERR_TOOMANY 0x22  /* too many commands outstanding, retry later */
#define NET_ERR_BRIDGE  0x23  /* ncb_bridge field not 00 or 01 */
#define NET_ERR_CANOCCR 0x24  /* command completed while cancel occuring */
#define NET_ERR_RESNAME 0x25  /* reserved name specified */
#define NET_ERR_CANCEL  0x26  /* command not valid to cancel */
#define NET_ERR_MULT    0x33  /* multiple requests for same session */
#define NET_ERR_SYSTEM  0x40  /* system error */
#define NET_ERR_ROM     0x41  /* ROM checksum failure */
#define NET_ERR_RAM     0x42  /* RAM test failure */
#define NET_ERR_DLF     0x43  /* digital loopback failure */
#define NET_ERR_ALF     0x44  /* analog loopback failure */
#define NET_ERR_IFAIL   0x45  /* interface failure */
#define NET_ERR_ADMALF  0x46  /* adapter malfunction */
#define NET_ERR_UNDEFERR 0x47 /* Undefined Network Error */
#define NET_ERR_LAST    0x47

/* EICONS specific errors */
#define NET_ERR_MDMMASK 0x90  /* Modem Not Ready Mask */
#define NET_ERR_NODSR   0x91  /* Modem Not Ready (No DSR) */
#define NET_ERR_NOCTS   0x92  /* Modem Not Ready (No CTS) */
#define NET_ERR_NOCLOCK 0x93  /* Modem Not Ready (No Clock) */

#define NET_ERR_LNKMASK 0xA0  /* Link level not ready */
#define NET_ERR_PCKMASK 0xB0  /* Packet level not ready */

#define NET_ERR_PENDING 0xFF  /* asynchronous command is not yet finished */

/* nabios errors */
#define NET_NERR_GOODRET   0x00  /* good return */
#define NET_NERR_BUFLEN    0x01  /* Illegal buffer length. */
#define NET_NERR_INVALPU   0x03  /* PU not active. */
#define NET_NERR_INCOMP    0x06  /* Message incomplete. */
#define NET_NERR_INVALSID  0x08  /* Invalid session number. */
#define NET_NERR_LUNOTACT  0x0a  /* LU not active. */
#define NET_NERR_CMDCAN    0x0b  /* Command cancelled. */
#define NET_NERR_DUPPU     0x0d  /* PU name already exists. */
#define NET_NERR_PUTFUL    0x0e  /* PU name table full. */
#define NET_NERR_LUTFUL    0x11  /* No circuits available. */
#define NET_NERR_UNSUCC    0x12  /* Call unsuccessful. */
#define NET_NERR_NORESP    0x14  /* No response from Server. */
#define NET_NERR_PUINUSE   0x16  /* Station Address already assigned to a PU. */
#define NET_NERR_PUNOTACT  0x17  /* PU name not active. */
#define NET_NERR_STATECHG  0x18  /* QLLC/SDLC state change. */
#define NET_NERR_DATATRAF  0x1A  /* Data traffic reset or in Receive state. */
#define NET_NERR_TOOMANY   0x22  /* Too many commands oustanding. */
#define NET_NERR_LAST      0x22

/* my errors */
#define NET_ERR_CONNECT   0x00      /* Connected to Netbios */
#define NET_ERR_ALRDYCON  0x01      /* Already Connected to Netbios */
#define NET_ERR_NONETBIO  0x02      /* Cannot find Netbios */
#define NET_ERR_CANTCONN  0x03      /* Can't Connect to Netbios */
#define NET_ERR_LINKERR   0x0D      /* Link level not ready */
#define NET_ERR_PACKERR   0x0E      /* Packet level not ready */
#define NET_ERR_ACTSESS   0x10      /* Packet level not ready */


/*****************************************************************************/
/* Type & Structure Definitions                                              */
/*****************************************************************************/

typedef enum
{
   CONV_CLIENT, 
   CONV_SERVER
}
CONVERSATION;


#define FARP struct longptr   /* 8088/86 long pointer */

FARP
{
   INT lp_offset;    /* offset */
   INT lp_seg;       /* segment */
};

typedef struct                /* Net Control Block 512 bytes */
{
   BYTE ncb_com;              /* command */
   BYTE ncb_ret;              /* return code */
   BYTE ncb_lsn;              /* local session # */
   BYTE ncb_num;              /* number of network name */
   FARP ncb_bfr;              /* pntr to message buffer */
   WORD ncb_len;              /* msg length in unsigned chars */
   CHAR ncb_rname[NAMSZ];     /* blank-padded name of * remote end of connection */
   CHAR ncb_lname[NAMSZ];     /* our blank-padded network name */
   BYTE ncb_rto;              /* rcv timeout/retry count */
   BYTE ncb_sto;              /* send timeout/sys timeout */
   FARP ncb_sig;              /* interrupt signal routine */
   BYTE ncb_lana;             /* reserved */
   BYTE ncb_cplt;             /* 0xff => commmand pending */

   FARP ncb_vrCallName;       /* ATT special - address of ISN name */
   BYTE ncb_vrCallNameSz;     /* ATT special - length of ISN name */
   BYTE ncb_res[9];           /* reserved */
/*
   BYTE ncb_res[14];          /* reserved *
*/
   BYTE ncb_buffer[NETBUFFERSIZE];
} NCB;

typedef NCB FAR *LPNCB;
typedef NCB NEAR *PNCB;

typedef struct                /* 18 bytes */
{                             /* Name entries */
   BYTE as_name[NAMSZ];       /* Name */
   BYTE as_number;            /* Name number */
   BYTE as_status;            /* Name status */
} ASTATNAME;

typedef struct                /* 60 + (16 * 18) =  348 bytes */
{
   BYTE as_uid[6];            /* Unit identification number */
   BYTE as_ejs;               /* External jumper status */
   BYTE as_lst;               /* Results of last self-test */
   BYTE as_ver;               /* Software version number */
   BYTE as_rev;               /* Software revision number */
   WORD as_dur;               /* Duration of reporting period */
   WORD as_crc;               /* Number of CRC errors */
   WORD as_align;             /* Number of alignment errors */
   WORD as_coll;              /* Number of collisions */
   WORD as_abort;             /* Number of aborted transmissions */
   LONG as_spkt;              /* Number of successful packets sent */
   LONG as_rpkt;              /* No. of successful packets rec'd */
   WORD as_retry;             /* Number of retransmissions */
   WORD as_exhst;             /* Number of times exhausted */
   BYTE as_res0[8];           /* Reserved */
   WORD as_ncbfree;           /* Free ncbs */
   WORD as_numncb;            /* number of ncbs configured */
   WORD as_maxncb;            /* max configurable ncbs */
   BYTE as_res1[4];           /* Reserved */
   WORD as_sesinuse;          /* sessions in use */
   WORD as_numses;            /* number of sessions configured */
   WORD as_maxses;            /* Max configurable sessions */
   WORD as_maxdat;            /* Max. data packet size */
   WORD as_names;             /* No. of names in local table */
   ASTATNAME as_struct[16];   /* Name entries */
} ASTATSTRUCT;

typedef ASTATSTRUCT FAR *LPASTATSTRUCT;
typedef ASTATSTRUCT NEAR *PASTATSTRUCT;

typedef struct                /* 36 bytes */
{                             /* Name entries */
   BYTE ss_session;           /* local session number */
   BYTE ss_status;            /* session status */
   BYTE ss_lname[NAMSZ];      /* local name */
   BYTE ss_rname[NAMSZ];      /* remote name */
   BYTE ss_recout;            /* outstanding receive commands */
   BYTE ss_sndout;            /* outstanding send & chain send commands */
} SSTATNAME;

typedef struct                /* 4 + (16 * 36) = 580 bytes */
{
   BYTE ss_numses;            /* sessions */
   BYTE ss_numsesname;        /* sessions with this name */
   BYTE ss_recdgout;          /* outstanding rec & rec broad datagram commands */
   BYTE ss_recanyout;         /* outstanding rec any commands */
   SSTATNAME ss_struct[16];   /* Name entries */
} SSTATSTRUCT;

typedef SSTATSTRUCT FAR *LPSSTATSTRUCT;
typedef SSTATSTRUCT NEAR *PSSTATSTRUCT;

typedef struct
{
   INT hNCB;                  /* handles for NCB's */
   LPNCB lpNCB;               /* long pointers for NCB's */
   WORD whenWaitID;           /* ARG_WHEN_LISTEN, ARG_WHEN_RECEIVE or NULL = wait */
} TASKNCBDATA;


/*****************************************************************************/
/* Variable Definitions                                                      */
/*****************************************************************************/

struct                                       /* mbbx 2.00.04: rkhx netbios... */
{
   BYTE  lsn;
   BYTE  num;
   WORD  maxSendBytes;                       /* mbbx 2.00.06: netbios extensions */

   WORD  netbiosStatus;
   WORD  nabiosStatus;
   WORD  attbiosStatus;
} dcIoStat;

INT debugFileHandle;


/* task netbios data */
TASKNCBDATA taskNcbs[MAXTASKNCBS];  /* (1 + 2 + 1) * 32 = 128 bytes */
WORD taskNcbStatus;                 /* general status info for task ncb's */
HANDLE hScriptNcb;                  /* handle for script ncb */
HANDLE hAdapterStatus;              /* handle for script adapter status */
HANDLE hSessionStatus;              /* handle for script session status */

/* comm netbios data */
HANDLE hCommNcbs;
LPNCB lpCommNcbs;

/* tge gold 005 changed from a constant, used with checkbox in dialog */
BYTE NETNAMEPADDING;

/*****************************************************************************/
/* Macro Definitions                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* Forward Procedure Definitions                                             */
/*****************************************************************************/

VOID NET_exitSerial();
VOID NET_SetDefaults();
WORD NET_Connect(BOOL);
VOID NET_resetSerial(recTrmParams *, BOOL);  /* mbbx 2.01.141 */

VOID NET_modemSendBreak(INT);
INT NET_ReadComm(LPSTR, INT);
VOID NET_modemBytes();
INT NET_WriteComm(LPSTR, INT);
BOOL NET_modemWrite(LPSTR, INT);

BYTE NETBIOS_Reset(LPNCB, BOOL, BYTE, BYTE, BYTE, BYTE); /* General Commands */
BYTE NETBIOS_AdapterStatus(LPNCB, BOOL, LPBYTE, WORD, BYTE, BYTE);
BYTE NETBIOS_Cancel(LPNCB, WORD, BYTE, BYTE);
BYTE NETBIOS_Unlink(LPNCB, BYTE, BYTE);

BYTE NETBIOS_AddName(LPNCB, BOOL, LPBYTE, BYTE, BYTE);   /* Name Support Commands */
BYTE NETBIOS_AddGroupName(LPNCB, BOOL, LPBYTE, BYTE, BYTE);
BYTE NETBIOS_DeleteName(LPNCB, BOOL, LPBYTE, BYTE, BYTE);

BYTE NETBIOS_Call(LPNCB, BOOL, LPBYTE, LPBYTE, BYTE, BYTE, BYTE, BYTE); /* Session Support Commands */
BYTE NETBIOS_Listen(LPNCB, BOOL, LPBYTE, LPBYTE, BYTE, BYTE, BYTE, BYTE);
BYTE NETBIOS_Hangup(LPNCB, BOOL, BYTE, BYTE, BYTE);
BYTE NETBIOS_Send(LPNCB, BOOL, BYTE, LPBYTE, WORD, BYTE, BYTE);
BYTE NETBIOS_ChainSend(LPNCB, BOOL, LPBYTE, WORD, BYTE, BYTE);
BYTE NETBIOS_Receive(LPNCB, BOOL, BYTE, WORD, BYTE, BYTE);
BYTE NETBIOS_ReceiveAny(LPNCB, BOOL, BYTE, WORD, BYTE, BYTE);
BYTE NETBIOS_SessionStatus(LPNCB, BOOL, LPBYTE, WORD, BYTE, BYTE);
BYTE NETBIOS_SendDG(LPNCB, BOOL, LPBYTE, LPBYTE, WORD, BYTE, BYTE, BYTE);  /* Datagram Support Commands */
BYTE NETBIOS_SendBDG(LPNCB, BOOL, LPBYTE, WORD, BYTE, BYTE, BYTE);
BYTE NETBIOS_ReceiveDG(LPNCB, BOOL, BYTE, WORD, BYTE, BYTE);
BYTE NETBIOS_ReceiveBDG(LPNCB, BOOL, BYTE, WORD, BYTE, BYTE);

/*** UB Extensions ***/
BYTE NETBIOS_UB_CallNIU(LPNCB, LPBYTE, BYTE, BYTE, BYTE, BYTE);
BYTE NETBIOS_UB_ListenNIU(LPNCB, LPBYTE, BYTE, BYTE, BYTE, BYTE);
BYTE NETBIOS_UB_SendPacket(LPNCB, BYTE, LPBYTE, WORD, BYTE, BYTE);  /* mbbx 2.00.06 */
BYTE NETBIOS_UB_ReceivePacket(LPNCB, BYTE, WORD, BYTE, BYTE);  /* mbbx 2.00.06 */

/*** AT&T Extensions ***/
BYTE NETBIOS_ATT_CallISN(LPNCB, BOOL, LPBYTE, LPBYTE, BYTE, BYTE);
BYTE NETBIOS_ATT_Break(LPNCB, BOOL, BYTE, BYTE, BYTE);

BOOL NET_NetbiosExist(WORD, BOOL);
BYTE NET_CallNetbios(LPNCB, BYTE);
BOOL NET_AllocCommNCBS();
VOID NET_ClearNcb(LPNCB);
VOID NET_DoCommError(LPNCB, BOOL);
WORD NET_Hangup();
WORD NET_CommandWait(WORD);


VOID NET_ExecCmdWait(WORD);   /* tsknet.c prototypes */
VOID NET_ExecCmdWhen(WORD);
VOID NET_WaitEnd(VOID);
BOOL NEAR NET_WhenNetwork(WORD);
INT  NET_AllocTaskNCB();
VOID NET_FreeNCB(WORD);
BOOL NEAR NET_CallNetbiosMulti(WORD, WORD, LPBYTE, LONG, LONG);
LPSTR memJugler(WORD, HANDLE*, DWORD);
VOID freeNetworkData();

VOID NET_TaskProcess(VOID);   /* netproc.c prototypes */
VOID NEAR NET_CommandComplete(WORD, BYTE);
VOID NEAR NET_PushWhenStack(INT, BYTE);
VOID NET_PopWhenStack(WORD);



/*****************************************************************************/
/* COMBIOS: definitions for combios interface                                */
/*****************************************************************************/

/*****************************************************************************/
/* Constant Definitions                                                      */
/*****************************************************************************/

#define INT_COMBIOS              0x14

#define COM_CALL_INIT            0x00
#define COM_CALL_WRITE           0x01
#define COM_CALL_READ            0x02
#define COM_CALL_STATUS          0x03

#define COM_CALL_BREAK           0x04  /* EICONS & ETHERTERM */
#define COM_CALL_WRITESTRING     0x06  /* EICONS */
#define COM_CALL_READSTRING      0x07  /* EICONS */

#define COM_PORT_COM1            0x00
#define COM_PORT_COM2            0x01
#define COM_PORT_COM3            0x02
#define COM_PORT_COM4            0x03

/* com init parameters */
#define COM_BAUD_110             0x00
#define COM_BAUD_300             0x40
#define COM_BAUD_600             0x60        /* mbbx 2.00.04: allow any baud */
#define COM_BAUD_1200            0x80
#define COM_BAUD_2400            0xA0
#define COM_BAUD_4800            0xC0
#define COM_BAUD_9600            0xE0

#define COM_PARITY_NONE          0x00
#define COM_PARITY_ODD           0x08
#define COM_PARITY_EVEN          0x18

#define COM_STOP_1               0x00
#define COM_STOP_2               0x04

#define COM_DATA_7               0x02
#define COM_DATA_8               0x03

/* port status bits */
#define COM_PORTSTAT_CMDTMO      0x8000
#define COM_PORTSTAT_TSREGMT     0x4000
#define COM_PORTSTAT_THREGMT     0x2000
#define COM_PORTSTAT_BREAK       0x1000
#define COM_PORTSTAT_FRAME       0x0800
#define COM_PORTSTAT_PARITY      0x0400
#define COM_PORTSTAT_OVERRUN     0x0200
#define COM_PORTSTAT_READY       0x0100

/* modem status bits */
#define COM_MDMSTAT_RLSD         0x0080
#define COM_MDMSTAT_RI           0x0040
#define COM_MDMSTAT_DSR          0x0020
#define COM_MDMSTAT_CTS          0x0010
#define COM_MDMSTAT_DLTARLSD     0x0008
#define COM_MDMSTAT_TERI         0x0004
#define COM_MDMSTAT_DLTADSR      0x0002
#define COM_MDMSTAT_DLTACTS      0x0001


/*****************************************************************************/
/* Forward Procedure Definitions                                             */
/*****************************************************************************/

/* VOID COM_exitSerial(VOID); */
VOID COM_resetSerial(recTrmParams *, BOOL);  /* mbbx 2.01.141 */

BOOL COM_mdmConnect();
VOID COM_modemReset();
VOID COM_modemSendBreak(INT);
INT NEAR COM_ReadComm(LPSTR, INT);
VOID COM_modemBytes();
INT NEAR COM_WriteComm(LPSTR, INT);
BOOL COM_modemWrite(LPSTR, INT);
WORD NEAR COM_CallBios(BYTE, BYTE, BYTE, LPBYTE, WORD);


/*****************************************************************************/
/* UBNETCI: definitions for Ungermann-Bass Command interpreter interface     */
/*****************************************************************************/
/* tge gold 007: split UB definitions into netubci.h */
#include "netubci.h"

/*****************************************************************************/
/* DEVICE: definitions for DOS device interface                              */
/*****************************************************************************/

/*****************************************************************************/
/* Constant Definitions                                                      */
/*****************************************************************************/
#define INT_DOS            0x21

/* int 21h calls */
#define DOS_CALL_OPEN      0x3D
#define DOS_CALL_CLOSE     0x3E
#define DOS_CALL_READ      0x3F
#define DOS_CALL_WRITE     0x40
#define DOS_CALL_IOCTL     0x44

#define DOS_IOCTL_CMD_GETINFO      0x00
#define DOS_IOCTL_CMD_SETINFO      0x01
#define DOS_IOCTL_CMD_READ         0x02
#define DOS_IOCTL_CMD_WRITE        0x03
#define DOS_IOCTL_CMD_READDRIVE    0x04
#define DOS_IOCTL_CMD_WRITEDRIVE   0x05
#define DOS_IOCTL_CMD_GETINSTAT    0x06
#define DOS_IOCTL_CMD_GETOUTSTAT   0x07
#define DOS_IOCTL_CMD_REMOVABLE    0x08
#define DOS_IOCTL_CMD_DEV_LCL_RMT  0x09
#define DOS_IOCTL_CMD_HNDL_LCL_RMT 0x0A
#define DOS_IOCTL_CMD_SHARECOUNT   0x0B

#define DOS_OPEN_ERR_NOSHAR   0x01
#define DOS_OPEN_ERR_NOFILE   0x02
#define DOS_OPEN_ERR_NOPATH   0x03
#define DOS_OPEN_ERR_NOHNDL   0x04
#define DOS_OPEN_ERR_NOACES   0x05
#define DOS_OPEN_ERR_NOCODE   0x0C

#define DOS_CLOSE_ERR_NOHNDL  0x06

#define DOS_READ_ERR_NOACES   0x05
#define DOS_READ_ERR_NOTOPN   0x06

#define DOS_WRITE_ERR_NOACES  0x05
#define DOS_WRITE_ERR_NOTOPN  0x06

#define DOS_IOCTL_ERR_NOSHAR  0x01
#define DOS_IOCTL_ERR_NOHNDL  0x04
#define DOS_IOCTL_ERR_NOACES  0x05
#define DOS_IOCTL_ERR_NOTOPN  0x06
#define DOS_IOCTL_ERR_BADDAT  0x0D
#define DOS_IOCTL_ERR_BADDRV  0x0F


/*****************************************************************************/
/* Forward Procedure Definitions                                             */
/*****************************************************************************/

VOID DEV_exitSerial();
VOID DEV_resetSerial(recTrmParams *, BOOL);  /* mbbx 2.01.141 */

VOID DEV_modemBytes();
BOOL DEV_modemWrite(LPSTR, INT);
