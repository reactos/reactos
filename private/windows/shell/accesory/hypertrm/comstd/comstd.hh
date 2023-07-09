/* comstd.hh -- Private header file for stdcom communications driver module
 *
 *  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *  $Revision: 2 $
 *  $Date: 2/05/99 3:19p $
 */

// -=-=-=-=-=-=-=-=-=-=-=-=-=- DEBUG CONTROL -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// These constants are used to turn on various types of debug display
//#define DEBUG_THREAD 	  // Thread startup and shutdown
//#define DEBUG_READ		  // Read operations, main & thread
//#define DEBUG_WRITE		  // Write operations, main & thread
//#define DEBUG_EVENTS       // Com Events handled in thread
//#define DEBUG_AD           // Auto Detect

#if defined(DEBUG_THREAD)
    #define DEBUGSTR
    #define DBG_THREAD(s,a1,a2,a3,a4,a5) DbgOutStr(s,a1,a2,a3,a4,a5)
#else
    #define DBG_THREAD(s,a1,a2,a3,a4,a5)
#endif

#if defined(DEBUG_READ)
    #define DEBUGSTR
    #define DBG_READ(s,a1,a2,a3,a4,a5) DbgOutStr(s,a1,a2,a3,a4,a5)
#else
    #define DBG_READ(s,a1,a2,a3,a4,a5)
#endif

#if defined(DEBUG_WRITE)
    #define DEBUGSTR
    #define DBG_WRITE(s,a1,a2,a3,a4,a5) DbgOutStr(s,a1,a2,a3,a4,a5)
#else
    #define DBG_WRITE(s,a1,a2,a3,a4,a5)
#endif

#if defined(DEBUG_EVENTS)
    #define DEBUGSTR
    #define DBG_EVENTS(s,a1,a2,a3,a4,a5) DbgOutStr(s,a1,a2,a3,a4,a5)
#else
    #define DBG_EVENTS(s,a1,a2,a3,a4,a5)
#endif

#if defined(DEBUG_AD)
    #define DEBUGSTR
    #define DBG_AD(s,a1,a2,a3,a4,a5) DbgOutStr(s,a1,a2,a3,a4,a5)
#else
    #define DBG_AD(s,a1,a2,a3,a4,a5)
#endif


// -=-=-=-=-=-=-=-=-=-=-=-=-=- DEFINITIONS -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#define SIZE_INQ     1152
#define SIZE_OUTQ    2048
#define MAX_READSIZE 80L

// Set these values to make auto detection work
#define MIN_AD_TOTAL 15
#define MIN_AD_MIX   6

// Values for nBestGuess
#define AD_DONT_KNOW 0
#define AD_8N1       1
#define AD_7O1       2
#define AD_7E1       3

// Flag values in afHandshake
#define HANDSHAKE_RCV_X     0x0001
#define HANDSHAKE_RCV_DTR   0x0002
#define HANDSHAKE_RCV_RTS   0x0004
#define HANDSHAKE_SND_X     0x0008
#define HANDSHAKE_SND_CTS   0x0010
#define HANDSHAKE_SND_DSR   0x0020
#define HANDSHAKE_SND_DCD   0x0040

#define MDMSTAT_CTS 0x10
#define MDMSTAT_DSR 0x20
#define MDMSTAT_DCD 0x80

#define STDCOM struct s_stdcom *

// System value item IDs

#define SFID_COMSTD_BAUD          0x1011
#define SFID_COMSTD_DATABITS      0x1012
#define SFID_COMSTD_STOPBITS      0x1013
#define SFID_COMSTD_PARITY        0x1014
#define SFID_COMSTD_HANDSHAKING   0x1015
#define SFID_COMSTD_XON           0x1016
#define SFID_COMSTD_XOFF          0x1017
#define SFID_COMSTD_BREAK         0x1018
#define SFID_COMSTD_AUTODETECT    0x1019

// Identifiers for events in ahEvents
#define EVENT_COMEVENT  0
#define EVENT_READ      1
#define EVENT_WRITE     2
#define EVENT_COUNT     3

#if defined(INCL_WINSOCK)
// Size of receive circular buffer
#define WSOCK_SIZE_INQ       	1500

// Size of send circular bufffer. Don't make too big or status displays
//  during file transfers will act oddly.
#define WSOCK_SIZE_OUTQ      	3000

#define WSOCK_MAX_READSIZE 	1500L
#define IP_ADDR_LEN     128

// Note: This was once "HA/WinSock" however this would cause HyperTerminal
// to crash if HAWin16 was running and using WinSock. Since this was an
// obvious cut-and-paste job, I have changed this to be unique for
// HyperTerminal. - cab:12/06/96
//
#define	WINSOCK_EVENT_WINDOW_CLASS	"HyperTrm/WinSock"

