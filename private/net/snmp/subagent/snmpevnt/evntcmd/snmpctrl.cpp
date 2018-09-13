#include <stdafx.h>
#include "Errors.h"
#include "SNMPCtrl.h"
#include "EventCmd.h"

CSNMPController gSNMPController;

CSNMPController::CSNMPController()
{
    m_hSNMPService = NULL;
    m_hServiceController = NULL;
}

CSNMPController::~CSNMPController()
{
    if (m_hSNMPService != NULL)
        CloseServiceHandle(m_hSNMPService);
    if (m_hServiceController != NULL)
        CloseServiceHandle(m_hServiceController);
}

DWORD CSNMPController::LoadSvcHandle()
{
    if (m_hSNMPService == NULL)
    {
        if (m_hServiceController == NULL)
        {
            m_hServiceController = OpenSCManager(
                gCommandLine.m_szSystem,
		        "ServicesActive",
		        GENERIC_EXECUTE);

            if (m_hServiceController == NULL)
                return _E(GetLastError(), IDS_ERR25);
        }
	
	    m_hSNMPService = OpenService(
            m_hServiceController,
            "SNMP",
            SERVICE_CONTROL_INTERROGATE | SERVICE_START | SERVICE_STOP);

        if (m_hSNMPService == NULL)
            return _E(GetLastError(), IDS_ERR26);
    }
    return ERROR_SUCCESS;
}

BOOL CSNMPController::IsSNMPRunning()
{
    SERVICE_STATUS snmpStatus;

    if (LoadSvcHandle() != ERROR_SUCCESS)
        return FALSE;

    if (!QueryServiceStatus(m_hSNMPService, &snmpStatus))
        return _E(GetLastError(), IDS_ERR27);

    _W(WARN_TRACK, IDS_TRCK_WRN49, snmpStatus.dwCurrentState);

    return snmpStatus.dwCurrentState == SERVICE_RUNNING;
}

DWORD CSNMPController::StartSNMP()
{
    DWORD           retCode;
    SERVICE_STATUS  svcStatus;
    DWORD           dwRetries;

    if ((retCode = LoadSvcHandle()) != ERROR_SUCCESS)
        return retCode;

    if (!StartService(m_hSNMPService, 0, NULL))
        return _E(GetLastError(), IDS_ERR28);

    for (dwRetries = 10; dwRetries > 0; dwRetries--)
    {
        printf("."); fflush(stdout);
        if (!QueryServiceStatus(m_hSNMPService, &svcStatus))
            return _E(GetLastError(), IDS_ERR29);
        if (svcStatus.dwCurrentState == SERVICE_RUNNING)
            break;
        if (svcStatus.dwCurrentState == SERVICE_START_PENDING)
        {
            if (svcStatus.dwWaitHint < 200)
                svcStatus.dwWaitHint = 200;
            if (svcStatus.dwWaitHint > 1000)
                svcStatus.dwWaitHint = 1000;
            Sleep(svcStatus.dwWaitHint);
        }
        else
            return _E(ERROR_INVALID_STATE, IDS_ERR06, svcStatus.dwWaitHint);
    }
    printf("\n");

    return retCode;
}

DWORD CSNMPController::StopSNMP()
{
    DWORD           retCode;
    SERVICE_STATUS  svcStatus;
    DWORD           dwRetries;

    if ((retCode = LoadSvcHandle()) != ERROR_SUCCESS)
        return retCode;

    if (!ControlService(m_hSNMPService, SERVICE_CONTROL_STOP, &svcStatus))
    {
        retCode = GetLastError();
        if (retCode == ERROR_SERVICE_NOT_ACTIVE)
        {
            _W(WARN_TRACK, IDS_TRCK_WRN50);
            return ERROR_SUCCESS;
        }
        return _E(GetLastError(), IDS_ERR30);
    }

    for (dwRetries = 10; dwRetries > 0; dwRetries--)
    {
        printf("."); fflush(stdout);
        if (!QueryServiceStatus(m_hSNMPService, &svcStatus))
            return _E(GetLastError(), IDS_ERR31);
        if (svcStatus.dwCurrentState == SERVICE_STOPPED)
            break;
        if (svcStatus.dwCurrentState == SERVICE_STOP_PENDING)
        {
            if (svcStatus.dwWaitHint < 200)
                svcStatus.dwWaitHint = 200;
            if (svcStatus.dwWaitHint > 1000)
                svcStatus.dwWaitHint = 1000;
            Sleep(svcStatus.dwWaitHint);
        }
        else
            return _E(ERROR_INVALID_STATE, IDS_ERR06, svcStatus.dwWaitHint);
    }
    printf("\n");

    if (dwRetries == 0)
        return _E(ERROR_TIMEOUT, IDS_ERR07);

    return retCode;
}
