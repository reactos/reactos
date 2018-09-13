
/*************************************************************************
*
* muext.c
*
* HYDRA EXPLORER Multi-User extensions
*
* copyright notice: Copyright 1997, Microsoft
*
*
*************************************************************************/

/*
 *  Includes
 */
#include "cabinet.h"
#include "rcids.h"
#include "startmnu.h"

#ifdef WINNT // this file is hydra specific

#include <winsta.h>

/*=============================================================================
==   defines
=============================================================================*/

/*=============================================================================
==   Global data
=============================================================================*/

typedef BOOLEAN (*PWINSTATION_SET_INFORMATION) (
                                                HANDLE hServer,
                                                ULONG SessionId,
                                                WINSTATIONINFOCLASS WinStationInformationClass,
                                                PVOID  pWinStationInformation,
                                                ULONG WinStationInformationLength
                                                );

//****************************************

BOOL
InitWinStationFunctionsPtrs()

{
    return TRUE;
}

/*****************************************************************************
 *
 *  MuSecurity
 *
 *  Invoke security dialogue box
 *
 * ENTRY:
 *   nothing
 *
 * EXIT:
 *   nothing
 *
 ****************************************************************************/

VOID
MuSecurity( VOID )
{
    static PWINSTATION_SET_INFORMATION pfnWinStationSetInformation = NULL;
    
    //
    // Do nothing on the console
    //

    if (IsRemoteSession())
    {
        if (pfnWinStationSetInformation == NULL)
        {
            HANDLE          dllHandle;

            //
            // Load winsta.dll
            //
            dllHandle = LoadLibraryW(L"winsta.dll");
            if (dllHandle != NULL) 
            {
                //WinStationSetInformationW
                pfnWinStationSetInformation = (PWINSTATION_SET_INFORMATION) GetProcAddress(dllHandle, "WinStationSetInformationW");
            }
        }

        if (pfnWinStationSetInformation != NULL)
        {
            pfnWinStationSetInformation( SERVERNAME_CURRENT,
                                          LOGONID_CURRENT,
                                          WinStationNtSecurity,
                                          NULL, 0 );
        }
        else
        {
            // It should never happen. What shall we do ?
            // How about assert false?
            ASSERT(FALSE);
        }
    }
}

#endif // WINNT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   