#define	WM_WINSOCK_NOTIFY	(WM_USER+133)
#define	WM_WINSOCK_RESOLVE	(WM_USER+134)
#define WM_WINSOCK_STARTUP  (WM_USER+135)

#define MODE_MAX    5
#endif  // defined(INCL_WINSOCK)

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-= TYPES =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

typedef struct s_stdcom_settings
    {
    long     lBaud;
    int      nDataBits;
    int      nStopBits;
    int      nParity;
    unsigned afHandshake;
    TCHAR    chXON;
    TCHAR    chXOFF;
    int      nBreakDuration;
    int      fAutoDetect;
    } ST_STDCOM_SETTINGS;

#if defined(INCL_WINSOCK)
struct stOptionStateData
	{
	int option;
	int us;
	int usq;
	int him;
	int himq;
	};
typedef struct stOptionStateData STOPT;
typedef STOPT FAR * PSTOPT;
#endif  // defined(INCL_WINSOCK)

typedef struct s_stdcom
    {
    // Configuration details
    ST_STDCOM_SETTINGS stFileSettings;
    ST_STDCOM_SETTINGS stWorkSettings;

    // These are control variables private to the stdcom driver
    HCOM     hCom;                 // access to com routines
    HANDLE   hWinComm;             // handle to windows comm device
    HTIMER   hTmrBreak;            // Timer to control break duration
    int      fBreakSignalOn;       // TRUE while sending break signal
    int      fNotifyRcv;           // TRUE if driver should send notification
    DWORD    dwEventMask;          // Mask used in Set/GetCommEventmask
    int      fSending;             // TRUE if we've sent a buffer of data
    long     lSndTimer;            // started when we issue a WriteComm
    long     lSndLimit;            // amt. of time to wait for send
    long     lSndStuck;            // amt. of time we've been waiting for
                                   //   handshaking
    HWND     hwndEvents;           // window to process event messages
                                   //  generate messages for us
    BYTE     bLastMdmStat;
    OVERLAPPED stWriteOv;

    // Control fields for receiving
    int     nRBufrSize;            // Size of receive buffer
    BYTE   *pbBufrStart;           // Address of receive buffer
    BYTE   *pbBufrEnd;
    BYTE   *pbReadEnd;             // Where read thread left off
    BYTE   *pbComStart;            // Marks area being unloaded by main Com
    BYTE   *pbComEnd;
    int     fBufrFull;             // True when driver buffers are full
    int     fBufrEmpty;            // True when driver buffers are empty

    // Control fields for sending
    DWORD   dwBytesToSend;          // Number of bytes pending for write
    DWORD   dwSndOffset;            // Offset into send buffer for write
    BYTE   *pbSndBufr;              // Buffer used by pending write calls

    // Error counts
    int     nParityErrors;
    int     nFramingErrors;
    int     nOverrunErrors;
    int     nOverflowErrors;

    // Auto detection fields
    int  fADRunning;
    int  nADTotal;
    int  nADMix;
    int  nAD7o1;
    int  nADHighBits;
    int  nADBestGuess;
    char chADLastChar;
    int  fADToggleParity;
    int  fADReconfigure;

    HANDLE  hComstdThread;         // Thread to handle ongoing activity
    int     fHaltThread;           // To control when thread shuts down

    // Data access control
    CRITICAL_SECTION csect;        // To synchronize access to driver vars.
    HANDLE  ahEvent[EVENT_COUNT];  // To control scheduling

#if defined(INCL_WINSOCK)
    // Configuration details
	// These are control variables private to the tcpip comm driver
	SOCKET          hSocket;            // connected socket
	int             fConnected;         // -1 initially, 0 if init'd, 1 connected
	TCHAR           szRemoteAddr[IP_ADDR_LEN];// IP address
	short           nPort;              // Port number
	unsigned long   ulCompatibility;    // to pass flags into dialogs

	ULONG  ulAddr;

	// These are control variables private to the stdcom driver
	int		 fSndBreak;		   	   // emulate a break sequence

    // Control fields for receiving
	BYTE	*pbSendBufr;			// Pointer to data passed to SndBufrSend
	int	    nSendBufrLen;			// Amount of data to send
	BYTE	abSndBufr[WSOCK_SIZE_OUTQ*2];	// Buffer to send when Winsock is ready
	BYTE    *pbSndPtrEnd;			// Current offset in auchSndBufr

    HANDLE  hComReadThread;        	// Thread to handle reading from network
    HANDLE  hComWriteThread;       	// Thread to handle writing to network
    HANDLE  hComConnectThread;      // Thread to handle connecting to host
	int		fClearSendBufr;			// flag to signal send buffer to be cleared
	int		fEscapeFF;				// We are escaping FF characters (by doubling)
	int		fSeenFF;				// We have just encountered and FF character

	BYTE	pstHostBuf[MAXGETHOSTSTRUCT]; // used by WSAGetHostByName

	struct sockaddr_in  stHost;

	// Pointers to socket library functions (porting note: these are just pasted in from winsock.h
	// with the function pointer declaration fixed up)
	int (PASCAL * FAR accept)( int, struct sockaddr *, int * );
	int (PASCAL * FAR bind)( int, struct sockaddr *, int );
	int (PASCAL * FAR connect)( int, struct sockaddr *, int );
	int (PASCAL * FAR gethostid)(void);
	int (PASCAL * FAR listen)( int, int);
	int (PASCAL * FAR recv)( int, char *, int, int );
	int (PASCAL * FAR send)( int, char *, int, int );
	int (PASCAL * FAR sock_init)( void );
	int (PASCAL * FAR sock_errno)( void );
	int (PASCAL * FAR socket)( int, int, int );
	int (PASCAL * FAR soclose)( int );
	int (PASCAL * FAR shutdown)(int, int);
	int (PASCAL * FAR setsockopt)( int, int, int, char *, int );

	// from netdb.h
	struct hostent * (PASCAL * FAR gethostbyname)( char * );

	// from netinet/in.h
	unsigned long (PASCAL * FAR inet_addr)(char *);

	// from utils.h
	unsigned short (PASCAL * FAR bswap)(unsigned short);
	
	// Telnet emulation data
	int         NVTstate;       // Current state of Network Virtual Terminal
	STOPT       stMode[MODE_MAX];

#ifdef INCL_CALL_ANSWERING
	// Answer mode variables - cab:11/19/96
	//
	int	fAnswer;			// Are we waiting for a call?
#endif
	
#endif  // defined(INCL_WINSOCK)
    } ST_STDCOM;


