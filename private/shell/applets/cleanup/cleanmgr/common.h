/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Manager
** File:    common.h
**
** Purpose: various common stuff for this module
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/
#ifndef COMMON_H
#define COMMON_H


/*
**------------------------------------------------------------------------------
**  Microsoft C++ include files
**------------------------------------------------------------------------------
*/
#ifndef STRICT
    #define STRICT
#endif

#define INC_OLE2           // WIN32, get ole2 from windows.h

#include <windows.h>
#include <windowsx.h>

#include <shlobj.h>

#include <shlwapi.h>
#include <shlwapip.h>

#include <initguid.h>
#include <regstr.h>

#include "resource.h"

#include "cmstrings.h"
#include "ids.h"

#ifdef _DEBUG
   #define DEBUG
#endif


#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

#define FLAG_SAGESET                0x00000001
#define FLAG_SAGERUN                0x00000002
#define FLAG_TUNEUP                 0x00000004
#define FLAG_SAVE_STATE             0x80000000

#define STATE_SELECTED              0x00000001
#define STATE_SAGE_SELECTED         0x00000002

#define RETURN_SUCCESS              0x00000001
#define RETURN_CLEANER_FAILED       0x00000002
#define RETURN_USER_CANCELED_SCAN   0x00000003
#define RETURN_USER_CANCELED_PURGE  0x00000004

#ifdef NEC_98
#define SZ_DEFAULT_DRIVE            TEXT("A:\\")
#else
#define SZ_DEFAULT_DRIVE            TEXT("C:\\")
#endif

#define SZ_CLASSNAME                TEXT("CLEANMGR")
#define SZ_STATE                    TEXT("StateFlags")
#define SZ_DEFAULTICONPATH          TEXT("CLSID\\%s\\DefaultIcon")

#define SZ_CVT1                     TEXT("\\CVT1.EXE")
#define SZ_RUNDLL32                 TEXT("RUNDLL32.EXE")
#define SZ_INSTALLED_PROGRAMS       TEXT("shell32.dll,Control_RunDLL appwiz.cpl")

#define SZ_SYSOCMGR                 TEXT("sysocmgr.exe")
#define SZ_WINDOWS_SETUP            TEXT("/i:%s\\sysoc.inf")

#define DEFAULT_PRIORITY            200

#define MAX_GUID_STRING_LEN         39
#define INTER_ITEM_SPACE            10
#define DESCRIPTION_LENGTH          512
#define BUTTONTEXT_LENGTH           50
#define MAX_DESC_LEN                100
#define INDENT                      5
#define  cbRESOURCE                 256

extern HINSTANCE    g_hInstance;
extern HWND         g_hDlg;

#ifdef DEBUG
#define MI_TRAP                     _asm int 3

void
DebugPrint(
    HRESULT hr,
    LPCSTR  lpFormat,
    ...
    );


#define MiDebugMsg( args )          DebugPrint args

#else

#define MI_TRAP
#define MiDebugMsg( args )

#endif // DEBUG


#endif // COMMON_H
/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/
