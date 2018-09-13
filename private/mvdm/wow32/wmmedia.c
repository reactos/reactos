/*---------------------------------------------------------------------*\
*
*  WOW v1.0
*
*  Copyright (c) 1991, Microsoft Corporation
*
*  WMMEDIA.C
*  WOW32 16-bit MultiMedia API support
*
*  Contains:
*       General support apis
*       Timer support apis
*       MCI apis
*
*  History:
*  Created 21-Jan-1992 by Mike Tricker (MikeTri), after jeffpar
*  Changed 15-Jul-1992 by Mike Tricker (MikeTri), fixing GetDevCaps calls
*          26-Jul-1992 by Stephen Estrop (StephenE) thunks for mciSendCommand
*          30-Jul-1992 by Mike Tricker (MikeTri), fixing Wave/Midi/MMIO
*          03-Aug-1992 by Mike Tricker (MikeTri), added proper error handling
*          08-Oct-1992 by StephenE used correct thunk macro for UINT's
*                      also split file into 3 because it was getting to big.
*
\*---------------------------------------------------------------------*/

//
// We define NO_STRICT so that the compiler doesn't moan and groan when
// I use the FARPROC type for the Multi-Media api loading.
//
#define NO_STRICT
#define OEMRESOURCE

#include "precomp.h"
#pragma hdrstop
#include <stdlib.h>







MODNAME(wmmedia.c);

PCALLBACK_DATA      pCallBackData;  // A 32 bit ptr to the 16 bit callback data
CRITICAL_SECTION    mmCriticalSection;
CRITICAL_SECTION    mmHandleCriticalSection;

//
// All this stuff is required for the dynamic linking of Multi-Media to WOW32
//
HANDLE       hWinmm              = NULL;
FARPROC      mmAPIEatCmdEntry    = NULL;
FARPROC      mmAPIGetParamSize   = NULL;
FARPROC      mmAPIUnlockCmdTable = NULL;
FARPROC      mmAPISendCmdW       = NULL;
FARPROC      mmAPIFindCmdItem    = NULL;
FARPROC      mmAPIGetYieldProc   = NULL;

VOID FASTCALL Set_MultiMedia_16bit_Directory( PVDMFRAME pFrame );


/*++

 GENERIC FUNCTION PROTOTYPE:
 ==========================

ULONG FASTCALL WMM32<function name>(PVDMFRAME pFrame)
{
    ULONG ul;
    register P<function name>16 parg16;

    GETARGPTR(pFrame, sizeof(<function name>16), parg16);

    <get any other required pointers into 16 bit space>

    ALLOCVDMPTR
    GETVDMPTR
    GETMISCPTR
    et cetera

    <copy any complex structures from 16 bit -> 32 bit space>
    <ALWAYS use the FETCHxxx macros>

    ul = GET<return type>16(<function name>(parg16->f1,
                                                :
                                                :
                                            parg16->f<n>);

    <copy any complex structures from 32 -> 16 bit space>
    <ALWAYS use the STORExxx macros>

    <free any pointers to 16 bit space you previously got>

    <flush any areas of 16 bit memory if they were written to>

    FLUSHVDMPTR

    FREEARGPTR(parg16);
    RETURN(ul);
}

NOTE:

  The VDM frame is automatically set up, with all the function parameters
  available via parg16->f<number>.

  Handles must ALWAYS be mapped for 16 -> 32 -> 16 space via the mapping tables
  laid out in WALIAS.C.

  Any storage you allocate must be freed (eventually...).

  Further to that - if a thunk which allocates memory fails in the 32 bit call
  then it must free that memory.

  Also, never update structures in 16 bit land if the 32 bit call fails.

--*/


/* ---------------------------------------------------------------------
** General Support API's
** ---------------------------------------------------------------------
*/

