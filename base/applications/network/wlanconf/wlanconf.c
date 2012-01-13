/*
 * PROJECT:     ReactOS WLAN command-line configuration utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/network/wlanconf/wlanconf.c
 * PURPOSE:     Allows WLAN configuration via the command prompt
 * COPYRIGHT:   Copyright 2012 Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntddndis.h>
#include <nuiouser.h>
#include <iphlpapi.h>

BOOL bScan = FALSE;

BOOL bConnect = FALSE;
char* sSsid = NULL;
char* sWepKey = NULL;
BOOL bAdhoc = FALSE;

BOOL bDisconnect = FALSE;

DWORD DoFormatMessage(DWORD ErrorCode)
{
    LPVOID lpMsgBuf;
    DWORD RetVal;

    if ((RetVal = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            ErrorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
            (LPTSTR) &lpMsgBuf,
            0,
            NULL )))
    {
        _tprintf(_T("%s"), (LPTSTR)lpMsgBuf);

        LocalFree(lpMsgBuf);

        /* return number of TCHAR's stored in output buffer
         * excluding '\0' - as FormatMessage does*/
        return RetVal;
    }
    else
        return 0;
}

HANDLE
OpenDriverHandle(VOID)
{
    HANDLE hDriver;
    DWORD dwBytesReturned;
    BOOL bSuccess;

    /* Open a handle to the NDISUIO driver */
    hDriver = CreateFileW(NDISUIO_DEVICE_NAME,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);
    if (hDriver == INVALID_HANDLE_VALUE)
        return INVALID_HANDLE_VALUE;
    
    /* Wait for binds */
    bSuccess = DeviceIoControl(hDriver,
                               IOCTL_NDISUIO_BIND_WAIT,
                               NULL,
                               0,
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
    {
        CloseHandle(hDriver);
        return INVALID_HANDLE_VALUE;
    }
    
    return hDriver;
}

BOOL
IsWlanAdapter(HANDLE hAdapter)
{
    BOOL bSuccess;
    DWORD dwBytesReturned;
    NDISUIO_QUERY_OID QueryOid;

    /* WLAN drivers must support this OID */
    QueryOid.Oid = OID_GEN_PHYSICAL_MEDIUM;

    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               &QueryOid,
                               sizeof(QueryOid),
                               &QueryOid,
                               sizeof(QueryOid),
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess || *(PULONG)QueryOid.Data != NdisPhysicalMediumWirelessLan)
        return FALSE;

    return TRUE;
}

