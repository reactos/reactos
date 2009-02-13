/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Running Wine Tests automatically
 * COPYRIGHT:   Copyright 2008-2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

/**
 * Runs a specific test for a specific module.
 * If we get results for a test, they are submitted to the Web Service.
 *
 * @param CommandLine
 * Command line to run (should be the path to a module's test suite together with a parameter for the specified test)
 *
 * @param hReadPipe
 * Handle to the Read Pipe set up in RunWineTests.
 *
 * @param StartupInfo
 * Pointer to the StartupInfo structure set up in RunWineTests.
 *
 * @param GetSuiteIDData
 * Pointer to the GetSuiteIDData structure set up in IntRunModuleTests.
 *
 * @param SubmitData
 * Pointer to the SubmitData structure set up in RunWineTests.
 *
 * @return
 * TRUE if everything went well, FALSE otherwise.
 */
static BOOL
IntRunTest(PWSTR CommandLine, HANDLE hReadPipe, LPSTARTUPINFOW StartupInfo, PWINE_GETSUITEID_DATA GetSuiteIDData, PWINE_SUBMIT_DATA SubmitData)
{
    BOOL BreakLoop = FALSE;
    BOOL ReturnValue = FALSE;
    DWORD BytesAvailable;
    DWORD LogAvailable = 0;
    DWORD LogLength = 0;
    DWORD LogPosition = 0;
    DWORD Temp;
    PCHAR Buffer = NULL;
    PROCESS_INFORMATION ProcessInfo = {0};

    if(AppOptions.Submit)
    {
        /* Allocate one block for the log */
        SubmitData->Log = HeapAlloc(hProcessHeap, 0, BUFFER_BLOCKSIZE);
        LogAvailable = BUFFER_BLOCKSIZE;
        LogLength = BUFFER_BLOCKSIZE;
    }

    /* Allocate a buffer with the exact size of the output string.
       We have to output this string in one call to prevent a race condition, when another application also outputs a string over the debug line. */
    Buffer = HeapAlloc(hProcessHeap, 0, 27 + strlen(GetSuiteIDData->Module) + 8 + strlen(GetSuiteIDData->Test) + 2);
    sprintf(Buffer, "Running Wine Test, Module: %s, Test: %s\n", GetSuiteIDData->Module, GetSuiteIDData->Test);
    StringOut(Buffer);
    HeapFree(hProcessHeap, 0, Buffer);
    Buffer = NULL;

    /* Execute the test */
    if(!CreateProcessW(NULL, CommandLine, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, StartupInfo, &ProcessInfo))
    {
        StringOut("CreateProcessW for running the test failed\n");
        goto Cleanup;
    }

    /* Receive all the data from the pipe */
    do
    {
        /* When the application finished, make sure that we peek the pipe one more time, so that we get all data.
           If the following condition would be the while() condition, we might hit a race condition:
              - We check for data with PeekNamedPipe -> no data available
              - The application outputs its data and finishes
              - WaitForSingleObject reports that the application has finished and we break the loop without receiving any data
        */
        if(WaitForSingleObject(ProcessInfo.hProcess, 0) == WAIT_OBJECT_0)
            BreakLoop = TRUE;

        if(!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &BytesAvailable, NULL))
        {
            StringOut("PeekNamedPipe failed for the test run\n");
            goto Cleanup;
        }

        if(BytesAvailable)
        {
            /* There is data, so get it and output it */
            Buffer = HeapAlloc(hProcessHeap, 0, BytesAvailable + 1);

            if(!ReadFile(hReadPipe, Buffer, BytesAvailable, &Temp, NULL))
            {
                StringOut("ReadFile failed for the test run\n");
                goto Cleanup;
            }

            /* Output all test output through StringOut, even while the test is still running */
            Buffer[BytesAvailable] = 0;
            StringOut(Buffer);

            if(AppOptions.Submit)
            {
                /* Also store it in the buffer */
                if(BytesAvailable > LogAvailable)
                {
                    /* Allocate enough new blocks to hold all available data */
                    Temp = ((BytesAvailable - LogAvailable) / BUFFER_BLOCKSIZE + 1) * BUFFER_BLOCKSIZE;
                    LogAvailable += Temp;
                    LogLength += Temp;
                    SubmitData->Log = HeapReAlloc(hProcessHeap, 0, SubmitData->Log, LogLength);
                }

                memcpy(&SubmitData->Log[LogPosition], Buffer, BytesAvailable);
                LogPosition += BytesAvailable;
                LogAvailable -= BytesAvailable;
            }

            HeapFree(hProcessHeap, 0, Buffer);
            Buffer = NULL;
        }
    }
    while(!BreakLoop);

    if(AppOptions.Submit)
    {
        SubmitData->Log[LogLength - LogAvailable] = 0;

        /* If we got any output, submit it to the web service */
        if(*SubmitData->Log)
        {
            /* We don't want to waste any ID's, so only request them if we can be sure that we have results to submit. */

            /* Get a Test ID if we don't have one yet */
            if(!SubmitData->General.TestID)
            {
                SubmitData->General.TestID = GetTestID(WineTest);

                if(!SubmitData->General.TestID)
                    goto Cleanup;
            }

            /* Get a Suite ID for this combination */
            SubmitData->General.SuiteID = GetSuiteID(WineTest, GetSuiteIDData);

            if(!SubmitData->General.SuiteID)
                goto Cleanup;

            /* Submit the stuff */
            Submit(WineTest, SubmitData);
        }
    }

    StringOut("\n\n");

    ReturnValue = TRUE;

