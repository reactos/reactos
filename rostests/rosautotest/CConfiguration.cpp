/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class for managing all the configuration parameters
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

#define CONFIGURATION_FILENAMEA   "rosautotest.ini"
#define CONFIGURATION_FILENAMEW   L"rosautotest.ini"

typedef void (WINAPI *GETSYSINFO)(LPSYSTEM_INFO);

/**
 * Constructs an empty CConfiguration object
 */
CConfiguration::CConfiguration()
{
    WCHAR WindowsDirectory[MAX_PATH];

    /* Zero-initialize variables */
    m_CrashRecovery = false;
    m_Shutdown = false;
    m_Submit = false;

    /* Check if we are running under ReactOS from the SystemRoot directory */
    if(!GetWindowsDirectoryW(WindowsDirectory, MAX_PATH))
        FATAL("GetWindowsDirectoryW failed");

    m_IsReactOS = !_wcsnicmp(&WindowsDirectory[3], L"reactos", 7);
}

/**
 * Parses the passed parameters and sets the appropriate configuration settings.
 *
 * @param argc
 * The number of parameters (argc parameter of the wmain function)
 *
 * @param argv
 * Pointer to a wchar_t array containing the parameters (argv parameter of the wmain function)
 */
void
CConfiguration::ParseParameters(int argc, wchar_t* argv[])
{
    /* Parse the command line arguments */
    for(int i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-' || argv[i][0] == '/')
        {
            switch(argv[i][1])
            {
                case 'c':
                    ++i;
                    m_Comment = UnicodeToAscii(argv[i]);
                    break;

                case 'r':
                    m_CrashRecovery = true;
                    break;

                case 's':
                    m_Shutdown = true;
                    break;

                case 'w':
                    m_Submit = true;
                    break;

                default:
                    throw CInvalidParameterException();
            }
        }
        else
        {
            /* Which parameter is this? */
            if(m_Module.empty())
            {
                /* Copy the parameter */
                m_Module = argv[i];
            }
            else if(m_Test.empty())
            {
                /* Copy the parameter converted to ASCII */
                m_Test = UnicodeToAscii(argv[i]);
            }
            else
            {
                throw CInvalidParameterException();
            }
        }
    }

    /* The /r and /w options shouldn't be used in conjunction */
    if(m_CrashRecovery && m_Submit)
        throw CInvalidParameterException();
}

/**
 * Gets information about the running system and sets the appropriate configuration settings.
 */
void
CConfiguration::GetSystemInformation()
{
    char ProductType;
    GETSYSINFO GetSysInfo;
    HMODULE hKernel32;
    OSVERSIONINFOEXW os;
    stringstream ss;
    SYSTEM_INFO si;

    /* Get the build from the define */
    ss << "&revision=";
    ss << KERNEL_VERSION_BUILD_HEX;

    ss << "&platform=";

    if(m_IsReactOS)
    {
        ss << "reactos";
    }
    else
    {
        /* No, then use the info from GetVersionExW */
        os.dwOSVersionInfoSize = sizeof(os);

        if(!GetVersionExW((LPOSVERSIONINFOW)&os))
            FATAL("GetVersionExW failed\n");

        if(os.dwMajorVersion < 5)
            EXCEPTION("Application requires at least Windows 2000!\n");

        if(os.wProductType == VER_NT_WORKSTATION)
            ProductType = 'w';
        else
            ProductType = 's';

        /* Print all necessary identification information into the Platform string */
        ss << os.dwMajorVersion << '.'
           << os.dwMinorVersion << '.'
           << os.dwBuildNumber << '.'
           << os.wServicePackMajor << '.'
           << os.wServicePackMinor << '.'
           << ProductType << '.';
    }

    /* We also need to know about the processor architecture.
       To retrieve this information accurately, check whether "GetNativeSystemInfo" is exported and use it then, otherwise fall back to "GetSystemInfo". */
    hKernel32 = GetModuleHandleW(L"KERNEL32.DLL");
    GetSysInfo = (GETSYSINFO)GetProcAddress(hKernel32, "GetNativeSystemInfo");

    if(!GetSysInfo)
        GetSysInfo = (GETSYSINFO)GetProcAddress(hKernel32, "GetSystemInfo");

    GetSysInfo(&si);
    ss << si.wProcessorArchitecture;

    m_SystemInfoRequestString = ss.str();
}

/**
 * Reads additional configuration options from the INI file.
 *
 * ParseParameters should be called before this function to get the desired result.
 */
void
CConfiguration::GetConfigurationFromFile()
{
    DWORD Length;
    string Value;
    WCHAR ConfigFile[MAX_PATH];

    /* Most values are only needed if we're going to submit anything */
    if(m_Submit)
    {
        /* Build the path to the configuration file from the application's path */
        GetModuleFileNameW(NULL, ConfigFile, MAX_PATH);
        Length = wcsrchr(ConfigFile, '\\') - ConfigFile + 1;
        wcscpy(&ConfigFile[Length], CONFIGURATION_FILENAMEW);

        /* Check if it exists */
        if(GetFileAttributesW(ConfigFile) == INVALID_FILE_ATTRIBUTES)
            EXCEPTION("Missing \"" CONFIGURATION_FILENAMEA "\" configuration file!\n");

        /* Get the user name */
        m_AuthenticationRequestString = "&username=";
        Value = GetINIValue(L"Login", L"UserName", ConfigFile);

        if(Value.empty())
            EXCEPTION("UserName is missing in the configuration file\n");

        m_AuthenticationRequestString += EscapeString(Value);

        /* Get the password */
        m_AuthenticationRequestString += "&password=";
        Value = GetINIValue(L"Login", L"Password", ConfigFile);

        if(Value.empty())
            EXCEPTION("Password is missing in the configuration file\n");

        m_AuthenticationRequestString += EscapeString(Value);

        /* If we don't have any Comment string yet, try to find one in the INI file */
        if(m_Comment.empty())
            m_Comment = GetINIValue(L"Submission", L"Comment", ConfigFile);
    }
}
