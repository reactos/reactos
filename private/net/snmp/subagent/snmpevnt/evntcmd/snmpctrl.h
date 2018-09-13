#ifndef _SNMPCTRL_H
#define _SNMPCTRL_H

class CSNMPController
{
    SC_HANDLE    m_hServiceController;
    SC_HANDLE    m_hSNMPService;

public:
    CSNMPController();
    ~CSNMPController();

    DWORD LoadSvcHandle();
    BOOL  IsSNMPRunning();
    DWORD StartSNMP();
    DWORD StopSNMP();
};

extern CSNMPController gSNMPController;

#endif