/*****************************Private*Routine******************************\
* WMM32CallProc32
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
ULONG FASTCALL
WMM32CallProc32(
    PVDMFRAME pFrame
    )
{
    register DWORD  dwReturn;
    PMMCALLPROC3216 parg16;


    GETARGPTR(pFrame, sizeof(PMMCALLPROC32), parg16);


    // Don't call to Zero

    if (parg16->lpProcAddress == 0) {
        LOGDEBUG(LOG_ALWAYS,("MMCallProc32 - Error calling to 0 not allowed"));
        return(0);
    }

    //
    // Make sure we have the correct 16 bit directory set.
    //
    if (parg16->fSetCurrentDirectory != 0) {

            UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    }


    dwReturn = ((FARPROC)parg16->lpProcAddress)( parg16->p5, parg16->p4,
                                                 parg16->p3, parg16->p2,
                                                 parg16->p1);


    FREEARGPTR(parg16);
    return dwReturn;
}


/******************************Public*Routine******************************\
* WOW32ResolveMemory
*
* Enable multi-media (and others) to reliably map memory from 16 bit land
* to 32 bit land.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
LPVOID APIENTRY
WOW32ResolveMemory(
    VPVOID  vp
    )
{
    LPVOID  lpReturn;

    GETMISCPTR( vp, lpReturn );
    return lpReturn;
}


/**********************************************************************\
* WOW32ResolveHandle
*
* This is a general purpose handle mapping function.  It allows WOW thunk
* extensions to get access to 32 bit handles given a 16 bit handle.
*
\**********************************************************************/
BOOL APIENTRY WOW32ResolveHandle( UINT uHandleType, UINT uMappingDirection,
                                  WORD wHandle16_In, LPWORD lpwHandle16_Out,
                                  DWORD dwHandle32_In, LPDWORD lpdwHandle32_Out )
{
    BOOL                fReturn = FALSE;
    DWORD               dwHandle32;
    WORD                wHandle16;
    static   FARPROC    mmAPI = NULL;

    GET_MULTIMEDIA_API( "WOW32ResolveMultiMediaHandle", mmAPI,
                        MMSYSERR_NODRIVER );

    if ( uMappingDirection == WOW32_DIR_16IN_32OUT ) {

        switch ( uHandleType ) {

        case WOW32_USER_HANDLE:
            dwHandle32 = (DWORD)USER32( wHandle16_In );
            break;


        case WOW32_GDI_HANDLE:
            dwHandle32 = (DWORD)GDI32( wHandle16_In );
            break;


        case WOW32_WAVEIN_HANDLE:
        case WOW32_WAVEOUT_HANDLE:
        case WOW32_MIDIOUT_HANDLE:
        case WOW32_MIDIIN_HANDLE:
            (*mmAPI)( uHandleType, uMappingDirection, wHandle16_In,
                      lpwHandle16_Out, dwHandle32_In, lpdwHandle32_Out );
            dwHandle32 = 0;
            fReturn = TRUE;
            break;
        }

        /*
        ** Protect ourself from being given a duff pointer.
        */
        try {

            if ( dwHandle32 ) {

                if ( *lpdwHandle32_Out = dwHandle32 ) {
                    fReturn = TRUE;
                }
                else {
                    fReturn = FALSE;
                }

            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {
            fReturn = FALSE;
        }
    }
    else if ( uMappingDirection == WOW32_DIR_32IN_16OUT ) {

        switch ( uHandleType ) {

        case WOW32_USER_HANDLE:
            wHandle16 = (WORD)USER16( dwHandle32_In );
            break;


        case WOW32_GDI_HANDLE:
            wHandle16 = (WORD)GDI16( dwHandle32_In );
            break;


        case WOW32_WAVEIN_HANDLE:
        case WOW32_WAVEOUT_HANDLE:
        case WOW32_MIDIOUT_HANDLE:
        case WOW32_MIDIIN_HANDLE:
            (*mmAPI)( uHandleType, uMappingDirection, wHandle16_In,
                      lpwHandle16_Out, dwHandle32_In, lpdwHandle32_Out );
            wHandle16 = 0;
            fReturn = TRUE;
            break;
        }

        /*
        ** Protect ourself from being given a duff pointer.
        */
        try {
            if ( wHandle16 ) {
                if ( *lpwHandle16_Out = wHandle16 ) {
                    fReturn = TRUE;
                }
                else {
                    fReturn = FALSE;
                }
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {
            fReturn = FALSE;
        }
    }
    return fReturn;
}


/**********************************************************************\
*
* WOW32DriverCallback
*
* Callback stub, which invokes the "real" 16 bit callback.
* The parameters to this function must be in the format that the 16 bit
* code expects,  i.e. all handles must be 16 bit handles, all addresses must
* be 16:16 ones.
*
*
* It is possible that this function will have been called with the
* DCB_WINDOW set in which case the 16 bit interrupt handler will call
* PostMessage.  Howver, it is much more efficient if PostMessage is called
* from the 32 bit side.
*
\**********************************************************************/
BOOL APIENTRY WOW32DriverCallback( DWORD dwCallback, DWORD dwFlags,
                                   WORD wID, WORD wMsg,
                                   DWORD dwUser, DWORD dw1, DWORD dw2 )
{
    static   FARPROC    mmAPI = NULL;

    GET_MULTIMEDIA_API( "WOW32DriverCallback", mmAPI, MMSYSERR_NODRIVER );

    /*
    ** Just pass the call onto winmm
    */
    return (*mmAPI)( dwCallback, dwFlags, wID, wMsg, dwUser, dw1, dw2 );
}


/**********************************************************************\
*
* Get_MultiMedia_ProcAddress
*
* This function gets the address of the given Multi-Media api.  It loads
* Winmm.dll if this it has not already been loaded.
*
\**********************************************************************/
FARPROC Get_MultiMedia_ProcAddress( LPSTR lpstrProcName )
{
    /*
    ** Either this is the first time this function has been called
    ** or the Multi-Media sub-system is in a bad way.
    */
    if ( hWinmm == NULL ) {

        // dprintf2(( "Attempting to load WINMM.DLL" ));
        hWinmm = SafeLoadLibrary( "WINMM.DLL" );

        if ( hWinmm == NULL ) {

            /* Looks like the Multi-Media sub-system is in a bad way */
            // dprintf2(( "FAILED TO LOAD WINMM.DLL!!" ));
            return NULL;
        }

    }

    return GetProcAddress( hWinmm, lpstrProcName );

}
