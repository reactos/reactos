/* connect.h */

/*************************************************************************
 *  If defined, the following flags inhibit definition
 *     of the indicated items.
 *
 *  JUST COPY AND PASTE THE DEFINES THAT YOU NEED
 *************************************************************************
#define NOORDINALS       TRUE
#define NOPROTOTYPES     TRUE
#define NODLLPROTOTYPES  TRUE
#define NODCPROTOTYPES   TRUE
#define NOCONSTANTS      TRUE
#define NOCONNECTORS     TRUE
#define NOGLOBALS        TRUE
#define NO  TRUE

 *************************************************************************
 */

#ifndef MINCONSTR             /* seh nova 005 */
#define MINCONSTR       64    /* seh nova 005 */
#endif
#ifndef MINRESSTR
#define MINRESSTR       32    /* seh nova 005 this must match dynacomm.h */
#endif

#ifndef NOORDINALS
/* define ordinal values for all exported functions */
#define     ORD_GETDLLTYPE          959
#define     ORD_GETCONNECTCAPS      962
#define     ORD_SETPARAMETERS       800
#define     ORD_GETEXTENDEDINFO     801
#define     ORD_RESETCONNECTOR      802
#define     ORD_EXITCONNECTOR       803
#define     ORD_CONNECTCONNECTOR    804
#define     ORD_READCONNECTOR       805
#define     ORD_WRITECONNECTOR      806
#define     ORD_COMMANDCONNECTOR    807
#define     ORD_DISCONNECTCONNECTOR 808
#define     ORD_SETUPCONNECTOR      809

#endif /* NOORDINALS */



#ifndef NOCONSTANTS
/* constants defined here (listed by function that uses them */

/* GetDLLType */
#define GDT_SHOW_EGO       TRUE
#define GDT_QUIET          FALSE
#define DC_CONNECTOR       4257


/* GetConnectCaps, SetParameters */
#define SET_PARAMETERS     0x0001

#define SP_GETCAPS         0x0001
#define SP_QUIET           0x0000
#define SP_SHOW            0x0002
#define SP_GETDEFAULT      0x0004
#define SP_SETDEFAULT      0x0008

#ifdef ORGCODE
#define SP_PARITY          0x0001
#define SP_BAUD            0x0002
#define SP_DATABITS        0x0004
#define SP_STOPBITS        0x0008
#define SP_HANDSHAKING     0x0010
#define SP_PARITY_CHECK    0x0020
#define SP_CARRIER_DETECT  0x0040
#endif

/* available baud rates */
#define GP_AVAIL_BAUD      0x0002
/* values can be.... */
#ifdef ORGCODE
#define BAUD_075           0x0001
#define BAUD_110           0x0002
#define BAUD_300           0x0004
#define BAUD_600           0x0008

#define BAUD_120           0x0010
#define BAUD_240           0x0020
#define BAUD_480           0x0040
#define BAUD_960           0x0080
#define BAUD_192           0x0100
#else
#define BAUD_120           BAUD_1200
#define BAUD_240           BAUD_2400
#define BAUD_480           BAUD_4800
#define BAUD_960           BAUD_9600
#define BAUD_192           BAUD_19200

#endif

#ifdef ORGCODE
#define BAUD_USER          0x0200
#endif

#define BAUD_ALL (BAUD_075+BAUD_110+BAUD_300+BAUD_600+BAUD_120+BAUD_240+BAUD_480+BAUD_960+BAUD_192+BAUD_USER)
#define BAUD_OFFSET        0x1000      /* tge used for resources */

/* available data bits */
#define GP_AVAIL_DATABITS  0x0004
/* values can be.... */
#ifdef ORGCODE
#define DATABITS_5         0x0001
#define DATABITS_6         0x0002
#define DATABITS_7         0x0004
#define DATABITS_8         0x0008
#endif

#define DATABITS_ALL       (DATABITS_5+DATABITS_6+DATABITS_7+DATABITS_8)
#define DATABITS_OFFSET    0x2000      /* tge used for resources */

