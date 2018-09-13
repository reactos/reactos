/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    intl.h

Abstract:

    This module contains the header information for the Regional Options
    applet.

Revision History:

--*/



//
//  Include Files.
//

#include <windows.h>
#include <prsht.h>
#include <prshtp.h>
#include <shellapi.h>
#include <winnls.h>
#include "intlid.h"
#include "intlsup.h"




//
//  Constant Declarations.
//

//
//  Used in string and other array declarations.
//
#define cInt_Str             10        // length of the array of int strings
#define SIZE_64              64        // frequently used buffer size
#define SIZE_128             128       // frequently used buffer size
#define SIZE_300             300       // frequently used buffer size
#define MAX_SAMPLE_SIZE      100       // limit on Sample text for display


//
//  Character constants.
//
#define CHAR_SML_D           TEXT('d')
#define CHAR_CAP_M           TEXT('M')
#define CHAR_SML_Y           TEXT('y')
#define CHAR_SML_G           TEXT('g')

#define CHAR_SML_H           TEXT('h')
#define CHAR_CAP_H           TEXT('H')
#define CHAR_SML_M           TEXT('m')
#define CHAR_SML_S           TEXT('s')
#define CHAR_SML_T           TEXT('t')

#define CHAR_NULL            TEXT('\0')
#define CHAR_QUOTE           TEXT('\'')
#define CHAR_SPACE           TEXT(' ')
#define CHAR_COMMA           TEXT(',')
#define CHAR_SEMICOLON       TEXT(';')
#define CHAR_COLON           TEXT(':')
#define CHAR_STAR            TEXT('*')
#define CHAR_HYPHEN          TEXT('-')
#define CHAR_DECIMAL         TEXT('.')
#define CHAR_INTL_CURRENCY   TEXT('¤')
#define CHAR_GRAVE           TEXT('`')

#define CHAR_ZERO            TEXT('0')
#define CHAR_NINE            TEXT('9')


//
//  Setup command line switch values.
//
#define SETUP_SWITCH_NONE    0
#define SETUP_SWITCH_R       1
#define SETUP_SWITCH_I       2


//
//  Flags to assist in updating property sheet pages once the regional locale
//  setting has changed.  As pages are updated, their process flag value is
//  deleted from the Verified_Regional_Chg variable.
//
#define INTL_CHG             0x000f    // locale has changed = sum of all pages
#define Process_Num          0x0001    // number page not yet updated
#define Process_Curr         0x0002    // currency page not yet updated
#define Process_Time         0x0004    // time page not yet updated
#define Process_Date         0x0008    // date page not yet updated


//
//  Each of these change flags will be used to update the appropriate property
//  sheet pages change word when their associated combobox notifies the
//  property sheet of a change.  The change values are used to determine which
//  locale settings must be updated.
//

//
//  Region Change.
//
#define RC_EverChg           0x0001
#define RC_SystemLocale      0x0002
#define RC_UserLocale        0x0004
#define RC_LangGroups        0x0008
#define RC_UILanguage        0x0010

//
//  Number Change.
//
#define NC_EverChg           0x0001
#define NC_DSymbol           0x0002
#define NC_NSign             0x0004
#define NC_SList             0x0008
#define NC_SThousand         0x0010
#define NC_IDigits           0x0020
#define NC_DGroup            0x0040
#define NC_LZero             0x0080
#define NC_NegFmt            0x0100
#define NC_Measure           0x0200
#define NC_NativeDigits      0x0400
#define NC_DigitSubst        0x0800

//
//  Currency Change.
//
#define CC_EverChg           0x0001
#define CC_SCurrency         0x0002
#define CC_CurrSymPos        0x0004
#define CC_NegCurrFmt        0x0008
#define CC_SMonDec           0x0010
#define CC_ICurrDigits       0x0020
#define CC_SMonThousand      0x0040
#define CC_DMonGroup         0x0080

//
//  Time Change.
//
#define TC_EverChg           0x0001
#define TC_1159              0x0002
#define TC_2359              0x0004
#define TC_STime             0x0008
#define TC_TimeFmt           0x0010
#define TC_AllChg            0x001F
#define TC_FullTime          0x0031

