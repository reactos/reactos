#include <stdio.h>
#include <wchar.h>
#include <windows.h>

/* DON'T CHANGE ORDER!!!! */
PCWSTR devices[3] = { L"\\\\.\\VBoxMiniRdrDN", L"\\??\\VBoxMiniRdrDN", L"\\Device\\VBoxMiniRdr" };

/* Taken from VBox header */
#define _MRX_MAX_DRIVE_LETTERS 26
#define IOCTL_MRX_VBOX_BASE FILE_DEVICE_NETWORK_FILE_SYSTEM
#define _MRX_VBOX_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_MRX_VBOX_BASE, request, method, access)
#define IOCTL_MRX_VBOX_ADDCONN       _MRX_VBOX_CONTROL_CODE(100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_GETLIST       _MRX_VBOX_CONTROL_CODE(103, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_GETGLOBALLIST _MRX_VBOX_CONTROL_CODE(104, METHOD_BUFFERED, FILE_ANY_ACCESS)
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

    for (i = 0; i < _MRX_MAX_DRIVE_LETTERS; i += 2)
    {
        wprintf(L"%c: %s\t%c: %s\n", 'A' + i, (outputBuffer[i] & 0x80 ? L"Active" : L"Inactive"),
                'A' + (i + 1), (outputBuffer[i + 1] & 0x80 ? L"Active" : L"Inactive"));
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
    else
    {
        printUsage();
        return 0;
    }
}
