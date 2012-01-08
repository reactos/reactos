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

BOOL bScan = FALSE;

BOOL bConnect = FALSE;
char* sSsid = NULL;

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

    /* Open a handle to this NDISUIO driver */
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

    /* NDIS 5.1 WLAN drivers must support this OID */
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

HANDLE
OpenAdapterHandle(DWORD Index)
{
    HANDLE hDriver;
    BOOL bSuccess;
    DWORD dwBytesReturned;
    char Buffer[1024];
    PNDISUIO_QUERY_BINDING QueryBinding = (PNDISUIO_QUERY_BINDING)Buffer;
    
    /* Open the driver handle */
    hDriver = OpenDriverHandle();
    if (hDriver == INVALID_HANDLE_VALUE)
        return INVALID_HANDLE_VALUE;

    /* Query for bindable adapters */
    QueryBinding->BindingIndex = 0;
    do {
        bSuccess = DeviceIoControl(hDriver,
                                   IOCTL_NDISUIO_QUERY_BINDING,
                                   NULL,
                                   0,
                                   NULL,
                                   0,
                                   &dwBytesReturned,
                                   NULL);
        if (QueryBinding->BindingIndex == Index)
            break;
        QueryBinding->BindingIndex++;
    } while (bSuccess);

    if (!bSuccess)
    {
        CloseHandle(hDriver);
        return INVALID_HANDLE_VALUE;
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
        CloseHandle(hDriver);
        return INVALID_HANDLE_VALUE;
    }
    
    return hDriver;
}

/* Only works with the first adapter for now */
HANDLE
OpenWlanAdapter(VOID)
{
    DWORD dwCurrentIndex;
    HANDLE hCurrentAdapter;

    for (dwCurrentIndex = 0; ; dwCurrentIndex++)
    {
        hCurrentAdapter = OpenAdapterHandle(dwCurrentIndex);
        if (hCurrentAdapter == INVALID_HANDLE_VALUE)
            break;
        
        if (IsWlanAdapter(hCurrentAdapter))
            return hCurrentAdapter;
        else
            CloseHandle(hCurrentAdapter);
    }

    return INVALID_HANDLE_VALUE;
}

BOOL
WlanDisconnect(HANDLE hAdapter)
{
    BOOL bSuccess;
    DWORD dwBytesReturned;
    NDISUIO_SET_OID SetOid;
    
    SetOid.Oid = OID_802_11_DISASSOCIATE;
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_SET_OID_VALUE,
                               &SetOid,
                               sizeof(SetOid),
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);

    return bSuccess;
}

BOOL
WlanConnect(HANDLE hAdapter)
{
    BOOL bSuccess;
    DWORD dwBytesReturned;
    PNDISUIO_SET_OID SetOid;
    PNDIS_802_11_SSID Ssid;

    SetOid = HeapAlloc(GetProcessHeap(), 0, sizeof(NDISUIO_SET_OID) + sizeof(NDIS_802_11_SSID));
    if (!SetOid)
        return FALSE;

    SetOid->Oid = OID_802_11_SSID;
    Ssid = SetOid->Data;
    
    /* Fill the OID data buffer */
    RtlCopyMemory(Ssid->Ssid, sSsid, strlen(sSsid));
    Ssid->SsidLength = strlen(sSsid);
    
    bSuccess = DeviceIoControl(hAdapter,
                               IOCTL_NDISUIO_SET_OID_VALUE,
                               &SetOid,
                               sizeof(SetOid),
                               NULL,
                               0,
                               &dwBytesReturned,
                               NULL);
    
    HeapFree(GetProcessHeap(), 0, SetOid);
    
    return bSuccess;
}

