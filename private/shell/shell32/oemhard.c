#include "shellprv.h"
#pragma  hdrstop

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif


#if defined(_X86_)

void RegGetMachineIdentifierValue(IN OUT PULONG Value)
{
    TCHAR  tchData[256];
    PTCHAR ptchData = tchData;
    DWORD  dwData = sizeof(tchData);
    DWORD  dwType;

    //
    // Set default as PC/AT
    //
    *Value = MACHINEID_MS_PCAT;
    //
    // Open registry key
    //
    if (ERROR_SUCCESS == 
        SHGetValue(HKEY_LOCAL_MACHINE, REGISTRY_HARDWARE_SYSTEM, 
                   REGISTRY_MACHINE_IDENTIFIER, &dwType, ptchData, &dwData) )
   {    
        //
        // Determine platform.
        //
        if ( lstrcmpi(ptchData, FUJITSU_FMR_NAME) == 0 )
        {
            //
            // Fujitsu FMR Series.
            //
            *Value = MACHINEID_FUJITSU_FMR;
        } 
        else if ( lstrcmpi(ptchData, NEC_PC98_NAME) == 0 )
        {
            //
            // NEC PC-9800 Seriss
            //
            *Value = MACHINEID_NEC_PC98;
        } 
    }
}

DWORD dwMachineId = (DWORD)-1;

STDAPI_(BOOL) IsNEC_PC9800(VOID)
{
    if (dwMachineId == (DWORD)-1)
    {
        if (g_uCodePage == 932)
            RegGetMachineIdentifierValue(&dwMachineId);
        else
            dwMachineId = MACHINEID_MS_PCAT;
    }

    return (dwMachineId & PC_9800_COMPATIBLE) ? TRUE : FALSE;
}
#endif // defined(_X86_)
