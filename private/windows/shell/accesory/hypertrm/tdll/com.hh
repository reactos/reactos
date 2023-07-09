/* com.hh -- Internal definitions for communications routines
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */

// -=-=-=-=-=-=-=-=-=-=-=-=-=- DEBUG CONTROL -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// These constants are used to turn on various types of debug display
// #define DBG_NORMAL
// #define DBG_RCV
// #define DBG_SEND

#if defined(DBG_NORMAL)
	#define DEBUGSTR
	#define DBGOUT_NORMAL(s,a1,a2,a3,a4,a5) DbgOutStr(s,a1,a2,a3,a4,a5)
#else
	#define DBGOUT_NORMAL(s,a1,a2,a3,a4,a5)
#endif

#if defined(DBG_RCV)
	#define DEBUGSTR
	#define DBGOUT_RCV(s,a1,a2,a3,a4,a5) DbgOutStr(s,a1,a2,a3,a4,a5)
#else
	#define DBGOUT_RCV(s,a1,a2,a3,a4,a5)
#endif

#if defined(DBG_SEND)
	#define DEBUGSTR
	#define DBGOUT_SEND(s,a1,a2,a3,a4,a5) DbgOutStr(s,a1,a2,a3,a4,a5)
#else
	#define DBGOUT_SEND(s,a1,a2,a3,a4,a5)
#endif

// -=-=-=-=-=-=-=-=-=-=-=-=-=- DEFINITIONS -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#define STANDARD_RBUFR_SIZE 100
#define STANDARD_SBUFR_SIZE 100


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-= TYPES =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


typedef struct
	{
	TCHAR szDeviceFile[MAX_PATH];			 // Name of driver file
	TCHAR szPortName[COM_MAX_PORT_NAME];	 // Name of individual port
	} ST_COM_SETTINGS;

// This is the main com structure used to hang on to all the working details
typedef struct s_com
	{
	// The s_com_control structure must always be the first item in this
	//	 structure so that a pointer to this structure can be cast into
	//	 a pointer to it.
	ST_COM_CONTROL stComCntrl; // The exported part of the structure

	// User-settable com values
	ST_COM_SETTINGS stFileSettings;
	ST_COM_SETTINGS stWorkSettings;

	// Control fields for com routines
	HSESSION	hSession;			 // Session we belong to
	int 		fPortActive;		 // TRUE when port has been activated
	int 		fErrorReported; 	 // TRUE if driver has encountered error
	HINSTANCE	hDriverModule;		 // Handle of .DLL module
	TCHAR		szDeviceName[COM_MAX_DEVICE_NAME]; // Name of device type
	TCHAR		chDummy;			 // Dummy address for buffer pointers
	unsigned	afOverride; 		 // Allows tmp. override of com details
	HANDLE		hRcvEvent;			 // Signalled whenever rcv data is avail
    HANDLE      hSndReady;           // Signalled whenever send is not busy

	// Control	fields for sending:
	TCHAR	  * puchSendBufr1;		 // allocated buffers for holding
	TCHAR	  * puchSendBufr2;		 //   chars. waiting to be sent
	TCHAR	  * puchSendBufr;		 // pointer to bufr being filled
	TCHAR	  * puchSendPut;		 // Insertion point in current buffer
	TCHAR		auchDummyBufr[5];	 // Place to rest idle pointers
	int 		nSBufrSize; 		 // Size of send buffers
	int 		nSendCount; 		 // Chars. in current bufr.
	int 		fUserCalled;		 // TRUE if status function called
	STATUSFUNCT pfUserFunction; 	 // User supplied status function

	void		(*pfIdleFunction)(void);  // Registered idle function

	// These funtion pointers get set to point into whatever com device
	// driver is loaded. (If no driver is loaded or if the driver is invalid,
	// they are set to point to default, mostly do-nothing routines

	// There are two groups of function pointers, the first group is set to
	// point into the device driver as soon as the driver is loaded and stay
	// there until the driver us unloaded. The second group are not set to
	// point into the driver until an actual port is activated. These are
	// set to point back to default routines whenever the port is deactivated.

	// -------------------------------------------------------------------
	// These are the function pointers that point into the driver whenever
	// it is loaded.

	int 	 (WINAPI *pfDeviceClose)(void *pvDevData);
	int 	 (WINAPI *pfDeviceDialog)(void *pvDevData, HWND hwndParent);
	int 	 (WINAPI *pfDeviceGetCommon)(void *pvDevData, ST_COMMON *pstCommon);
	int 	 (WINAPI *pfDeviceSetCommon)(void *pvDevData, ST_COMMON *pstCommon);
	int 	 (WINAPI *pfDeviceSpecial)(void *, const TCHAR *, TCHAR *, int);
	int 	 (WINAPI *pfDeviceLoadHdl)(void *pvDevData, SF_HANDLE sfHdl);
	int 	 (WINAPI *pfDeviceSaveHdl)(void *pvDevData, SF_HANDLE sfHdl);
	int 	 (WINAPI *pfPortConfigure)(void *pvDevData);
	int 	 (WINAPI *pfPortActivate)(void *pvDevData,
						TCHAR *pszPortName,
						DWORD_PTR dwMediaHdl);
	int 	 (WINAPI *pfPortPreconnect)(void *pvDevData,
						TCHAR *pszPortName, HWND hwndParent);

	// These function pointers point into the driver only when a port has
	// been activated.

	int 	 (WINAPI *pfPortDeactivate)(void *pvDevData);
	int 	 (WINAPI *pfPortConnected)(void *pvDevData);


	int 	 (WINAPI *pfRcvRefill)(void *pvDevData);
	int 	 (WINAPI *pfRcvClear)(void *pvDevData);

	int 	 (WINAPI *pfSndBufrSend)(void *pvDevData,
				 void *pvBufr, int nCount);
	int 	 (WINAPI *pfSndBufrIsBusy)(void *pvDevData);
	int 	 (WINAPI *pfSndBufrClear)(void *pvDevData);
	int 	 (WINAPI *pfSndBufrQuery)(void *pvDevData, unsigned *pafStatus,
				 long *plHandshakeDelay);
	int 	 (WINAPI *pfSendXon)(void *pvDevData);

	//----------------------------------------------------------------------

	void *	 pvDriverData;		  // Space for drivers data

	unsigned nGuard;			  // to check for memory overwrites
	} ST_COM;




