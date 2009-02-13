/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main implementation file
 * COPYRIGHT:   Copyright 2008-2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

typedef void (WINAPI *GETSYSINFO)(LPSYSTEM_INFO);

APP_OPTIONS AppOptions = {0};
HANDLE hProcessHeap;
PCHAR AuthenticationRequestString = NULL;
PCHAR SystemInfoRequestString = NULL;

/**
 * Gets a value from a specified INI file and returns it converted to ASCII.
 *
 * @param AppName
 * The INI section to look in (lpAppName parameter passed to GetPrivateProfileStringW)
 *
 * @param KeyName
 * The key to look for in the specified section (lpKeyName parameter passed to GetPrivateProfileStringW)
 *
 * @param FileName
 * The path to the INI file
 *
 * @param ReturnedValue
 * Pointer to a CHAR pointer, which will receive the read and converted value.
 * The caller needs to HeapFree that value manually.
 *
 * @return
 * Returns the string length of the read value (in characters) or zero if we didn't get any value.
 */
static DWORD
IntGetINIValueA(PCWCH AppName, PCWCH KeyName, PCWCH FileName, char** ReturnedValue)
{
    DWORD Length;
    WCHAR Buffer[2048];

    /* Load the value into a temporary Unicode buffer */
    Length = GetPrivateProfileStringW(AppName, KeyName, NULL, Buffer, sizeof(Buffer) / sizeof(WCHAR), FileName);

    if(!Length)
        return 0;

    /* Convert the string to ANSI charset */
    *ReturnedValue = HeapAlloc(hProcessHeap, 0, Length + 1);
    WideCharToMultiByte(CP_ACP, 0, Buffer, Length + 1, *ReturnedValue, Length + 1, NULL, NULL);

    return Length;
}

/**
 * Gets the username and password from the "rosautotest.ini" file if the user enabled submitting the results to the web service.
 * The "rosautotest.ini" file should look like this:
 *
 * [Login]
 * UserName=TestMan
 * Password=TestPassword
 */
static BOOL
IntGetConfigurationValues()
{
    const CHAR PasswordProp[] = "&password=";
    const CHAR UserNameProp[] = "&username=";

    BOOL ReturnValue = FALSE;
    DWORD DataLength;
    DWORD Length;
    PCHAR Password = NULL;
    PCHAR UserName = NULL;
    WCHAR ConfigFile[MAX_PATH];

    /* We only need this if the results are going to be submitted */
    if(!AppOptions.Submit)
    {
        ReturnValue = TRUE;
        goto Cleanup;
    }

    /* Build the path to the configuration file from the application's path */
    GetModuleFileNameW(NULL, ConfigFile, MAX_PATH);
    Length = wcsrchr(ConfigFile, '\\') - ConfigFile;
    wcscpy(&ConfigFile[Length], L"\\rosautotest.ini");

    /* Check if it exists */
    if(GetFileAttributesW(ConfigFile) == INVALID_FILE_ATTRIBUTES)
    {
        StringOut("Missing \"rosautotest.ini\" configuration file!\n");
        goto Cleanup;
    }

    /* Get the required length of the authentication request string */
    DataLength = sizeof(UserNameProp) - 1;
    Length = IntGetINIValueA(L"Login", L"UserName", ConfigFile, &UserName);

    if(!Length)
    {
        StringOut("UserName is missing in the configuration file\n");
        goto Cleanup;
    }

    /* Some characters might need to be escaped and an escaped character takes 3 bytes */
    DataLength += 3 * Length;

    DataLength += sizeof(PasswordProp) - 1;
    Length = IntGetINIValueA(L"Login", L"Password", ConfigFile, &Password);

    if(!Length)
    {
        StringOut("Password is missing in the configuration file\n");
        goto Cleanup;
    }

    DataLength += 3 * Length;

    /* Build the string */
    AuthenticationRequestString = HeapAlloc(hProcessHeap, 0, DataLength + 1);

    strcpy(AuthenticationRequestString, UserNameProp);
    EscapeString(&AuthenticationRequestString[strlen(AuthenticationRequestString)], UserName);

    strcat(AuthenticationRequestString, PasswordProp);
    EscapeString(&AuthenticationRequestString[strlen(AuthenticationRequestString)], Password);

    ReturnValue = TRUE;

Cleanup:
    if(UserName)
        HeapFree(hProcessHeap, 0, UserName);

    if(Password)
        HeapFree(hProcessHeap, 0, Password);

    return ReturnValue;
}

/**
 * Determines on which platform we're running on.
 * Prepares the appropriate request strings needed if we want to submit test results to the web service.
 */
