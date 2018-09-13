/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    tllp.c

Abstract:

    This implements the local transport layer for OSDebug versions
    2 and 4 on Win32.

Author:

    Jim Schaad (jimsch)
    Kent Forschmiedt (kentf)


--*/
#ifdef WIN32
#include <windows.h>
#endif

#include <stdlib.h>

#include <string.h>
#include <memory.h>

#include "tchar.h"

#include "cvinfo.h"

#include "odtypes.h"

#include "od.h"
#include "odp.h"
#include "odassert.h"
#include "emdm.h"

#include "dbgver.h"
extern AVS Avs;

#ifndef CVWS
int _acrtused = 0;
#endif


// debug monitor function definitions.

#ifdef WIN32
LPDMINIT        LpDmInit;
LPDMFUNC        LpDmFunc;
LPDMDLLINIT     LpDmDllInit;
void FAR PASCAL LOADDS DMInit (DMTLFUNCTYPE, LPVOID);
#else
void FAR PASCAL LOADDS DMInit (DMTLFUNCTYPE);
#endif

LPDBF lpdbf = (LPDBF)0;         // the debugger helper functions
LOCAL TLCALLBACKTYPE TLCallBack;    // central osdebug callback function

DWORD tls_index_DM = 0;
DWORD tls_index_EM = 1;
BOOL Flag[2];

void
ClearReply(
    DWORD n
    )
{
    Flag[n] = FALSE;
}

int
TestReply(
    DWORD n
    )
{
    return Flag[n];
}

void
SetReply(
    DWORD n
    )
{
    Flag[n] = TRUE;
}

#define ClearReplyDM() ClearReply(tls_index_DM)
#define TestReplyDM() TestReply(tls_index_DM)
#define SetReplyDM() SetReply(tls_index_DM)

#define ClearReplyEM() ClearReply(tls_index_EM)
#define TestReplyEM() TestReply(tls_index_EM)
#define SetReplyEM() SetReply(tls_index_EM)

XOSD TLFunc ( TLF, HPID, DWORD64, DWORD64);
XOSD DMTLFunc ( TLF, HPID, DWORD64, DWORD64);

BOOL fConDM = FALSE;
BOOL fConEM = FALSE;
BOOL fConnected = FALSE;

LPBYTE  lpbDM;
LPARAM  ibMaxDM;
LPARAM  ibDM;

LPBYTE  lpbEM;
LPARAM  ibMaxEM;
LPARAM  ibEM;

HANDLE  HDm = NULL;



TLIS Tlis = {
    FALSE,                // fCanSetup
    0xffffffff,           // dwMaxPacket
    0xffffffff,           // dwOptPacket
    TLISINFOSIZE,         // dwInfoSize ?? what is this for ??
    FALSE,                // fRemote
#if defined(_M_IX86)
    mptix86,              // mpt
    mptix86,              // mptRemote
#elif defined(_M_MRX000)
    mptmips,              // mpt
    mptmips,              // mptRemote
#elif defined(_M_ALPHA)
    mptdaxp,              // mpt
    mptdaxp,              // mptRemote
#elif defined(_M_PPC)
    mptntppc,
    mptntppc,
#elif defined(_M_IA64)
    mptia64,
    mptia64,
#else
#error( "unknown target machine" );
#endif
    {  "Local Transport Layer (LOCAL:)" } // rgchInfo
};



/**** DBGVersionCheck                                                   ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *      To export our version information to the debugger.                 *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *      NONE.                                                              *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *      Returns - A pointer to the standard version information.           *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *      Just returns a pointer to a static structure.                      *
 *                                                                         *
 ***************************************************************************/

#ifdef DEBUGVER
DEBUG_VERSION('T','L',"Local Transport Layer (Debug)")
#else
RELEASE_VERSION('T','L',"Local Transport Layer")
#endif

DBGVERSIONCHECK()



BOOL
DllVersionMatch(
    HANDLE hMod,
    LPSTR pType
    )
{
    DBGVERSIONPROC  pVerProc;
    LPAVS           pavs;

    pVerProc = (DBGVERSIONPROC)GetProcAddress(hMod, DBGVERSIONPROCNAME);
    if (!pVerProc) {
        return(FALSE);  // no version entry point
    } else {
        pavs = (*pVerProc)();

        if ((pType[0] != pavs->rgchType[0] || pType[1] != pavs->rgchType[1]) ||
          (Avs.iApiVer != pavs->iApiVer) ||
          (Avs.iApiSubVer != pavs->iApiSubVer)) {
            return(FALSE);
        }
    }

    return(TRUE);
}



