/* *********************************************************************
 * DevInfo.h	Header file for DOS ConfigMgr Internal structure
 * 
 * Microsoft Corporation 
 * Copyright 1993
 * 
 * Author:	Nagarajan Subramaniyan 
 * Created:	9/1/93
 *
 * Modification history:
 *
 * **********************************************************************
*/

#ifndef _INC_DEVINFO
#define _INC_DEVINFO

/* XLATOFF */
#ifndef FAR
#ifdef	IS_32
#define	FAR
#else
#define	FAR	far
#endif
#endif
/* XLATON */

#define MAX_DEVID_LENGTH	10
			// device id string length 
			// EISA/PNP/PCI ids are only (max)8 bytes in length
#define MAX_SERNO_LENGTH	MAX_DEVID_LENGTH

#define CONFIG_DEVICE_NAME	"CONFIG$"

#define BUSTYPE_UNKNOWN 0
#define	BUSTYPE_ISA	1
		// ISA PNP bus is also treated as ISA 
#define BUSTYPE_EISA	2
#define BUSTYPE_PCI	4
#define BUSTYPE_PCMCIA	8
#define BUSTYPE_PNPISA	0x10
#define BUSTYPE_MCA	0x20
#define BUSTYPE_BIOS	0x40

struct Device_ID_s {
	DWORD	dwBusID;
			// 0 	undefined
			// 1 	ISA
			// 2	EISA
			// 4	PCI
			// 8	PCMCIA
			// 0x10	PNP
			// 0x20 MCA
	DWORD	dwDevId;
			// physical device ID ; -1 is undefined
	DWORD	dwSerialNum;	// 0 undefined
	DWORD	dwLogicalID;
		// Log device ID for PNP ISA
		// Class code for PCI, -1 undefined
	DWORD	dwFlags;
		// Bit 0:	Device has been inited
		// Bit 1:	Device is enabled
		// Bit 2: 	Device Config has been locked
	
};

/* values for DevId->dwFlags field */

#define DEV_INITED		0x1
			// device has been initialized
#define DEV_ENABLED		0x2
			// device has been enabled
#define DEV_CONFIG_LOCKED	0x4
			// device config was locked

#define DEV_STATIC_FLAGS	(DEV_INITED | DEV_ENABLED | DEV_CONFIG_LOCKED )
		// for static, motherboard devices, all 3 bits have to be set


#define SERNO_UNKNOWN	0x0
			// unknown serial no

#define UNKNOWN_VALUE	0xFFFFFFFF
		// unknown devid, log id etc..


typedef struct Device_ID_s	DEVICE_ID;
typedef DEVICE_ID 	*PDEVICE_ID;
typedef struct Device_ID_s FAR *LPDEVICE_ID;


/* XLATOFF */
union	Bus_Access	{
/* XLATON */

	struct	PCIAccess_s 	{
		BYTE	bBusNumber;	// Bus no 0-255
		BYTE	bDevFuncNumber;	// Device # in bits 7:3 and
					// Function # in bits 2:0
		WORD	wPCIReserved;	// 
	} sPCIAccess;
	struct EISAAccess_s	{
		BYTE	bSlotNumber;	// EISA board slot number
		BYTE	bFunctionNumber;
		WORD	wEisaReserved;
	} sEISAAccess;
	struct PnPAccess_s	{
		BYTE	bCSN;	// card slot number
		BYTE	bLogicalDevNumber;	// Logical Device #
		WORD	wReadDataPort;		// Read data port
	} sPnPAccess;
	struct PCMCIAAccess_s	{
		BYTE	bAdapterNumber;     // Card adapter number
		BYTE	bSocketNumber;	    // Card socket #
		WORD	wPCMCIAReserved;    // Reserved
	} sPCMCIAAccess;
	struct BIOSAccess_s	{
		BYTE	bBIOSNode;	    // Node number
	} sBIOSAccess;
/* XLATOFF */
};

typedef union Bus_Access	UBUS_ACCESS;
typedef union Bus_Access	*PUBUS_ACCESS;
typedef union Bus_Access FAR	*LPUBUS_ACCESS;


/* XLATON */

