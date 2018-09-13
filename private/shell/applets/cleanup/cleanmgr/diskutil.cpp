/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    diskutil.cpp
**
** Purpose: Disk utility functions
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"
#include "diskutil.h"   
#include "msprintf.h"
#include "resource.h"


#define Not_VxD
#include <vwin32.h>



/*
**------------------------------------------------------------------------------
** fIsSingleDrive
**
** Purpose:    gets a drive letter from a drive string
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL 
fIsSingleDrive (
    LPTSTR	lpDrive
    )
{
    //  
    //Is it a valid drive string ?!?
    //
    if (!fIsValidDriveString(lpDrive))
        return FALSE;

    //  
    //Is it a valid drive ?!?
    //
    BOOL rc = FALSE;

    UINT driveType = GetDriveType (lpDrive);
    switch (driveType)
    {
        case 0:                 // Unknown drive type
        case 1:                 // Invalid drive type
            break;

        case DRIVE_REMOVABLE:   // Removable drive (floppy,bernoulli,syquest,etc)
        case DRIVE_FIXED:       // Hard disk
            // We support removeable and fixed drives
            rc = TRUE;
            break;
      
        case DRIVE_REMOTE:      // Network
        case DRIVE_CDROM:       // CD-ROM
            break;

        case DRIVE_RAMDISK:     // RAM disk
            // We support ram drives - even though it's stupid
            rc = TRUE;
            break;

        default:                // Unknown drive type
            break;
    }

    return rc;
}

/*
**------------------------------------------------------------------------------
** fIsValidDriveString
**
** Purpose:    determines if a drive is a valid drive string
** Note:       assumes a drive string consists of a drive letter,
**             colon, and slash characters and nothing else.
**             Example:  "C:\"
** BUGBUG:     This probably needs to be updated for localization purposes
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL 
fIsValidDriveString(
    const TCHAR * szDrive
    )
{
    //  
    //Make sure we have a valid string
    //
    if ((szDrive == NULL) || (szDrive[0] == 0))
        return FALSE;

    //  
    //Make sure length is equal to length of valid drive string "C:\"
    //
    INT iLen = lstrlen(szDrive);
    if (iLen != 3)
        return FALSE;

    //  
    //Check drive letter
    //
    TCHAR ch = szDrive[0];
    if ((ch >= 'a') && (ch <= 'z'))   
        ;
    else if ((ch >= 'A') && (ch <= 'Z'))
        ;
    else
        return FALSE;

    //  
    //Check colon
    //
    ch = szDrive[1];
    if (ch != ':')
        return FALSE;

    //  
    //Check slash
    //
    ch = szDrive[2];
    if (ch != '\\')
        return FALSE;

    //  
    //Check zero terminating byte
    //
    ch = szDrive[3];
    if (ch != 0)
        return FALSE;

    return TRUE;
}

/*
**------------------------------------------------------------------------------
** GetDriveFromString
**
** Purpose:    gets a drive letter from a drive string
** BUGBUG:     This probably needs to be updated for localization purposes
** Mod Log:    Created by Jason Cobb (4/97)
**------------------------------------------------------------------------------
*/
BOOL 
GetDriveFromString(
    const TCHAR * szDrive, 
    drenum & dre
    )
{
    dre = Drive_INV;

    //
    //Make sure we have a valid string
    //
    if ((szDrive == NULL) || (szDrive[0] == 0))
        return FALSE;

    //  
    //Get drive number from drive letter
    //
    TCHAR chDrive = szDrive[0];
    if ((chDrive >= TCHAR('a')) && (chDrive <= TCHAR('z')))
        dre = (drenum)(chDrive - TCHAR('a'));
    else if ((chDrive >= TCHAR('A')) && (chDrive <= TCHAR('Z')))
        dre = (drenum)(chDrive - TCHAR('A'));
    else
        return FALSE;

    return TRUE;
}

