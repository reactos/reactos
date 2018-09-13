
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    proj.h

Abstract:

    Battery Class Installer header

Author:

    Scott Brenden

Environment:

Notes:


Revision History:

--*/

#include <windows.h>
#include <windowsx.h>

#ifdef WIN95
#include <setupx.h>         // PnP setup/installer services
#else
#include <setupapi.h>       // PnP setup/installer services
#include <cfgmgr32.h>
#endif


//
// Debug stuff
//

#if DBG > 0 && !defined(DEBUG)
#define DEBUG
#endif

#if DBG > 0 && !defined(FULL_DEBUG)
#define FULL_DEBUG
#endif


#define DEBUG_PRINT_BUFFER_LEN      1030
#define MAX_BUF                     260


//
// Trace flags
//

#define TF_WARNING          0x00000001
#define TF_ERROR            0x00000002
#define TF_GENERAL          0x00000004      // Standard messages
#define TF_FUNC             0x00000008      // Trace function calls




//
// Calling declarations
//
#define PUBLIC                      FAR PASCAL
#define CPUBLIC                     FAR CDECL
#define PRIVATE                     NEAR PASCAL


#ifdef DEBUG

void    
CPUBLIC 
CommonDebugMsgW(
    DWORD mask, 
    LPCSTR pszMsg, 
    ...
    );


void    
CPUBLIC 
CommonDebugMsgA(
    DWORD mask, 
    LPCSTR pszMsg, 
    ...
    );

#ifdef UNICODE
#define TRACE_MSG   CommonDebugMsgW
#else
#define TRACE_MSG   CommonDebugMsgA
#endif

extern DWORD    BattDebugPrintLevel;

#else   // !defined(DEBUG)

#define TRACE_MSG

#endif



//
// Prototypes
//

DWORD
PRIVATE
InstallCompositeBattery (
    IN     HDEVINFO                DevInfoHandle,
    IN     PSP_DEVINFO_DATA        DevInfoData,         OPTIONAL
    IN OUT PSP_DEVINSTALL_PARAMS   DevInstallParams
    );