BOOL
OpenAdapterHandle(DWORD Index, HANDLE *hAdapter, IP_ADAPTER_INDEX_MAP *IpInfo)
{
    HANDLE hDriver;
    BOOL bSuccess;
    DWORD dwBytesReturned;
    DWORD QueryBindingSize = sizeof(NDISUIO_QUERY_BINDING) + (1024 * sizeof(WCHAR));
    PNDISUIO_QUERY_BINDING QueryBinding;
    DWORD dwStatus, dwSize, i;
    PIP_INTERFACE_INFO InterfaceInfo = NULL;
    
    /* Open the driver handle */
    hDriver = OpenDriverHandle();
    if (hDriver == INVALID_HANDLE_VALUE)
        return FALSE;
    
    /* Allocate the binding struct */
    QueryBinding = HeapAlloc(GetProcessHeap(), 0, QueryBindingSize);
    if (!QueryBinding)
    {
        CloseHandle(hDriver);
        return FALSE;
    }

    /* Query the adapter binding information */
    QueryBinding->BindingIndex = Index;
    bSuccess = DeviceIoControl(hDriver,
                               IOCTL_NDISUIO_QUERY_BINDING,
                               QueryBinding,
                               QueryBindingSize,
                               QueryBinding,
                               QueryBindingSize,
                               &dwBytesReturned,
                               NULL);

    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, QueryBinding);
        CloseHandle(hDriver);
        return FALSE;
    }
    
    /* Bind to the adapter */
    bSuccess = DeviceIoControl(hDriver,
                               IOCTL_NDISUIO_OPEN_DEVICE,
                               (PUCHAR)QueryBinding + QueryBinding->DeviceNameOffset,
                               QueryBinding->DeviceNameLength,
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);

    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, QueryBinding);
        CloseHandle(hDriver);
        return FALSE;
    }

    /* Get interface info from the IP helper */
    dwSize = sizeof(IP_INTERFACE_INFO);
    do {
        if (InterfaceInfo) HeapFree(GetProcessHeap(), 0, InterfaceInfo);
        InterfaceInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(IP_INTERFACE_INFO));
        if (!InterfaceInfo)
        {
            HeapFree(GetProcessHeap(), 0, QueryBinding);
            CloseHandle(hDriver);
            return FALSE;
        }
        dwStatus = GetInterfaceInfo(InterfaceInfo, &dwSize);
    } while (dwStatus == ERROR_INSUFFICIENT_BUFFER);
    
    if (dwStatus != NO_ERROR)
    {
        HeapFree(GetProcessHeap(), 0, QueryBinding);
        HeapFree(GetProcessHeap(), 0, InterfaceInfo);
        return FALSE;
    }
    
    for (i = 0; i < InterfaceInfo->NumAdapters; i++)
    {
        if (wcsstr((PWCHAR)((PUCHAR)QueryBinding + QueryBinding->DeviceNameOffset),
                   InterfaceInfo->Adapter[i].Name))
        {
            *IpInfo = InterfaceInfo->Adapter[i];
            *hAdapter = hDriver;
            
            HeapFree(GetProcessHeap(), 0, QueryBinding);
            HeapFree(GetProcessHeap(), 0, InterfaceInfo);
            
            return TRUE;
        }
    }

    HeapFree(GetProcessHeap(), 0, QueryBinding);
    HeapFree(GetProcessHeap(), 0, InterfaceInfo);
    CloseHandle(hDriver);

    return FALSE;
}

/* Only works with the first adapter for now */
BOOL
OpenWlanAdapter(HANDLE *hAdapter, IP_ADAPTER_INDEX_MAP *IpInfo)
{
    DWORD dwCurrentIndex;

    for (dwCurrentIndex = 0; ; dwCurrentIndex++)
    {
        if (!OpenAdapterHandle(dwCurrentIndex, hAdapter, IpInfo))
            break;
        
        if (IsWlanAdapter(*hAdapter))
            return TRUE;
        else
            CloseHandle(*hAdapter);
    }

    return FALSE;
}

BOOL
WlanDisconnect(HANDLE hAdapter, PIP_ADAPTER_INDEX_MAP IpInfo)
{
    BOOL bSuccess;
    DWORD dwBytesReturned;
    NDISUIO_SET_OID SetOid;
    
    /* Release this IP address */
    IpReleaseAddress(IpInfo);

    /* Disassociate from the AP */
    SetOid.Oid = OID_802_11_DISASSOCIATE;
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_SET_OID_VALUE,
                               &SetOid,
                               sizeof(SetOid),
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
        return FALSE;

    _tprintf(_T("The operation completed successfully.\n"));
    return TRUE;
}

static
UCHAR
CharToHex(CHAR Char)
{
    Char = toupper(Char);
    
    switch (Char)
    {
        case '0':
            return 0x0;
        case '1':
            return 0x1;
        case '2':
            return 0x2;
        case '3':
            return 0x3;
        case '4':
            return 0x4;
        case '5':
            return 0x5;
        case '6':
            return 0x6;
        case '7':
            return 0x7;
        case '8':
            return 0x8;
        case '9':
            return 0x9;
        case 'A':
            return 0xA;
        case 'B':
            return 0xB;
        case 'C':
            return 0xC;
        case 'D':
            return 0xD;
        case 'E':
            return 0xE;
        case 'F':
            return 0xF;
        default:
            return 0;
    }
}

