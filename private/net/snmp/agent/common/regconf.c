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
#include "evtlog.h"
#include "wellknow.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

BOOL                enableAuthTraps = FALSE;

CfgTrapDestinations *trapDests   = NULL;
INT                 trapDestsLen = 0;

CfgValidCommunities *validComms   = NULL;
INT                 validCommsLen = 0;

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define KEY_TRAVERSE 0

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

static char *pszSnmpSrvTeKey = SNMP_REG_SRV_TEKEY;
static char *pszSnmpSrvTdKey = SNMP_REG_SRV_TDKEY;
static char *pszSnmpSrvVcKey = SNMP_REG_SRV_VCKEY;

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

// configure trap destinations
BOOL tdConfig(
    OUT CfgTrapDestinations **trapDests,
    OUT INT *trapDestsLen)
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
    DWORD dwValue;

    *trapDests = NULL;
    *trapDestsLen = 0;

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: loading trap destinations.\n"));
    enableAuthTraps = FALSE;

    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszSnmpSrvTeKey,
                               0, (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                               KEY_TRAVERSE), &hkResult)) != ERROR_SUCCESS)
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d opening EnableAuthenticationTraps subkey.\n",status));
        SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pszSnmpSrvTeKey, status);
        return FALSE;
        }

    iValue = 0;

    dummySize = MAX_PATH;
    valueSize = MAX_PATH;

    if ((status = RegEnumValue(hkResult, iValue, dummy, &dummySize,
                               NULL, &dwType, (LPBYTE)&dwValue, &valueSize))
        != ERROR_SUCCESS)
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d enumerating EnableAuthenticationTraps subkey.\n",status));
        SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pszSnmpSrvTeKey, status);
        RegCloseKey(hkResult);
        return FALSE;
        }

    enableAuthTraps = dwValue ? TRUE : FALSE;

    RegCloseKey(hkResult);


    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  pszSnmpSrvTdKey,
                               0, (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                               KEY_TRAVERSE), &hkResult)) != ERROR_SUCCESS)
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d opening TrapConfiguration subkey.\n",status));
        SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pszSnmpSrvTdKey, status);
        return FALSE;
        }

    // traverse over keys...

    iValue = 0;

    valueSize = MAX_PATH;

    while((status = RegEnumKey(hkResult, iValue, value, MAX_PATH)) !=
          ERROR_NO_MORE_ITEMS)
        {
        if (status != ERROR_SUCCESS)
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d enumerating TrapConfiguration subkey.\n",status));
            SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pszSnmpSrvTdKey, status);
            return FALSE;
            }

        // save community name and init empty addr list here...

        (*trapDestsLen)++;
        if ((*trapDests = (CfgTrapDestinations *)SnmpUtilMemReAlloc(*trapDests,
          (*trapDestsLen * sizeof(CfgTrapDestinations)))) == NULL) {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: out of memory.\n"));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

        if ((pTemp = (LPSTR)SnmpUtilMemAlloc(valueSize+1)) == NULL) {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: out of memory.\n"));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }
        value[valueSize] = TEXT('\0');
#ifdef UNICODE
        SnmpUtilUnicodeToAnsi(&pTemp, value, FALSE);
#else
        memcpy(pTemp, value, valueSize+1);
#endif

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: processing community %s\n", pTemp));
        (*trapDests)[iValue].communityName = pTemp;
        (*trapDests)[iValue].addrLen       = 0;
        (*trapDests)[iValue].addrList      = NULL;

        if ((status = RegOpenKeyEx(hkResult, value, 0, (KEY_QUERY_VALUE |
                                   KEY_ENUMERATE_SUB_KEYS | KEY_TRAVERSE),
                                   &hkResult2)) != ERROR_SUCCESS)
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d opening %s subkey.\n", status, value));
            pTemp = value; // make pointer for event api
            SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pTemp, status);
            return FALSE;
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
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d enumerating %s subkey.\n", status, value));
                pTemp = value; // make pointer for event api
                SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pTemp, status);
                return FALSE;
                }

            (*trapDests)[iValue].addrLen++;
            if (((*trapDests)[iValue].addrList =
              (AdrList *)SnmpUtilMemReAlloc((*trapDests)[iValue].addrList,
              ((*trapDests)[iValue].addrLen * sizeof(AdrList)))) == NULL) {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: out of memory.\n"));
                SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, ERROR_NOT_ENOUGH_MEMORY);
                return(FALSE);
            }

            if ((pTemp = (LPSTR)SnmpUtilMemAlloc(valueSize+1)) == NULL) {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: out of memory.\n"));
                SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, ERROR_NOT_ENOUGH_MEMORY);
                return(FALSE);
            }
            value[valueSize] = TEXT('\0');