// -=-=-=-=-=-=-=-=-=-=-=- INTERNAL PROTOTYPES -=-=-=-=-=-=-=-=-=-=-=-=-=-=-

extern void ComReportError(const HCOM pstCom,
						  int iErrStr,
					const TCHAR * const pszOptInfo,
					const int fFirstOnly);
extern void ComFreeDevice(const HCOM pstCom);
extern int	ComValidHandle(HCOM pstCom);
extern int	ComSendDefaultStatusFunction(int iReason,
					unsigned fusHsStatus, long lDelay);

// These are the set of default functions that the function pointers in
//	the com handle point to when there is no com driver loaded or when
//	it is inactive
int  WINAPI ComDefDoNothing(void *pvDriverData);
int  WINAPI ComDefPortPreconnect(void *pvDriverData,
		TCHAR *pszPortName,
		HWND hwndParent);
int  WINAPI ComDefDeviceDialog(void *pvDriverData, HWND hwndParent);
int  WINAPI ComDefPortActivate(void *pvDriverData,
		TCHAR *pszPortName,
		DWORD_PTR dwMediaHdl);
int  WINAPI ComDefBufrRefill(void *pvDriverData);
int  WINAPI ComDefSndBufrSend(void *pvDriverData, void *pvBufr, int nCount);
int  WINAPI ComDefSndBufrBusy(void *pvDriverData);
int  WINAPI ComDefSndBufrClear(void *pvDriverData);
int  WINAPI ComDefSndBufrQuery(void *pvDriverData,
		unsigned *pafStatus,
		long *plHandshakeDelay);
void WINAPI ComDefIdle(void);
int  WINAPI ComDefDeviceGetCommon(void *pvPrivate, ST_COMMON *pstCommon);
int  WINAPI ComDefDeviceSetCommon(void *pvPrivate, struct s_common *pstCommon);
int  WINAPI ComDefDeviceSpecial(void *pvPrivate,
		const TCHAR *pszInstructions,
		TCHAR *pszResult,
		int nBufrSize);
int  WINAPI ComDefDeviceLoadSaveHdl(void *pvPrivate, SF_HANDLE sfHdl);

// Functions made available to driver .DLLs through func. pointers
extern void * ComMalloc(size_t size);
extern void   ComFree(void *pvItem);
