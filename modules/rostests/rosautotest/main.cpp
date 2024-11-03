/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main implementation file
 * COPYRIGHT:   Copyright 2008-2009 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <cstdio>
#include <ndk/setypes.h>
#include <ndk/exfuncs.h>

CConfiguration Configuration;

/**
 * Prints the application usage.
 */
static void
IntPrintUsage()
{
    cout << "rosautotest - ReactOS Automatic Testing Utility" << endl
         << "Usage: rosautotest [options] [module] [test]" << endl
         << "  options:" << endl
         << "    /?           - Shows this help." << endl
         << "    /c <comment> - Specifies the comment to be submitted to the Web Service." << endl
         << "                   Skips the comment set in the configuration file (if any)." << endl
         << "                   Only has an effect when /w is also used." << endl
         << "    /n           - Do not print test output to console" << endl
         << "    /r           - Maintain information to resume from ReactOS crashes" << endl
         << "                   Can only be run under ReactOS and relies on sysreg2," << endl
         << "                   so incompatible with /w" << endl
         << "    /s           - Shut down the system after finishing the tests." << endl
         << "    /t <num>     - Repeat the test <num> times (1-10000)" << endl
         << "    /w           - Submit the results to the webservice." << endl
         << "                   Requires a \"rosautotest.ini\" with valid login data." << endl
         << "                   Incompatible with the /r option." << endl
         << "    /l           - List all modules that would run." << endl
         << endl
         << "  module:" << endl
         << "    The module to be tested (i.e. \"advapi32\")" << endl
         << "    If this parameter is specified without any test parameter," << endl
         << "    all tests of the specified module are run." << endl
         << endl
         << "  test:" << endl
         << "    The test to be run. Needs to be a test of the specified module." << endl;
}

static
VOID
SetNtGlobalFlags()
{
    ULONG NtGlobalFlags = 0;
    BOOLEAN PrivilegeEnabled;
    NTSTATUS Status;

    /* Enable SeDebugPrivilege */
    Status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &PrivilegeEnabled);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("Failed to enable SeDebugPrivilege: 0x%08lx\n", Status);
        return;
    }

    /* Get current NtGlobalFlags */
    Status = NtQuerySystemInformation(SystemFlagsInformation, &NtGlobalFlags, sizeof(NtGlobalFlags), NULL);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("Failed to get NtGlobalFlags: 0x%08lx\n", Status);
        return;
    }

    /* Disable debug prompts */
    NtGlobalFlags |= FLG_DISABLE_DEBUG_PROMPTS;

    /* Set new NtGlobalFlags */
    Status = NtSetSystemInformation(SystemFlagsInformation, &NtGlobalFlags, sizeof(NtGlobalFlags));
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("Failed to set NtGlobalFlags: 0x%08lx\n", Status);
    }
}

/**
 * Main entry point
 */
extern "C" int
wmain(int argc, wchar_t* argv[])
{
    int ReturnValue = 1;

    SetNtGlobalFlags();

    try
    {
        stringstream ss;

        /* Set up the configuration */
        Configuration.ParseParameters(argc, argv);
        Configuration.GetSystemInformation();
        Configuration.GetConfigurationFromFile();

        ss << endl
           << endl
           << "[ROSAUTOTEST] System uptime " << setprecision(2) << fixed;
        ss << ((float)GetTickCount()/1000) << " seconds" << endl;
        StringOut(ss.str());

        /* Report tests startup */
        InitLogs();
        ReportEventW(hLog,
                      EVENTLOG_INFORMATION_TYPE,
                      0,
                      MSG_TESTS_STARTED,
                      NULL,
                      0,
                      0,
                      NULL,
                      NULL);

        if (Configuration.GetRepeatCount() > 1)
        {
            stringstream ss1;

            ss1 << "[ROSAUTOTEST] The test will be repeated " << Configuration.GetRepeatCount() << " times" << endl;
            StringOut(ss1.str());
        }

        /* Run the tests */
        for (unsigned long i = 0; i < Configuration.GetRepeatCount(); i++)
        {
            CWineTest WineTest;

            if (Configuration.GetRepeatCount() > 1)
            {
                stringstream ss;
                ss << "[ROSAUTOTEST] Running attempt #" << i+1 << endl;
                StringOut(ss.str());
            }
            WineTest.Run();
        }

        /* For sysreg2 */
        DbgPrint("SYSREG_CHECKPOINT:THIRDBOOT_COMPLETE\n");

        ReturnValue = 0;
    }
    catch(CInvalidParameterException)
    {
        IntPrintUsage();
    }
    catch(CSimpleException& e)
    {
        stringstream ss;

        // e.GetMessage() must include ending '\n'.
        ss << "[ROSAUTOTEST] " << e.GetMessage();
        StringOut(ss.str());
    }
    catch(CFatalException& e)
    {
        stringstream ss;

        // e.GetMessage() must include ending '\n'.
        ss << "An exception occured in rosautotest." << endl
           << "Message: " << e.GetMessage()
           << "File: " << e.GetFile() << endl
           << "Line: " << e.GetLine() << endl
           << "Last Win32 Error: " << GetLastError() << endl;
        StringOut(ss.str());
    }

    /* For sysreg2 to notice if rosautotest itself failed */
    if(ReturnValue == 1)
        DbgPrint("SYSREG_ROSAUTOTEST_FAILURE\n");

    /* Report successful end of tests */
    ReportEventW(hLog,
                  EVENTLOG_SUCCESS,
                  0,
                  MSG_TESTS_SUCCESSFUL,
                  NULL,
                  0,
                  0,
                  NULL,
                  NULL);
    FreeLogs();

    /* Shut down the system if requested, also in case of an exception above */
    if(Configuration.DoShutdown() && !ShutdownSystem())
        ReturnValue = 1;

    return ReturnValue;
}
