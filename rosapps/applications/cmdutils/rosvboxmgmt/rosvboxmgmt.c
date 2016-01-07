/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS VBox Shared Folders Management
 * FILE:            cmdutils/hackssign/client.c
 * PURPOSE:         Communicate with VBox mini redirector to deal with shared folders
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>

/* DON'T CHANGE ORDER!!!! */
PCWSTR devices[3] = { L"\\\\.\\VBoxMiniRdrDN", L"\\??\\VBoxMiniRdrDN", L"\\Device\\VBoxMiniRdr" };

#define MAX_LEN 255

/* Taken from VBox header */
#define _MRX_MAX_DRIVE_LETTERS 26
#define IOCTL_MRX_VBOX_BASE FILE_DEVICE_NETWORK_FILE_SYSTEM
#define _MRX_VBOX_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_MRX_VBOX_BASE, request, method, access)
#define IOCTL_MRX_VBOX_ADDCONN       _MRX_VBOX_CONTROL_CODE(100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_GETLIST       _MRX_VBOX_CONTROL_CODE(103, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_GETGLOBALLIST _MRX_VBOX_CONTROL_CODE(104, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_GETGLOBALCONN _MRX_VBOX_CONTROL_CODE(105, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_START         _MRX_VBOX_CONTROL_CODE(106, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL performDevIoCtl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize)
{
    short i;
    BOOL ret;
    DWORD lpBytesReturned;
    HANDLE dev = INVALID_HANDLE_VALUE;

    wprintf(L"Trying to open a VBoxSRV device\n");
    for (i = 0; i < 3; ++i)
    {
        dev = CreateFile(devices[i],
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL, OPEN_EXISTING, 0, NULL);
        if (dev != INVALID_HANDLE_VALUE)
        {
            break;
        }
    }

    if (dev == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    wprintf(L"%s opened.\n", devices[i]);

    ret = DeviceIoControl(dev, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, &lpBytesReturned, NULL);
    wprintf(L"Done: it %s with error: %lx\n", (ret != 0 ? L"succeed" : L"failed"), (ret != 0 ? ERROR_SUCCESS : GetLastError()));

    CloseHandle(dev);

    return ret;
}

int startVBoxSrv(void)
{
    return (performDevIoCtl(IOCTL_MRX_VBOX_START, NULL, 0, NULL, 0) == FALSE);
}

int addConn(PCWSTR letter, PCWSTR path)
{
    BOOL ret;
    PWSTR inputBuffer;
    DWORD inputBufferSize;

    if (iswalpha(letter[0]) == 0)
    {
        wprintf(L"Invalid letter provided\n");
        return 1;
    }

    if (wcschr(path, L'\\') != NULL)
    {
        wprintf(L"Only give the name of a share\n");
        return 1;
    }

    inputBufferSize = (wcslen(path) + wcslen(devices[2]) + wcslen(L"\\;Z:\\vboxsvr\\")) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    inputBuffer = malloc(inputBufferSize);
    if (inputBuffer == NULL)
    {
        wprintf(L"Memory failure\n");
        return 1;
    }

    swprintf(inputBuffer, L"%s\\;%c:\\vboxsvr\\%s", devices[2], towupper(letter[0]), path);
    wprintf(L"Will create the following connection: %s\n", inputBuffer);
    ret = performDevIoCtl(IOCTL_MRX_VBOX_ADDCONN, inputBuffer, inputBufferSize, NULL, 0);
    free(inputBuffer);

    return (ret == FALSE);
}

int getList(void)
{
    short i;
    BOOL ret;
    DWORD outputBufferSize;
    char outputBuffer[_MRX_MAX_DRIVE_LETTERS];

    outputBufferSize = sizeof(outputBuffer);
    ret = performDevIoCtl(IOCTL_MRX_VBOX_GETLIST, NULL, 0, &outputBuffer, outputBufferSize);
    if (ret == FALSE)
    {
        return 1;
    }

    for (i = 0; i < _MRX_MAX_DRIVE_LETTERS; i += 2)
    {
        wprintf(L"%c: %s\t%c: %s\n", 'A' + i, (outputBuffer[i] == 0 ? L"FALSE" : L"TRUE"),
                'A' + (i + 1), (outputBuffer[i + 1] == 0 ? L"FALSE" : L"TRUE"));
    }

    return 0;
}

PCWSTR getGlobalConn(CHAR id)
{
    BOOL ret;
    static WCHAR name[MAX_LEN];

    ret = performDevIoCtl(IOCTL_MRX_VBOX_GETGLOBALCONN, &id, sizeof(id), name, sizeof(name));
    if (ret == FALSE)
    {
        return NULL;
    }

    name[MAX_LEN - 1] = 0;
    return name;
}

int getGlobalList(void)
{
    short i;
    BOOL ret;
    DWORD outputBufferSize;
    char outputBuffer[_MRX_MAX_DRIVE_LETTERS];

    outputBufferSize = sizeof(outputBuffer);
    memset(outputBuffer, 0, outputBufferSize);
    ret = performDevIoCtl(IOCTL_MRX_VBOX_GETGLOBALLIST, NULL, 0, &outputBuffer, outputBufferSize);
    if (ret == FALSE)
    {
        return 1;
    }

    for (i = 0; i < _MRX_MAX_DRIVE_LETTERS; ++i)
    {
        CHAR id = outputBuffer[i];
        BOOL active = ((id & 0x80) == 0x80);
        PCWSTR name = NULL;

        if (active)
        {
            name = getGlobalConn(id);
        }
        if (name == NULL)
        {
            name = L"None";
        }

        wprintf(L"%c: %s (%s)%c", 'A' + i, (active ? L"Active" : L"Inactive"), name, (i & 1 ? '\n' : '\t'));
    }

    return 0;
}

BOOL CreateUNCShortcut(PCWSTR share)
{
    HRESULT res;
    IShellLink *link;
    IPersistFile *persist;
    WCHAR path[MAX_PATH];
    WCHAR linkPath[MAX_PATH];

    res = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (void**)&link);
    if (FAILED(res))
    {
        return FALSE;
    }

    res = link->lpVtbl->QueryInterface(link, &IID_IPersistFile, (void **)&persist);
    if (FAILED(res))
    {
        link->lpVtbl->Release(link);
        return FALSE;
    }

    wcscpy(path, L"\\\\vboxsvr\\");
    wcscat(path, share);
    link->lpVtbl->SetPath(link, path);

    res = SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path);
    if (FAILED(res))
    {
        persist->lpVtbl->Release(persist);
        link->lpVtbl->Release(link);
        return FALSE;
    }

    wsprintf(linkPath, L"%s\\Browse %s (VBox).lnk", path, share);
    res = persist->lpVtbl->Save(persist, linkPath, TRUE);

    persist->lpVtbl->Release(persist);
    link->lpVtbl->Release(link);

    return SUCCEEDED(res);
}

