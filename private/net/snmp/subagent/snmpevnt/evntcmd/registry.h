#ifndef _REGISTRY_H
#define _REGISTRY_H

#define REGPATH_SNMPTRAPS   "SYSTEM\\CurrentControlSet\\Services\\SNMP\\Parameters\\TrapConfiguration"
#define REGPATH_EVNTAGENT   "Software\\Microsoft\\SNMP_EVENTS\\EventLog\\Sources"

#define REG_SRC_ENTOID     "EnterpriseOID"
#define REG_SRC_APPEND     "Append"

#define REG_EVNT_COUNT     "Count"
#define REG_EVNT_ID        "FullID"
#define REG_EVNT_TIME      "Time"

#define REG_FLG_NEEDRESTART 1

class CRegistry
{
    HKEY    m_hRegRoot;
    HKEY    m_hRegSnmpTraps;
    HKEY    m_hRegEvntSources;

    DWORD ConfigureRegSource(HKEY hRegSource, char *szEventSource);
    DWORD ConfigureRegEvent(HKEY hRegEvent, DWORD dwEventID, DWORD dwCount, DWORD dwTime);
    DWORD ScanForTrap(HKEY hRegCommunity, char *szAddress, char *szName, DWORD & nNameLen);

public:
    DWORD   m_dwFlags;

    CRegistry();
    ~CRegistry();

    DWORD Connect();

    DWORD AddEvent(char *szEventSource, DWORD dwEventID, DWORD dwCount, DWORD dwTime);
    DWORD DelEvent(char *szEventSource, DWORD dwEventID);
    DWORD AddTrap(char *szCommunity, char *szAddress);
    DWORD DelTrap(char *szCommunity, char *szAddress);
};

extern CRegistry gRegistry;

#endif
