/*
 *  CTTY.C - ctty (Change TTY) command.
 *
 *  This command redirects the first three standard handles
 *  stdin, stdout, stderr to another terminal.
 *
 *
 *  History:
 *
 * 14 Aug 1998 (John P Price)
 * - Created dummy command.
 *
 * 2000/01/14 ska
 * + Added to change the first three handles to the given device name
 * + Supports only redirection of stdin and stdout, e.g.:
 *    C:\> CTTY COM1 >file
 *  -or-
 *    C:\> echo Hallo | CTTY COM1 | echo du
 *  The CTTY command effects the commands on the _next_ line.
 *
 * 20 Oct 2016 (Hermes Belusca-Maito)
 *     Port it to NT.
 */

#include "precomp.h"

#if defined(INCLUDE_CMD_CTTY) && defined(FEATURE_REDIRECTION)

static WORD
CheckTerminalDeviceType(IN LPCTSTR pszName)
{
    /* Console reserved "file" names */
    static const LPCWSTR DosLPTDevice = L"LPT";
    static const LPCWSTR DosCOMDevice = L"COM";
    static const LPCWSTR DosPRNDevice = L"PRN";
    static const LPCWSTR DosAUXDevice = L"AUX";
    static const LPCWSTR DosCONDevice = L"CON";
    static const LPCWSTR DosNULDevice = L"NUL";

    LPCWSTR DeviceName;
    ULONG DeviceNameInfo;
    WORD DeviceType = 0; // 0: Unknown; 1: CON; 2: COM etc...

#ifndef _UNICODE // UNICODE means that TCHAR == WCHAR == UTF-16
    /* Convert from the current process/thread's codepage to UTF-16 */
    DWORD len = strlen(pszName) + 1;
    WCHAR *buffer = cmd_alloc(len * sizeof(WCHAR));
    if (!buffer)
    {
        // SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    len = (DWORD)MultiByteToWideChar(CP_THREAD_ACP, // CP_ACP, CP_OEMCP
                                     0, pszName, (INT)len, buffer, (INT)len);
    DeviceName = buffer;
#else
    DeviceName = pszName;
#endif

    /*
     * Check whether we deal with a DOS device, and if so,
     * strip the path till the file name.
     * Therefore, things like \\.\CON or C:\some_path\COM1
     * are transformed into CON or COM1, for example.
     */
    DeviceNameInfo = RtlIsDosDeviceName_U(DeviceName);
    if (DeviceNameInfo != 0)
    {
        DeviceName = (LPCWSTR)((ULONG_PTR)DeviceName + ((DeviceNameInfo >> 16) & 0xFFFF));

        if (_wcsnicmp(DeviceName, DosCONDevice, 3) == 0)
        {
            DeviceType = 1;
        }
        else
        if ( _wcsnicmp(DeviceName, DosLPTDevice, 3) == 0 ||
             _wcsnicmp(DeviceName, DosCOMDevice, 3) == 0 ||
             _wcsnicmp(DeviceName, DosPRNDevice, 3) == 0 ||
             _wcsnicmp(DeviceName, DosAUXDevice, 3) == 0 ||
             _wcsnicmp(DeviceName, DosNULDevice, 3) == 0 )
        {
            DeviceType = 2;
        }
        // else DeviceType = 0;
    }

#ifndef _UNICODE
    cmd_free(buffer);
#endif

    return DeviceType;
}

/*
 * See also redir.c!PerformRedirection().
 *
 * The CTTY command allows only the usage of CON, COM, AUX, LPT, PRN and NUL
 * DOS devices as valid terminal devices. Everything else is forbidden.
 *
 * CTTY does not set ERRORLEVEL on error.
 */
