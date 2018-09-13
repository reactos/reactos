#ifndef _EALOAD_H
#define _EALOAD_H

// Primary Module Parameters
//---------------------------
// "ParameterMessageFile" param flag
#define PMP_PARAMMSGFILE    0x00000001

typedef struct
{
    DWORD   dwParams;   // bitmask of PMP_* values, identifying the valid parameters in the structure
    HMODULE hModule;    // place holder for the "ParameterMessageFile" param
} tPrimaryModuleParms;

// Returns in 'Params' the requested parameters (identified by Params.dwParams field)
DWORD LoadPrimaryModuleParams(
         IN  HKEY hkLogFile,               // opened registry key to HKLM\System\CurrentControlSet\Services\EventLog\<LogFile>
         IN  LPCTSTR tchPrimModule,        // name of the PrimaryModule as it is defined in the "PrimaryModule" value of the key above
         OUT tPrimaryModuleParms &Params); // allocated output buffer, ready to receive the values for the requested parameters

#endif
