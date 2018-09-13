#include <stdafx.h>
#include "Errors.h"
#include "EventCmd.h"
#include "Registry.h"
#include "Operation.h"

CRegistry gRegistry;

DWORD CRegistry::ConfigureRegSource(HKEY hRegSource, char *szEventSource)
{
    DWORD retCode;
    char *szSourceOID;
    char *szEventDup;
    int   nIndex;
    DWORD dwAppend;

    szSourceOID = new char[4*strlen(szEventSource) + 9];
    if (szSourceOID == NULL)
        return _E(ERROR_OUTOFMEMORY, IDS_ERR01);

    nIndex = sprintf(szSourceOID, "%u.", strlen(szEventSource));
    for (szEventDup = szEventSource; *szEventDup != '\0'; szEventDup++)
        nIndex += sprintf(szSourceOID+nIndex,"%d.",*szEventDup);

    if (nIndex == 0)
    {
        retCode = _E(ERROR_FUNCTION_FAILED, IDS_ERR05, szEventSource);
        goto done;
    }
    szSourceOID[--nIndex]='\0';

    retCode = RegSetValueEx(
                hRegSource,
                REG_SRC_ENTOID,
                0,
                REG_SZ,
                (const BYTE *)szSourceOID,
                nIndex);
    if (retCode != ERROR_SUCCESS)
    {
        retCode = _E(retCode, IDS_ERR11, REG_SRC_ENTOID, szEventSource);
        goto done;
    }

    dwAppend = 1;
    retCode = RegSetValueEx(
                hRegSource,
                REG_SRC_APPEND,
                0,
                REG_DWORD,
                (const BYTE *)&dwAppend,
                sizeof(DWORD));
    if (retCode != ERROR_SUCCESS)
    {
        retCode = _E(retCode, IDS_ERR11, REG_SRC_APPEND, szEventSource);
        goto done;
    }

    _W(WARN_TRACK, IDS_TRCK_WRN33, szEventSource);
done:
    delete szSourceOID;
    return retCode;
}

DWORD CRegistry::ConfigureRegEvent(HKEY hRegEvent, DWORD dwEventID, DWORD dwCount, DWORD dwTime)
{
    DWORD retCode = ERROR_SUCCESS;

    retCode = RegSetValueEx(
                hRegEvent,
                REG_EVNT_ID,
                0,
                REG_DWORD,
                (const BYTE *)&dwEventID,
                sizeof(DWORD));
    if (retCode != ERROR_SUCCESS)
        return _E(retCode, IDS_ERR13, REG_EVNT_ID, dwEventID);
    retCode = RegSetValueEx(
                hRegEvent,
                REG_EVNT_COUNT,
                0,
                REG_DWORD,
                (const BYTE *)&dwCount,
                sizeof(DWORD));
    if (retCode != ERROR_SUCCESS)
        return _E(retCode, IDS_ERR13, REG_EVNT_COUNT, dwEventID);
    retCode = RegSetValueEx(
                hRegEvent,
                REG_EVNT_TIME,
                0,
                REG_DWORD,
                (const BYTE *)&dwTime,
                sizeof(DWORD));
    if (retCode != ERROR_SUCCESS)
        return _E(retCode, IDS_ERR13, REG_EVNT_TIME, dwEventID);

    return retCode;
}

DWORD CRegistry::ScanForTrap(HKEY hRegCommunity, char *szAddress, char *szName, DWORD & nNameLen)
{
    DWORD   retCode;
    DWORD   nMaxName = 1;
    char    szDataBuffer[64];

    for (DWORD i = 0, iNameLen = nNameLen, iDataLen = 64;
         (retCode = RegEnumValue(
                     hRegCommunity,
                     i,
                     szName,
                     &iNameLen,
                     0,
                     NULL,
                     (BYTE *)szDataBuffer,
                     &iDataLen)) == ERROR_SUCCESS;
         i++, iNameLen = nNameLen, iDataLen = 64)
    {
        DWORD nNameNum;

        if (strcmp(szDataBuffer, szAddress) == 0)
            return ERROR_ALREADY_EXISTS;

        nNameNum = atoi(szName);
        if (nMaxName <= nNameNum)
            nMaxName = nNameNum+1;
    }

    if (retCode != ERROR_NO_MORE_ITEMS)
        return _E(retCode, IDS_ERR12);

    nNameLen = sprintf(szName, "%u", nMaxName);
    retCode = ERROR_SUCCESS;

    return retCode;
}

CRegistry::CRegistry()
{
    m_hRegRoot = HKEY_LOCAL_MACHINE;
    m_hRegSnmpTraps = NULL;
    m_hRegEvntSources = NULL;
    m_dwFlags = 0;
}

