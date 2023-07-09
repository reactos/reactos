/*	File: cloop.hh (created 12/27/93, JKH)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:39p $
 */

// These constants are used to turn on various types of debug display
//#define DBG_NORMAL
//#define DBG_YIELD
//#define DBG_OUT

#if defined(DBG_NORMAL)
	#define DEBUGSTR
	#define DBGOUT_NORMAL(s,a1,a2,a3,a4,a5) DbgOutStr(s,a1,a2,a3,a4,a5)
#else
	#define DBGOUT_NORMAL(s,a1,a2,a3,a4,a5)
#endif

#if defined(DBG_YIELD)
	#define DEBUGSTR
	#define DBGOUT_YIELD(s,a1,a2,a3,a4,a5) DbgOutStr(s,a1,a2,a3,a4,a5)
#else
	#define DBGOUT_YIELD(s,a1,a2,a3,a4,a5)
#endif

/* --- Constants --- */

#define STD_BLOCKSIZE 512
#define CLOOP_TRACKING_DELAY 1000L

#define SF_CLOOP_SENDCRLF		0x200
#define SF_CLOOP_EXPAND 		0x201
#define SF_CLOOP_LOCALECHO		0x202
#define SF_CLOOP_LINEWAIT		0x203
#define SF_CLOOP_WAITCHAR		0x204
#define SF_CLOOP_EXTABSOUT		0x205
#define SF_CLOOP_TABSIZEOUT 	0x206
#define SF_CLOOP_LINEDELAY		0x207
#define SF_CLOOP_CHARDELAY		0x208
#define SF_CLOOP_ADDLF			0x209
#define SF_CLOOP_ASCII7 		0x20A
#define SF_CLOOP_ECHOPLEX		0x20B

//Removed.	Moved to emulator settings.  0x20C now available.
//#define SF_CLOOP_WRAPLINES	  0x20C

#define SF_CLOOP_SHOWHEX		0x20D
#define SF_CLOOP_TABSIZEIN		0x20E
#define SF_CLOOP_DBCS		    0x20F

// Values for the usOptions field in ST_CLOOP_OUT
// The first three are defined in cloop.h because they can be
//	used in the options arguments of CLoopSend()
// #define CLOOP_KEYS			0x0001
// #define CLOOP_ALLOCATED		0x0002
// #define CLOOP_SHARED 		0x0004

#define CLOOP_CHARACTERS	 0x0008
#define CLOOP_TEXTFILE		 0x0010
#define CLOOP_OPENFILE		 0x0020
#define CLOOP_TEXTDSP		 0x0040
#define CLOOP_STDSIZE		 0x8000


// offsets into event array for blocking
#define EVENT_CONTROL	0
#define EVENT_RCV		1
#define EVENT_SEND		2

/* --- Typedefs --- */

// Support for queued output routines
typedef struct s_cloop_out ST_CLOOP_OUT;
typedef struct s_func_chain ST_FCHAIN;
typedef struct s_cloop_settings ST_CLOOP_SETTINGS;
typedef struct s_cloop ST_CLOOP;


/* --- Structure definitions --- */

struct s_cloop_out
	{
	ST_CLOOP_OUT *pstNext;
	unsigned	  uOptions; 	  // keys/chars? allocated? shared?
	HGLOBAL 	  hdlShared;	   // shared mem handle
	TCHAR		 *puchData; 	   // address of data block
	TCHAR		 *puchLimit;	   // first addr. past data block
	TCHAR		 *puchHead; 	   // where data is added to block
	TCHAR		 *puchTail; 	   // where data is removed from block
	unsigned long ulBytes;		   // size of file being sent
	TCHAR		  chLastChar;	   // last byte out
	};

struct s_func_chain
	{
	ST_FCHAIN *pstNext; 	// pointer to next in chain
	ST_FCHAIN *pstPrior;	// pointer to prior in chain
	ST_CLOOP  *pstParent;	// handle of owning CLoop
	CHAINFUNC  pfFunc;		// function to call
	void	  *pvUserData;	// data supplied by caller when registered
	};


