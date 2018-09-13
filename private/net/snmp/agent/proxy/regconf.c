/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    regconf.c

Abstract:

    Registry configuration routines.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

// microsoft has indicated that registry requires unicode strings
#if 0
#define UNICODE
#endif

#include <windows.h>
#include <winsock.h>
#include <wsipx.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <string.h>
#include <ctype.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>

#include "regconf.h"
#include "..\common\evtlog.h"
#include "..\common\wellknow.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

CfgExtensionAgents *extAgents   = NULL;
INT                extAgentsLen = 0;

CfgPermittedManagers *permitMgrs   = NULL;
INT                  permitMgrsLen = 0;

//--------------------------- PRIVATE CONSTANTS -----------------------------

// OPENISSUE - microsoft changed this, dont know what it should be?
#define KEY_TRAVERSE 0


//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

static char *pszSnmpSrvEaKey = SNMP_REG_SRV_EAKEY;
static char *pszSnmpSrvPmKey = SNMP_REG_SRV_PMKEY;

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------


BOOL eaConfig(
    OUT CfgExtensionAgents   **extAgents,
    OUT INT *extAgentsLen)
    {
    LONG  status;
    HKEY  hkResult;
    HKEY  hkResult2;
    DWORD iValue;
    DWORD iValue2;
//    DWORD dwTitle;
    DWORD dwType;
    TCHAR dummy[MAX_PATH+1];
    DWORD dummySize;
    TCHAR value[MAX_PATH+1];
    DWORD valueSize;
    LPSTR pTemp;
    DWORD valueReqSize;
    LPSTR pTempExp;

    *extAgents = NULL;
    *extAgentsLen = 0;

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: loading extension agents.\n"));

    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszSnmpSrvEaKey,
                               0, (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                               KEY_TRAVERSE), &hkResult)) != ERROR_SUCCESS)
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d opening ExtensionAgents subkey.\n", status));
        SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pszSnmpSrvEaKey, status);
        return FALSE;
        }

    iValue = 0;

    dummySize = MAX_PATH;
    valueSize = MAX_PATH;


    while((status = RegEnumValue(hkResult, iValue, dummy, &dummySize,
                                 NULL, &dwType, (LPBYTE)value, &valueSize))
          != ERROR_NO_MORE_ITEMS)
        {
        if (status != ERROR_SUCCESS)
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d enumerating ExtensionAgents subkey.\n", status));
            SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pszSnmpSrvEaKey, status);
            RegCloseKey(hkResult);
            return FALSE;
            }

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: processing %s.\n", value));
        if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, value, 0,
                                   (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS
                              | KEY_TRAVERSE), &hkResult2)) != ERROR_SUCCESS)
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d opening registy key %s.\n", status, value));
            pTemp = value; // make pointer for event api
            SnmpSvcReportEvent(SNMP_EVENT_INVALID_EXTENSION_AGENT_KEY, 1, &pTemp, status);
            goto process_next; // ignore bogus registry key and process next one...
            }

        iValue2 = 0;

        dummySize = MAX_PATH;
        valueSize = MAX_PATH;

        while((status = RegEnumValue(hkResult2, iValue2, dummy, &dummySize,
                                     NULL, &dwType, (LPBYTE)value, &valueSize))
              != ERROR_NO_MORE_ITEMS)
            {
            if (status != ERROR_SUCCESS)
                {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d enumerating registry key %s.\n", status, value));
                pTemp = value; // make pointer for event api
                SnmpSvcReportEvent(SNMP_EVENT_INVALID_EXTENSION_AGENT_KEY, 1, &pTemp, status);
                break; // ignore bogus registry key and process next one...
                }

            if (!lstrcmpi(dummy, TEXT("Pathname")))
                {
                (*extAgentsLen)++;
                *extAgents = (CfgExtensionAgents *) SnmpUtilMemReAlloc(*extAgents,
                    (*extAgentsLen * sizeof(CfgExtensionAgents)));

                pTemp = (LPSTR)SnmpUtilMemAlloc(valueSize+1);
#ifdef UNICODE
                SnmpUtilUnicodeToAnsi(&pTemp, value, FALSE);
#else
                memcpy(pTemp, value, valueSize+1);
#endif

                valueReqSize = valueSize + 10;
                pTempExp = NULL;
                do {
                    pTempExp = (LPSTR)SnmpUtilMemReAlloc(pTempExp, valueReqSize);
                    valueSize = valueReqSize;
                    valueReqSize = ExpandEnvironmentStringsA(
                                          pTemp,
                                          pTempExp,
                                          valueSize);

                } while (valueReqSize > valueSize );
                if (valueReqSize == 0) {
                    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: error %d expanding %s.\n", GetLastError(), value));
                    (*extAgents)[(*extAgentsLen)-1].pathName = pTemp;
                } else {
                    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: processing %s.\n", pTempExp));
                    (*extAgents)[(*extAgentsLen)-1].pathName = pTempExp;
                    SnmpUtilMemFree(pTemp);
                }

                break;
                }

            dummySize = MAX_PATH;
            valueSize = MAX_PATH;

            iValue2++;
            } // end while()

        RegCloseKey(hkResult2);