/* ASM 
; Following is supposed to be the max size of the above UNION
; This is done since H2INC doesn't know how to handle Unions

UBUS_ACCESS_SIZE	equ	size PnPAccess_s
;
; Structure definition for Config_Info_s
;
Config_Info_s	STRUC
	sDeviceId	db	size Device_ID_s dup (?)	
	uBusAccess	db	UBUS_ACCESS_SIZE	dup (?)
	sConfig_Data	db	size Config_Buff_s	dup (?)
Config_Info_s	ENDS

; End assembly
*/

/* XLATOFF */

struct Config_Info_s	{
	DEVICE_ID 		sDeviceId;	// DEVICE ID Info
	UBUS_ACCESS		uBusAccess;	// Bus specific data
	CMCONFIG		sConfig_Data;	// configuration data
						// 	defined in configmg.h
};
typedef struct Config_Info_s CONFIGINFO;
typedef struct Config_Info_s *PCONFIGINFO;
typedef struct Config_Info_s FAR *LPCONFIGINFO;

/* XLATON */

struct	Dev_Info_s	{

	struct Dev_Info_s FAR *lpNxtDevInfo;	// ptr to next dev info record
	struct Config_Info_s	sConfigInfo;	// config data info
						// variable length
};

typedef struct Dev_Info_s DEVINFO;
typedef struct Dev_Info_s *PDEVINFO;
typedef struct Dev_Info_s FAR *LPDEVINFO;


#define MAX_STATE_DATA_SIZE	300
#define DEVHDR_SIGNATURE	0x4d435744	// "DWCM" = Dos/Windows Config Manager

struct Dev_Header_s	{
	DWORD	DH_Signature;	// for verification == DEVHDR_SIGNATURE
	DWORD	DH_DevCount;	// no of devices we know of (and have data)
	DWORD	DH_TotalSize;	// size in bytes (incl. this header	)
	DWORD	DH_LinearAddr;	// Linear address if XMS.
	char	DH_StateData[MAX_STATE_DATA_SIZE];
				// Hardware state data
	struct Config_Info_s	DH_DevInfo;	// array of Config Info records
					// start here
};

typedef struct Dev_Header_s DEVHEADER;
typedef struct Dev_Header_s *PDEVHEADER;
typedef struct Dev_Header_s FAR *LPDEVHEADER;
	
/* Structures and definitions for the IOCTL_READ call to CONFIG$ */

struct ConfigDataPtr_s {
	DWORD	lpConfigPtr;
	BYTE	bConfigFlags;
};

typedef struct ConfigDataPtr_s CONFIGDATAPTR;
typedef struct ConfigDataPtr_s *PCONFIGDATAPTR;
typedef struct ConfigDataPtr_s FAR *LPCONFIGDATAPTR;

/* Definitions for bConfigFlags */

#define DC_API_ENABLED	1
#define DC_DATA_IN_XMS	2		
			// default: data in conv mem
/* if Data in conv mem, lpConfigPtr is actually a far ptr to the data 
 * if Data is in xms, LOWORD(lpConfigPtr) = 0 and HIWORD(lpConfigPtr) is the
 * XMS handle. 
*/


#define MAX_CONFIG 9
#define MAX_PROFILE_LEN 80
#define ULDOCK_ZERO 0xFFFFFFF0

typedef WORD CONFIG;
typedef WORD *PCONFIG;
typedef WORD FAR *LPCONFIG;

struct Map_s	{
	DWORD	MP_dwDock;
	DWORD	MP_dwSerialNo;
	WORD	MP_wChecksum;
	CONFIG	MP_cfg;
};

typedef struct Map_s MAP;
typedef struct Map_s *PMAP;
typedef struct Map_s FAR *LPMAP;

struct Map_DB_s	{
	WORD	MD_imapMax;
	struct	Map_s MD_rgmap[MAX_CONFIG];
};

typedef struct Map_DB_s MAPDB;
typedef struct Map_DB_s *PMAPDB;
typedef struct Map_DB_s FAR *LPMAPDB;

struct Config_Data_s	{
	// this data put into CONFIG$ device
	DWORD	CD_dwDock;
	DWORD	CD_dwSerialNo;
	WORD	CD_wChecksum;
	CONFIG	CD_cfg;
	char	CD_szFriendlyName[MAX_PROFILE_LEN];
	// this data not put into CONFIG$ device
	struct Map_DB_s	CD_mapdb;
};

typedef struct Config_Data_s CONFIGDATA;
typedef struct Config_Data_s *PCONFIGDATA;
typedef struct Config_Data_s FAR *LPCONFIGDATA;

#endif /* _INC_DEVINFO */