XOSD
WINAPI
TLFunc (
    TLF wCommand,
    HPID hpid,
    DWORD64 wParam,
    DWORD64 lParam
    )

/*++

Routine Description:

    This function contains the dispatch loop for commands comming into
    the transport layer.  The address of this procedure is exported to
    users of the DLL.

Arguments:

    wCommand    - Supplies the command to be executed.

    hpid        - Supplies the hpid for which the command is to be executed.

    wParam      - Supplies information about the command.

    lParam      - Supplies information about the command.

Return Value:

    XOSD error code.  xosdNone means that no errors occured.  Other error
    codes are defined in osdebug\include\od.h.

--*/

{
    XOSD xosd = xosdNone;

    Unreferenced( hpid );

    switch ( wCommand ) {

    case tlfInit:

        lpdbf = (LPDBF) wParam;
        TLCallBack = (TLCALLBACKTYPE) lParam;
        xosd = xosdNone;
        break;

    case tlfLoadDM:

#if defined(DOLPHIN) // Don't reload on quick restart
        if (HDm != NULL) {
        } else
#endif
        HDm = LoadLibrary(((LPLOADDMSTRUCT)lParam)->lpDmName);
        if (HDm == NULL) {
            xosd = xosdUnknown;
            break;
        }


        // Do DM dll version check here
        if (! DllVersionMatch(HDm, "DM")) {
            xosd = xosdBadVersion;
            FreeLibrary(HDm);
            HDm = NULL;
            break;
        }

        if ((LpDmInit = (LPDMINIT) GetProcAddress(HDm, "DMInit")) == NULL) {
            xosd = xosdUnknown;
            FreeLibrary(HDm);
            HDm = NULL;
            break;
        }

        if ((LpDmFunc = (LPDMFUNC) GetProcAddress(HDm, "DMFunc")) == NULL) {
            xosd = xosdUnknown;
            FreeLibrary(HDm);
            HDm = NULL;
            break;
        }

        if ((LpDmDllInit = (LPDMDLLINIT) GetProcAddress(HDm, "DmDllInit"))
                                                                  == NULL) {
            xosd = xosdUnknown;
            FreeLibrary(HDm);
            HDm = NULL;
            break;
        }

        if (LpDmDllInit(lpdbf) == FALSE) {
            xosd = xosdUnknown;
            FreeLibrary(HDm);
            HDm = NULL;
            break;
        }

        LpDmInit(DMTLFunc, (LPVOID) ((LPLOADDMSTRUCT)lParam)->lpDmParams);

        break;

    case tlfDestroy:
        if (HDm) {
            FreeLibrary(HDm);
        }

        HDm = NULL;
        break;

    case tlfGetProc:
        *((TLFUNCTYPE FAR *) lParam) = TLFunc;
        break;

    case tlfConnect:

        fConEM = TRUE;
        fConnected = fConDM;
        break;

    case tlfDisconnect:

        fConDM = fConnected = FALSE;
        break;

    case tlfSetBuffer:

        lpbDM = (LPBYTE) lParam;
        ibMaxDM = (DWORD)wParam;
        break;

    case tlfReply:

        if (!fConnected) {
                xosd = xosdLineNotConnected;
        } else {
            if ( wParam <= (DWORD64)ibMaxEM ) {
                _fmemcpy ( lpbEM, (LPBYTE) lParam, (size_t) wParam );
                ibEM = (DWORD)wParam;
            } else {
                ibEM = 0;
                xosd = xosdInvalidParameter;
            }
            SetReplyEM();
        }
        break;


    case tlfDebugPacket:

        if ( !fConnected ) {
            xosd = xosdLineNotConnected;
        }
        else {
#if    DBG
            static LPBYTE  lpb = NULL;
            static LPARAM  cb  = 0;

            if (wParam > (DWORD64)cb) {
                if (lpb != NULL) {
                    MHFree(lpb);
                }
                lpb = MHAlloc((size_t) wParam);
                cb = (DWORD)wParam;
            }
            memcpy(lpb, (char *) lParam, (size_t) wParam);
            LpDmFunc( (DWORD) wParam, lpb);
#else  // DBG
            LpDmFunc ( (DWORD) wParam, (LPBYTE) lParam );
#endif // DBG
        }
        break;

    case tlfRequest:

        if ( !fConnected ) {
                xosd = xosdLineNotConnected;
        } else {
            ibDM = 0;
            ClearReplyDM();
#if    DBG
            {
                static LPBYTE  lpb = NULL;
                static LPARAM  cb  = 0;

                if (wParam > (DWORD64)cb) {
                    if (lpb != NULL) {
                        MHFree(lpb);
                    }
                    lpb =  MHAlloc((size_t) wParam);
                    cb = (DWORD)wParam;
                }
                memcpy(lpb, (char *) lParam, (size_t) wParam);
                LpDmFunc( (DWORD) wParam, lpb);
            }
#else  // DBG
            LpDmFunc ( (DWORD) wParam, (LPBYTE) lParam );
#endif // DBG
            while (TestReplyDM() == FALSE) {
                Sleep(100);
            }
            ClearReplyDM();
        }
        break;

    case tlfGetVersion:     // don't need to do remote version check
        *((AVS*)lParam) = Avs;
        xosd = xosdNotRemote;
        break;


    case tlfGetInfo:

        _fmemcpy((LPTLIS)lParam, &Tlis, sizeof(TLIS));
        break;

    case tlfSetup:
        break;

    default:

        assert ( FALSE );
        break;
    }

    return xosd;
}                               /* TLFunc() */

