/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Hackssign application & driver
 * FILE:            cmdutils/hackssign/client.c
 * PURPOSE:         Client: Assign drive letter to shared folders for VMware/VBox VMs
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <stdio.h>
#include <wchar.h>
#include <windows.h>

#include "ioctl.h"

/* DON'T MESS AROUND THIS! */
typedef enum _HV_TYPES
{
    vmVMware,
    vmVirtualBox,
    vmMax,
} HV_TYPES;
typedef BOOL (*HV_DET)(void);
BOOL isVMware(void)
{
    HANDLE dev = CreateFile(L"\\\\.\\hgfs",
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, 0, NULL);
    if (dev != INVALID_HANDLE_VALUE)
    {
        wprintf(L"VMware detected\n");
        CloseHandle(dev);
        return TRUE;
    }

    return FALSE;
}
BOOL isVBox(void)
{
    HANDLE dev = CreateFile(L"\\\\.\\VBoxMiniRdrDN",
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, 0, NULL);
    if (dev != INVALID_HANDLE_VALUE)
    {
        wprintf(L"VirtualBox detected\n");
        CloseHandle(dev);
        return TRUE;
    }

    return FALSE;
}
PCWSTR dev[] = { L"\\Device\\hgfs\\;", L"\\Device\\VBoxMiniRdr\\;" };
PCWSTR unc[] = { L":\\vmware-host\\Shared Folders\\", L":\\vboxsvr\\" };
HV_DET det[] = { isVMware, isVBox };

HV_TYPES detectVM(void)
{
    HV_TYPES vm;

    for (vm = vmVMware; vm < vmMax; ++vm)
    {
        if (det[vm]() == TRUE)
        {
            break;
        }
    }

    return vm;
}

BOOL performDevIoCtl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize)
{
    BOOL ret;
    HANDLE dev;
    DWORD lpBytesReturned;

    dev = CreateFile(L"\\\\.\\hackssign",
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL, OPEN_EXISTING, 0, NULL);
    if (dev == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Opening device failed\n");
        return FALSE;
    }

    ret = DeviceIoControl(dev, dwIoControlCode, lpInBuffer, nInBufferSize, NULL, 0, &lpBytesReturned, NULL);
    wprintf(L"Done: it %s with error: %lx\n", (ret != 0 ? L"succeed" : L"failed"), (ret != 0 ? ERROR_SUCCESS : GetLastError()));

    CloseHandle(dev);

    return ret;
}


BOOL startService()
{
    PWSTR fileName;
    WCHAR path[MAX_PATH];
    SC_HANDLE hManager, hService;

    hManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hManager == NULL)
    {
        wprintf(L"Service manager opening failed\n");
        return FALSE;
    }

    hService = OpenService(hManager, L"hackssign", SERVICE_START | SERVICE_STOP | DELETE);
    if (hService == NULL)
    {
        if (GetModuleFileName(NULL, path, MAX_PATH) == 0)
        {
            wprintf(L"Getting own path failed\n");
            CloseServiceHandle(hManager);
            return FALSE;
        }

        fileName = wcsrchr(path, L'\\');
        if (fileName == NULL)
        {
            wprintf(L"Invalid path: %s\n", path);
            CloseServiceHandle(hManager);
            return FALSE;
        }

        ++fileName;
        *fileName = UNICODE_NULL;
        wcscat(path, L"hackssign_driver.sys");

        if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
        {
            wprintf(L"Driver not found: %s\n", path);
            CloseServiceHandle(hManager);
            return FALSE;
        }

        hService = CreateService(hManager, L"hackssign", L"hackssign",
                                 SERVICE_START | SERVICE_STOP | DELETE, SERVICE_KERNEL_DRIVER,
                                 SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, path, NULL, NULL,
                                 NULL, NULL, NULL);
        if (hService == NULL)
        {
            wprintf(L"Creating service failed\n");
            CloseServiceHandle(hManager);
            return FALSE;
        }
    }

    if (!StartService(hService, 0, NULL) &&
        GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
    {
        wprintf(L"Starting service failed\n");
        CloseServiceHandle(hService);
        CloseServiceHandle(hManager);
        return FALSE;
    }

    wprintf(L"Starting service succeed\n");
    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
    return TRUE;
}

