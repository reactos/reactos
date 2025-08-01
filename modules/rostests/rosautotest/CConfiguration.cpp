/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class for managing all the configuration parameters
 * COPYRIGHT:   Copyright 2009-2011 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <versionhelpers.h>

#define CONFIGURATION_FILENAME   "rosautotest.ini"

static void (WINAPI *pGetSystemInfo)(LPSYSTEM_INFO);
static void(NTAPI *pRtlGetVersion)(RTL_OSVERSIONINFOEXW *);


/**
 * Constructs an empty CConfiguration object
 */
CConfiguration::CConfiguration()
    : m_CrashRecovery(false),
      m_IsInteractive(false),
      m_PrintToConsole(true),
      m_RepeatCount(1),
      m_Shutdown(false),
      m_Submit(false),
      m_ListModules(false)
{
    WCHAR WindowsDirectory[MAX_PATH];
    WCHAR Interactive[32];

    /* Check if we are running under ReactOS from the SystemRoot directory */
    if(!GetWindowsDirectoryW(WindowsDirectory, MAX_PATH))
        FATAL("GetWindowsDirectoryW failed\n");

    m_IsReactOS = !_wcsnicmp(&WindowsDirectory[3], L"reactos", 7);
    m_IsReactOS = m_IsReactOS || IsReactOS();

    if(GetEnvironmentVariableW(L"WINETEST_INTERACTIVE", Interactive, _countof(Interactive)))
        m_IsInteractive = _wtoi(Interactive);
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
            unsigned long tmp_RepeatCount;

            switch(argv[i][1])
            {
                case 'c':
                    ++i;
                    if (i >= argc)
                    {
                        throw CInvalidParameterException();
                    }

                    m_Comment = UnicodeToAscii(argv[i]);
                    break;

                case 'n':
                    m_PrintToConsole = false;
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

                case 't':
                    ++i;
                    if (i >= argc)
                    {
                        throw CInvalidParameterException();
                    }

                    tmp_RepeatCount = wcstoul(argv[i], NULL, 10);

                    if (tmp_RepeatCount == 0 || tmp_RepeatCount > 10000)
                    {
                        throw CInvalidParameterException();
                    }

                    m_RepeatCount = tmp_RepeatCount;
                    break;

                case 'l':
                    m_ListModules = true;
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
    stringstream ss;
    SYSTEM_INFO si;

    /* Get the build from the define */
    ss << "&revision=";
    ss << KERNEL_VERSION_COMMIT_HASH;

    ss << "&platform=";

    if(m_IsReactOS)
    {
        ss << "reactos";
    }
    else
    {
        RTL_OSVERSIONINFOEXW rtlinfo = {0};
        /* No, then use the info from GetVersionExW */
        rtlinfo.dwOSVersionInfoSize = sizeof(rtlinfo);

        if (!pRtlGetVersion)
        {
            HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
            pRtlGetVersion = (decltype(pRtlGetVersion))GetProcAddress(hNtdll, "RtlGetVersion");
        }

        pRtlGetVersion(&rtlinfo);

        if (rtlinfo.dwMajorVersion < 5)
            EXCEPTION("Application requires at least Windows 2000!\n");

        if (rtlinfo.wProductType == VER_NT_WORKSTATION)
            ProductType = 'w';
        else
            ProductType = 's';

        /* Print all necessary identification information into the Platform string */
        ss << rtlinfo.dwMajorVersion << '.'
           << rtlinfo.dwMinorVersion << '.'
           << rtlinfo.dwBuildNumber << '.'
           << rtlinfo.wServicePackMajor << '.'
           << rtlinfo.wServicePackMinor << '.'
           << ProductType << '.';
    }

    /* We also need to know about the processor architecture.
       To retrieve this information accurately, check whether "GetNativeSystemInfo" is exported and use it then, otherwise fall back to "GetSystemInfo". */
    if (!pGetSystemInfo)
    {
        HMODULE hKernel32 = GetModuleHandleW(L"KERNEL32.DLL");
        pGetSystemInfo = (decltype(pGetSystemInfo))GetProcAddress(hKernel32, "GetNativeSystemInfo");

        if (!pGetSystemInfo)
            pGetSystemInfo = GetSystemInfo;
    }

    pGetSystemInfo(&si);
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
        wcscpy(&ConfigFile[Length], TEXT(CONFIGURATION_FILENAME));

        /* Check if it exists */
        if(GetFileAttributesW(ConfigFile) == INVALID_FILE_ATTRIBUTES)
            EXCEPTION("Missing \"" CONFIGURATION_FILENAME "\" configuration file!\n");

        /* Get the user name */
        m_AuthenticationRequestString = "&sourceid=";
        Value = GetINIValue(L"Login", L"SourceID", ConfigFile);

        if(Value.empty())
            EXCEPTION("SourceID is missing in the configuration file\n");

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