//
// DMTLFunc is what the debug monitor will call when it has something
// to do.
//

XOSD
WINAPI
DMTLFunc (
    TLF wCommand,
    HPID hpid,
    DWORD64 wParam,
    DWORD64 lParam
    )
{
    XOSD xosd = xosdNone;

    switch ( wCommand ) {

        case tlfInit:

            break;

        case tlfDestroy:

            break;

        case tlfConnect:

            fConDM = TRUE;
            fConnected = fConEM;
            break;

        case tlfDisconnect:

            fConEM = fConnected = FALSE;
            break;

        case tlfSetBuffer:

            lpbEM = (LPBYTE) lParam;
            ibMaxEM = (DWORD)wParam;
            break;

        case tlfReply:
            if (!fConnected) {
                xosd = xosdLineNotConnected;
             } else {
                if ( wParam <= (DWORD64)ibMaxDM ) {
                    ibDM = (DWORD)wParam;
                    _fmemcpy ( lpbDM, (LPBYTE) lParam, (size_t) wParam );
                } else {
                    ibDM = 0;
                }
                SetReplyDM();
            }
            break;

        case tlfDebugPacket:
            if (!fConnected) {
                xosd = xosdLineNotConnected;
            } else {
#if    DBG
                static LPBYTE lpb = NULL;
                static LPARAM cb  = 0;

                if (wParam > (DWORD64)cb) {
                    if (lpb != NULL) {
                        MHFree(lpb);
                    }
                    lpb = MHAlloc((size_t) wParam);
                    cb = (DWORD)wParam;
                }

                memcpy(lpb, (char *) lParam, (size_t) wParam);
                TLCallBack( hpid, wParam, (DWORD64)lpb);
#else  // DBG
                TLCallBack ( hpid, wParam, lParam );
#endif // DBG
            }
            break;

        case tlfRequest:
            if (!fConnected) {
                xosd = xosdLineNotConnected;
            } else {
#if    DBG
                static LPBYTE  lpb = NULL;
                static LPARAM  cb  = 0;

                if (wParam > (DWORD64)cb) {
                    if (lpb != NULL) {
                        MHFree(lpb);
                    }
                    lpb = MHAlloc((size_t) wParam);
                    cb = (DWORD)wParam;
                }
                memcpy(lpb, (char *) lParam, (size_t)wParam);
                TLCallBack( hpid, wParam, (DWORD64) lpb);
#else  // DBG
                TLCallBack ( hpid, wParam, lParam );
#endif // DBG

                while (TestReplyEM() == FALSE) {
                    Sleep(100);
                }
                ClearReplyEM();

                if ( ibEM == 0 ) {
                    xosd = xosdInvalidParameter;
                }
            }
            break;

        default:

            assert ( FALSE );
            break;
    }

    return xosd;

}

int
WINAPI
DllMain(
    HANDLE hModule,
    DWORD dwReason,
    DWORD dwReserved
    )
{
        switch(dwReason) {
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
                break;

        case DLL_PROCESS_ATTACH:
                DisableThreadLibraryCalls(hModule);
                break;

        case DLL_PROCESS_DETACH:
                break;
        }
        return(TRUE);
}