#ifdef UNICODE
            SnmpUtilUnicodeToAnsi(&pTemp, value, FALSE);
#else
            memcpy(pTemp, value, valueSize+1);
#endif

            SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: processing trap destination %s\n", pTemp));
            (*trapDests)[iValue].addrList[(*trapDests)[iValue].addrLen-1].addrText = pTemp;

            if (!SnmpSvcAddrToSocket((*trapDests)[iValue].addrList[(*trapDests)[iValue].addrLen-1].addrText,
                &((*trapDests)[iValue].addrList[(*trapDests)[iValue].addrLen-1].addrEncoding))) {

                (*trapDests)[iValue].addrLen--;
                if (((*trapDests)[iValue].addrList =
                  (AdrList *)SnmpUtilMemReAlloc((*trapDests)[iValue].addrList,
                  ((*trapDests)[iValue].addrLen * sizeof(AdrList)))) == NULL) {
                    SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: out of memory.\n"));
                    SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, ERROR_NOT_ENOUGH_MEMORY);
                    return(FALSE);
                }

                SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: invalid trap destination %s.\n", pTemp));
                SnmpSvcReportEvent(SNMP_EVENT_INVALID_TRAP_DESTINATION, 1, &pTemp, NO_ERROR);
                SnmpUtilMemFree(pTemp);
            }

            dummySize = MAX_PATH;
            valueSize = MAX_PATH;

            iValue2++;
            }

        valueSize = MAX_PATH;

        iValue++;
        }

    return TRUE;

    } // end tdConfig()


BOOL vcConfig(
    OUT CfgValidCommunities  **validComms,
    OUT INT *validCommsLen)
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

    *validComms = NULL;
    *validCommsLen = 0;

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: loading valid communities.\n"));

    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszSnmpSrvVcKey,
                               0, (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                               KEY_TRAVERSE), &hkResult)) != ERROR_SUCCESS)
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d opening ValidCommunities subkey.\n", status));
        SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pszSnmpSrvVcKey, status);
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
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d enumerating ValidCommunities subkey.\n", status));
            SnmpSvcReportEvent(SNMP_EVENT_INVALID_REGISTRY_KEY, 1, &pszSnmpSrvVcKey, status);
            RegCloseKey(hkResult);
            return FALSE;
            }

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: processing community %s\n", value));
        (*validCommsLen)++;
        *validComms = (CfgValidCommunities *)SnmpUtilMemReAlloc(*validComms,
            (*validCommsLen * sizeof(CfgValidCommunities)));

        pTemp = (LPSTR)SnmpUtilMemAlloc(valueSize+1);
        value[valueSize] = TEXT('\0');
#ifdef UNICODE
        SnmpUtilUnicodeToAnsi(&pTemp, value, FALSE);
#else
        memcpy(pTemp, value, valueSize+1);
#endif

        (*validComms)[iValue].communityName = pTemp;

        dummySize = MAX_PATH;
        valueSize = MAX_PATH;

        iValue++;
        }

    RegCloseKey(hkResult);

    return TRUE;

    } // end vcConfig()

//-------------------------------- END --------------------------------------
