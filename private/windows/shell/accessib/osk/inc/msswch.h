/***** Normal use, default configuration *****/

typedef HANDLE HSWITCHPORT;

HSWITCHPORT APIENTRY swchOpenSwitchPort(
	HWND				hWnd,
	DWORD				dwPortStyle );

#define PS_POLLING	1
#define PS_EVENTS		2

BOOL APIENTRY swchCloseSwitchPort(
	HSWITCHPORT		hSwitchPort );

BOOL APIENTRY swchReadSwitches( 
	HSWITCHPORT		hSwitchPort,
	PDWORD			pdwSwitches );

#define NUM_SWITCHES		6
#define BIT_SWITCHES		0x003F

#define SWITCH_NONE 		0x0000
#define SWITCH_1			0x0001
#define SWITCH_2			0x0002
#define SWITCH_3			0x0004
#define SWITCH_4			0x0008
#define SWITCH_5			0x0010
#define SWITCH_6			0x0020

// These messages need to be reserved with Microsoft
#define SW_SWITCHDOWNBASE	0x00E0
#define SW_SWITCH1DOWN		(SW_SWITCHDOWNBASE + 1)
#define SW_SWITCH2DOWN		(SW_SWITCHDOWNBASE + 2)
#define SW_SWITCH3DOWN		(SW_SWITCHDOWNBASE + 3)
#define SW_SWITCH4DOWN		(SW_SWITCHDOWNBASE + 4)
#define SW_SWITCH5DOWN		(SW_SWITCHDOWNBASE + 5)
#define SW_SWITCH6DOWN		(SW_SWITCHDOWNBASE + 6)

// These messages need to be reserved with Microsoft
#define SW_SWITCHUPBASE		0x00F0
#define SW_SWITCH1UP			(SW_SWITCHUPBASE + 1)
#define SW_SWITCH2UP			(SW_SWITCHUPBASE + 2)
#define SW_SWITCH3UP			(SW_SWITCHUPBASE + 3)
#define SW_SWITCH4UP			(SW_SWITCHUPBASE + 4)
#define SW_SWITCH5UP			(SW_SWITCHUPBASE + 5)
#define SW_SWITCH6UP			(SW_SWITCHUPBASE + 6)

/***** Configuration *****/

typedef HANDLE HSWITCHDEVICE;

// make this a dword rather then a handle this is to make this 64 bit portable
typedef DWORD  HJOYDEVICE;

// This message needs to be reserved with Microsoft
#define SW_SWITCHCONFIGCHANGED	0x00D0

#define SC_TYPE_COM				1
#define SC_TYPE_LPT				2
#define SC_TYPE_JOYSTICK		3
#define SC_TYPE_KEYS				4

// Not defined yet
#define SC_TYPE_USB				5
#define SC_TYPE_1394				6

typedef struct _SWITCHLIST {
 DWORD dwSwitchCount;
 HSWITCHDEVICE hsd[ANYSIZE_ARRAY];
} SWITCHLIST, *PSWITCHLIST;

typedef struct _SWITCHCONFIG_COM {
 DWORD dwComStatus;
} SWITCHCONFIG_COM, *PSWITCHCONFIG_COM;

typedef struct _SWITCHCONFIG_LPT {
 DWORD dwReserved1;	// possible future Status register preset
 DWORD dwReserved2;	// possible future Data register preset
} SWITCHCONFIG_LPT, *PSWITCHCONFIG_LPT;

typedef struct _SWITCHCONFIG_JOYSTICK {
 DWORD dwJoySubType;
 DWORD dwJoyThresholdMinX;
 DWORD dwJoyThresholdMaxX;
 DWORD dwJoyThresholdMinY;
 DWORD dwJoyThresholdMaxY;
 DWORD dwJoyHysteresis;
} SWITCHCONFIG_JOYSTICK, *PSWITCHCONFIG_JOYSTICK;

typedef struct _SWITCHCONFIG_KEYS {
 DWORD dwKeySwitch1;
 DWORD dwKeySwitch2;
} SWITCHCONFIG_KEYS, *PSWITCHCONFIG_KEYS;

typedef struct _SWITCHCONFIG_USB {
 // *** NOT DEFINED YET ***
 DWORD dwReserved;
} SWITCHCONFIG_USB, *PSWITCHCONFIG_USB;

typedef struct _SWITCHCONFIG_IEEE1394 {
 // *** NOT DEFINED YET ***
 DWORD dwReserved;
} SWITCHCONFIG_IEEE1394, *PSWITCHCONFIG_IEEE1394;

typedef struct _SWITCHCONFIG {
 DWORD cbSize;
 UINT uiDeviceType;
 UINT uiDeviceNumber;
 DWORD dwFlags;
 DWORD dwSwitches;
 DWORD dwErrorCode;
 union {
   SWITCHCONFIG_COM  Com;
   SWITCHCONFIG_LPT  Lpt;
   SWITCHCONFIG_JOYSTICK   Joystick;
   SWITCHCONFIG_KEYS Keys;
   SWITCHCONFIG_USB  USB;
   SWITCHCONFIG_IEEE1394 IEEE1394;
 } u;
} SWITCHCONFIG, *PSWITCHCONFIG;

BOOL swchGetSwitchList(
	HSWITCHPORT		hSwitchPort,
	PSWITCHLIST		pSL,
	DWORD				dwSize,
	PDWORD			pdwReturnSize );

HSWITCHDEVICE swchGetSwitchDevice(
	HSWITCHPORT		hSwitchPort,
	UINT				uiDeviceType,
	UINT				uiDeviceNumber	);

UINT swchGetDeviceType( 
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd );

UINT swchGetPortNumber(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd	);

BOOL swchGetSwitchConfig(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );

BOOL swchSetSwitchConfig(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );

//v-mjgran: API to modify return value in keyboard hook. Avoid to send the scan char.
void APIENTRY swchAvoidScanChar (BOOL fSendScanCharacter);

// bitflags
#define SC_FLAG_ACTIVE			0x00000001
#define SC_FLAG_DEFAULT			0x00000002
#define SC_FLAG_ERROR			0x00000004
#define SC_FLAG_UNAVAILABLE	0x00000080

// bitflags
#define SC_COM_DTR		0x00000010
#define SC_COM_RTS		0x00000020
#define SC_COM_DEFAULT	SC_COM_RTS

// bitflags
#define SC_LPT_STROBE	0x00000100
#define SC_LPT_AF			0x00000200
#define SC_LPT_INIT		0x00000400
#define SC_LPT_SLCTIN	0x00000800
#define SC_LPT_DEFAULT	0

#define SC_LPTDATA_DEFAULT  0x000000FF

#define SC_JOY_BUTTONS	0
#define SC_JOY_XYSWITCH	1
#define SC_JOY_XYANALOG	2
#define SC_JOY_DEFAULT	SC_JOY_BUTTONS

#define SC_JOYVALUE_DEFAULT	0


// Error return values
#define SWCHERR_NO_ERROR				0
#define SWCHERR_ERROR					1
#define SWCHERR_INVALID_PARAMETER	2
#define SWCHERR_MAXIMUM_USERS			3
#define SWCHERR_ALREADY_OPEN			4
#define SWCHERR_NULL_POINTER			5
#define SWCHERR_INVALID_BUFFER_SIZE	6
#define SWCHERR_ALLOCATING_MEMORY	7
