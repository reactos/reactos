////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

/// Initial drive selection from command line.
CHAR szDisc[4] = "";
BOOL bChanger = FALSE;

/// Return CD-RW device type
JS_DEVICE_TYPE
CheckCDType(
    PCHAR m_szFile,
    PCHAR VendorId
    )
{
    HANDLE hDevice;
    CHAR szDeviceName[256];
    CHAR ioBuf[4096];
    ULONG RC;
    BOOL DvdRW  = false;
    BOOL DvdpRW = false;
    BOOL DvdRAM = false;
    BOOL DvdpR = false;
    BOOL DvdR = false;

    bChanger = FALSE;

    // Make string representing full path
    sprintf(szDeviceName, "\\\\.\\%s", m_szFile);
    if (szDeviceName[strlen(szDeviceName)-1] == '\\') szDeviceName[strlen(szDeviceName)-1] = '\0';

    // Open device volume
    hDevice = OpenOurVolume(szDeviceName);

    if (hDevice == ((HANDLE)-1)) {   
        strcpy(VendorId,"");
        return BUSY;
    } else {             
         
        // Get our cdrw.sys signature
        RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_SIGNATURE,hDevice,
                            &ioBuf,sizeof(GET_SIGNATURE_USER_OUT),
                            &ioBuf,sizeof(GET_SIGNATURE_USER_OUT),FALSE,NULL);

        if (RC == 1) {
            // Get device information
            RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_DEVICE_INFO,hDevice,
                               &ioBuf,sizeof(GET_DEVICE_INFO_USER_OUT),
                               &ioBuf,sizeof(GET_DEVICE_INFO_USER_OUT),FALSE,NULL);
            if (RC != 1) {
                strcpy(VendorId,"Unknown Vendor");
            } else {
                if(((PGET_DEVICE_INFO_USER_OUT)&ioBuf)->Features & CDRW_FEATURE_CHANGER)
                    bChanger = TRUE;
                strcpy(VendorId,(PCHAR)&(((PGET_DEVICE_INFO_USER_OUT)&ioBuf)->VendorId[0]));
                if (((PGET_DEVICE_INFO_USER_OUT)&ioBuf)->Features & CDRW_FEATURE_GET_CFG) {
                    DvdRW = (((PGET_DEVICE_INFO_USER_OUT)&ioBuf)->Features2[0] >> PFNUM_DVDRW_RESTRICTED_OVERWRITE) & 1;
                    DvdRAM = (((PGET_DEVICE_INFO_USER_OUT)&ioBuf)->Features2[0] >> PFNUM_DVDRAM) & 1;
                    DvdpRW = (((PGET_DEVICE_INFO_USER_OUT)&ioBuf)->Features2[0] >> PFNUM_DVDpRW) & 1;
                    DvdR = (((PGET_DEVICE_INFO_USER_OUT)&ioBuf)->Features2[0] >> PFNUM_DVDR) & 1;
                    DvdpR = (((PGET_DEVICE_INFO_USER_OUT)&ioBuf)->Features2[0] >> PFNUM_DVDpR) & 1;
                }
            }

            // Get device capabilities
            RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_CAPABILITIES,hDevice,
                                &ioBuf,sizeof(GET_CAPABILITIES_USER_OUT),
                                &ioBuf,sizeof(GET_CAPABILITIES_USER_OUT),FALSE,NULL);
            if(RC != 1) {
                CloseHandle(hDevice);
                return OTHER;
            }

            // Check capabilities
            if(((PGET_CAPABILITIES_USER_OUT)&ioBuf)->WriteCap & (DevCap_write_cd_r | DevCap_write_cd_rw | DevCap_write_dvd_ram | DevCap_write_dvd_r) ||
              DvdRW || DvdpRW || DvdRAM) {
                
                if (DvdRAM || ((PGET_CAPABILITIES_USER_OUT)&ioBuf)->WriteCap & DevCap_write_dvd_ram) {
                    CloseHandle(hDevice);
                    return DVDRAM;
                }
/*                if (DvdR) {
                    CloseHandle(hDevice);
                    return DVDR;
                }*/
                if (DvdRW) {
                    CloseHandle(hDevice);
                    return DVDRW;
                }
                if (DvdpRW) {
                    CloseHandle(hDevice);
                    return DVDPRW;
                }
/*                if (DvdpR) {
                    CloseHandle(hDevice);
                    return DVDPR;
                }*/
                if (((PGET_CAPABILITIES_USER_OUT)&ioBuf)->WriteCap & DevCap_write_dvd_r) {
                    CloseHandle(hDevice);
                    return DVDR;
                }
                if (((PGET_CAPABILITIES_USER_OUT)&ioBuf)->WriteCap & DevCap_write_cd_rw) {
                    CloseHandle(hDevice);
                    return CDRW;
                }
                if (((PGET_CAPABILITIES_USER_OUT)&ioBuf)->WriteCap & DevCap_write_cd_r) {
                    CloseHandle(hDevice);
                    return CDR;
                }
            }
            else {
                CloseHandle(hDevice);
                return OTHER;
            }
        } else {
            strcpy(VendorId,"Unknown Vendor");
        }
        CloseHandle(hDevice);
    }
                
    return OTHER;
} // end CheckCDType()

