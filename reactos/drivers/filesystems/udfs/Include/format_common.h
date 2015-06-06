////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

JS_DEVICE_TYPE
CheckCDType(
    PCHAR m_szFile,
    PCHAR VendorId
    );

typedef void (*PADD_DEVICE)(HWND hwndControl, PCHAR Drive, PCHAR Line1, PCHAR Line2,BOOL bSelect);

void
InitDeviceList(
    HWND hDlg,
    HWND hwndControl,
    PADD_DEVICE CallBack
    );

extern CHAR szDisc[4];
extern BOOL bChanger;

HANDLE
FmtAcquireDrive(
    PCHAR Drive,
    CHAR Level
    );

#define FmtAcquireDriveW(Drive, Level)    FmtAcquireDrive((PCHAR)(Drive), Level)

VOID
FmtReleaseDrive(
    HANDLE evt
    );

BOOLEAN
FmtIsDriveAcquired(
    PCHAR Drive,
    CHAR Level
    );