// Structure to hold user-settable values
struct s_cloop_settings
	{
	//	for sending:
	int 	  fSendCRLF;				// TRUE to add LF to CR when sent
	int 	  fExpandBlankLines;		// TRUE to expand blank lines with space
	int 	  fLocalEcho;				// TRUE to echo typed chars.
	int 	  fLineWait;				// TRUE to wait for mcWaitChar
	TCHAR	  chWaitChar;				// Char to wait for after sending line
	int 	  fExpandTabsOut;
	int 	  nTabSizeOut;				// Default outgoing tab alignment
	int 	  nLineDelay;				// Delay in msecs. after each line
	int 	  nCharDelay;				// Delay in msecs. after each char.

	//	for receiving
	int 	  fAddLF;					// TRUE to append LF to rcvd. CR
	int 	  fASCII7;					// TRUE to strip high bit on input
	int 	  fEchoplex;				// TRUE to echo rmt. chars.
	int 	  fShowHex; 				// TRUE to show hex val. for ctrl chars.
	int 	  nTabSizeIn;				// Default incoming tab alignment
	};


// CLOOP Handle definition structure
struct s_cloop
	{
	HSESSION	  hSession; 			// Parent session
	HEMU		  hEmu; 				// Emulator handle for quick ref.
	HCOM		  hCom; 				// Com handle for quick ref.
	unsigned	  afControl;			// Bits that control CLoop actions
	unsigned	  afRcvBlocked; 		// Bits to inhibit receiving
	unsigned	  afSndBlocked; 		// Bits to inhibit sending
	int 		  nRcvBlkCnt;			// Script receive block count
	int 		  fRcvBlkOverride;		// Override for receive block count
	int 		  fSuppressDsp; 		// TRUE to suppress display of data
	int 		  nPriorityCount;		// Keeps track of how often we yield

	// Receive delay control to implement cursor tracking
	int 		  fDataReceived;		// TRUE when new data received
	HTIMER		  htimerRcvDelay;		// Timer to catch delay in data
	HTIMER		  htimerCharDelay;		// Timer for char&line delays
	TIMERCALLBACK pfRcvDelay;			// Call back when rcv. delay timer fires
	TIMERCALLBACK pfCharDelay;			// Call back when char delay timer fires

	// send control
	ST_CLOOP_OUT *pstFirstOutBlock; 	// End of output chain to send from
	ST_CLOOP_OUT *pstLastOutBlock;		// End of output chain to add to
	unsigned long ulOutCount;			// # of chars or keys awaiting output
	HANDLE		  hOutFile; 			// handle of file being sent
	KEY_T		  keyLastKey;
	KEY_T		  keyHoldKey;

	// Send display window control
	int 		  fTextDisplay;
	HWND		  hwndDsp;
	HGLOBAL 	  hDisplayBlock;		// Memory from GlobalAlloc for DDE link
	unsigned long ulTotalSend;
	unsigned long ulSentSoFar;
	HTIMER		  hTimerTextDsp;
	long		  lTimeStarted;
	TIMERCALLBACK pfTimerProc;

	// function chaining
	ST_FCHAIN	 *pstRmtChain;			// Chain of functions to call with
	ST_FCHAIN	 *pstRmtChainNext;		//	with each input char. from rmt.
	int 		  fRmtChain;			// Quick test if anything is in chain

	// User settings
	ST_CLOOP_SETTINGS stOrigSettings;	// Settings as stored in file
	ST_CLOOP_SETTINGS stWorkSettings;	// Settings in current use

	// for learning
	LPVOID	  lpLearn;					// A generic handle

	// Translation table pointers
	TCHAR		 *panFltrIn;			// Translation table - incoming data
	TCHAR		 *panFltrOut;			// Translation table - outgoing data
	TCHAR		 *panFltrInHold;		// Holding place for the pointer
	TCHAR		 *panFltrOutHold;		// Holding place for the pointer

	// Engine thread handle
	HANDLE		  hEngineThread;

	// Synchronization objects
	CRITICAL_SECTION csect;
	HANDLE		  ahEvents[3];			// 0=loop control,
										// 1=receive control
										// 2=send control
	// added for MBCS input support
	int			 fDoMBCS;				// Enables MBCS processing
	TCHAR		 cLeadByte;				// If this is non-zero, then the last
										// character was a lead byte and we are
										// building a multi-byte character
	TCHAR		 cLocalEchoLeadByte;	// Ditto
	};




/* --- Internal function prototypes --- */

DWORD WINAPI CLoop(LPVOID pvData);
void CLoopCharIn(ST_CLOOP *pstCLoop, ECHAR chIn);
void CALLBACK CLoopRcvDelayProc(void *pvData, long lSince);
void CALLBACK CLoopCharDelayProc(void *pvData, long lSince);
int 		  CLoopGetNextOutput(ST_CLOOP * const pstCLoop, KEY_T * const pkKey);
ST_CLOOP_OUT *CLoopNewOutBlock(ST_CLOOP *pstCLoop, const size_t sizetData);
void		  CLoopRemoveFirstOutBlock(ST_CLOOP * const pstCLoop);
