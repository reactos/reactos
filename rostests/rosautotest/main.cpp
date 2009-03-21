/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main implementation file
 * COPYRIGHT:   Copyright 2008-2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"
#include <cstdio>

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
         << "    /r           - Maintain information to resume from ReactOS crashes" << endl
         << "                   Can only be run under ReactOS and relies on sysreg2," << endl
         << "                   so incompatible with /w" << endl
         << "    /s           - Shut down the system after finishing the tests." << endl
         << "    /w           - Submit the results to the webservice." << endl
         << "                   Requires a \"rosautotest.ini\" with valid login data." << endl
         << "                   Incompatible with the /r option." << endl
         << endl
         << "  module:" << endl
         << "    The module to be tested (i.e. \"advapi32\")" << endl
         << "    If this parameter is specified without any test parameter," << endl
         << "    all tests of the specified module are run." << endl
         << endl
         << "  test:" << endl
         << "    The test to be run. Needs to be a test of the specified module." << endl;
}

/**
 * Main entry point
 */
extern "C" int
wmain(int argc, wchar_t* argv[])
{
    CWineTest WineTest;
    int ReturnValue = 1;

    try
    {
        /* Set up the configuration */
        Configuration.ParseParameters(argc, argv);
        Configuration.GetSystemInformation();
        Configuration.GetConfigurationFromFile();

        /* Run the tests */
        WineTest.Run();

        /* For sysreg */
        DbgPrint("SYSREG_CHECKPOINT:THIRDBOOT_COMPLETE\n");

        ReturnValue = 0;
    }
    catch(CInvalidParameterException)
    {
        IntPrintUsage();
    }
    catch(CSimpleException& e)
    {
        StringOut(e.GetMessage());
    }
    catch(CFatalException& e)
    {
        stringstream ss;

        ss << "An exception occured in rosautotest." << endl
           << "Message: " << e.GetMessage() << endl
           << "File: " << e.GetFile() << endl
           << "Line: " << e.GetLine() << endl
           << "Last Win32 Error: " << GetLastError() << endl;
        StringOut(ss.str());
    }

    /* Shut down the system if requested, also in case of an exception above */
    if(Configuration.DoShutdown() && !ShutdownSystem())
        ReturnValue = 1;

    return ReturnValue;
}