/* available stops bits */
#define GP_AVAIL_STOPBITS  0x0008
/* values can be.... */
#ifdef ORGCODE
#define STOPBITS_10        0x0001
#define STOPBITS_15        0x0002
#define STOPBITS_20        0x0004
#endif

#define STOPBITS_ALL       (STOPBITS_10+STOPBITS_15+STOPBITS_20)
#define STOPBITS_OFFSET    0x3000      /* tge used for resources */

/* available parity options */
#define GP_AVAIL_PARITY    0x0010
/* values can be ... */
#ifdef ORGCODE
#define PARITY_NONE        0x0001
#define PARITY_ODD         0x0002
#define PARITY_EVEN        0x0004
#define PARITY_MARK        0x0008
#define PARITY_SPACE       0x0010
#endif

#define PARITY_ALL         (PARITY_NONE+PARITY_ODD+PARITY_EVEN+PARITY_MARK+PARITY_SPACE)
#define PARITY_OFFSET      0x4000      /* tge used for resources */

/* available handshaking */
#define GP_AVAIL_HANDSHAKE 0x0020
/* values can  be...*/
#define HANDSHAKE_XONXOFF  0x0001
#define HANDSHAKE_HARDWARE 0x0002
#define HANDSHAKE_NONE     0x0004
#define HANDSHAKE_ETXFLOW  0x0008

#define HANDSHAKE_ALL      (HANDSHAKE_XONXOFF+HANDSHAKE_HARDWARE+HANDSHAKE_NONE+HANDSHAKE_ETXFLOW)
#define HANDSHAKE_OFFSET   0x5000      /* tge used for resources */

/* misc. options */
#define GP_AVAIL_MISC      0x0040
/* values can be... */
#define MISC_CARRIER_DETECT   0x0001
#define MISC_PARITY_CHECK     0x0002

#define MISC_ALL           (MISC_CARRIER_DETECT+MISC_PARITY_CHECK)
#define MISC_NONE          0x0000
#define MISC_OFFSET        0x6000      /* tge used for resources */


/* GetExtendedInfo */
#define GI_STRSIZE         80
/* if GetExtendedInfo does not have extended info for what we want,
   it gives us this message */
#define GIN_NOINFO                 0xffff
/* else, the following stuff is used */

/* we send GetExtendedInfo this in parameter one and error code in param 2 */
#define GI_ERROR                   0x0000
/* and GetExtended info returns this ... */
#define GIN_ERRWARNING             0x0000
#define GIN_ERRDLLCRASH            0x0001
#define GIN_ERRAPPCRASH            0x0002
#define GIN_ERRSYSCRASH            0x0003
#define GIN_ERRFATAL               0x0004

/* we send GetExtendedInfo this is param 1 */
#define GI_IDENTIFY                0x0001
/* and one of these in param 2 */
#define GI_DLLFILENAME             0x0000    /* put dll file name is param 3 */
#define GI_DLLNAME                 0x0001    /* put dll name is param 3 (used in listbox) */
#define GI_CLIENTNAME              0x0002    /* put client name is param 3 */
#define GI_SERVERNAME              0x0003    /* put server name is param 3 */
#define GI_DLLVERSION              0x0004    /* put version # in param 3 */
#define GI_DLLINIFILENAME          0x0005    /* put ini filename in  param 3 */
#define GI_SETUPBOX                0x0006    /* return TRUE if setup button should be enabled */
/* and GetExtended info returns this ... */
#define GI_OK                      TRUE

/* connector read and write errors */
#define CONNECT_READ_ERROR        0xffff
#define CONNECT_WRITE_ERROR       (CONNECT_READ_ERROR)

/* connector ini list error */
#define CONNECT_NO_CONNECTORS     (-2)


/* CommandConnector */
#define DLL_CMD_BREAK                0x0001

