
//========================================================================
//
//  NDDEAPI.H  include file for dde share apis
//
//  Revision History:
//  5-13-19     ClausGi created
//
//========================================================================
// tabstop = 4


#include "windows.h"
#include "nddeapi.h"


#define NDDE_NOT_RUNNING	 	      18

BOOL WINAPI
NDdeIsValidPassword (
    LPTSTR  password    // name to check for validity
)
{
    return TRUE;
}

BOOL WINAPI
NDdeIsValidTopic (
    LPTSTR  targetTopic // name to check for validity
)
{
    return TRUE;
}


HWND WINAPI NDdeGetWindow ( VOID )
{
    return (HWND)1;
}
