/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1992,1993
 *  All Rights Reserved.
 *
 *
 *  PIFINFP.H
 *  Private PIFMGR include file
 *
 *  History:
 *  Created 22-Mar-1993 2:58pm by Jeff Parsons
 */


/*
 *  APPS.INF [pif95] section fields
 */
#define APPSINF_FILENAME        0       //
#define APPSINF_TITLE           1       //
#define APPSINF_ICONFILE        2       // default is APPSINF_DEFAULT_ICONFILE
#define APPSINF_ICONINDEX       3       //
#define APPSINF_NOWORKDIR       4       //
#define APPSINF_SECTIONID       5       //
#define APPSINF_OTHERFILE       6       //
#define APPSINF_NOPIF           7       //

#define APPSINF_DEFAULT_SECTION  TEXT("default")


/*
 *  APPS.INF section key IDs
 */
#define APPSINF_KEY             0       // field number

#define KEY_PARAMS              TEXT("params")
#define KEY_BATCHFILE           TEXT("batchfile")
#define KEY_LOWMEM              TEXT("lowmem")
#define KEY_EMSMEM              TEXT("emsmem")
#define KEY_XMSMEM              TEXT("xmsmem")
#define KEY_DPMIMEM             TEXT("dpmimem")
#define KEY_ENABLE              TEXT("enable")
#define KEY_DISABLE             TEXT("disable")

#define MAX_KEY_SIZE            16

#define KEYID_UNKNOWN           -1
#define KEYID_NONE              0
#define KEYID_PARAMS            1
#define KEYID_BATCHFILE         2
#define KEYID_LOWMEM            3
#define KEYID_EMSMEM            4
#define KEYID_XMSMEM            5
#define KEYID_DPMIMEM           6
#define KEYID_ENABLE            7
#define KEYID_DISABLE           8


/*
 *  APPS.INF string switches used to set PIF options
 */
#define APPSINF_KEYVAL          1       // field number
#define APPSINF_KEYVAL2         2       // field number

#define KEYVAL_WINDOWED         TEXT("win")   // (formerly DISPUSAGE)
#define KEYVAL_BACKGROUND       TEXT("bgd")   // (formerly EXECFLAGS)
#define KEYVAL_EXCLUSIVE        TEXT("exc")   // (formerly EXECFLAGS)
#define KEYVAL_DETECTIDLE       TEXT("dit")   // (formerly PROCMEMFLAGS)
#define KEYVAL_LOWLOCKED        TEXT("lml")   // (formerly PROCMEMFLAGS:lam)
#define KEYVAL_EMSLOCKED        TEXT("eml")   // (formerly PROCMEMFLAGS)
#define KEYVAL_XMSLOCKED        TEXT("xml")   // (formerly PROCMEMFLAGS)
#define KEYVAL_USEHMA           TEXT("hma")   // (formerly PROCMEMFLAGS)
#define KEYVAL_EMULATEROM       TEXT("emt")   // (formerly DISPFLAGS)
#define KEYVAL_RETAINVRAM       TEXT("rvm")   // (formerly DISPFLAGS)
#define KEYVAL_FASTPASTE        TEXT("afp")   // (formerly OTHEROPTIONS)
#define KEYVAL_ALTTAB           TEXT("ata")   // (formerly OTHEROPTIONS)
#define KEYVAL_ALTESC           TEXT("aes")   // (formerly OTHEROPTIONS)
#define KEYVAL_CTRLESC          TEXT("ces")   // (formerly OTHEROPTIONS)
#define KEYVAL_PRTSCRN          TEXT("psc")   // (formerly OTHEROPTIONS)
#define KEYVAL_ALTPRTSCRN       TEXT("aps")   // (formerly OTHEROPTIONS)
#define KEYVAL_ALTSPACE         TEXT("asp")   // (formerly OTHEROPTIONS)
#define KEYVAL_ALTENTER         TEXT("aen")   // (formerly OTHEROPTIONS)
#define KEYVAL_WINLIE           TEXT("lie")   // (NEW)
#define KEYVAL_GLOBALMEM        TEXT("gmp")   // (NEW)
#define KEYVAL_REALMODE         TEXT("dos")   // (NEW)
#define KEYVAL_MOUSE            TEXT("mse")   // (NEW)
#define KEYVAL_EMS              TEXT("ems")   // (NEW)
#define KEYVAL_CDROM            TEXT("cdr")   // (NEW)
#define KEYVAL_NETWORK          TEXT("net")   // (NEW)
#define KEYVAL_DISKLOCK         TEXT("dsk")   // (NEW)
#define KEYVAL_PRIVATECFG       TEXT("cfg")   // (NEW)
#define KEYVAL_CLOSEONEXIT      TEXT("cwe")   // (NEW)
#define KEYVAL_ALLOWSSAVER      TEXT("sav")     // (NEW)
#define KEYVAL_UNIQUESETTINGS   TEXT("uus")     // (NEW)
#ifdef LATER
#define KEYVAL_DISPLAYTBAR      TEXT("dtb")   // (NEW)
#define KEYVAL_RESTOREWIN       TEXT("rws")   // (NEW)
#define KEYVAL_QUICKEDIT        TEXT("qme")   // (NEW)
#define KEYVAL_EXCLMOUSE        TEXT("exm")   // (NEW)
#define KEYVAL_WARNIFACTIVE     TEXT("wia")   // (NEW)
#endif

