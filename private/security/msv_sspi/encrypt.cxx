/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    encrypt.cxx

Abstract:

    Contains routine to check whether encryption is supported on this
    system or not.

Author:

    Mike Swift (MikeSw) 2-Aug-1994

Revision History:

    ChandanS  03-Aug-1996 Stolen from net\svcdlls\ntlmssp\common\encrypt.c
--*/

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <rpc.h>

extern "C"
BOOLEAN
IsEncryptionPermitted(VOID)
/*++

Routine Description:

    This routine checks whether encryption is getting the system default
    LCID and checking whether the country code is CTRY_FRANCE.

Arguments:

    none


Return Value:

    TRUE - encryption is permitted
    FALSE - encryption is not permitted


--*/

{

//
// sfield: permission to remove FRANCE check obtained 08-21-1999
//

#if 0
    LCID DefaultLcid;
    WCHAR CountryCode[10];
    ULONG CountryValue;

    DefaultLcid = GetSystemDefaultLCID();

    //
    // Check if the default language is Standard French
    //

    if (LANGIDFROMLCID(DefaultLcid) == 0x40c) {
        return(FALSE);
    }

    //
    // Check if the users's country is set to FRANCE
    //

    if (GetLocaleInfo(DefaultLcid,LOCALE_ICOUNTRY,CountryCode,10) == 0) {
        return(FALSE);
    }
    CountryValue = (ULONG) wcstol(CountryCode,NULL,10);
    if (CountryValue == CTRY_FRANCE) {
        return(FALSE);
    }
#endif

    return(TRUE);
}