/* ConnectConnector */
#define STAT_CONNECTED                 1        /* slc nova xxx */
#define STAT_NOT_CONNECTED             2        /* slc nova xxx */
#define STAT_WAIT_CONNECT              3        /* bjw nova 002 */
#define STAT_ERROR                     4        /* bjw nova 002 */

/* control block constants */
#define TYPE_MODEM                     0x0001
#define TYPE_NETWORK                   0x0002
#define TYPE_PHYSICAL                  0x0003

#endif /* NOCONSTANTS */


#ifndef NOCONNECTORS
/* User Union (not to be mistaken with the AFL-CIO */

#define  CCB_UNION_SIZE    512               /* seh/slc nova */

typedef union
{
   BYTE     Byte[CCB_UNION_SIZE];
   WORD     Word[CCB_UNION_SIZE / sizeof(WORD)];
   LONG     Long[CCB_UNION_SIZE / sizeof(LONG)];
   PSTR     Pstr[CCB_UNION_SIZE / sizeof(PSTR)];
   LPSTR    Lpstr[CCB_UNION_SIZE / sizeof(LPSTR)];
   HANDLE   Handle[CCB_UNION_SIZE / sizeof(HANDLE)];
}  USER_UNION;

/* Connector Control Block */
typedef struct
{
   WORD     wVersion;            /* version number (always equals 100) */
   HANDLE   hConnectorInst;      /* instance handle of connector DLL   */
   WORD     wType;               /* Type of connection (network, physical, ect...) */
   WORD     wStatus;             /* Status (connected, not connected, ect... */
   WPARAM wParamFlags;         /* flags returned by GetConnectCaps(SET_PARAMETERS) */
   WORD     wBaudFlags;          /* flags returned by GetConnectCaps(GP_AVAIL_BAUD) */
   WORD     wDataBitFlags;       /* flags returned by GetConnectCaps(GP_AVAIL_DATABITS) */
   WORD     wStopBitFlags;       /* flags returned by GetConnectCaps(GP_AVAIL_STOPBITS) */
   WORD     wParityFlags;        /* flags returned by GetConnectCaps(GP_AVAIL_PARITY) */
   WORD     wHandshakeFlags;     /* flags returned by GetConnectCaps(GP_AVAIL_HANDSHAKE) */
   WORD     wMiscFlags;          /* flags returned by GetConnectCaps(GP_AVAIL_MISC) */

   WORD     wSpeed;              /* currently set baud rate (actual value) */ /* seh nova 005 */
   WORD     wBaudSet;            /* currently set baud rate (control id) */
   WORD     wDataBitSet;         /* currently set data bits (control id) */
   WORD     wStopBitSet;         /* currently set stop bits (control id) */
   WORD     wParitySet;          /* currently set parity (control id) */
   WORD     wHandshakeSet;       /* currently set handshaking (control id) */
   WORD     wMiscSet;            /* currently set misc (bit flags) */

   BYTE     szPhoneNumber[MINRESSTR];  /* Phone Number to Dial */
   BYTE     szDLLFileName[MINRESSTR];  /* DOS filename for DLL */
   BYTE     szDLLName[MINRESSTR];      /* Name used in connectors listbox */

   BYTE     szClient[16];        /* client name (for network DLL use) */
   BYTE     szServer[64];        /* server name (for network DLL use) */
   WORD     wNetBiosLNum;        /* slc nova NetBIOS Local Session Number  */
   WORD     wNetBiosRNum;        /* slc nova NetBIOS Remote Session Number */
   WORD     byPadChar;           /* char used for blank padding         */

   WORD     wReadBufferSize;     /* size of read transfer buffer        */
   WORD     wReadBufferRead;     /* actual bytes read                   */ /* bjw nova 002 */
   LPSTR    lpReadBuffer;        /* address of read transfer buffer     */
   HANDLE   hReadBuffer;         /* handle to read transfer buffer      */
   WORD     wWriteBufferSize;    /* size of write transfer buffer       */
   WORD     wWriteBufferUsed;    /* size of write transfer buffer used  */ /* seh nova 005 */
   LPSTR    lpWriteBuffer;       /* address of write transfer buffer    */
   HANDLE   hWriteBuffer;        /* handle to write transfer buffer     */
   LPSTR    lpNCB;               /* address of NetBIOS Control Block    */
   HANDLE   hNCB;                /* handle to NetBIOS Control Block     */
   LPSTR    lpNCBWrite;          /* address of NetBIOS Control Block    */
   HANDLE   hNCBWrite;           /* handle to NetBIOS Control Block     */
   LPSTR    lpNCBRead;           /* address of NetBIOS Control Block    */
   HANDLE   hNCBRead;            /* handle to NetBIOS Control Block     */
   WORD     wTimeRemain;         /* NetBIOS Listen Timeout              */
   BYTE     configBuffer[32];    /* Saved in DCS file (for DLL use)     */

   USER_UNION  User;             /* DLL's are free to use this space
                                    in anyway that they would like to */
}  CONNECTOR_CONTROL_BLOCK, *PCONNECTOR_CONTROL_BLOCK, FAR *LPCONNECTOR_CONTROL_BLOCK;

