/*	File: D:\WACKER\cncttapi\cncttapi.hh (Created: 10-Feb-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:19p $
 */

typedef struct stCnctDrvPrivate *HHDRIVER;

/* --- Macros --- */

#define TAPI_VER (MAKELONG(4,1))

// Required in lineGetDevConfig calls
//
#define DEVCLASS		"comm/datamodem"
#define REDIAL_MAX      40

// Essentially, I picked what I hope is a permanent line ID a TAPI
// service provider would never use.  My odds are pretty good since
// UNIMODEM numbers them from 1. - mrw
//
#define DIRECT_COM1		    0x5A2175D1
#define DIRECT_COM2		    (DIRECT_COM1+1)
#define DIRECT_COM3		    (DIRECT_COM1+2)
#define DIRECT_COM4		    (DIRECT_COM1+3)
#define DIRECT_COMWINSOCK	(DIRECT_COM1+4)
#define DIRECT_COM_DEVICE   (DIRECT_COM1 - 1)

#if defined(INCL_WINSOCK)
#define MAX_IP_ADDR_LEN		128
#endif

// Trap is similar to assert except it displays the error return code.
//
#if defined(NDEBUG)
#define TRAP(x) x
#else
#define TRAP(x) tapiTrap(x, __FILE__, __LINE__)
#endif

// Connection driver handle

struct stCnctDrvPrivate
	{
	HCNCT	 hCnct; 		// public connection handle passed to create func
	HSESSION hSession;		// public session handle passed to create func

	CRITICAL_SECTION cs;	// critical section semaphore

	int 	 iStatus;		// connection status

	HLINEAPP hLineApp;		// returned for lineInitialize(), main TAPI handle
	HLINE	 hLine; 		// handle to line we're using from lineOpen()
	HCALL	 hCall; 		// handle returned by lineMakeCall()
	LONG	 lMakeCallId;	// ID returned by lineMakeCall() used in callback
	DWORD	 dwLineCnt; 	// number of available lines from lineInitialize
	DWORD	 dwLine;		// current line we're using
	DWORD	 dwAPIVersion;	// current api version
	DWORD	 dwCountryID;	// internal TAPI ID for selected country
	DWORD	 dwCountryCode; // set in TranslateAddress()
	DWORD	 dwPermanentLineId; // identifies the modem as saved

	BOOL	 fMatchedPermanentLineID,
			 fHotPhone; 	// TAPI for is it Direct Connect

	TCHAR	 achDest[(TAPIMAXDESTADDRESSSIZE/2)+1],	// local portion of phone num.
			 achAreaCode[10],
			 achDefaultAreaCode[10],// As reported from TAPI
			 achDialableDest[TAPIMAXDESTADDRESSSIZE+1],
			 achDisplayableDest[TAPIMAXDESTADDRESSSIZE+1],
			 achCanonicalDest[TAPIMAXDESTADDRESSSIZE+1];

	TCHAR	 achLineName[100];	// used in dialing dialog
    TCHAR    achComDeviceName[256]; // used with enumerated ports.

	HWND	 hwndCnctDlg;		// connection dialog handle
	HWND	 hwndStupid;		// stupid TAPI reinit
	HWND	 hwndPCMCIA;		// valid only when PCMCIA dialog showing.

	LINECALLPARAMS 	stCallPar;	// Call params used for lineMakeCall()
	
	BOOL	 fUseCCAC;			// Use country code & area code flag.
	unsigned int uDiscnctFlags;	// Used for asychronous disconnects.

    int      iRedialCnt;        // Counter for redials.
    int      fRedialOnBusy;     // TRUE, we redial
    int      iRedialSecsRemaining; // seconds remaining to redial

	int		 iPort;

	TCHAR	 achDestAddr[128];

#ifdef INCL_CALL_ANSWERING
	int  fAnswering;		// Are we answering?
	int  fRestoreSettings;	// Do we need to restore our ASCII settings?
	int	 nSendCRLF;			// Temporary storage for ASCII setting.
	int  nLocalEcho;		// Temporary storage for ASCII setting.
	int  nAddLF;			// Temporary storage for ASCII setting.
	int  nEchoplex;			// Temporary storage for ASCII setting.
	void *pvUnregister;		// Un-registration data for CLoop callback.
#endif
	};