BOOL
WlanPrintCurrentStatus(HANDLE hAdapter)
{
    PNDISUIO_QUERY_OID QueryOid;
    DWORD QueryOidSize;
    BOOL bSuccess;
    DWORD dwBytesReturned;
    PNDIS_802_11_SSID SsidInfo;
    CHAR SsidBuffer[NDIS_802_11_LENGTH_SSID + 1];
    DWORD i;
    
    QueryOidSize = FIELD_OFFSET(NDISUIO_QUERY_OID, Data) + sizeof(NDIS_802_11_SSID);
    QueryOid = HeapAlloc(GetProcessHeap(), 0, QueryOidSize);
    if (!QueryOid)
        return FALSE;
    
    QueryOid->Oid = OID_802_11_SSID;
    SsidInfo = (PNDIS_802_11_SSID)QueryOid->Data;

    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, QueryOid);
        return FALSE;
    }

    if (SsidInfo->SsidLength == 0)
    {
        _tprintf(_T("\nWLAN disconnected\n"));
        HeapFree(GetProcessHeap(), 0, QueryOid);
        return TRUE;
    }
    else
    {
        _tprintf(_T("\nCurrent wireless association information:\n\n"));
    }
    
    /* Copy the SSID to our internal buffer and terminate it */
    RtlCopyMemory(SsidBuffer, SsidInfo->Ssid, SsidInfo->SsidLength);
    SsidBuffer[SsidInfo->SsidLength] = 0;
    
    _tprintf(_T("SSID: %s\n"), SsidBuffer);

    HeapFree(GetProcessHeap(), 0, QueryOid);
    QueryOidSize = FIELD_OFFSET(NDISUIO_QUERY_OID, Data) + sizeof(NDIS_802_11_MAC_ADDRESS);
    QueryOid = HeapAlloc(GetProcessHeap(), 0, QueryOidSize);
    if (!QueryOid)
        return FALSE;

    QueryOid->Oid = OID_802_11_BSSID;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, QueryOid);
        return FALSE;
    }

    _tprintf(_T("BSSID: "));
    for (i = 0; i < sizeof(NDIS_802_11_MAC_ADDRESS); i++)
    {
        UINT BssidData = QueryOid->Data[i];

        _tprintf(_T("%.2x"), BssidData);

        if (i != sizeof(NDIS_802_11_MAC_ADDRESS) - 1)
            _tprintf(_T(":"));
    }
    _tprintf(_T("\n"));
    
    HeapFree(GetProcessHeap(), 0, QueryOid);
    QueryOidSize = sizeof(NDISUIO_QUERY_OID);
    QueryOid = HeapAlloc(GetProcessHeap(), 0, QueryOidSize);
    if (!QueryOid)
        return FALSE;
    
    QueryOid->Oid = OID_802_11_INFRASTRUCTURE_MODE;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, QueryOid);
        return FALSE;
    }
    
    _tprintf(_T("Network mode: %s\n"), (*(PUINT)QueryOid->Data == Ndis802_11IBSS) ? "Adhoc" : "Infrastructure");
    
    QueryOid->Oid = OID_802_11_WEP_STATUS;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, QueryOid);
        return FALSE;
    }
    
    _tprintf(_T("WEP enabled: %s\n"), (*(PUINT)QueryOid->Data == Ndis802_11WEPEnabled) ? "Yes" : "No");
    
    _tprintf("\n");
    QueryOid->Oid = OID_802_11_RSSI;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (bSuccess)
    {
        /* This OID is optional */
        _tprintf(_T("RSSI: %i dBm\n"), *(PINT)QueryOid->Data);
    }
    
    QueryOid->Oid = OID_802_11_TX_POWER_LEVEL;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (bSuccess)
    {
        /* This OID is optional */
        _tprintf(_T("Transmission power: %d mW\n"), *(PUINT)QueryOid->Data);
    }
    
    _tprintf(_T("\n"));
    
    QueryOid->Oid = OID_802_11_NUMBER_OF_ANTENNAS;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (bSuccess)
    {
        /* This OID is optional */
        _tprintf(_T("Antenna count: %d\n"), *(PUINT)QueryOid->Data);
    }
    
    QueryOid->Oid = OID_802_11_TX_ANTENNA_SELECTED;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (bSuccess)
    {
        UINT TransmitAntenna = *(PUINT)QueryOid->Data;
        
        if (TransmitAntenna != 0xFFFFFFFF)
            _tprintf(_T("Transmit antenna: %d\n"), TransmitAntenna);
        else
            _tprintf(_T("Transmit antenna: Any\n"));
    }
    
    QueryOid->Oid = OID_802_11_RX_ANTENNA_SELECTED;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (bSuccess)
    {
        UINT ReceiveAntenna = *(PUINT)QueryOid->Data;
        
        if (ReceiveAntenna != 0xFFFFFFFF)
            _tprintf(_T("Receive antenna: %d\n"), ReceiveAntenna);
        else
            _tprintf(_T("Receive antenna: Any\n"));
    }
    
    _tprintf(_T("\n"));
    
    QueryOid->Oid = OID_802_11_FRAGMENTATION_THRESHOLD;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (bSuccess)
    {
        /* This OID is optional */
        _tprintf(_T("Fragmentation threshold: %d bytes\n"), *(PUINT)QueryOid->Data);
    }
    
    QueryOid->Oid = OID_802_11_RTS_THRESHOLD;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (bSuccess)
    {
        /* This OID is optional */
        _tprintf(_T("RTS threshold: %d bytes\n"), *(PUINT)QueryOid->Data);
    }
    
    HeapFree(GetProcessHeap(), 0, QueryOid);
    
    _tprintf(_T("\n"));
    return TRUE;
}