Cleanup:
    if(Buffer)
        HeapFree(hProcessHeap, 0, Buffer);

    if(ProcessInfo.hProcess)
        HeapFree(hProcessHeap, 0, ProcessInfo.hProcess);

    if(ProcessInfo.hThread)
        HeapFree(hProcessHeap, 0, ProcessInfo.hThread);

    if(SubmitData->General.SuiteID)
    {
        HeapFree(hProcessHeap, 0, SubmitData->General.SuiteID);
        SubmitData->General.SuiteID = NULL;
    }

    if(SubmitData->Log)
    {
        HeapFree(hProcessHeap, 0, SubmitData->Log);
        SubmitData->Log = NULL;
    }

    return ReturnValue;
}

/**
 * Runs the desired tests for a specified module.
 *
 * @param File
 * The file name of the module's test suite.
 *
 * @param FilePath
 * The full path to the file of the module's test suite.
 *
 * @param hReadPipe
 * Handle to the Read Pipe set up in RunWineTests.
 *
 * @param StartupInfo
 * Pointer to the StartupInfo structure set up in RunWineTests.
 *
 * @param SubmitData
 * Pointer to the SubmitData structure set up in RunWineTests.
 *
 * @return
 * TRUE if everything went well, FALSE otherwise.
 */
static BOOL
IntRunModuleTests(PWSTR File, PWSTR FilePath, HANDLE hReadPipe, LPSTARTUPINFOW StartupInfo, PWINE_SUBMIT_DATA SubmitData)
{
    BOOL ReturnValue = FALSE;
    DWORD BytesAvailable;
    DWORD Length;
    DWORD Temp;
    PCHAR Buffer = NULL;
    PCHAR pStart;
    PCHAR pEnd;
    PROCESS_INFORMATION ProcessInfo = {0};
    PWSTR pUnderscore;
    size_t FilePosition;
    WINE_GETSUITEID_DATA GetSuiteIDData = {0};

    /* Build the full command line */
    FilePosition = wcslen(FilePath);
    FilePath[FilePosition++] = ' ';
    FilePath[FilePosition] = 0;
    wcscat(FilePath, L"--list");

    /* Find the underscore in the file name */
    pUnderscore = wcschr(File, L'_');

    if(!pUnderscore)
    {
        StringOut("Invalid test file name: ");

        Length = wcslen(File);
        Buffer = HeapAlloc(hProcessHeap, 0, Length + 1);
        WideCharToMultiByte(CP_ACP, 0, File, Length + 1, Buffer, Length + 1, NULL, NULL);

        StringOut(Buffer);
        StringOut("\n");

        goto Cleanup;
    }

    /* Store the tested module name */
    Length = pUnderscore - File;
    GetSuiteIDData.Module = HeapAlloc(hProcessHeap, 0, Length + 1);
    WideCharToMultiByte(CP_ACP, 0, File, Length, GetSuiteIDData.Module, Length, NULL, NULL);
    GetSuiteIDData.Module[Length] = 0;

    /* Start the process for getting all available tests */
    if(!CreateProcessW(NULL, FilePath, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, StartupInfo, &ProcessInfo))
    {
        StringOut("CreateProcessW for getting the available tests failed\n");
        goto Cleanup;
    }

    /* Wait till this process ended */
    if(WaitForSingleObject(ProcessInfo.hProcess, INFINITE) == WAIT_FAILED)
    {
        StringOut("WaitForSingleObject failed for the test list\n");
        goto Cleanup;
    }

    /* Read the output data into a buffer */
    if(!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &BytesAvailable, NULL))
    {
        StringOut("PeekNamedPipe failed for the test list\n");
        goto Cleanup;
    }

    /* Check if we got any */
    if(!BytesAvailable)
    {
        StringOut("The --list command did not return any data for ");

        Length = wcslen(File);
        Buffer = HeapAlloc(hProcessHeap, 0, Length + 1);
        WideCharToMultiByte(CP_ACP, 0, File, Length + 1, Buffer, Length + 1, NULL, NULL);

        StringOut(Buffer);
        StringOut("\n");

        goto Cleanup;
    }

    /* Read the data */
    Buffer = HeapAlloc(hProcessHeap, 0, BytesAvailable);

    if(!ReadFile(hReadPipe, Buffer, BytesAvailable, &Temp, NULL))
    {
        StringOut("ReadFile failed\n");
        goto Cleanup;
    }

    /* Jump to the first available test */
    pStart = strchr(Buffer, '\n');
    pStart += 5;

    while(pStart < (Buffer + BytesAvailable))
    {
        /* Get start and end of this test name */
        pEnd = pStart;

        while(*pEnd != '\r')
            ++pEnd;

        /* Store the test name */
        GetSuiteIDData.Test = HeapAlloc(hProcessHeap, 0, pEnd - pStart + 1);
        memcpy(GetSuiteIDData.Test, pStart, pEnd - pStart);
        GetSuiteIDData.Test[pEnd - pStart] = 0;

        /* If the user gave us a test to run, we check whether the module's test suite really provides this test. */
        if(!AppOptions.Test || (AppOptions.Test && !strcmp(AppOptions.Test, GetSuiteIDData.Test)))
        {
            /* Build the command line for this test */
            Length = MultiByteToWideChar(CP_ACP, 0, pStart, pEnd - pStart, NULL, 0);
            MultiByteToWideChar(CP_ACP, 0, pStart, pEnd - pStart, &FilePath[FilePosition], Length * sizeof(WCHAR));
            FilePath[FilePosition + Length] = 0;

            if(!IntRunTest(FilePath, hReadPipe, StartupInfo, &GetSuiteIDData, SubmitData))
                goto Cleanup;
        }

        /* Cleanup */
        HeapFree(hProcessHeap, 0, GetSuiteIDData.Test);
        GetSuiteIDData.Test = NULL;

        /* Move to the next test */
        pStart = pEnd + 6;
    }

    ReturnValue = TRUE;