/* --- Line id struct used to store info in combo boxes	--- */

struct _stLineIds
	{
	DWORD dwLineId;
	DWORD dwPermanentLineId;
	};

typedef	struct _stLineIds * PSTLINEIDS;

/* --- Function Prototypes --- */

void cnctdrvLock(const HHDRIVER hhDriver);
void cnctdrvUnlock(const HHDRIVER hhDriver);
INT_PTR CALLBACK ConfirmDlg(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar);
int CplConfigDlg(const HWND hwnd, const int ordinal);
int EnumerateCountryCodes(const HHDRIVER hhDriver, const HWND hwndCB);
int EnumerateAreaCodes(const HHDRIVER hhDriver, const HWND hwndCB);
int EnumerateLines(const HHDRIVER hhDriver, const HWND hwndCB);
int EnumerateLinesNT(const HHDRIVER hhDriver, const HWND hwndCB);
long TranslateAddress(const HHDRIVER hhDriver);
INT_PTR CALLBACK DialingDlg(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar);
void DialingMessage(const HHDRIVER hhDriver, const int resID);
void CALLBACK lineCallbackFunc(DWORD hDevice, DWORD dwMsg, DWORD_PTR dwCallback,
                               DWORD_PTR dwParm1, DWORD_PTR dwParm2, DWORD_PTR dwParm3);

void PostDisconnect(const HHDRIVER hhDriver, const unsigned int uFlags);
int Handoff(const HHDRIVER hhDriver);
void SetStatus(const HHDRIVER hhDriver, const int iStatus);
int DoLineGetCountry(const DWORD dwCountryID, const DWORD dwAPIVersion,
    LPLINECOUNTRYLIST *ppcl);

int EnumerateTapiLocations(const HHDRIVER hhDriver, const HWND hwndCB,
								  const HWND hwndTB);

int CheckHotPhone(const HHDRIVER hhDriver, const DWORD dwLine, int *pfHotPhone);
DWORD tapiTrap(const DWORD dw, const TCHAR *file, const int line);
void ResetComboBox(const HWND hwnd);
void EnableDialNow(const HWND hwndDlg, const int fEnable);
int fCountryUsesAreaCode(const DWORD dwCountryId, const DWORD dwAPIVersion);
INT_PTR CALLBACK PCMCIADlg(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar);
int fIsStringEmpty(LPTSTR ach);



#if defined(INCL_WINSOCK)
BOOL CALLBACK cnctwsNewPhoneDlg(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar);
#endif

/* --- Driver entry points --- */

HDRIVER WINAPI cnctdrvCreate(const HCNCT hCnct, const HSESSION hSession);
int WINAPI cnctdrvDestroy(const HHDRIVER hhDriver);
int WINAPI cnctdrvInit(const HHDRIVER hhDriver);
int WINAPI cnctdrvLoad(const HHDRIVER hhDriver);
int WINAPI cnctdrvSave(const HHDRIVER hhDriver);

int WINAPI cnctdrvSetDestination(const HHDRIVER hhDriver, TCHAR * const ach,
								 const size_t cb);

int WINAPI cnctdrvQueryStatus(const HHDRIVER hhDriver);
int WINAPI cnctdrvConnect(const HHDRIVER hhDriver, const unsigned int uFlags);
int WINAPI cnctdrvDisconnect(const HHDRIVER hhDriver, const unsigned int uFlags);
int WINAPI cnctdrvComEvent(const HHDRIVER hhDriver, const enum COM_EVENTS event);

int cnctdrvGetComSettingsString(const HHDRIVER hhDriver, LPTSTR pachStr,
								const size_t cb);