/*
**------------------------------------------------------------------------------
** GetDriveIcon
**
** Purpose:    
** Parameters:
**    dre - driver letter
**    bSmallIcon - TRUE if small icon is desired.
** Return:     Drive Icon returned by the shell
** Notes:   
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
HICON
GetDriveIcon(
    drenum dre,
    BOOL bSmallIcon
	)
{
	TCHAR		szDrive[10];
	SHFILEINFO	fi;

	CreateStringFromDrive(dre, szDrive, sizeof(szDrive));

	if (bSmallIcon)
    	SHGetFileInfo(szDrive, 0, &fi, sizeof(fi), SHGFI_ICON | SHGFI_DISPLAYNAME | SHGFI_SMALLICON);
	else
    	SHGetFileInfo(szDrive, 0, &fi, sizeof(fi), SHGFI_ICON | SHGFI_DISPLAYNAME | SHGFI_LARGEICON);

	return fi.hIcon;
}

BOOL
GetDriveDescription(
    drenum dre, 
    TCHAR *psz
    )
{
    TCHAR *desc;
    TCHAR szVolumeName[MAX_PATH];
    TCHAR szDrive[MAX_PATH];

    *szVolumeName = 0;
    CreateStringFromDrive(dre, szDrive, sizeof(szDrive));
    GetVolumeInformation(szDrive, szVolumeName, sizeof(szVolumeName), NULL, NULL, NULL, NULL, 0);

    desc = SHFormatMessage( MSG_VOL_NAME_DRIVE_LETTER, szVolumeName, (TCHAR)(dre + 'A'));
//    desc = msprintf (nszVolNameDriveLetter, "%s%c", szVolumeName, (TCHAR)(dre + 'A'));
    StrCpy (psz, desc);
    LocalFree (desc);

    return TRUE;
}


/*
**------------------------------------------------------------------------------
** GetHardwareType
**
** Purpose:    
** Parameters:
**    hwHardware  - hardware type of drive
** Return:     TRUE if compatible with our needs
**             FALSE otherwise
** Notes:   
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL 
GetHardwareType(
    drenum dre, 
    hardware &hwType
    )
{
    TCHAR szDrive[4];

    hwType = hwINVALID;


    //  
    //Get drive string from drive number
    //
    if (!CreateStringFromDrive(dre, szDrive, 4))
        return FALSE;

    UINT uiType = GetDriveType(szDrive);
    switch (uiType)
    {
        case 0:
            hwType = hwUnknown;
            return FALSE;

        case 1:
            hwType = hwINVALID;
            return FALSE;

        case DRIVE_REMOVABLE:
            hwType = hwRemoveable;
            break;

        case DRIVE_FIXED:
            hwType = hwFixed;
            break;

        case DRIVE_REMOTE:
            hwType = hwNetwork;
            return FALSE;

        case DRIVE_CDROM:
            hwType = hwCDROM;
            return FALSE;

        case DRIVE_RAMDISK:
            hwType = hwRamDrive;
            break;

        default:
            hwType = hwUnknown;
            return FALSE;
    }

    return TRUE;
}

/*
**------------------------------------------------------------------------------
** CreateStringFromDrive
**
** Purpose:    creates a drive string from a drive number
** BUGBUG:     This probably needs to be updated for localization purposes
** Mod Log:    Created by Jason Cobb (4/97)
**------------------------------------------------------------------------------
*/
BOOL 
CreateStringFromDrive(
    drenum dre, 
    TCHAR * szDrive, 
    ULONG cLen
    )
{
    if ((szDrive == NULL) || (cLen < 4))
        return FALSE;

    if (dre == Drive_INV)
        return FALSE;

    TCHAR ch = (CHAR)(dre + 'A');

    //
    //Drive string = Drive letter, colon, slash = "C:\"
    //
    szDrive[0] = ch;
    szDrive[1] = ':';
    szDrive[2] = '\\';
    szDrive[3] = 0;

    return TRUE;
}

#ifndef UNICODE
WORD 
DoVWIN32Ioctl(
	PDEVIOCTL_REGISTERS ptReg
	)
{
	HANDLE hDevice;
	BOOL  bResult;
	DWORD cb;

	//
	//Get VWIN32 handle
	//
	hDevice = CreateFile(TEXT("\\\\.\\vwin32"),
						 0,
						 0,
						 NULL,
						 0,
						 FILE_FLAG_DELETE_ON_CLOSE,
						 NULL);

	if (hDevice == (HANDLE)INVALID_HANDLE_VALUE)
		return (-1);

	//
	//Issue the IOCTL
	//
	bResult = DeviceIoControl(hDevice,
							  VWIN32_DIOC_DOS_IOCTL,
							  ptReg,
							  sizeof(*ptReg),
							  ptReg,
							  sizeof(*ptReg),
							  &cb,
							  0);

	if (!bResult)
		return (-1);

	//
	//IOCTL was issued, return DOS status
	//
	if (ptReg->reg_Flags & 0x0001)
		return ((WORD)ptReg->reg_EAX);
	else
		return (0);
}

ULARGE_INTEGER
GetFreeSpaceRatio(
	WORD dwDrive,
	ULARGE_INTEGER cbTotal
	)
{
	DEVIOCTL_REGISTERS tReg;
	WORD wRatio, wMaxRatio, wStatus;
	ULARGE_INTEGER cbLowSpace;

	//
	//We deal with drives in 0 based terms but the vwin32 api is 1 based. So,
	//we will simple add 1 before we call vwin32.vxd
	//
	dwDrive++;

	wRatio = 0;
	wMaxRatio = 0;

	ZeroMemory(&tReg, sizeof(tReg));
	tReg.reg_EAX = 0x440D;
	tReg.reg_EBX = dwDrive;
	tReg.reg_ECX = 0x0863;

	wStatus = DoVWIN32Ioctl(&tReg);
	wRatio = (WORD)tReg.reg_EDX;
	wMaxRatio = (WORD)tReg.reg_EAX;

	//
	//To get the low disk space threshold in MB we will take the total space
	//for this drive multiply it by the current low disk space ratio and then
	//divide it by the max disk space ratio.  Note that if the current low
	//disk space ratio is 0 then then the low disk space warning is not armed.
	//
	if (wRatio == 0 ||
		wMaxRatio == 0 ||
		cbTotal.QuadPart == 0)
		cbLowSpace.QuadPart = 0;

	else
		cbLowSpace.QuadPart = (cbTotal.QuadPart * wRatio) / wMaxRatio;

	MiDebugMsg((0, "Low disk space threshold is %d bytes", cbLowSpace.QuadPart));
	
	return (cbLowSpace);
}
#else
ULARGE_INTEGER
GetFreeSpaceRatio(
	WORD dwDrive,
	ULARGE_INTEGER cbTotal
	)
{
    // for now, hardcode it as a percentage...
    ULARGE_INTEGER cbMin;
    // for now use 1% as the test to go into aggressive mode...
    cbMin.QuadPart = cbTotal.QuadPart / 100;
    return cbMin;
}
#endif

