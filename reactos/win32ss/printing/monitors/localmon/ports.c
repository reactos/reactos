/*
 * PROJECT:     ReactOS Local Port Monitor
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to ports
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"


// Local Constants
static const WCHAR wszNonspooledPrefix[] = L"NONSPOOLED_";
static const DWORD cchNonspooledPrefix = _countof(wszNonspooledPrefix) - 1;


/**
 * @name _GetNonspooledPortName
 *
 * Prepends "NONSPOOLED_" to a port name without colon.
 *
 * @param pwszPortNameWithoutColon
 * Result of a previous GetPortNameWithoutColon call.
 *
 * @param ppwszNonspooledPortName
 * Pointer to a buffer that will contain the NONSPOOLED port name.
 * You have to free this buffer using DllFreeSplMem.
 *
 * @return
 * ERROR_SUCCESS if the NONSPOOLED port name was successfully copied into the buffer.
 * ERROR_NOT_ENOUGH_MEMORY if memory allocation failed.
 */
static __inline DWORD
_GetNonspooledPortName(PCWSTR pwszPortNameWithoutColon, PWSTR* ppwszNonspooledPortName)
{
    DWORD cchPortNameWithoutColon;

    cchPortNameWithoutColon = wcslen(pwszPortNameWithoutColon);

    *ppwszNonspooledPortName = DllAllocSplMem((cchNonspooledPrefix + cchPortNameWithoutColon + 1) * sizeof(WCHAR));
    if (!*ppwszNonspooledPortName)
    {
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CopyMemory(*ppwszNonspooledPortName, wszNonspooledPrefix, cchNonspooledPrefix * sizeof(WCHAR));
    CopyMemory(&(*ppwszNonspooledPortName)[cchNonspooledPrefix], pwszPortNameWithoutColon, (cchPortNameWithoutColon + 1) * sizeof(WCHAR));

    return ERROR_SUCCESS;
}

/**
 * @name _IsLegacyPort
 *
 * Checks if the given port name is a legacy port (COM or LPT).
 * This check is extra picky to not cause false positives (like file name ports starting with "COM" or "LPT").
 *
 * @param pwszPortName
 * The port name to check.
 *
 * @param pwszPortType
 * L"COM" or L"LPT"
 *
 * @return
 * TRUE if this is definitely the asked legacy port, FALSE if not.
 */
static __inline BOOL
_IsLegacyPort(PCWSTR pwszPortName, PCWSTR pwszPortType)
{
    const DWORD cchPortType = 3;
    PCWSTR p = pwszPortName;

    // The port name must begin with pwszPortType.
    if (_wcsnicmp(p, pwszPortType, cchPortType) != 0)
        return FALSE;

    p += cchPortType;

    // Now an arbitrary number of digits may follow.
    while (*p >= L'0' && *p <= L'9')
        p++;

    // Finally, the legacy port must be terminated by a colon.
    if (*p != ':')
        return FALSE;

    // If this is the end of the string, we have a legacy port.
    p++;
    return (*p == L'\0');
}

/**
 * @name _ClosePortHandles
 *
 * Closes a port of any type if it's open.
 * Removes any saved mapping or existing definition of a NONSPOOLED device mapping.
 *
 * @param pPort
 * The port you want to close.
 */
static void
_ClosePortHandles(PLOCALMON_PORT pPort)
{
    PWSTR pwszNonspooledPortName;
    PWSTR pwszPortNameWithoutColon;

    // A port is already fully closed if the file handle is invalid.
    if (pPort->hFile == INVALID_HANDLE_VALUE)
        return;

    // Close the file handle.
    CloseHandle(pPort->hFile);
    pPort->hFile = INVALID_HANDLE_VALUE;

    // A NONSPOOLED port was only created if pwszMapping contains the current port mapping.
    if (!pPort->pwszMapping)
        return;

    // Free the information about the current mapping.
    DllFreeSplStr(pPort->pwszMapping);
    pPort->pwszMapping = NULL;

    // Finally get the required strings and remove the DOS device definition for the NONSPOOLED port.
    if (GetPortNameWithoutColon(pPort->pwszPortName, &pwszPortNameWithoutColon) == ERROR_SUCCESS)
    {
        if (_GetNonspooledPortName(pwszPortNameWithoutColon, &pwszNonspooledPortName) == ERROR_SUCCESS)
        {
            DefineDosDeviceW(DDD_REMOVE_DEFINITION, pwszNonspooledPortName, NULL);
            DllFreeSplMem(pwszNonspooledPortName);
        }

        DllFreeSplMem(pwszPortNameWithoutColon);
    }
}

/**
 * @name _CreateNonspooledPort
 *
 * Queries the system-wide device definition of the given port.
 * If such a definition exists, it's a legacy port remapped to a named pipe by the spooler.
 * In this case, the function creates and opens a NONSPOOLED device definition to the most recent mapping before the current one (usually the physical device).
 *
 * @param pPort
 * Pointer to the LOCALMON_PORT structure of the desired port.
 *
 * @return
 * TRUE if a NONSPOOLED port was successfully created, FALSE otherwise.
 * A more specific error code can be obtained through GetLastError.
 * In particular, if the return value is FALSE and GetLastError returns ERROR_SUCCESS, no NONSPOOLED port is needed for this port.
 */
static BOOL
_CreateNonspooledPort(PLOCALMON_PORT pPort)
{
    const WCHAR wszLocalSlashes[] = L"\\\\.\\";
    const DWORD cchLocalSlashes = _countof(wszLocalSlashes) - 1;

    const WCHAR wszSpoolerNamedPipe[] = L"\\Device\\NamedPipe\\Spooler\\";
    const DWORD cchSpoolerNamedPipe = _countof(wszSpoolerNamedPipe) - 1;

    BOOL bReturnValue = FALSE;
    DWORD cchPortNameWithoutColon;
    DWORD dwErrorCode;
    HANDLE hToken = NULL;
    PWSTR p;
    PWSTR pwszDeviceMappings = NULL;
    PWSTR pwszNonspooledFileName = NULL;
    PWSTR pwszNonspooledPortName = NULL;
    PWSTR pwszPipeName = NULL;
    PWSTR pwszPortNameWithoutColon = NULL;

    // We need the port name without the trailing colon.
    dwErrorCode = GetPortNameWithoutColon(pPort->pwszPortName, &pwszPortNameWithoutColon);
    if (dwErrorCode == ERROR_INVALID_PARAMETER)
    {
        // This port has no trailing colon, so we also need no NONSPOOLED mapping for it.
        dwErrorCode = ERROR_SUCCESS;
        goto Cleanup;
    }
    else if (dwErrorCode != ERROR_SUCCESS)
    {
        // Another unexpected failure.
        goto Cleanup;
    }

    cchPortNameWithoutColon = wcslen(pwszPortNameWithoutColon);

    // The spooler has usually remapped the legacy port to a named pipe of the format in wszSpoolerNamedPipe.
    // Construct the device name of this pipe.
    pwszPipeName = DllAllocSplMem((cchSpoolerNamedPipe + cchPortNameWithoutColon + 1) * sizeof(WCHAR));
    if (!pwszPipeName)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    CopyMemory(pwszPipeName, wszSpoolerNamedPipe, cchSpoolerNamedPipe * sizeof(WCHAR));
    CopyMemory(&pwszPipeName[cchSpoolerNamedPipe], pwszPortNameWithoutColon, (cchPortNameWithoutColon + 1) * sizeof(WCHAR));

    // QueryDosDeviceW is one of the shitty APIs that gives no information about the required buffer size and wants you to know it by pure magic.
    // Examples show that a value of MAX_PATH * sizeof(WCHAR) is usually taken here, so we have no other option either.
    pwszDeviceMappings = DllAllocSplMem(MAX_PATH * sizeof(WCHAR));
    if (!pwszDeviceMappings)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Switch to the SYSTEM context, because we're only interested in creating NONSPOOLED ports for system-wide ports.
    // User-local ports (like _some_ redirected networked ones) aren't remapped by the spooler and can be opened directly.
    hToken = RevertToPrinterSelf();
    if (!hToken)
    {
        dwErrorCode = GetLastError();
        ERR("RevertToPrinterSelf failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // QueryDosDeviceW returns the current mapping and a list of prior mappings of this legacy port, which is managed as a DOS device in the system.
    if (!QueryDosDeviceW(pwszPortNameWithoutColon, pwszDeviceMappings, MAX_PATH))
    {
        // No system-wide port exists, so we also need no NONSPOOLED mapping.
        dwErrorCode = ERROR_SUCCESS;
        goto Cleanup;
    }

    // Check if this port has already been opened by _CreateNonspooledPort previously.
    if (pPort->pwszMapping)
    {
        // In this case, we just need to do something if the mapping has changed.
        // Therefore, check if the stored mapping equals the current mapping.
        if (wcscmp(pPort->pwszMapping, pwszDeviceMappings) == 0)
        {
            // We don't need to do anything in this case.
            dwErrorCode = ERROR_SUCCESS;
            goto Cleanup;
        }
        else
        {
            // Close the open file handle and free the memory for pwszMapping before remapping.
            CloseHandle(pPort->hFile);
            pPort->hFile = INVALID_HANDLE_VALUE;

            DllFreeSplStr(pPort->pwszMapping);
            pPort->pwszMapping = NULL;
        }
    }

    // The port is usually mapped to the named pipe and this is how we received our data for printing.
    // What we now need for accessing the actual port is the most recent mapping different from the named pipe.
    p = pwszDeviceMappings;

    for (;;)
    {
        if (!*p)
        {
            // We reached the end of the list without finding a mapping.
            ERR("Can't find a suitable mapping for the port \"%S\"!", pPort->pwszPortName);
            goto Cleanup;
        }

        if (_wcsicmp(p, pwszPipeName) != 0)
            break;

        // Advance to the next mapping in the list.
        p += wcslen(p) + 1;
    }

    // We now want to create a DOS device "NONSPOOLED_<PortName>" to this mapping, so that we're able to open it through CreateFileW.
    dwErrorCode = _GetNonspooledPortName(pwszPortNameWithoutColon, &pwszNonspooledPortName);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Delete a possibly existing NONSPOOLED device before creating the new one, so we don't stack up device definitions.
    DefineDosDeviceW(DDD_REMOVE_DEFINITION, pwszNonspooledPortName, NULL);

    if (!DefineDosDeviceW(DDD_RAW_TARGET_PATH, pwszNonspooledPortName, p))
    {
        dwErrorCode = GetLastError();
        ERR("DefineDosDeviceW failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // This is all we needed to do in SYSTEM context.
    ImpersonatePrinterClient(hToken);
    hToken = NULL;

    // Construct the file name to our created device for CreateFileW.
    pwszNonspooledFileName = DllAllocSplMem((cchLocalSlashes + cchNonspooledPrefix + cchPortNameWithoutColon + 1) * sizeof(WCHAR));
    if (!pwszNonspooledFileName)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    CopyMemory(pwszNonspooledFileName, wszLocalSlashes, cchLocalSlashes * sizeof(WCHAR));
    CopyMemory(&pwszNonspooledFileName[cchLocalSlashes], wszNonspooledPrefix, cchNonspooledPrefix * sizeof(WCHAR));
    CopyMemory(&pwszNonspooledFileName[cchLocalSlashes + cchNonspooledPrefix], pwszPortNameWithoutColon, (cchPortNameWithoutColon + 1) * sizeof(WCHAR));

    // Finally open it for reading and writing.
    pPort->hFile = CreateFileW(pwszNonspooledFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (pPort->hFile == INVALID_HANDLE_VALUE)
    {
        dwErrorCode = GetLastError();
        ERR("CreateFileW failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Store the current mapping of the port, so that we can check if it has changed.
    pPort->pwszMapping = AllocSplStr(pwszDeviceMappings);
    if (!pPort->pwszMapping)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    bReturnValue = TRUE;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (hToken)
        ImpersonatePrinterClient(hToken);

    if (pwszDeviceMappings)
        DllFreeSplMem(pwszDeviceMappings);

    if (pwszNonspooledFileName)
        DllFreeSplMem(pwszNonspooledFileName);

    if (pwszNonspooledPortName)
        DllFreeSplMem(pwszNonspooledPortName);

    if (pwszPipeName)
        DllFreeSplMem(pwszPipeName);

    if (pwszPortNameWithoutColon)
        DllFreeSplMem(pwszPortNameWithoutColon);

    SetLastError(dwErrorCode);
    return bReturnValue;
}

static PLOCALMON_PORT
_FindPort(PLOCALMON_HANDLE pLocalmon, PCWSTR pwszPortName)
{
    PLIST_ENTRY pEntry;
    PLOCALMON_PORT pPort;

    for (pEntry = pLocalmon->RegistryPorts.Flink; pEntry != &pLocalmon->RegistryPorts; pEntry = pEntry->Flink)
    {
        pPort = CONTAINING_RECORD(pEntry, LOCALMON_PORT, Entry);

        if (wcscmp(pPort->pwszPortName, pwszPortName) == 0)
            return pPort;
    }

    return NULL;
}

static DWORD
_LocalmonEnumPortsLevel1(PLOCALMON_HANDLE pLocalmon, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD cbPortName;
    DWORD dwErrorCode;
    DWORD dwPortCount = 0;
    PBYTE pPortInfo;
    PBYTE pPortString;
    PLIST_ENTRY pEntry;
    PLOCALMON_PORT pPort;
    PORT_INFO_1W PortInfo1;

    // Count the required buffer size and the number of datatypes.
    for (pEntry = pLocalmon->RegistryPorts.Flink; pEntry != &pLocalmon->RegistryPorts; pEntry = pEntry->Flink)
    {
        pPort = CONTAINING_RECORD(pEntry, LOCALMON_PORT, Entry);

        cbPortName = (wcslen(pPort->pwszPortName) + 1) * sizeof(WCHAR);
        *pcbNeeded += sizeof(PORT_INFO_1W) + cbPortName;
        dwPortCount++;
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Put the strings right after the last PORT_INFO_1W structure.
    pPortInfo = pPorts;
    pPortString = pPorts + dwPortCount * sizeof(PORT_INFO_1W);

    // Copy over all ports.
    for (pEntry = pLocalmon->RegistryPorts.Flink; pEntry != &pLocalmon->RegistryPorts; pEntry = pEntry->Flink)
    {
        pPort = CONTAINING_RECORD(pEntry, LOCALMON_PORT, Entry);

        // Copy the port name.
        PortInfo1.pName = (PWSTR)pPortString;
        cbPortName = (wcslen(pPort->pwszPortName) + 1) * sizeof(WCHAR);
        CopyMemory(pPortString, pPort->pwszPortName, cbPortName);
        pPortString += cbPortName;

        // Copy the structure and advance to the next one in the output buffer.
        CopyMemory(pPortInfo, &PortInfo1, sizeof(PORT_INFO_1W));
        pPortInfo += sizeof(PORT_INFO_1W);
    }

    *pcReturned = dwPortCount;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    return dwErrorCode;
}

static DWORD
_LocalmonEnumPortsLevel2(PLOCALMON_HANDLE pLocalmon, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD cbPortName;
    DWORD dwErrorCode;
    DWORD dwPortCount = 0;
    PBYTE pPortInfo;
    PBYTE pPortString;
    PLIST_ENTRY pEntry;
    PLOCALMON_PORT pPort;
    PORT_INFO_2W PortInfo2;

    // Count the required buffer size and the number of datatypes.
    for (pEntry = pLocalmon->RegistryPorts.Flink; pEntry != &pLocalmon->RegistryPorts; pEntry = pEntry->Flink)
    {
        pPort = CONTAINING_RECORD(pEntry, LOCALMON_PORT, Entry);

        cbPortName = (wcslen(pPort->pwszPortName) + 1) * sizeof(WCHAR);
        *pcbNeeded += sizeof(PORT_INFO_2W) + cbPortName + cbLocalMonitor + cbLocalPort;
        dwPortCount++;
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Put the strings right after the last PORT_INFO_2W structure.
    pPortInfo = pPorts;
    pPortString = pPorts + dwPortCount * sizeof(PORT_INFO_2W);

    // Copy over all ports.
    for (pEntry = pLocalmon->RegistryPorts.Flink; pEntry != &pLocalmon->RegistryPorts; pEntry = pEntry->Flink)
    {
        pPort = CONTAINING_RECORD(pEntry, LOCALMON_PORT, Entry);

        // All local ports are writable and readable.
        PortInfo2.fPortType = PORT_TYPE_WRITE | PORT_TYPE_READ;
        PortInfo2.Reserved = 0;

        // Copy the port name.
        PortInfo2.pPortName = (PWSTR)pPortString;
        cbPortName = (wcslen(pPort->pwszPortName) + 1) * sizeof(WCHAR);
        CopyMemory(pPortString, pPort->pwszPortName, cbPortName);
        pPortString += cbPortName;

        // Copy the monitor name.
        PortInfo2.pMonitorName = (PWSTR)pPortString;
        CopyMemory(pPortString, pwszLocalMonitor, cbLocalMonitor);
        pPortString += cbLocalMonitor;

        // Copy the description.
        PortInfo2.pDescription = (PWSTR)pPortString;
        CopyMemory(pPortString, pwszLocalPort, cbLocalPort);
        pPortString += cbLocalPort;

        // Copy the structure and advance to the next one in the output buffer.
        CopyMemory(pPortInfo, &PortInfo2, sizeof(PORT_INFO_2W));
        pPortInfo += sizeof(PORT_INFO_2W);
    }

    *pcReturned = dwPortCount;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    return dwErrorCode;
}

/**
 * @name _SetTransmissionRetryTimeout
 *
 * Checks if the given port is a physical one and sets the transmission retry timeout in this case using the value from registry.
 *
 * @param pPort
 * The port to operate on.
 *
 * @return
 * TRUE if the given port is a physical one, FALSE otherwise.
 */
static BOOL
_SetTransmissionRetryTimeout(PLOCALMON_PORT pPort)
{
    COMMTIMEOUTS CommTimeouts;

    // Get the timeout from the port.
    if (!GetCommTimeouts(pPort->hFile, &CommTimeouts))
        return FALSE;

    // Set the timeout using the value from registry.
    CommTimeouts.WriteTotalTimeoutConstant = GetLPTTransmissionRetryTimeout() * 1000;
    SetCommTimeouts(pPort->hFile, &CommTimeouts);

    return TRUE;
}

BOOL WINAPI
LocalmonClosePort(HANDLE hPort)
{
    PLOCALMON_PORT pPort = (PLOCALMON_PORT)hPort;

    TRACE("LocalmonClosePort(%p)\n", hPort);

    // Sanity checks
    if (!pPort)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Close the file handle, free memory for pwszMapping and delete any NONSPOOLED port.
    _ClosePortHandles(pPort);

    // Close any open printer handle.
    if (pPort->hPrinter)
    {
        ClosePrinter(pPort->hPrinter);
        pPort->hPrinter = NULL;
    }

    // Free virtual FILE: ports which were created in LocalmonOpenPort.
    if (pPort->PortType == PortType_FILE)
    {
        EnterCriticalSection(&pPort->pLocalmon->Section);
        RemoveEntryList(&pPort->Entry);
        LeaveCriticalSection(&pPort->pLocalmon->Section);
        DllFreeSplMem(pPort);
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI
LocalmonEndDocPort(HANDLE hPort)
{
    PLOCALMON_PORT pPort = (PLOCALMON_PORT)hPort;

    TRACE("LocalmonEndDocPort(%p)\n", hPort);

    // Sanity checks
    if (!pPort)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Ending a document requires starting it first :-P
    if (pPort->bStartedDoc)
    {
        // Close all ports opened in StartDocPort.
        // That is, all but physical LPT ports (opened in OpenPort).
        if (pPort->PortType != PortType_PhysicalLPT)
            _ClosePortHandles(pPort);

        // Report our progress.
        SetJobW(pPort->hPrinter, pPort->dwJobID, 0, NULL, JOB_CONTROL_SENT_TO_PRINTER);

        // We're done with the printer.
        ClosePrinter(pPort->hPrinter);
        pPort->hPrinter = NULL;

        // A new document can now be started again.
        pPort->bStartedDoc = FALSE;
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI
LocalmonEnumPorts(HANDLE hMonitor, PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    PLOCALMON_HANDLE pLocalmon = (PLOCALMON_HANDLE)hMonitor;

    TRACE("LocalmonEnumPorts(%p, %S, %lu, %p, %lu, %p, %p)\n", hMonitor, pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned);

    // Sanity checks
    if (!pLocalmon || (cbBuf && !pPorts) || !pcbNeeded || !pcReturned)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (Level > 2)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    EnterCriticalSection(&pLocalmon->Section);

    // The function behaves differently for each level.
    if (Level == 1)
        dwErrorCode = _LocalmonEnumPortsLevel1(pLocalmon, pPorts, cbBuf, pcbNeeded, pcReturned);
    else if (Level == 2)
        dwErrorCode = _LocalmonEnumPortsLevel2(pLocalmon, pPorts, cbBuf, pcbNeeded, pcReturned);

    LeaveCriticalSection(&pLocalmon->Section);

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

/*
 * @name LocalmonGetPrinterDataFromPort
 *
 * Performs a DeviceIoControl call for the given port.
 *
 * @param hPort
 * The port to operate on.
 *
 * @param ControlID
 * The dwIoControlCode passed to DeviceIoControl. Must not be zero!
 *
 * @param pValueName
 * This parameter is ignored.
 *
 * @param lpInBuffer
 * The lpInBuffer passed to DeviceIoControl.
 *
 * @param cbInBuffer
 * The nInBufferSize passed to DeviceIoControl.
 *
 * @param lpOutBuffer
 * The lpOutBuffer passed to DeviceIoControl.
 *
 * @param cbOutBuffer
 * The nOutBufferSize passed to DeviceIoControl.
 *
 * @param lpcbReturned
 * The lpBytesReturned passed to DeviceIoControl. Must not be zero!
 *
 * @return
 * TRUE if the DeviceIoControl call was successful, FALSE otherwise.
 * A more specific error code can be obtained through GetLastError.
 */
BOOL WINAPI
LocalmonGetPrinterDataFromPort(HANDLE hPort, DWORD ControlID, PWSTR pValueName, PWSTR lpInBuffer, DWORD cbInBuffer, PWSTR lpOutBuffer, DWORD cbOutBuffer, PDWORD lpcbReturned)
{
    BOOL bOpenedPort = FALSE;
    DWORD dwErrorCode;
    PLOCALMON_PORT pPort = (PLOCALMON_PORT)hPort;

    TRACE("LocalmonGetPrinterDataFromPort(%p, %lu, %p, %p, %lu, %p, %lu, %p)\n", hPort, ControlID, pValueName, lpInBuffer, cbInBuffer, lpOutBuffer, cbOutBuffer, lpcbReturned);

    // Sanity checks
    if (!pPort || !ControlID || !lpcbReturned)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // If this is a serial port, a temporary file handle may be opened.
    if (pPort->PortType == PortType_PhysicalCOM)
    {
        if (_CreateNonspooledPort(pPort))
        {
            bOpenedPort = TRUE;
        }
        else if (GetLastError() != ERROR_SUCCESS)
        {
            dwErrorCode = GetLastError();
            goto Cleanup;
        }
    }
    else if (pPort->hFile == INVALID_HANDLE_VALUE)
    {
        // All other port types need to be opened already.
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Pass the parameters to DeviceIoControl.
    if (!DeviceIoControl(pPort->hFile, ControlID, lpInBuffer, cbInBuffer, lpOutBuffer, cbOutBuffer, lpcbReturned, NULL))
    {
        dwErrorCode = GetLastError();
        ERR("DeviceIoControl failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (bOpenedPort)
        _ClosePortHandles(pPort);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalmonOpenPort(HANDLE hMonitor, PWSTR pName, PHANDLE pHandle)
{
    DWORD dwErrorCode;
    PLOCALMON_HANDLE pLocalmon = (PLOCALMON_HANDLE)hMonitor;
    PLOCALMON_PORT pPort;

    TRACE("LocalmonOpenPort(%p, %S, %p)\n", hMonitor, pName, pHandle);

    // Sanity checks
    if (!pLocalmon || !pName || !pHandle)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Check if this is a FILE: port.
    if (_wcsicmp(pName, L"FILE:") == 0)
    {
        // For FILE:, we create a virtual port for each request.
        pPort = DllAllocSplMem(sizeof(LOCALMON_PORT));
        pPort->pLocalmon = pLocalmon;
        pPort->PortType = PortType_FILE;
        pPort->hFile = INVALID_HANDLE_VALUE;

        // Add it to the list of file ports.
        EnterCriticalSection(&pLocalmon->Section);
        InsertTailList(&pLocalmon->FilePorts, &pPort->Entry);
    }
    else
    {
        EnterCriticalSection(&pLocalmon->Section);

        // Check if the port name is valid.
        pPort = _FindPort(pLocalmon, pName);
        if (!pPort)
        {
            LeaveCriticalSection(&pLocalmon->Section);
            dwErrorCode = ERROR_UNKNOWN_PORT;
            goto Cleanup;
        }

        // Even if this API is called OpenPort, port file handles aren't always opened here :-P
        // Windows only does this for physical LPT ports here to enable bidirectional communication with the printer outside of jobs (using ReadPort and WritePort).
        // The others are only opened per job in StartDocPort.
        if (_IsLegacyPort(pName, L"LPT"))
        {
            // Try to create a NONSPOOLED port and open it.
            if (_CreateNonspooledPort(pPort))
            {
                // Set the transmission retry timeout for the ReadPort and WritePort calls.
                // This also checks if this port is a physical one.
                if (_SetTransmissionRetryTimeout(pPort))
                {
                    // This is definitely a physical LPT port!
                    pPort->PortType = PortType_PhysicalLPT;
                }
                else
                {
                    // This is no physical port, so don't keep its handle open.
                    _ClosePortHandles(pPort);
                }
            }
            else if (GetLastError() != ERROR_SUCCESS)
            {
                LeaveCriticalSection(&pLocalmon->Section);
                dwErrorCode = GetLastError();
                goto Cleanup;
            }
        }
        else if (_IsLegacyPort(pName, L"COM"))
        {
            // COM ports can't be redirected over the network, so this is a physical one.
            pPort->PortType = PortType_PhysicalCOM;
        }
    }

    LeaveCriticalSection(&pLocalmon->Section);

    // Return our fetched LOCALMON_PORT structure in the handle.
    *pHandle = (PHANDLE)pPort;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

/*
 * @name LocalmonSetPortTimeOuts
 *
 * Performs a SetCommTimeouts call for the given port.
 *
 * @param hPort
 * The port to operate on.
 *
 * @param lpCTO
 * Pointer to a COMMTIMEOUTS structure that is passed to SetCommTimeouts.
 *
 * @param Reserved
 * Reserved parameter, must be 0.
 *
 * @return
 * TRUE if the SetCommTimeouts call was successful, FALSE otherwise.
 * A more specific error code can be obtained through GetLastError.
 */
BOOL WINAPI
LocalmonSetPortTimeOuts(HANDLE hPort, LPCOMMTIMEOUTS lpCTO, DWORD Reserved)
{
    BOOL bOpenedPort = FALSE;
    DWORD dwErrorCode;
    PLOCALMON_PORT pPort = (PLOCALMON_PORT)hPort;

    TRACE("LocalmonSetPortTimeOuts(%p, %p, %lu)\n", hPort, lpCTO, Reserved);

    // Sanity checks
    if (!pPort || !lpCTO)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // If this is a serial port, a temporary file handle may be opened.
    if (pPort->PortType == PortType_PhysicalCOM)
    {
        if (_CreateNonspooledPort(pPort))
        {
            bOpenedPort = TRUE;
        }
        else if (GetLastError() != ERROR_SUCCESS)
        {
            dwErrorCode = GetLastError();
            goto Cleanup;
        }
    }
    else if (pPort->hFile == INVALID_HANDLE_VALUE)
    {
        // All other port types need to be opened already.
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Finally pass the parameters to SetCommTimeouts.
    if (!SetCommTimeouts(pPort->hFile, lpCTO))
    {
        dwErrorCode = GetLastError();
        ERR("SetCommTimeouts failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (bOpenedPort)
        _ClosePortHandles(pPort);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalmonReadPort(HANDLE hPort, PBYTE pBuffer, DWORD cbBuffer, PDWORD pcbRead)
{
    BOOL bOpenedPort = FALSE;
    DWORD dwErrorCode;
    PLOCALMON_PORT pPort = (PLOCALMON_PORT)hPort;

    TRACE("LocalmonReadPort(%p, %p, %lu, %p)\n", hPort, pBuffer, cbBuffer, pcbRead);

    // Sanity checks
    if (!pPort || (cbBuffer && !pBuffer) || !pcbRead)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Reading is only supported for physical ports.
    if (pPort->PortType != PortType_PhysicalCOM && pPort->PortType != PortType_PhysicalLPT)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // If this is a serial port, a temporary file handle may be opened.
    if (pPort->PortType == PortType_PhysicalCOM)
    {
        if (_CreateNonspooledPort(pPort))
        {
            bOpenedPort = TRUE;
        }
        else if (GetLastError() != ERROR_SUCCESS)
        {
            dwErrorCode = GetLastError();
            goto Cleanup;
        }
    }

    // Pass the parameters to ReadFile.
    if (!ReadFile(pPort->hFile, pBuffer, cbBuffer, pcbRead, NULL))
    {
        dwErrorCode = GetLastError();
        ERR("ReadFile failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

Cleanup:
    if (bOpenedPort)
        _ClosePortHandles(pPort);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalmonStartDocPort(HANDLE hPort, PWSTR pPrinterName, DWORD JobId, DWORD Level, PBYTE pDocInfo)
{
    DWORD dwErrorCode;
    PDOC_INFO_1W pDocInfo1 = (PDOC_INFO_1W)pDocInfo;        // DOC_INFO_1W is the least common denominator for both DOC_INFO levels.
    PLOCALMON_PORT pPort = (PLOCALMON_PORT)hPort;

    TRACE("LocalmonStartDocPort(%p, %S, %lu, %lu, %p)\n", hPort, pPrinterName, JobId, Level, pDocInfo);

    // Sanity checks
    if (!pPort || !pPrinterName || (pPort->PortType == PortType_FILE && (!pDocInfo1 || !pDocInfo1->pOutputFile || !*pDocInfo1->pOutputFile)))
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (Level > 2)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Calling StartDocPort multiple times isn't considered a failure, but we don't need to do anything then.
    if (pPort->bStartedDoc)
    {
        dwErrorCode = ERROR_SUCCESS;
        goto Cleanup;
    }

    // Open a handle to the given printer for later reporting our progress using SetJobW.
    if (!OpenPrinterW(pPrinterName, &pPort->hPrinter, NULL))
    {
        dwErrorCode = GetLastError();
        ERR("OpenPrinterW failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // We need our Job ID for SetJobW as well.
    pPort->dwJobID = JobId;

    // Check the port type.
    if (pPort->PortType == PortType_PhysicalLPT)
    {
        // Update the NONSPOOLED mapping if the port mapping has changed since our OpenPort call.
        if (!_CreateNonspooledPort(pPort) && GetLastError() != ERROR_SUCCESS)
        {
            dwErrorCode = GetLastError();
            goto Cleanup;
        }

        // Update the transmission retry timeout as well.
        _SetTransmissionRetryTimeout(pPort);
    }
    else if(pPort->PortType == PortType_FILE)
    {
        // This is a FILE: port. Open the output file given in the Document Info.
        pPort->hFile = CreateFileW(pDocInfo1->pOutputFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
        if (pPort->hFile == INVALID_HANDLE_VALUE)
        {
            dwErrorCode = GetLastError();
            goto Cleanup;
        }
    }
    else
    {
        // This can be:
        //    - a physical COM port
        //    - a non-physical LPT port (e.g. with "net use LPT1 ...")
        //    - any other port (e.g. a file or a shared printer installed as a local port)
        //
        // For all these cases, we try to create a NONSPOOLED port per job.
        // If _CreateNonspooledPort reports that no NONSPOOLED port is necessary, we can just open the port name.
        if (!_CreateNonspooledPort(pPort))
        {
            if (GetLastError() == ERROR_SUCCESS)
            {
                pPort->hFile = CreateFileW(pPort->pwszPortName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
                if (pPort->hFile == INVALID_HANDLE_VALUE)
                {
                    dwErrorCode = GetLastError();
                    goto Cleanup;
                }
            }
            else
            {
                dwErrorCode = GetLastError();
                goto Cleanup;
            }
        }
    }

    // We were successful!
    dwErrorCode = ERROR_SUCCESS;
    pPort->bStartedDoc = TRUE;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalmonWritePort(HANDLE hPort, PBYTE pBuffer, DWORD cbBuf, PDWORD pcbWritten)
{
    BOOL bOpenedPort = FALSE;
    DWORD dwErrorCode;
    PLOCALMON_PORT pPort = (PLOCALMON_PORT)hPort;

    TRACE("LocalmonWritePort(%p, %p, %lu, %p)\n", hPort, pBuffer, cbBuf, pcbWritten);

    // Sanity checks
    if (!pPort || (cbBuf && !pBuffer) || !pcbWritten)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // If this is a serial port, a temporary file handle may be opened.
    if (pPort->PortType == PortType_PhysicalCOM)
    {
        if (_CreateNonspooledPort(pPort))
        {
            bOpenedPort = TRUE;
        }
        else if (GetLastError() != ERROR_SUCCESS)
        {
            dwErrorCode = GetLastError();
            goto Cleanup;
        }
    }
    else if (pPort->hFile == INVALID_HANDLE_VALUE)
    {
        // All other port types need to be opened already.
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Pass the parameters to WriteFile.
    if (!WriteFile(pPort->hFile, pBuffer, cbBuf, pcbWritten, NULL))
    {
        dwErrorCode = GetLastError();
        ERR("WriteFile failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // If something was written down, we consider that a success, otherwise it's a timeout.
    if (*pcbWritten)
        dwErrorCode = ERROR_SUCCESS;
    else
        dwErrorCode = ERROR_TIMEOUT;

Cleanup:
    if (bOpenedPort)
        _ClosePortHandles(pPort);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