static BOOL
IntGetBuildAndPlatform()
{
    const CHAR PlatformProp[] = "&platform=";
    const CHAR RevisionProp[] = "&revision=";

    CHAR BuildNo[BUILDNO_LENGTH];
    CHAR Platform[PLATFORM_LENGTH];
    CHAR PlatformArchitecture[3];
    CHAR ProductType;
    DWORD DataLength;
    GETSYSINFO GetSysInfo;
    HANDLE hKernel32;
    OSVERSIONINFOEXW os;
    SYSTEM_INFO si;
    WCHAR WindowsDirectory[MAX_PATH];

    /* Get the build from the define */
    _ultoa(KERNEL_VERSION_BUILD_HEX, BuildNo, 10);

    /* Check if we are running under ReactOS from the SystemRoot directory */
    GetWindowsDirectoryW(WindowsDirectory, MAX_PATH);

    if(!_wcsnicmp(&WindowsDirectory[3], L"reactos", 7))
    {
        /* Yes, we are most-probably under ReactOS */
        strcpy(Platform, "reactos");
    }
    else
    {
        /* No, then use the info from GetVersionExW */
        os.dwOSVersionInfoSize = sizeof(os);

        if(!GetVersionExW((LPOSVERSIONINFOW)&os))
        {
            StringOut("GetVersionExW failed\n");
            return FALSE;
        }

        if(os.dwMajorVersion < 5)
        {
            StringOut("Application requires at least Windows 2000!\n");
            return FALSE;
        }

        if(os.wProductType == VER_NT_WORKSTATION)
            ProductType = 'w';
        else
            ProductType = 's';

        /* Print all necessary identification information into the Platform string */
        sprintf(Platform, "%lu.%lu.%lu.%u.%u.%c", os.dwMajorVersion, os.dwMinorVersion, os.dwBuildNumber, os.wServicePackMajor, os.wServicePackMinor, ProductType);
    }

    /* We also need to know about the processor architecture.
       To retrieve this information accurately, check whether "GetNativeSystemInfo" is exported and use it then, otherwise fall back to "GetSystemInfo". */
    hKernel32 = GetModuleHandleW(L"KERNEL32.DLL");
    GetSysInfo = (GETSYSINFO)GetProcAddress(hKernel32, "GetNativeSystemInfo");

    if(!GetSysInfo)
        GetSysInfo = (GETSYSINFO)GetProcAddress(hKernel32, "GetSystemInfo");

    GetSysInfo(&si);

    PlatformArchitecture[0] = '.';
    _ultoa(si.wProcessorArchitecture, &PlatformArchitecture[1], 10);
    PlatformArchitecture[2] = 0;
    strcat(Platform, PlatformArchitecture);

    /* Get the required length of the system info request string */
    DataLength = sizeof(RevisionProp) - 1;
    DataLength += strlen(BuildNo);
    DataLength += sizeof(PlatformProp) - 1;
    DataLength += strlen(Platform);

    /* Now build the string */
    SystemInfoRequestString = HeapAlloc(hProcessHeap, 0, DataLength + 1);
    strcpy(SystemInfoRequestString, RevisionProp);
    strcat(SystemInfoRequestString, BuildNo);
    strcat(SystemInfoRequestString, PlatformProp);
    strcat(SystemInfoRequestString, Platform);

    return TRUE;
}

/**
 * Prints the application usage.
 */
static VOID
IntPrintUsage()
{
    printf("rosautotest - ReactOS Automatic Testing Utility\n");
    printf("Usage: rosautotest [options] [module] [test]\n");
    printf("  options:\n");
    printf("    /?  - Shows this help\n");
    printf("    /s  - Shut down the system after finishing the tests\n");
    printf("    /w  - Submit the results to the webservice\n");
    printf("          Requires a \"rosautotest.ini\" with valid login data.\n");
    printf("\n");
    printf("  module:\n");
    printf("    The module to be tested (i.e. \"advapi32\")\n");
    printf("    If this parameter is specified without any test parameter,\n");
    printf("    all tests of the specified module are run.\n");
    printf("\n");
    printf("  test:\n");
    printf("    The test to be run. Needs to be a test of the specified module.\n");
}

/**
 * Main entry point
 */
int
wmain(int argc, wchar_t* argv[])
{
    int ReturnValue = 0;
    UINT i;

    hProcessHeap = GetProcessHeap();

    /* Parse the command line arguments */
    for(i = 1; i < (UINT)argc; i++)
    {
        if(argv[i][0] == '-' || argv[i][0] == '/')
        {
            switch(argv[i][1])
            {
                case 's':
                    AppOptions.Shutdown = TRUE;
                    break;

                case 'w':
                    AppOptions.Submit = TRUE;
                    break;

                default:
                    ReturnValue = 1;
                    /* Fall through */

                case '?':
                    IntPrintUsage();
                    goto Cleanup;
            }
        }
        else
        {
            size_t Length;

            /* Which parameter is this? */
            if(!AppOptions.Module)
            {
                /* Copy the parameter */
                Length = (wcslen(argv[i]) + 1) * sizeof(WCHAR);
                AppOptions.Module = HeapAlloc(hProcessHeap, 0, Length);
                memcpy(AppOptions.Module, argv[i], Length);
            }
            else if(!AppOptions.Test)
            {
                /* Copy the parameter converted to ASCII */
                Length = WideCharToMultiByte(CP_ACP, 0, argv[i], -1, NULL, 0, NULL, NULL);
                AppOptions.Test = HeapAlloc(hProcessHeap, 0, Length);
                WideCharToMultiByte(CP_ACP, 0, argv[i], -1, AppOptions.Test, Length, NULL, NULL);
            }
            else
            {
                ReturnValue = 1;
                IntPrintUsage();
                goto Cleanup;
            }
        }
    }

    if(!IntGetConfigurationValues() || !IntGetBuildAndPlatform() || !RunWineTests())
    {
        ReturnValue = 1;
        goto Cleanup;
    }

    /* For sysreg */
    OutputDebugStringA("SYSREG_CHECKPOINT:THIRDBOOT_COMPLETE\n");

Cleanup:
    if(AppOptions.Module)
        HeapFree(hProcessHeap, 0, AppOptions.Module);

    if(AppOptions.Test)
        HeapFree(hProcessHeap, 0, AppOptions.Test);

    if(AuthenticationRequestString)
        HeapFree(hProcessHeap, 0, AuthenticationRequestString);

    if(SystemInfoRequestString)
        HeapFree(hProcessHeap, 0, SystemInfoRequestString);

    /* Shut down the system if requested */
    if(AppOptions.Shutdown && !ShutdownSystem())
        ReturnValue = 1;

    return ReturnValue;
}