// -=-=-=-=-=-=-=-=-=-=-=- Globals -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
extern   HINSTANCE hinstDLL;



// -=-=-=-=-=-=-=-=-=-=-=- EXPORTED PROTOTYPES -=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int    WINAPI DeviceInitialize(HCOM hCom, unsigned usInterfaceVersion,
                    void **ppvDriverData);
int    WINAPI DeviceClose(ST_STDCOM *pstPrivate);
int    WINAPI DeviceDialog(ST_STDCOM *pstPrivate, HWND hwndParent);
int    WINAPI DeviceGetCommon(ST_STDCOM *pstPrivate, ST_COMMON *pstCommon);
int    WINAPI DeviceSetCommon(ST_STDCOM *pstPrivate, ST_COMMON *pstCommon);
int    WINAPI DeviceSpecial(ST_STDCOM *pstPrivate,
                    const TCHAR *pszInstructions,
                    TCHAR *pszResult,
                    int   nBufrSize);
int    WINAPI DeviceLoadHdl(ST_STDCOM *pstPrivate, SF_HANDLE sfHdl);
int    WINAPI DeviceSaveHdl(ST_STDCOM *pstPrivate, SF_HANDLE sfHdl);
int    WINAPI PortConfigure(ST_STDCOM *pstPrivate);
int    WINAPI PortActivate(ST_STDCOM *pstPrivate,
                    TCHAR *pszPortName,
                    DWORD_PTR dwMediaHdl);
int    WINAPI PortDeactivate(ST_STDCOM *pstPrivate);
int    WINAPI PortConnected(ST_STDCOM *pstPrivate);
int           PortExtractSettings(ST_STDCOM *pstPrivate);

int    WINAPI RcvRefill(ST_STDCOM *pstPrivate);
int    WINAPI RcvClear(ST_STDCOM *pstPrivate);

int    WINAPI SndBufrSend(ST_STDCOM *pstPrivate, void *pvBufr,
                      int nSize);
int    WINAPI SndBufrIsBusy(ST_STDCOM *pstPrivate);
int    WINAPI SndBufrQuery(ST_STDCOM *pstPrivate, unsigned *pafStatus,
                      long *plHandshakeDelay);
int    WINAPI SndBufrClear(ST_STDCOM *pstPrivate);
DWORD  WINAPI ComstdThread(void *pvData);
void          AutoDetectAnalyze(ST_STDCOM *pstPrivate, int nBytes, char *pchBufr);
void          AutoDetectOutput(ST_STDCOM *pstPrivate, void *pvBufr, int nSize);
void          AutoDetectStart(ST_STDCOM *pstPrivate);
void          AutoDetectStop(ST_STDCOM *pstPrivate);

// Temporary til TAPI lets us change media handle settings correctly
int           ComstdGetAutoDetectResults(void *pvData, BYTE *bByteSize,
                BYTE *bParity, BYTE *bStopBits);