BOOL
WlanConnect(HANDLE hAdapter)
{
    BOOL bSuccess;
    DWORD dwBytesReturned, SetOidSize;
    PNDISUIO_SET_OID SetOid;
    PNDIS_802_11_SSID Ssid;
    DWORD i;
    
    SetOidSize = sizeof(NDISUIO_SET_OID);
    SetOid = HeapAlloc(GetProcessHeap(), 0, SetOidSize);
    if (!SetOid)
        return FALSE;

    /* Set the network mode */
    SetOid->Oid = OID_802_11_INFRASTRUCTURE_MODE;
    *(PULONG)SetOid->Data = bAdhoc ? Ndis802_11IBSS : Ndis802_11Infrastructure;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_SET_OID_VALUE,
                               SetOid,
                               SetOidSize,
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, SetOid);
        return FALSE;
    }

    /* Set the authentication mode */
    SetOid->Oid = OID_802_11_AUTHENTICATION_MODE;
    *(PULONG)SetOid->Data = sWepKey ? Ndis802_11AuthModeShared : Ndis802_11AuthModeOpen;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_SET_OID_VALUE,
                               SetOid,
                               SetOidSize,
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, SetOid);
        return FALSE;
    }
    
    if (sWepKey)
    {
        PNDIS_802_11_WEP WepData;
        
        HeapFree(GetProcessHeap(), 0, SetOid);

        SetOidSize = FIELD_OFFSET(NDISUIO_SET_OID, Data) +
                     FIELD_OFFSET(NDIS_802_11_WEP, KeyMaterial) +
                     (strlen(sWepKey) >> 1);
        SetOid = HeapAlloc(GetProcessHeap(), 0, SetOidSize);
        if (!SetOid)
            return FALSE;
        
        /* Add the WEP key */
        SetOid->Oid = OID_802_11_ADD_WEP;
        WepData = (PNDIS_802_11_WEP)SetOid->Data;

        WepData->KeyIndex = 0x80000000;
        WepData->KeyLength = strlen(sWepKey) >> 1;
        WepData->Length = FIELD_OFFSET(NDIS_802_11_WEP, KeyMaterial) + WepData->KeyLength;
        
        /* Assemble the hex key */
        i = 0;
        while (sWepKey[i << 1] != '\0')
        {
            WepData->KeyMaterial[i] = CharToHex(sWepKey[i << 1]) << 4;
            WepData->KeyMaterial[i] |= CharToHex(sWepKey[(i << 1) + 1]);
            i++;
        }
        
        bSuccess = DeviceIoControl(hAdapter,
                                   IOCTL_NDISUIO_SET_OID_VALUE,
                                   SetOid,
                                   SetOidSize,
                                   NULL,
                                   0,
                                   &dwBytesReturned,
                                   NULL);
        if (!bSuccess)
        {
            HeapFree(GetProcessHeap(), 0, SetOid);
            return FALSE;
        }
    }

    /* Set the encryption status */
    SetOid->Oid = OID_802_11_WEP_STATUS;
    *(PULONG)SetOid->Data = sWepKey ? Ndis802_11WEPEnabled : Ndis802_11WEPDisabled;

    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_SET_OID_VALUE,
                               SetOid,
                               SetOidSize,
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, SetOid);
        return FALSE;
    }
    
    HeapFree(GetProcessHeap(), 0, SetOid);
    SetOidSize = FIELD_OFFSET(NDISUIO_SET_OID, Data) + sizeof(NDIS_802_11_MAC_ADDRESS);
    SetOid = HeapAlloc(GetProcessHeap(), 0, SetOidSize);
    if (!SetOid)
        return FALSE;
    
    /* Set the BSSID */
    SetOid->Oid = OID_802_11_BSSID;
    RtlFillMemory(SetOid->Data, sizeof(NDIS_802_11_MAC_ADDRESS), 0xFF);
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_SET_OID_VALUE,
                               SetOid,
                               SetOidSize,
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, SetOid);
        return FALSE;
    }
    
    HeapFree(GetProcessHeap(), 0, SetOid);
    SetOidSize = FIELD_OFFSET(NDISUIO_SET_OID, Data) + sizeof(NDIS_802_11_SSID);
    SetOid = HeapAlloc(GetProcessHeap(), 0, SetOidSize);
    if (!SetOid)
        return FALSE;
    
    /* Finally, set the SSID */
    SetOid->Oid = OID_802_11_SSID;
    Ssid = (PNDIS_802_11_SSID)SetOid->Data;
    
    RtlCopyMemory(Ssid->Ssid, sSsid, strlen(sSsid));
    Ssid->SsidLength = strlen(sSsid);
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_SET_OID_VALUE,
                               SetOid,
                               SetOidSize,
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);
    
    HeapFree(GetProcessHeap(), 0, SetOid);
    
    if (!bSuccess)
        return FALSE;

    _tprintf(_T("The operation completed successfully.\n"));
    return TRUE;
}

