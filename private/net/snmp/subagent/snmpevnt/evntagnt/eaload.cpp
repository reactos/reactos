#include <windows.h>        // windows definitions
#include <snmp.h>           // snmp definitions
#include "snmpelea.h"       // global dll definitions
#include "snmpelmg.h"
#include "snmpelep.h"
#include "EALoad.h"

// Returns in 'Params' the requested parameters (identified by Params.dwParams field)
DWORD LoadPrimaryModuleParams(
         IN  HKEY hkLogFile,               // opened registry key to HKLM\System\CurrentControlSet\Services\EventLog\<LogFile>
         IN  LPCTSTR tchPrimModule,        // name of the PrimaryModule as it is defined in the "PrimaryModule" value of the key above
         OUT tPrimaryModuleParms &Params)  // allocated output buffer, ready to receive the values for the requested parameters
{
    DWORD retCode;
    HKEY  hkPrimaryModule;

    // open the 'HKLM\SYSTEM\CurrentControlSet\Services\EventLog\<LogFile\<PrimaryModule>' registry key
    retCode = RegOpenKeyEx(
				hkLogFile,
				tchPrimModule,
				0,
				KEY_READ,
				&hkPrimaryModule);
    if (retCode != ERROR_SUCCESS)
        return retCode;

    if (Params.dwParams & PMP_PARAMMSGFILE) // "ParameterMessageFile" is requested
    {
        DWORD dwType;
        TCHAR tszParamMsgFileName[MAX_PATH+1];
        DWORD dwParamMsgFileLen = MAX_PATH+1;

        // get the 'ParameterMessageFile' value from the '<PrimaryModule>' key
		retCode = RegQueryValueEx(
				    hkPrimaryModule,
				    EXTENSION_PARM_MODULE,
				    0,
				    &dwType,
				    (LPBYTE) tszParamMsgFileName,
				    &dwParamMsgFileLen);
        if (retCode == ERROR_SUCCESS)
        {
            TCHAR tszExpandedFileName[MAX_PATH+1];

            if (ExpandEnvironmentStrings(
                    tszParamMsgFileName,
                    tszExpandedFileName,
                    MAX_PATH+1) > 0)
            {
                Params.hModule = (HMODULE) LoadLibraryEx(tszExpandedFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);
                if (Params.hModule == NULL)
                {
                    retCode = GetLastError();
                    WriteLog(SNMPELEA_CANT_LOAD_PRIM_DLL, tszParamMsgFileName, retCode);
                }
            }
            else
                retCode = GetLastError();
        }
    }

    RegCloseKey(hkPrimaryModule);

    return retCode;
}