//
//  Date Change.
//
#define DC_EverChg           0x0001
#define DC_ShortFmt          0x0002
#define DC_LongFmt           0x0004
#define DC_SDate             0x0008
#define DC_Calendar          0x0010
#define DC_Arabic_Calendar   0x0020
#define DC_TwoDigitYearMax   0x0040




//
//  Global Variables.
//  Data that is shared betweeen the property sheets.
//

extern BOOL g_bCDROM;               // if setup from a CD-ROM

extern HANDLE g_hMutex;             // mutex handle
extern TCHAR szMutexName[];         // name of the mutex

extern HANDLE g_hEvent;             // event handle
extern TCHAR szEventName[];         // name of the event

extern TCHAR aInt_Str[cInt_Str][3]; // cInt_Str # of elements of int strings
extern TCHAR szSample_Number[];     // used for currency and number samples
extern TCHAR szNegSample_Number[];  // used for currency and number samples
extern TCHAR szTimeChars[];         // valid time characters
extern TCHAR szTCaseSwap[];         // invalid time chars to change case => valid
extern TCHAR szTLetters[];          // time NLS chars
extern TCHAR szSDateChars[];        // valid short date characters
extern TCHAR szSDCaseSwap[];        // invalid SDate chars to change case => valid
extern TCHAR szSDLetters[];         // short date NLS chars
extern TCHAR szLDateChars[];        // valid long date characters
extern TCHAR szLDCaseSwap[];        // invalid LDate chars to change case => valid
extern TCHAR szLDLetters[];         // long date NLS chars
extern TCHAR szStyleH[];            // date and time style H equivalent
extern TCHAR szStyleh[];            // date and time style h equivalent
extern TCHAR szStyleM[];            // date and time style M equivalent
extern TCHAR szStylem[];            // date and time style m equivalent
extern TCHAR szStyles[];            // date and time style s equivalent
extern TCHAR szStylet[];            // date and time style t equivalent
extern TCHAR szStyled[];            // date and time style d equivalent
extern TCHAR szStyley[];            // date and time style y equivalent
extern TCHAR szLocaleGetError[];    // shared locale info get error
extern TCHAR szIntl[];              // intl string

extern TCHAR szInvalidSDate[];      // invalid chars for date separator
extern TCHAR szInvalidSTime[];      // invalid chars for time separator

extern HINSTANCE hInstance;         // library instance
extern int Verified_Regional_Chg;   // used to determine when to verify
                                    //  regional changes in all prop sheet pgs
extern BOOL Styles_Localized;       // indicate whether or not style must be
                                    //  translated between NLS and local formats
extern LCID UserLocaleID;           // user locale
extern LCID SysLocaleID;            // system locale
extern LCID RegUserLocaleID;        // user locale stored in the registry
extern LCID RegSysLocaleID;         // system locale stored in the registry
extern BOOL bShowRtL;               // indicate if RTL date samples should be shown
extern BOOL bShowArabic;            // indicate if the other Arabic specific stuff should be shown
extern BOOL bLPKInstalled;          // if LPK is installed
extern TCHAR szSetupSourcePath[];   // buffer to hold setup source string
extern LPTSTR pSetupSourcePath;     // pointer to setup source string buffer
extern TCHAR szSetupSourcePathWithArchitecture[]; // buffer to hold setup source string with architecture-specific extension.
extern LPTSTR pSetupSourcePathWithArchitecture;   // pointer to setup source string buffer with architecture-specific extension.




//
//  Function Prototypes.
//

//
//  Callback functions for each of the propety sheet pages.
//
INT_PTR CALLBACK GeneralDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NumberDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CurrencyDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TimeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DateDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LocaleDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


//
//  In regdlg.c.
//
BOOL
Region_ValidateRegistryPreload(VOID);

void
Region_UpdateShortDate(VOID);

void
Region_DoUnattendModeSetup(
    LPCTSTR pUnattendFile,
    BOOL AllowProgressBar);


//
//  In intl.c.
//
BOOL
IsRtLLocale(
    LCID iLCID);