/* connector array structure */
typedef  struct
{
   WORD   wNumOfChannels;              /* number of opened channels */
   WORD   wTopChannel;                 /* if a session window is the top window, */
                                       /* then this is it's index into CCB array */
   HANDLE hCCBArray;
   LPCONNECTOR_CONTROL_BLOCK  lpCCB[1]; /* array of CCB's (dynamically allocated) */
}  CONNECTORS, *PCONNECTORS, FAR *LPCONNECTORS;

#endif  /* NOCONNECTORS */


#ifndef NOGLOBALS

HANDLE         ghConnectors;        /* global handle to connector data struct */
HANDLE         ghCCB;               /* global handle CONNECTOR_CONTROL_BLOCK */
LPCONNECTORS   xglpConnectors;       /* not used anymore - long pointer to connector data structure */
HANDLE         ghWorkConnector;     /* handle for temporary connector work (settings) */
BYTE           gszWork[MINCONSTR];  /* slc nova xxx */

#endif  /* NOGLOBALS */


#ifndef NOPROTOTYPES
/* exported functions  prototypes */

#ifndef NODLLPROTOTYPES
/* Connect DLL's */
WORD  APIENTRY GetDLLType(HWND, BOOL);
WORD  APIENTRY GetConnectCaps(WORD);
WORD  APIENTRY SetParameters(WORD, LPCONNECTOR_CONTROL_BLOCK);
WORD  APIENTRY GetExtendedInfo(WORD, WORD, LPSTR);
WORD  APIENTRY ResetConnector(HWND, LPCONNECTOR_CONTROL_BLOCK, BOOL);
WORD  APIENTRY ExitConnector(HWND, LPCONNECTOR_CONTROL_BLOCK, BOOL);
WORD  APIENTRY ConnectConnector(HWND, LPCONNECTOR_CONTROL_BLOCK, BOOL);
WORD  APIENTRY ReadConnector(LPCONNECTOR_CONTROL_BLOCK);
WORD  APIENTRY WriteConnector(LPCONNECTOR_CONTROL_BLOCK);
WORD  APIENTRY CommandConnector(HWND, LPCONNECTOR_CONTROL_BLOCK, WORD, LONG);
WORD  APIENTRY DisconnectConnector(HANDLE, LPCONNECTOR_CONTROL_BLOCK);

#endif /* NODLLPROTOTYPES */