/** Intialize asbtract device list via calls to CallBack function.
    \param hDlg Not used.
    \param hwndControl Passed to the CallBack function. See #PADD_DEVICE.
    \param CallBack Callback function. Called on each CD device in system.
*/
void
InitDeviceList(
    HWND hDlg,
    HWND hwndControl,
    PADD_DEVICE CallBack
    )
{
    char    Buffer[MAX_PATH] = "";
    char    VendorId[25];
    char    seps[] = ",";
    char*   token;
    char    info[MAX_PATH];
    bool    add_drive = false;

    JS_DEVICE_TYPE  drive_type;

    // Get all device letter in system
    GetLogicalDriveStrings((DWORD)MAX_PATH,(LPTSTR)&Buffer);
    token = (char *)&Buffer;
    // Replace all zeroes with comma.
    while (token != NULL) {
        token = (char *)memchr(Buffer,'\0',MAX_PATH);
        if (token) {
            if (*(token-1) == ',') {
                token = NULL;
            } else {
                *token=',';
            }
        }
    }
    // Parse string of drive letters separated by comma
    token = strtok((char *)&Buffer,seps);
    while (token != NULL) {
        add_drive = false;
        switch (GetDriveType(token)) {
/*
            case DRIVE_FIXED:   
                add_drive = true;
                break;
*/
            case DRIVE_CDROM:   
                // Determine CD/DVD-ROM type (R,RW,RAM,other)
                drive_type = CheckCDType(token,&VendorId[0]);
                add_drive = true;
                break;
        }
        if (add_drive) {

            // Append to drive letter VendorId
            strncpy(info,token,strlen(token)-1);
            info[strlen(token)-1]='\0';
            strcat(info,"  ");
            strcat(info,VendorId);

            BOOL bSelect = !strcmp(strupr(szDisc),strupr(token));
            if (drive_type != OTHER) {
                CallBack(hwndControl,token,info,MediaTypeStrings[drive_type],bSelect); 
            } else {
                CallBack(hwndControl,token,info,"[Unsupported]",FALSE); 
            }

        }
        // Move to the next drive letter in string
        token = strtok(NULL,seps);
    }
} // end InitDeviceList()

HANDLE
FmtAcquireDrive_(
    PCHAR _Drive,
    CHAR Level
    )
{
    WCHAR LockName[32];
    HANDLE evt;

    WCHAR Drive[1];
    Drive[0] = _Drive[0] & ~('a' ^ 'A');

    swprintf(LockName, L"DwFmtLock_%1.1S%d", Drive, Level);
    evt = CreatePublicEvent(LockName);
    if(!evt) {
        return NULL;
    }
    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(evt);
        return INVALID_HANDLE_VALUE;
    }
    return evt;
} // end FmtAcquireDrive_()

HANDLE
FmtAcquireDrive(
    PCHAR Drive,
    CHAR Level
    )
{
    HANDLE evt;

    evt = FmtAcquireDrive_(Drive, Level);
    if(!evt || evt == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    return evt;
} // end FmtAcquireDrive()

BOOLEAN
FmtIsDriveAcquired(
    PCHAR Drive,
    CHAR Level
    )
{
    HANDLE evt;

    evt = FmtAcquireDrive_(Drive, Level);
    if(evt == INVALID_HANDLE_VALUE) {
        return TRUE;
    }
    if(evt) {
        CloseHandle(evt);
    }
    return FALSE;
} // end FmtIsDriveAcquired()

VOID
FmtReleaseDrive(
    HANDLE evt
    )
{
    if(evt) {
        CloseHandle(evt);
    }
} // end FmtReleaseDrive()