INT cmd_ctty(LPTSTR param)
{
    static SECURITY_ATTRIBUTES SecAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    BOOL Success;
    WORD DeviceType;
    HANDLE hDevice, hStdHandles[3]; // hStdIn, hStdOut, hStdErr;

    /* The user asked for help */
    if (_tcsncmp(param, _T("/?"), 2) == 0)
    {
        ConOutResPaging(TRUE, STRING_CTTY_HELP);
        return 0;
    }

    if (!*param)
    {
        error_req_param_missing();
        return 1;
    }

    /* Check whether this is a valid terminal device name */
    DeviceType = CheckTerminalDeviceType(param);
    if (DeviceType == 1)
    {
        /*
         * Special case for CON device.
         *
         * We do not open CON with GENERIC_READ or GENERIC_WRITE as is,
         * but instead we separately open CONIN$ and CONOUT$ with both
         * GENERIC_READ | GENERIC_WRITE access.
         * We do so because otherwise, opening in particular CON with GENERIC_WRITE
         * only would open CONOUT$ with an handle not passing the IsConsoleHandle()
         * test, meaning that we could not use the full console functionalities.
         */
        BOOL bRetry = FALSE;

RetryOpenConsole:
        /*
         * If we previously failed in opening handles to the console,
         * this means the existing console is almost destroyed.
         * Close the existing console and allocate and open a new one.
         */
        if (bRetry)
        {
            FreeConsole();
            if (!AllocConsole())
                return 1;
        }

        /* Attempt to retrieve a handle for standard input */
        hStdHandles[0] = CreateFile(_T("CONIN$"),
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    &SecAttr,
                                    OPEN_EXISTING,
                                    0,
                                    NULL);
        if (hStdHandles[0] == INVALID_HANDLE_VALUE)
        {
            // TODO: Error
            // error_no_rw_device(param);

            if (bRetry)
                return 1;
            bRetry = TRUE;
            goto RetryOpenConsole;
        }

        /* Attempt to retrieve a handle for standard output.
         * Note that GENERIC_READ is needed for IsConsoleHandle() to succeed afterwards. */
        hStdHandles[1] = CreateFile(_T("CONOUT$"),
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    &SecAttr,
                                    OPEN_EXISTING,
                                    0,
                                    NULL);
        if (hStdHandles[1] == INVALID_HANDLE_VALUE)
        {
            // TODO: Error
            // error_no_rw_device(param);

            CloseHandle(hStdHandles[0]);

            if (bRetry)
                return 1;
            bRetry = TRUE;
            goto RetryOpenConsole;
        }

        /* Duplicate a handle for standard error */
        Success = DuplicateHandle(GetCurrentProcess(),
                                  hStdHandles[1],
                                  GetCurrentProcess(),
                                  &hStdHandles[2],
                                  0, // GENERIC_WRITE,
                                  TRUE,
                                  DUPLICATE_SAME_ACCESS /* 0 */);
        if (!Success)
        {
            // TODO: Error
            // error_no_rw_device(param);
            CloseHandle(hStdHandles[1]);
            CloseHandle(hStdHandles[0]);
            return 1;
        }
    }
    else if (DeviceType == 2)
    {
        /*
         * COM and the other devices can only be opened once.
         * Since we need different handles, we need to duplicate them.
         */

        /* Attempt to retrieve a handle to the device for read/write access */
        hDevice = CreateFile(param,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &SecAttr,
                             OPEN_EXISTING,
                             0, // FILE_FLAG_OVERLAPPED, // 0,
                             NULL);
        if (hDevice == INVALID_HANDLE_VALUE)
        {
            // TODO: Error
            // error_no_rw_device(param);
            return 1;
        }

        /* Duplicate a handle for standard input */
        Success = DuplicateHandle(GetCurrentProcess(),
                                  hDevice,
                                  GetCurrentProcess(),
                                  &hStdHandles[0],
                                  GENERIC_READ,
                                  TRUE,
                                  0);
        if (!Success)
        {
            // TODO: Error
            // error_no_rw_device(param);
            CloseHandle(hDevice);
            return 1;
        }

        /* Duplicate a handle for standard output */
        Success = DuplicateHandle(GetCurrentProcess(),
                                  hDevice,
                                  GetCurrentProcess(),
                                  &hStdHandles[1],
                                  GENERIC_WRITE,
                                  TRUE,
                                  0);
        if (!Success)
        {
            // TODO: Error
            // error_no_rw_device(param);
            CloseHandle(hStdHandles[0]);
            CloseHandle(hDevice);
            return 1;
        }

        /* Duplicate a handle for standard error */
        Success = DuplicateHandle(GetCurrentProcess(),
                                  hDevice,
                                  GetCurrentProcess(),
                                  &hStdHandles[2],
                                  GENERIC_WRITE,
                                  TRUE,
                                  0);
        if (!Success)
        {
            // TODO: Error
            // error_no_rw_device(param);
            CloseHandle(hStdHandles[1]);
            CloseHandle(hStdHandles[0]);
            CloseHandle(hDevice);
            return 1;
        }

        /* Now get rid of the main device handle */
        CloseHandle(hDevice);
    }
    else
    {
        // FIXME: Localize!
        ConOutPrintf(L"Invalid device '%s'\n", param);
        return 1;
    }

#if 0
  /* Now change the file descriptors:
    0 := rdonly
    1,2 := wronly

    if CTTY is called within a pipe or its I/O is redirected,
    oldinfd or oldoutfd is not equal to -1. In such case the
    old*fd is modified in order to effect the file descriptor
    after the redirections are restored. Otherwise a pipe or
    redirection would left CTTY in a half-made status.
  */
  // int failed;
  failed = dup2(f, 2);   /* no redirection support */
  if(oldinfd != -1)
  	dos_close(oldinfd);
  oldinfd = f;
  if(oldoutfd != -1)
  	dos_close(oldoutfd);
  if((oldoutfd = dup(f)) == -1)
  	failed = 1;

  if(failed)
    error_ctty_dup(param);

  return failed;
#endif

    /* Now set the standard handles */

    hDevice = GetHandle(0);
    if (hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(hDevice);
    SetHandle(0, hStdHandles[0]);

    hDevice = GetHandle(1);
    if (hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(hDevice);
    SetHandle(1, hStdHandles[1]);

    hDevice = GetHandle(2);
    if (hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(hDevice);
    SetHandle(2, hStdHandles[2]);

    return 0;
}

#endif /* INCLUDE_CMD_CTTY && FEATURE_REDIRECTION */

/* EOF */