#ifndef NODCPROTOTYPES
/* DynaComm */
BOOL     initConnectors(BOOL);
VOID     addConnectorList(HWND, WORD);   /* slc nova 031 */
HANDLE   loadConnector(HWND, HANDLE, LPSTR, BOOL); /* slc nova 031 */
WORD     getConnectorCaps(LPCONNECTOR_CONTROL_BLOCK); /* slc nova 031 */
WORD     setConnectorSettings(HWND, HANDLE, BOOL);
WORD     getConnectorSettings(LPCONNECTOR_CONTROL_BLOCK, BOOL);   /* slc nova 031 */
WORD     DLL_ResetConnector(HANDLE, BOOL);   /* slc nova 031 */
WORD     DLL_SetupConnector(HANDLE, BOOL);   /* slc nova 031 */
WORD     DLL_ExitConnector(HANDLE, recTrmParams *);
WORD     DLL_ConnectConnector(HANDLE, BOOL);    /* slc nova 031 */
WORD     DLL_ReadConnector(HANDLE);          /* slc nova 031 */
WORD     DLL_WriteConnector(HANDLE);         /* slc nova 031 */
WORD     DLL_CommandConnector(HANDLE, LPCONNECTOR_CONTROL_BLOCK, WORD, LONG);
WORD     DLL_DisconnectConnector(HANDLE hCCB);  /* slc nova 031 seh nova 005 */
WORD     DLL_modemSendBreak(HANDLE, INT);
WORD     DLL_ConnectBytes(HANDLE);           /* slc nova 031 */
WORD     getConnectType(HANDLE hConnector, HANDLE hCCB);  /* seh nova 005 */
VOID     ccbFromTrmParams(LPCONNECTOR_CONTROL_BLOCK, recTrmParams *);
VOID     ccbToTrmParams(recTrmParams *, LPCONNECTOR_CONTROL_BLOCK);
BOOL     DLL_HasSetupBox(HANDLE hConnector); /* seh nova 006 */

WORD     putCCB_BAUDITM(WORD);               /* slc nova xxx */
WORD     putCCB_BAUD(WORD);
WORD     putCCB_DATABITS(WORD);
WORD     putCCB_PARITY(WORD);
WORD     putCCB_STOPBITS(WORD);
WORD     putCCB_FLOWCTRL(WORD);
WORD     putCCB_MISCSET(WORD, WORD);

#endif /* NODCPROTOTYPES */
#endif /* NOPROTOTYPES */

/* taken from dcrc.h bjw nova 002 */
#ifdef NEED_DCRC
/* bjw gold 027  - the evil warning! */
/*****************************************************************************
 * B I G - T I M E ,   W A R N I N G !,  W A R N I N G !,  W A R N I N G !
 *
 * WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
 *
 * If any of the folling ID's change (and you better have a good reason!)
 * make sure and change the corresponding ID's in any DLL that uses them and
 * recompile DynaComm and ALL connector DLL's.
 * If you really have to change one of these items, DynaComm will no longer
 * be compatible with earlier settings files or connector DLL's.
 * I WARNED YOU!
 *
 ******************************************************************************/

#define ITMSETUP                 3           /* seh nova 005 */

#define IDDBCOMM                 7

#define ITMBD110                 11
#define ITMBD300                 12
#define ITMBD600                 13          /* mbbx 2.00: support 600 baud */
#define ITMBD120                 14
#define ITMBD240                 15
#define ITMBD480                 16
#define ITMBD960                 17
#define ITMBD192                 18

#define ITMDATA4                 21          /* not used */
#define ITMDATA5                 22
#define ITMDATA6                 23
#define ITMDATA7                 24
#define ITMDATA8                 25

#define ITMSTOP1                 31
#define ITMSTOP5                 32
#define ITMSTOP2                 33

#define ITMNOPARITY              41
#define ITMODDPARITY             42
#define ITMEVENPARITY            43
#define ITMMARKPARITY            44
#define ITMSPACEPARITY           45

#define ITMXONFLOW               51
#define ITMHARDFLOW              52
#define ITMNOFLOW                53
#define ITMETXFLOW               54          /* jtfx 2.01.75 ... */

#define ITMCONNECTOR             61

#define ITMPARITY                91
#define ITMCARRIER               92

#endif /* NEED_DCRC */

/* WINCIM typedef's */

typedef CONNECTOR_CONTROL_BLOCK 	CCB;
typedef CCB FAR *			LPCCB;