process_next:

        dummySize = MAX_PATH;
        valueSize = MAX_PATH;

        iValue++;
        } // end while()

    RegCloseKey(hkResult);

    return TRUE;

    } // end eaConfig()


BOOL pmConfig(
    OUT CfgPermittedManagers **permitMgrs,
    OUT INT *permitMgrsLen)
    {
    LONG  status;
    HKEY  hkResult;
    DWORD iValue;
//    DWORD dwTitle;
    DWORD dwType;
    TCHAR dummy[MAX_PATH+1];
    DWORD dummySize;
    TCHAR value[MAX_PATH+1];
    DWORD valueSize;
    LPSTR pTemp;

    *permitMgrs = NULL;
    *permitMgrsLen = 0;

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: loading permitted managers.\n"));

    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszSnmpSrvPmKey,
                               0, (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                               KEY_TRAVERSE), &hkResult)) != ERROR_SUCCESS)
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d opening PermittedManagers subkey.\n", status));
        SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pszSnmpSrvPmKey, status);
        return FALSE;
        }

    iValue = 0;

    dummySize = MAX_PATH;
    valueSize = MAX_PATH;

    while((status = RegEnumValue(hkResult, iValue, dummy, &dummySize,
                                 NULL, &dwType, (LPBYTE)value, &valueSize))
          != ERROR_NO_MORE_ITEMS)
        {
        if (status != ERROR_SUCCESS)
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d enumerating PermittedManagers subkey.\n", status));
            SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pszSnmpSrvPmKey, status);
            RegCloseKey(hkResult);
            return FALSE;
            }

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: processing permitted manager %s\n", value));
        (*permitMgrsLen)++;
        *permitMgrs = (CfgPermittedManagers *)SnmpUtilMemReAlloc(*permitMgrs,
            (*permitMgrsLen * sizeof(CfgPermittedManagers)));

        pTemp = (LPSTR)SnmpUtilMemAlloc(valueSize+1);
        value[valueSize] = TEXT('\0');
#ifdef UNICODE
        SnmpUtilUnicodeToAnsi(&pTemp, value, FALSE);
#else
        memcpy(pTemp, value, valueSize+1);
#endif

        (*permitMgrs)[iValue].addrText = pTemp;
        SnmpSvcAddrToSocket((*permitMgrs)[iValue].addrText,
                     &((*permitMgrs)[iValue].addrEncoding));

        dummySize = MAX_PATH;
        valueSize = MAX_PATH;

        iValue++;
        }

    RegCloseKey(hkResult);

    return TRUE;

    } // end pmConfig()


//--------------------------- PUBLIC PROCEDURES -----------------------------


BOOL regconf(VOID)
    {
    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: reading registry parameters.\n"));
    if (pmConfig(&permitMgrs, &permitMgrsLen) &&
        eaConfig(&extAgents,  &extAgentsLen)) {

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: registry parameters successfully read.\n"));
        return TRUE;
    }
    else
    {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: unable to successfully read registry parameters.\n"));
        return FALSE;
    }

    } // end regconf()


//-------------------------------- END --------------------------------------