BOOL
WlanScan(HANDLE hAdapter)
{
    BOOL bSuccess;
    DWORD dwBytesReturned;
    NDISUIO_SET_OID SetOid;
    PNDISUIO_QUERY_OID QueryOid;
    DWORD QueryOidSize;
    PNDIS_802_11_BSSID_LIST BssidList;
    DWORD i, j;

    SetOid.Oid = OID_802_11_BSSID_LIST_SCAN;
    
    /* Send the scan OID */
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_SET_OID_VALUE,
                               &SetOid,
                               sizeof(SetOid),
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
        return FALSE;
    
    /* Allocate space for 15 networks to be returned */
    QueryOidSize = sizeof(NDISUIO_QUERY_OID) + (sizeof(NDIS_WLAN_BSSID) * 15);
    QueryOid = HeapAlloc(GetProcessHeap(), 0, QueryOidSize);
    if (!QueryOid)
        return FALSE;
    
    QueryOid->Oid = OID_802_11_BSSID_LIST;
    BssidList = (PNDIS_802_11_BSSID_LIST)QueryOid->Data;

    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_QUERY_OID_VALUE,
                               QueryOid,
                               QueryOidSize,
                               QueryOid,
                               QueryOidSize,
                               &dwBytesReturned,
                               NULL);
    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, QueryOid);
        return FALSE;
    }

    if (BssidList->NumberOfItems == 0)
    {
        _tprintf(_T("No networks found in range\n"));
    }
    else
    {
        PNDIS_WLAN_BSSID BssidInfo = BssidList->Bssid;
        for (i = 0; i < BssidList->NumberOfItems; i++)
        {
            PNDIS_802_11_SSID Ssid = &BssidInfo->Ssid;
            NDIS_802_11_RSSI Rssi = BssidInfo->Rssi;
            NDIS_802_11_NETWORK_INFRASTRUCTURE NetworkType = BssidInfo->InfrastructureMode;
            CHAR SsidBuffer[NDIS_802_11_LENGTH_SSID + 1];
            UINT Rate;

            /* SSID member is a non-null terminated ASCII string */
            RtlCopyMemory(SsidBuffer, Ssid->Ssid, Ssid->SsidLength);
            SsidBuffer[Ssid->SsidLength] = 0;
            
            _tprintf(_T("\nSSID: %s\n"
                        "Encrypted: %s\n"
                        "Network Type: %s\n"
                        "RSSI: %i dBm\n"
                        "Supported Rates (Mbps): "),
                        SsidBuffer,
                        BssidInfo->Privacy == 0 ? "No" : "Yes",
                        NetworkType == Ndis802_11IBSS ? "Adhoc" : "Infrastructure",
                        (int)Rssi);
            
            for (j = 0; j < NDIS_802_11_LENGTH_RATES; j++)
            {
                Rate = BssidInfo->SupportedRates[j];
                if (Rate != 0)
                {
                    /* Bit 7 is the basic rates bit */
                    Rate = Rate & 0x7F;

                    /* SupportedRates are in units of .5 */
                    if (Rate & 0x01)
                    {
                        /* Bit 0 is set so we need to add 0.5 */
                        _tprintf(_T("%u.5 "), (Rate >> 1));
                    }
                    else
                    {
                        /* Bit 0 is clear so just print the conversion */
                        _tprintf(_T("%u "), (Rate >> 1));
                    }
                }
            }
            _tprintf(_T("\n"));
            
            /* Move to the next entry */
            BssidInfo = (PNDIS_WLAN_BSSID)((PUCHAR)BssidInfo + BssidInfo->Length);
        }
    }
    
    HeapFree(GetProcessHeap(), 0, QueryOid);
    
    return bSuccess;
}