BOOL
WlanScan(HANDLE hAdapter)
{
    BOOL bSuccess;
    DWORD dwBytesReturned;
    NDISUIO_SET_OID SetOid;
    PNDISUIO_QUERY_OID QueryOid;
    DWORD QueryOidSize;
    PNDIS_802_11_BSSID_LIST_EX BssidList;
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
    QueryOidSize = sizeof(NDISUIO_QUERY_OID) + (sizeof(NDIS_WLAN_BSSID_EX) * 15);
    QueryOid = HeapAlloc(GetProcessHeap(), 0, QueryOidSize);
    if (!QueryOid)
        return FALSE;
    
    QueryOid->Oid = OID_802_11_BSSID_LIST;
    BssidList = QueryOid->Data;

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
        for (i = 0; i < BssidList->NumberOfItems; i++)
        {
            PNDIS_WLAN_BSSID_EX BssidInfo = BssidList->Bssid[i];
            PNDIS_802_11_SSID Ssid = &BssidInfo->Ssid;
            UCHAR SupportedRates[16] = &BssidInfo->SupportedRates;
            NDIS_802_11_RSSI Rssi = BssidInfo->Rssi;
            NDIS_802_11_NETWORK_INFRASTRUCTURE NetworkType = BssidInfo->InfrastructureMode;
            CHAR SsidBuffer[33];

            /* SSID member is a non-null terminated ASCII string */
            RtlCopyMemory(SsidBuffer, Ssid->Ssid, Ssid->SsidLength);
            SsidBuffer[Ssid->SsidLength] = 0;
            
            _tprintf(_T("\nSSID: %s\n"
                        "Encrypted: %s"
                        "Network Type: %s\n"
                        "RSSI: %i\n"
                        "Supported Rates: "),
                        SsidBuffer,
                        BssidInfo->Privacy == 0 ? "No" : "Yes",
                        NetworkType == Ndis802_11IBSS ? "Adhoc" : "Infrastructure",
                        (int)Rssi);
            
            for (j = 0; j < 16; j++)
            {
                if (SupportedRates[j] != 0)
                {
                    /* SupportedRates are in units of .5 */
                    _tprintf(_T("%d "), (SupportedRates[j] << 2));
                }
            }
            _tprintf(_T("\n"));
        }
    }
    
    HeapFree(GetProcessHeap(), 0, QueryOid);
    
    return bSuccess;
}

VOID Usage()
{
    _tprintf(_T("\nConfigures a WLAN adapter.\n\n"
    "WLANCONF [-c SSID] [-d] [-s]\n\n"
    "  -c SSID       Connects to a supplied SSID.\n"
    "  -d            Disconnects from the current AP.\n"
    "  -s            Scans and displays a list of access points in range.\n"));
}


BOOL ParseCmdline(int argc, char* argv[])
{
    INT i;
    
    for (i = 1; i < argc; i++)
    {
        if ((argc > 1) && (argv[i][0] == '-'))
        {
            TCHAR c;
            
            while ((c = *++argv[i]) != '\0')
            {
                switch (c)
                {
                    case 's' :
                        bScan = TRUE;
                        break;
                    case 'd' :
                        bDisconnect = TRUE;
                        break;
                    case 'c' :
                        bConnect = TRUE;
                        sSsid = argv[++i];
                        break;
                    default :
                        Usage();
                        return FALSE;
                }
            }
        }
    }

    if (!bScan && !bDisconnect && !bConnect)
    {
        Usage();
        return FALSE;
    }

    return TRUE;
}

int main(int argc, char* argv[])
{
    HANDLE hAdapter;

    if (!ParseCmdline(argc, argv))
        return -1;
    
    hAdapter = OpenWlanAdapter();
    if (hAdapter == INVALID_HANDLE_VALUE)
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
        if (!WlanDisconnect(hAdapter))
        {
            DoFormatMessage(GetLastError());
            CloseHandle(hAdapter);
            return -1;
        }
    }
    else
    {
        if (!WlanConnect(hAdapter))
        {
            DoFormatMessage(GetLastError());
            CloseHandle(hAdapter);
            return -1;
        }
    }

    CloseHandle(hAdapter);
    return 0;
}