CRegistry::~CRegistry()
{
    if (m_hRegRoot != HKEY_LOCAL_MACHINE)
        RegCloseKey(m_hRegRoot);
    if (m_hRegSnmpTraps != NULL)
        RegCloseKey(m_hRegSnmpTraps);
    if (m_hRegEvntSources != NULL)
        RegCloseKey(m_hRegEvntSources);
}

DWORD CRegistry::Connect()
{
    DWORD retCode = ERROR_SUCCESS;

    if (gCommandLine.m_szSystem != NULL)
    {
        _W(WARN_ATTENTION,IDS_ATTN_WRN34, gCommandLine.m_szSystem);
        retCode = RegConnectRegistry(gCommandLine.m_szSystem, HKEY_LOCAL_MACHINE, &m_hRegRoot);
    }
    if (retCode != ERROR_SUCCESS)
        return _E(retCode, IDS_ERR14, gCommandLine.m_szSystem);
    return retCode;
}

DWORD CRegistry::AddEvent(char *szEventSource, DWORD dwEventID, DWORD dwCount, DWORD dwTime)
{
    DWORD retCode;
    DWORD dwDisposition;
    HKEY  hRegSource;
    HKEY  hRegEvent;
    char  szEventID[64];

    if (m_hRegEvntSources == NULL)
    {
        retCode = RegOpenKeyEx(
                    m_hRegRoot,
                    REGPATH_EVNTAGENT,
                    0,
                    KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_WRITE,
                    &m_hRegEvntSources);
        if (retCode != ERROR_SUCCESS)
            return _E(retCode, IDS_ERR15);
    }

    retCode = RegCreateKeyEx(
                m_hRegEvntSources,
                szEventSource,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hRegSource,
                &dwDisposition);
    if (retCode != ERROR_SUCCESS)
        return _E(retCode, IDS_ERR16, szEventSource);
    if (dwDisposition == REG_CREATED_NEW_KEY)
    {
        retCode = ConfigureRegSource(hRegSource, szEventSource);
        if (retCode != ERROR_SUCCESS)
        {
            RegCloseKey(hRegSource);
            return retCode;
        }
    }
    sprintf(szEventID,"%u",dwEventID);
    retCode = RegCreateKeyEx(
                hRegSource,
                szEventID,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hRegEvent,
                &dwDisposition);
    if (retCode != ERROR_SUCCESS)
    {
        RegCloseKey(hRegSource);
        return _E(retCode, IDS_ERR17, szEventSource);
    }

    retCode = ConfigureRegEvent(hRegEvent, dwEventID, dwCount, dwTime);

    _W(WARN_ATTENTION, IDS_ATTN_WRN35,
        dwDisposition == REG_CREATED_NEW_KEY ? "new" : "existing",
        dwEventID);
 
    RegCloseKey(hRegSource);
    RegCloseKey(hRegEvent);

    return retCode;
}

DWORD CRegistry::DelEvent(char *szEventSource, DWORD dwEventID)
{
    DWORD       retCode;
    char        szEventID[64];
    DWORD       nSzEventID = 64;
    HKEY        hRegSource;
    FILETIME    ft;

    if (m_hRegEvntSources == NULL)
    {
        retCode = RegOpenKeyEx(
                    m_hRegRoot,
                    REGPATH_EVNTAGENT,
                    0,
                    KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_WRITE,
                    &m_hRegEvntSources);
        if (retCode != ERROR_SUCCESS)
            return _E(retCode, IDS_ERR18);
    }

    retCode = RegOpenKeyEx(
                m_hRegEvntSources,
                szEventSource,
                0,
                KEY_ENUMERATE_SUB_KEYS | KEY_WRITE,
                &hRegSource);
    if (retCode != ERROR_SUCCESS)
    {
        _W(WARN_ATTENTION, IDS_ATTN_WRN36, szEventSource);
        return ERROR_SUCCESS;
    }

    sprintf(szEventID,"%u",dwEventID);

    retCode = RegDeleteKey(hRegSource, szEventID);
    if (retCode != ERROR_SUCCESS)
    {
        _W(WARN_ATTENTION, IDS_ATTN_WRN37, szEventID);
        RegCloseKey(hRegSource);
        return ERROR_SUCCESS;
    }

    _W(WARN_ATTENTION, IDS_ATTN_WRN38, dwEventID);

    retCode = RegEnumKeyEx(
                hRegSource,
                0,
                szEventID,
                &nSzEventID,
                0,
                NULL,
                NULL,
                &ft);
    RegCloseKey(hRegSource);
    if (retCode == ERROR_NO_MORE_ITEMS)
    {
        retCode = RegDeleteKey(m_hRegEvntSources, szEventSource);
        if (retCode != ERROR_SUCCESS)
            return _E(retCode, IDS_ERR19, szEventSource);
        _W(WARN_TRACK, IDS_TRCK_WRN39, szEventSource);
        retCode = ERROR_SUCCESS;
    }
    else if (retCode != ERROR_SUCCESS)
    {
        _W(WARN_TRACK, IDS_TRCK_WRN40, retCode, szEventSource);
        retCode = ERROR_SUCCESS;
    }

    return retCode;
}