VOID Usage()
{
    _tprintf(_T("\nConfigures a WLAN adapter.\n\n"
    "WLANCONF [-c SSID [-w WEP] [-a]] [-d] [-s]\n\n"
    "  -c SSID       Connects to a supplied SSID.\n"
    "     -w WEP     Specifies a WEP key to use.\n"
    "     -a         Specifies the target network is ad-hoc\n"
    "  -d            Disconnects from the current AP.\n"
    "  -s            Scans and displays a list of access points in range.\n\n"
    " Passing no parameters will print information about the current WLAN connection\n"));
}


BOOL ParseCmdline(int argc, char* argv[])
{
    INT i;
    
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
                case 's':
                    bScan = TRUE;
                    break;
                case 'd':
                    bDisconnect = TRUE;
                    break;
                case 'c':
                    if (i == argc - 1)
                    {
                        Usage();
                        return FALSE;
                    }
                    bConnect = TRUE;
                    sSsid = argv[++i];
                    break;
                case 'w':
                    if (i == argc - 1)
                    {
                        Usage();
                        return FALSE;
                    }
                    sWepKey = argv[++i];
                    break;
                case 'a':
                    bAdhoc = TRUE;
                    break;
                default :
                    Usage();
                    return FALSE;
            }

        }
        else
        {
            Usage();
            return FALSE;
        }
    }

    return TRUE;
}

int main(int argc, char* argv[])
{
    HANDLE hAdapter;
    IP_ADAPTER_INDEX_MAP IpInfo;

    if (!ParseCmdline(argc, argv))
        return -1;
    
    if (!OpenWlanAdapter(&hAdapter, &IpInfo))
    {
        DoFormatMessage(GetLastError());
        return -1;
    }
    
    if (bScan)
    {
        if (!WlanScan(hAdapter))
        {
            DoFormatMessage(GetLastError());
            CloseHandle(hAdapter);
            return -1;
        }
    }
    else if (bDisconnect)
    {
        if (!WlanDisconnect(hAdapter, &IpInfo))
        {
            DoFormatMessage(GetLastError());
            CloseHandle(hAdapter);
            return -1;
        }
    }
    else if (bConnect)
    {
        if (!WlanConnect(hAdapter))
        {
            DoFormatMessage(GetLastError());
            CloseHandle(hAdapter);
            return -1;
        }
    }
    else
    {
        if (!WlanPrintCurrentStatus(hAdapter))
        {
            DoFormatMessage(GetLastError());
            CloseHandle(hAdapter);
            return -1;
        }
    }

    CloseHandle(hAdapter);
    return 0;
}
