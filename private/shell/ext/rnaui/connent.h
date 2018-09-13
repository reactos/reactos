//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       connent.h
//  Content:    This file contain all the declaration for the connection entry
//              process UI.
//  History:
//      Fri 26-Mar-1993 12:28:38  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#if defined(MULTILINK_ENABLED) && defined(MULTILINK_PROP_PAGE)
#define MAX_CONNENT_PAGES       4
#else
#define MAX_CONNENT_PAGES       3
#endif

typedef struct  tagCountryCode {
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
}   COUNTRYCODE, *PCOUNTRYCODE, FAR* LPCOUNTRYCODE;

#define MAX_COUNTRY             512
#define DEF_COUNTRY_INFO_SIZE   1024
#define MAX_COUNTRY_NAME        36
#define MAX_AREA_LIST           20
#define MAX_DISPLAY_NAME        36

//****************************************************************************
// Function Prototypes
//****************************************************************************

//
//  Property sheet callback functions
//
void CALLBACK CallEntry_ReleasePropSheet (LPPROPSHEETPAGE psp);

//
//  Dialog boxes callback procedures
//
int  CALLBACK _export ConnEntryDlgProc  (HWND    hWnd,
                                         UINT    message,
                                         WPARAM  wParam,
                                         LPARAM  lParam);

BOOL FAR PASCAL Remote_PropertySheet (LPSTR szObjPath, HWND hWnd);

PROPSHEETPAGE* NEAR PASCAL CallEntry_GetPropSheet (PROPSHEETHEADER *psh,
                                                   PCONNENTDLG  pConnEntDlg);

BOOL       NEAR PASCAL ConnEntryHandler(HWND hWnd, PCONNENTDLG pConnEntDlg, WPARAM wParam, LPARAM lParam);
BOOL       NEAR PASCAL InitConnEntryDlg (HWND hWnd, PCONNENTDLG pConnEntDlg);
void       NEAR PASCAL AdjustConnEntryDlg (HWND hWnd, PCONNENTDLG pConnEntDlg);
void       NEAR PASCAL DeInitConnEntryDlg(HWND hWnd, PCONNENTDLG pConnEntDlg);

BOOL       NEAR PASCAL InitNameAndDevice(HWND hWnd, PCONNENTRY pConnEntry);
void       NEAR PASCAL InitPhoneNumber (HWND hWnd, PCONNENTRY pConnEntry);
void       NEAR PASCAL AdjustPhone(HWND hWnd);
void       NEAR PASCAL InitAreaCodeList (HWND hLB, LPSTR szSelectAreaCode);
void       NEAR PASCAL InitCountryCodeList (HWND hLB, DWORD dwSelectCountryID,
                                            BOOL fAll);
void       NEAR PASCAL CompleteCountryCodeList (HWND hLB);
void       NEAR PASCAL DeInitCountryCodeList (HWND hLB);

void       NEAR PASCAL InitDeviceList (HWND);
void       NEAR PASCAL AdjustDeviceList (HWND hwnd, PCONNENTDLG pConnEntDlg, DWORD dwCommand);
void       NEAR PASCAL AdjustDevice(HWND hWnd, PCONNENTDLG pConnEntDlg);
void       NEAR PASCAL DeInitDeviceList (HWND hCtrl);

BOOL       NEAR PASCAL IsInvalidConnEntry(HWND hWnd);
BOOL       NEAR PASCAL ValidateEntry (HWND hWnd, LPBYTE szEntry, UINT idCtrl, UINT uErrMsg);
BOOL       NEAR PASCAL IsValidDevice(LPSTR szDeviceName);

BOOL       NEAR PASCAL GetDeviceConfig(PCONNENTDLG pConnEntDlg);
void       NEAR PASCAL GetPhoneNumber (HWND hWnd, LPPHONENUM lppn);
BOOL       NEAR PASCAL GetConnectionSetting(HWND hWnd, PCONNENTDLG pConnEntDlg);

DWORD      NEAR PASCAL BuildSMMList (PCONNENTDLG pConnEntDlg,
                                     PDEVCONFIG  pDevConfig);
void       NEAR PASCAL FreeSMMList (PCONNENTDLG pConnEntDlg);

void       NEAR PASCAL BuildSubConnList (PCONNENTDLG pConnEntDlg);
DWORD      NEAR PASCAL SaveSubConnList (PCONNENTDLG pConnEntDlg);
void       NEAR PASCAL FreeSubConnList (PCONNENTDLG pConnEntDlg);
