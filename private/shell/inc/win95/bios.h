/*****************************************************************************
 *
 *   (C) Copyright MICROSOFT Corp., 1994
 *
 *   Title:	BIOS.H - PnP BIOS Enumerator VxD
 *
 *   Version:	4.00
 *
 *   Date:	6-Feb-1994
 *
 *   Author:	MSq
 *
 *------------------------------------------------------------------------------
 *
 *   Change log:
 *
 *      DATE	REV		    DESCRIPTION
 *   ----------- --- -----------------------------------------------------------
 *    6-Feb-1994 MSq Original
 *****************************************************************************/

/*XLATOFF*/
#define	BIOS_Service	Declare_Service
/*XLATON*/

/*MACROS*/
Begin_Service_Table(BIOS, VxD)
BIOS_Service	(_BIOSGetVersion, VxD_CODE)
BIOS_Service	(_BIOSSoftUndock, VxD_CODE)
BIOS_Service	(_BIOSGetCapabilities, VxD_CODE)
BIOS_Service	(_BIOSGetAPMTable, VxD_CODE)
End_Service_Table(BIOS, VxD)
/*ENDMACROS*/

/*
 * One can add a VxD to fix a broken BIOS. This VxD must have
 * BIOS_EXT_DEVICE_ID as device ID and must export three services, in that
 * order:
 *
 * GetVersion: must return in eax 0x00000100 for this release and carry
 * clear (ie standard version code).
 *
 * GetHeader: must return in eax the linear address to an installation
 * structure, this need not be in BIOS space, but need to have the correct
 * values for building BIOS selectors. The structure will be used instead of
 * the one found by the scan. Also, the BIOS EXT VxD must use this time to
 * initialize. If initialization failed, the value 0 must be return.
 *
 * CallBIOS: will be called instead of calling the BIOS entry point. The
 * stack will be the exact same as if we were calling the BIOS except that
 * the return address is a 32-bit ret to BIOS.VxD. Also CS=DS=ES=SS=Flat
 * segment. Thus the BIOS_EXT VxD can pop the return address in a local
 * variable (this API will never be reentered) and do stack munging before
 * calling its internal function or call BIOS even. All registers except
 * eax, which is the return value, must be preserved. The high word of eax
 * is discarded by BIOS.VxD.
 */

/*XLATOFF*/
#define	BIOS_EXT_Service	Declare_Service
/*XLATON*/

/*MACROS*/
Begin_Service_Table(BIOS_EXT, VxD)
BIOS_EXT_Service	(_BIOSEXTGetVersion, VxD_CODE)
BIOS_EXT_Service	(_BIOSEXTGetHeader, VxD_CODE)
BIOS_EXT_Service	(_BIOSEXTCallBIOS, VxD_CODE)
End_Service_Table(BIOS_EXT, VxD)
/*ENDMACROS*/

#define	PNPBIOS_SERVICE_GETVERSION		0x000
#define	PNPBIOS_SERVICE_SOFTUNDOCK		0x100
#define	PNPBIOS_SERVICE_GETDOCKCAPABILITIES	0x200
#define	PNPBIOS_SERVICE_GETAPMTABLE		0x300

struct BIOSPARAMSTAG {
	DWORD bp_ret;
	WORD *bp_pTableSize;
	char *bp_pTable;
};

typedef struct BIOSPARAMSTAG BIOSPARAMS;
typedef struct BIOSPARAMSTAG *PBIOSPARAMS;

#define	PNPBIOS_ERR_NONE			0x00
#define	PNPBIOS_ERR_SUCCESS			PNPBIOS_ERR_NONE
#define	PNPBIOS_WARN_NOT_SET_STATICALLY		0x7F
#define	PNPBIOS_ERR_UNKNOWN_FUNCTION		0x81
#define	PNPBIOS_ERR_FUNCTION_NOT_SUPPORTED	0x82
#define	PNPBIOS_ERR_INVALID_HANDLE		0x83
#define	PNPBIOS_ERR_BAD_PARAMETER		0x84
#define	PNPBIOS_ERR_SET_FAILED			0x85
#define	PNPBIOS_ERR_EVENTS_NOT_PENDING		0x86
#define	PNPBIOS_ERR_SYSTEM_NOT_DOCKED		0x87
#define	PNPBIOS_ERR_NO_ISA_PNP_CARDS		0x88
#define	PNPBIOS_ERR_CANT_DETERMINE_DOCKING	0x89
#define	PNPBIOS_ERR_CHANGE_FAILED_NO_BATTERY	0x8A
#define	PNPBIOS_ERR_CHANGE_FAILED_CONFLICT	0x8B
#define	PNPBIOS_ERR_BUFFER_TOO_SMALL		0x8C
#define	PNPBIOS_ERR_USE_ESCD_SUPPORT		0x8D
#define	PNPBIOS_ERR_MS_INTERNAL			0xFE

#define PNPBIOS_DOCK_CAPABILITY_VCR		0x0001
#define PNPBIOS_DOCK_CAPABILITY_TEMPERATURE	0x0006
#define PNPBIOS_DOCK_CAPABILITY_COLD		0x0000
#define PNPBIOS_DOCK_CAPABILITY_WARM		0x0002
#define PNPBIOS_DOCK_CAPABILITY_HOT		0x0004