DWORD CRegistry::AddTrap(char *szCommunity, char *szAddress)
{
    DWORD   retCode;
    DWORD   dwDisposition;
    char    szTrapName[64];
    DWORD   nLenTrapName = 64;
    HKEY    hRegCommunity;

    if (m_hRegSnmpTraps == NULL)
    {
        retCode = RegOpenKeyEx(
                    m_hRegRoot,
                    REGPATH_SNMPTRAPS,
                    0,
                    KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_WRITE,
                    &m_hRegSnmpTraps);
        if (retCode != ERROR_SUCCESS)
            return _E(retCode, IDS_ERR20);
    }

    retCode = RegCreateKeyEx(
                m_hRegSnmpTraps,
                szCommunity,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hRegCommunity,
                &dwDisposition);
    if (retCode != ERROR_SUCCESS)
        return _E(retCode, IDS_ERR21, szCommunity);
    if (dwDisposition == REG_CREATED_NEW_KEY)
        _W(WARN_TRACK, IDS_TRCK_WRN41, szCommunity);
        
    retCode = ScanForTrap(hRegCommunity, szAddress, szTrapName, nLenTrapName);
    if (retCode != ERROR_SUCCESS)
    {
        RegCloseKey(hRegCommunity);
        if (retCode == ERROR_ALREADY_EXISTS)
        {
            _W(WARN_ATTENTION, IDS_ATTN_WRN42, szAddress, szTrapName);
            retCode = ERROR_SUCCESS;
        }
        return retCode;
    }

    retCode = RegSetValueEx(
                hRegCommunity,
                szTrapName,
                0,
                REG_SZ,
                (const BYTE*)szAddress,
                strlen(szAddress));
    
    if (retCode != ERROR_SUCCESS)
        _E(retCode, IDS_ERR22, szAddress);
    else
    {
        m_dwFlags |= REG_FLG_NEEDRESTART;
        _W(WARN_ATTENTION, IDS_ATTN_WRN43, szAddress);
    }

    RegCloseKey(hRegCommunity);
    return retCode;
}

DWORD CRegistry::DelTrap(char *szCommunity, char *szAddress)
{
    DWORD   retCode;
    char    szTrapName[64];
    DWORD   nLenTrapName = 64;
    HKEY    hRegCommunity;

    if (m_hRegSnmpTraps == NULL)
    {
        retCode = RegOpenKeyEx(
                    m_hRegRoot,
                    REGPATH_SNMPTRAPS,
                    0,
                    KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_WRITE,
                    &m_hRegSnmpTraps);
        if (retCode != ERROR_SUCCESS)
            return _E(retCode, IDS_ERR23);
    }

    if ((retCode = RegOpenKeyEx(
                    m_hRegSnmpTraps,
                    szCommunity,
                    0,
                    KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_WRITE,
                    &hRegCommunity)) == ERROR_SUCCESS  &&
        (retCode = ScanForTrap(hRegCommunity, szAddress, szTrapName, nLenTrapName)) == ERROR_ALREADY_EXISTS)
    {
        retCode = RegDeleteValue(
                    hRegCommunity,
                    szTrapName);
    
        if (retCode != ERROR_SUCCESS)
        {
            RegCloseKey(hRegCommunity);
            return _E(retCode, IDS_ERR24, szAddress);
        }
        else
        {
            m_dwFlags |= REG_FLG_NEEDRESTART;
            _W(WARN_ATTENTION, IDS_ATTN_WRN44, szAddress);
        }
    }
    else
    {
        _W(WARN_ATTENTION, IDS_ATTN_WRN45, szAddress, szCommunity);
        retCode = ERROR_SUCCESS;
    }

    retCode =  RegEnumValue(
                hRegCommunity,
                0,
                szTrapName,
                &nLenTrapName,
                0,
                NULL,
                NULL,
                NULL);

    RegCloseKey(hRegCommunity);

    if (retCode == ERROR_NO_MORE_ITEMS)
    {
        retCode = RegDeleteKey(m_hRegSnmpTraps, szCommunity);
        if (retCode != ERROR_SUCCESS)
            _W(WARN_ERROR, IDS_ERRO_WRN46, retCode, szCommunity);
        else
            _W(WARN_TRACK, IDS_TRCK_WRN47, szCommunity);
        retCode = ERROR_SUCCESS;
    }
    else if (retCode != ERROR_SUCCESS)
    {
        _W(WARN_TRACK, IDS_TRCK_WRN48, retCode, szCommunity);
        retCode = ERROR_SUCCESS;
    }
 
    return retCode;
}