int autoStart(void)
{
    short i;
    BOOL ret;
    DWORD outputBufferSize;
    char outputBuffer[_MRX_MAX_DRIVE_LETTERS];

    if (startVBoxSrv() != 0)
    {
        return 1;
    }

    outputBufferSize = sizeof(outputBuffer);
    memset(outputBuffer, 0, outputBufferSize);
    ret = performDevIoCtl(IOCTL_MRX_VBOX_GETGLOBALLIST, NULL, 0, &outputBuffer, outputBufferSize);
    if (ret == FALSE)
    {
        return 1;
    }

    CoInitialize(NULL);
    for (i = 0; i < _MRX_MAX_DRIVE_LETTERS; ++i)
    {
        CHAR id = outputBuffer[i];
        BOOL active = ((id & 0x80) == 0x80);
        PCWSTR name = NULL;

        if (active)
        {
            name = getGlobalConn(id);
        }
        if (name == NULL)
        {
            continue;
        }

        CreateUNCShortcut(name);
    }

    return 0;
}

void printUsage(void)
{
    wprintf(L"ReactOS VBox Shared Folders Management\n");
    wprintf(L"\tstart: start the VBox Shared folders (mandatory prior any operation!)\n");
    wprintf(L"\taddconn <letter> <share name>: add a connection\n");
    wprintf(L"\tgetlist: list connections\n");
    wprintf(L"\tgetgloballist: list available shares\n");
    wprintf(L"\tauto: automagically configure the VBox Shared folders and creates desktop folders\n");
}

int wmain(int argc, wchar_t *argv[])
{
    PCWSTR cmd;

    if (argc == 1)
    {
        printUsage();
        return 0;
    }

    cmd = argv[1];

    if (_wcsicmp(cmd, L"start") == 0)
    {
        return startVBoxSrv();
    }
    else if (_wcsicmp(cmd, L"addconn") == 0)
    {
        if (argc < 4)
        {
            printUsage();
            return 0;
        }

        return addConn(argv[2], argv[3]);
    }
    else if (_wcsicmp(cmd, L"getlist") == 0)
    {
        return getList();
    }
    else if (_wcsicmp(cmd, L"getgloballist") == 0)
    {
        return getGlobalList();
    }
    else if (_wcsicmp(cmd, L"auto") == 0)
    {
        return autoStart();
    }
    else
    {
        printUsage();
        return 0;
    }
}
