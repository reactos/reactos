// MSSWCHH.H
// Functions and defines global to MSSWCH but not exported 
// to the rest of the world

// Functions with an X prefix must be called within the context
// of the helper window.

#ifndef _INC_TCHAR
	#include <tchar.h>
#endif
#define SZ_DLLMODULENAME	_TEXT("MSSWCH")


// MSSWCH is the main module and communicates with the outside world

BOOL swchPostSwitches(
	HSWITCHDEVICE	hsd,
	DWORD				dwSwitch );
BOOL swchPostConfigChanged( void );
void XswchStoreLastError(
	HSWITCHPORT		hSwitchPort,
	DWORD				dwError );
void swchSetLastError(
	DWORD				dwError );

// The List module distributes calls to the rest of the modules

BOOL XswcListInit( void );
BOOL XswcListEnd( void );
BOOL swcListGetList(
	HSWITCHPORT		hSwitchPort,
	PSWITCHLIST		pSL,
	DWORD				dwSize,
	PDWORD			pdwReturnSize );
HSWITCHDEVICE swcListGetSwitchDevice(
	HSWITCHPORT		hSwitchPort,
	UINT				uiDeviceType,
	UINT				uiDeviceNumber );
UINT swcListGetDeviceType(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd );
UINT swcListGetDeviceNumber(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd	);
BOOL swcListGetConfig(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );
BOOL XswcListSetConfig(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );
DWORD XswcListPollSwitches( void );


// Here are the rest of the modules

BOOL XswcComInit( HSWITCHDEVICE	hsd );
BOOL XswcComEnd( HSWITCHDEVICE	hsd );
BOOL swcComGetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );
BOOL XswcComSetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );
DWORD XswcComPollStatus( HSWITCHDEVICE	hsd );

BOOL XswcJoyInit( HSWITCHDEVICE	hsd );
BOOL XswcJoyEnd( HSWITCHDEVICE	hsd );
BOOL swcJoyGetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );
BOOL XswcJoySetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );
DWORD XswcJoyPollStatus( HSWITCHDEVICE	hsd );

BOOL XswcKeyInit( HSWITCHDEVICE	hsd );
BOOL XswcKeyEnd( HSWITCHDEVICE	hsd );
BOOL swcKeyGetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );
BOOL XswcKeySetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );
DWORD XswcKeyPollStatus( HSWITCHDEVICE	hsd );

BOOL XswcLptInit( HSWITCHDEVICE	hsd );
BOOL XswcLptEnd( HSWITCHDEVICE	hsd );
BOOL swcLptGetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );
BOOL XswcLptSetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc );
DWORD XswcLptPollStatus( HSWITCHDEVICE	hsd );