int assignLetter(WCHAR letter, PCWSTR path)
{
    BOOL ret;
    DWORD len;
    PWSTR str;
    HV_TYPES vm;
    WCHAR capsLetter;
    DWORD inputBufferSize;
    PASSIGN_INPUT inputBuffer;

    vm = detectVM();
    if (vm == vmMax)
    {
        wprintf(L"Unsupported VM type\n");
        return 1;
    }

    if (iswalpha(letter) == 0)
    {
        wprintf(L"Invalid letter provided\n");
        return 1;
    }

    capsLetter = towupper(letter);
    if (capsLetter == L'C')
    {
        wprintf(L"Looks suspicious, won't proceed\n");
        return 1;
    }

    len = wcslen(path);
    if (len == 0)
    {
        wprintf(L"Invalid share name\n");
        return 1;
    }

    if (wcschr(path, L'\\') != NULL)
    {
        wprintf(L"Only give the name of a share\n");
        return 1;
    }

    inputBufferSize = len * sizeof(WCHAR) + sizeof(ASSIGN_INPUT) + (wcslen(dev[vm]) + wcslen(unc[vm]) + 2) * sizeof(WCHAR);
    inputBuffer = malloc(inputBufferSize);
    if (inputBuffer == NULL)
    {
        wprintf(L"Memory failure\n");
        return 1;
    }

    inputBuffer->letter = capsLetter;
    inputBuffer->offset = sizeof(ASSIGN_INPUT);
    inputBuffer->len = len * sizeof(WCHAR) + (wcslen(dev[vm]) + wcslen(unc[vm]) + 1) * sizeof(WCHAR);
    str = (PWSTR)((ULONG_PTR)inputBuffer + inputBuffer->offset);
    swprintf(str, L"%s%c%s%s", dev[vm], capsLetter, unc[vm], path);

    if (!startService())
    {
        free(inputBuffer);
        return 1;
    }

    Sleep(500);

    ret = performDevIoCtl(FSCTL_HACKSSIGN_ASSIGN, inputBuffer, inputBufferSize);
    free(inputBuffer);

    return (ret == FALSE);
}

int deleteLetter(WCHAR letter)
{
    WCHAR capsLetter;

    if (iswalpha(letter) == 0)
    {
        wprintf(L"Invalid letter provided\n");
        return 1;
    }

    capsLetter = towupper(letter);
    if (capsLetter == L'C')
    {
        wprintf(L"Looks suspicious, won't proceed\n");
        return 1;
    }

    if (!startService())
    {
        return 1;
    }

    Sleep(500);

    return (performDevIoCtl(FSCTL_HACKSSIGN_DELETE, &capsLetter, sizeof(WCHAR)) == FALSE);
}

int detect(void)
{
    HV_TYPES vm;

    vm = detectVM();
    if (vm == vmMax)
    {
        wprintf(L"Unsupported VM type\n");
        return 1;
    }

    return 0;
}

void printUsage(void)
{
    wprintf(L"ReactOS Hackssign application\n");
    wprintf(L"\assign <letter> <share name>: Assign a drive letter to the share\n");
    wprintf(L"\tdelete <letter>: delete driver letter assignation\n");
    wprintf(L"\tdetect: detect VM type\n");
}

int wmain(int argc, wchar_t *argv[])
{
    PCWSTR cmd;

    if (argc < 2)
    {
        printUsage();
        return 0;
    }

    cmd = argv[1];

    if (_wcsicmp(cmd, L"assign") == 0)
    {
        if (argc < 4)
        {
            printUsage();
            return 0;
        }

        return assignLetter(argv[2][0], argv[3]);
    }
    else if (_wcsicmp(cmd, L"delete") == 0)
    {
        if (argc < 3)
        {
            printUsage();
            return 0;
        }

        return deleteLetter(argv[2][0]);
    }
    else if (_wcsicmp(cmd, L"detect") == 0)
    {
        return detect();
    }
    else
    {
        printUsage();
        return 0;
    }
}