Cleanup:
    if(GetSuiteIDData.Module)
        HeapFree(hProcessHeap, 0, GetSuiteIDData.Module);

    if(GetSuiteIDData.Test)
        HeapFree(hProcessHeap, 0, GetSuiteIDData.Test);

    if(Buffer)
        HeapFree(hProcessHeap, 0, Buffer);

    if(ProcessInfo.hProcess)
        CloseHandle(ProcessInfo.hProcess);

    if(ProcessInfo.hThread)
        CloseHandle(ProcessInfo.hThread);

    return ReturnValue;
}

/**
 * Runs the Wine tests according to the options specified by the parameters.
 *
 * @return
 * TRUE if everything went well, FALSE otherwise.
 */
BOOL
RunWineTests()
{
    BOOL ReturnValue = FALSE;
    GENERAL_FINISH_DATA FinishData;
    HANDLE hFind = NULL;
    HANDLE hReadPipe = NULL;
    HANDLE hWritePipe = NULL;
    SECURITY_ATTRIBUTES SecurityAttributes;
    STARTUPINFOW StartupInfo = {0};
    size_t PathPosition;
    WCHAR FilePath[MAX_PATH];
    WIN32_FIND_DATAW fd;
    WINE_SUBMIT_DATA SubmitData = { {0} };

    /* Create a pipe for getting the output of the tests */
    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.bInheritHandle = TRUE;
    SecurityAttributes.lpSecurityDescriptor = NULL;

    if(!CreatePipe(&hReadPipe, &hWritePipe, &SecurityAttributes, 0))
    {
        StringOut("CreatePipe failed\n");
        goto Cleanup;
    }

    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    StartupInfo.hStdOutput = hWritePipe;

    /* Build the path for finding the tests */
    if(GetWindowsDirectoryW(FilePath, MAX_PATH) > MAX_PATH - 60)
    {
        StringOut("Windows directory path is too long\n");
        goto Cleanup;
    }

    wcscat(FilePath, L"\\bin\\");
    PathPosition = wcslen(FilePath);

    if(AppOptions.Module)
    {
        /* Find a test belonging to this module */
        wcscat(FilePath, AppOptions.Module);
        wcscat(FilePath, L"_*.exe");
    }
    else
    {
        /* Find all tests */
        wcscat(FilePath, L"*.exe");
    }

    hFind = FindFirstFileW(FilePath, &fd);

    if(hFind == INVALID_HANDLE_VALUE)
    {
        StringOut("FindFirstFileW failed\n");
        goto Cleanup;
    }

    /* Run the tests */
    do
    {
        /* Build the full path to the test suite */
        wcscpy(&FilePath[PathPosition], fd.cFileName);

        /* Run it */
        if(!IntRunModuleTests(fd.cFileName, FilePath, hReadPipe, &StartupInfo, &SubmitData))
            goto Cleanup;
    }
    while(FindNextFileW(hFind, &fd));

    /* Close this test run if necessary */
    if(SubmitData.General.TestID)
    {
        FinishData.TestID = SubmitData.General.TestID;

        if(!Finish(WineTest, &FinishData))
            goto Cleanup;
    }

    ReturnValue = TRUE;

Cleanup:
    if(SubmitData.General.TestID)
        HeapFree(hProcessHeap, 0, SubmitData.General.TestID);

    if(hFind && hFind != INVALID_HANDLE_VALUE)
        FindClose(hFind);

    if(hReadPipe)
        CloseHandle(hReadPipe);

    if(hWritePipe)
        CloseHandle(hWritePipe);

    return ReturnValue;
}