#define MAX_KEYVAL_SIZE         6

#define KEYVAL_ID_UNKNOWN       -1
#define KEYVAL_ID_NONE          0
#define KEYVAL_ID_WINDOWED      1
#define KEYVAL_ID_BACKGROUND    2
#define KEYVAL_ID_EXCLUSIVE     3
#define KEYVAL_ID_DETECTIDLE    4
#define KEYVAL_ID_LOWLOCKED     5
#define KEYVAL_ID_EMSLOCKED     6
#define KEYVAL_ID_XMSLOCKED     7
#define KEYVAL_ID_USEHMA        8
#define KEYVAL_ID_EMULATEROM    9
#define KEYVAL_ID_RETAINVRAM    10
#define KEYVAL_ID_FASTPASTE     11
#define KEYVAL_ID_ALTTAB        12
#define KEYVAL_ID_ALTESC        13
#define KEYVAL_ID_CTRLESC       14
#define KEYVAL_ID_PRTSCRN       15
#define KEYVAL_ID_ALTPRTSCRN    16
#define KEYVAL_ID_ALTSPACE      17
#define KEYVAL_ID_ALTENTER      18
#define KEYVAL_ID_WINLIE        19
#define KEYVAL_ID_GLOBALMEM     20
#define KEYVAL_ID_REALMODE      21
#define KEYVAL_ID_MOUSE         22
#define KEYVAL_ID_EMS           23
#define KEYVAL_ID_CDROM         24
#define KEYVAL_ID_NETWORK       25
#define KEYVAL_ID_DISKLOCK      26
#define KEYVAL_ID_PRIVATECFG    27
#define KEYVAL_ID_CLOSEONEXIT   28
#define KEYVAL_ID_ALLOWSSAVER   29
#define KEYVAL_ID_UNIQUESETTINGS 30
#ifdef LATER
#define KEYVAL_ID_DISPLAYTBAR   31
#define KEYVAL_ID_RESTOREWIN    32
#define KEYVAL_ID_QUICKEDIT     33
#define KEYVAL_ID_EXCLMOUSE     34
#define KEYVAL_ID_WARNIFACTIVE  35
#endif


/*
 *  Internal function prototypes
 */

#include <setupapi.h>

#ifdef UNICODE
BOOL GetAppsInfData(PPROPLINK ppl, LPPROPPRG lpPrg, LPPROPNT40 lpnt40, HINF hInf, LPCTSTR lpszApp, BOOL fNotAmbiguous, int flOpt);
#else
BOOL GetAppsInfData(PPROPLINK ppl, LPPROPPRG lpPrg, HINF hInf, LPCTSTR lpszApp, BOOL fNotAmbiguous, int flOpt);
#endif
void GetAppsInfSectionData(PINFCONTEXT pInfContext, LPCTSTR lpszSection, PPROPLINK ppl);
int  GetKeyID(PINFCONTEXT pInfContext);
int  GetKeyValID(PINFCONTEXT pInfContext, int i);

#ifdef UNICODE
void InitWorkDir(PPROPLINK ppl, LPPROPPRG lpPrg, LPPROPNT40 lpnt40);
#else
void InitWorkDir(PPROPLINK ppl, LPPROPPRG lpPrg);
#endif
