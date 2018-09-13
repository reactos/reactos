/* comdev.h -- Exported definitions for communications device routines
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:40p $
 */

#define COM_MAX_DEVICE_NAME 40
#define COM_MAX_PORT_NAME	120

typedef struct
	{
	int 	 iAction;
	int 	 iReserved;
	TCHAR	 szDeviceName[COM_MAX_DEVICE_NAME];
	TCHAR	 szFileName[MAX_PATH];
	HANDLE	 hFind;
	WIN32_FIND_DATA stFindData;
	} COM_FIND_DEVICE;

typedef struct
	{
	int 	  iAction;
	TCHAR	  szPortName[COM_MAX_PORT_NAME];
	HINSTANCE hModule;
	long	  lReserved1;
	long	  lReserved2;
	long	  lReserved3;
	long	  lReserved4;
	void	 *pvData;
	} COM_FIND_PORT;

// Values for usAction field in COM_FIND_DEVICE and COM_FIND_PORT
#define COM_FIND_FIRST	0
#define COM_FIND_NEXT	1
#define COM_FIND_DONE	2


// -=-=-=-=-=-=-=-=-=-=-=- EXPORTED PROTOTYPES -=-=-=-=-=-=-=-=-=-=-=-=-=-=-

extern int ComFindDevices(COM_FIND_DEVICE * const pstFind);

int ComGetFileNameFromDeviceName(const TCHAR * const pszDeviceName,
							TCHAR * const pszFileName,
							const int nSize);
