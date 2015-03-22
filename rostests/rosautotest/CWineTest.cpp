/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class implementing functions for handling Wine tests
 * COPYRIGHT:   Copyright 2009-2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static const DWORD ListTimeout = 10000;

/**
 * Constructs a CWineTest object.
 */
CWineTest::CWineTest()
    : m_hFind(NULL), m_ListBuffer(NULL)
{
    WCHAR wszDirectory[MAX_PATH];

    /* Set up m_TestPath */
    if (GetEnvironmentVariableW(L"ROSAUTOTEST_DIR", wszDirectory, MAX_PATH))
    {
        m_TestPath = wszDirectory;
        if (*m_TestPath.rbegin() != L'\\')
            m_TestPath += L'\\';
    }
    else
    {
        if (!GetWindowsDirectoryW(wszDirectory, MAX_PATH))
            FATAL("GetWindowsDirectoryW failed");

        m_TestPath = wszDirectory;
        m_TestPath += L"\\bin\\";
    }
}

/**
 * Destructs a CWineTest object.
 */
CWineTest::~CWineTest()
{
    if(m_hFind)
        FindClose(m_hFind);

    if(m_ListBuffer)
        delete m_ListBuffer;
}

/**
 * Gets the next module test file using the FindFirstFileW/FindNextFileW API.
 *
 * @return
 * true if we found a next file, otherwise false.
 */
bool
CWineTest::GetNextFile()
{
    bool FoundFile = false;
    WIN32_FIND_DATAW fd;

    /* Did we already begin searching for files? */
    if(m_hFind)
    {
        /* Then get the next file (if any) */
        if(FindNextFileW(m_hFind, &fd))
            FoundFile = true;
    }
    else
    {
        /* Start searching for test files */
        wstring FindPath = m_TestPath;

        /* Did the user specify a module? */
        if(Configuration.GetModule().empty())
        {
            /* No module, so search for all files in that directory */
            FindPath += L"*.exe";
        }
        else
        {
            /* Search for files with the pattern "modulename_*" */
            FindPath += Configuration.GetModule();
            FindPath += L"_*.exe";
        }

        /* Search for the first file and check whether we got one */
        m_hFind = FindFirstFileW(FindPath.c_str(), &fd);

        if(m_hFind != INVALID_HANDLE_VALUE)
            FoundFile = true;
    }

    if(FoundFile)
        m_CurrentFile = fd.cFileName;

    return FoundFile;
}

/**
 * Executes the --list command of a module test file to get information about the available tests.
 *
 * @return
 * The number of bytes we read into the m_ListBuffer member variable by capturing the output of the --list command.
 */
DWORD
CWineTest::DoListCommand()
{
    DWORD BytesAvailable;
    DWORD Temp;
    wstring CommandLine;
    CPipe Pipe;

    /* Build the command line */
    CommandLine = m_TestPath;
    CommandLine += m_CurrentFile;
    CommandLine += L" --list";

    {
        /* Start the process for getting all available tests */
        CPipedProcess Process(CommandLine, Pipe);

        /* Wait till this process ended */
        if(WaitForSingleObject(Process.GetProcessHandle(), ListTimeout) == WAIT_FAILED)
            TESTEXCEPTION("WaitForSingleObject failed for the test list\n");
    }

    /* Read the output data into a buffer */
    if(!Pipe.Peek(NULL, 0, NULL, &BytesAvailable))
        TESTEXCEPTION("CPipe::Peek failed for the test list\n");

    /* Check if we got any */
    if(!BytesAvailable)
    {
        stringstream ss;

        ss << "The --list command did not return any data for " << UnicodeToAscii(m_CurrentFile) << endl;
        TESTEXCEPTION(ss.str());
    }

    /* Read the data */
    m_ListBuffer = new char[BytesAvailable];

    if(!Pipe.Read(m_ListBuffer, BytesAvailable, &Temp))
        TESTEXCEPTION("CPipe::Read failed\n");

    return BytesAvailable;
}

/**
 * Gets the next test from m_ListBuffer, which was filled with information from the --list command.
 *
 * @return
 * true if a next test was found, otherwise false.
 */
bool
CWineTest::GetNextTest()
{
    PCHAR pEnd;
    static DWORD BufferSize;
    static PCHAR pStart;

    if(!m_ListBuffer)
    {
        /* Perform the --list command */
        BufferSize = DoListCommand();

        /* Move the pointer to the first test */
        pStart = strchr(m_ListBuffer, '\n');
        pStart += 5;
    }

    /* If we reach the buffer size, we finished analyzing the output of this test */
    if(pStart >= (m_ListBuffer + BufferSize))
    {
        /* Clear m_CurrentFile to indicate that */
        m_CurrentFile.clear();

        /* Also free the memory for the list buffer */
        delete[] m_ListBuffer;
        m_ListBuffer = NULL;

        return false;
    }

    /* Get start and end of this test name */
    pEnd = pStart;

    while(*pEnd != '\r')
        ++pEnd;

    /* Store the test name */
    m_CurrentTest = string(pStart, pEnd);

    /* Move the pointer to the next test */
    pStart = pEnd + 6;

    return true;
}

/**
 * Interface to CTestList-derived classes for getting all information about the next test to be run.
 *
 * @return
 * Returns a pointer to a CTestInfo object containing all available information about the next test.
 */
CTestInfo*
CWineTest::GetNextTestInfo()
{
    while(!m_CurrentFile.empty() || GetNextFile())
    {
        try
        {
            while(GetNextTest())
            {
                /* If the user specified a test through the command line, check this here */
                if(!Configuration.GetTest().empty() && Configuration.GetTest() != m_CurrentTest)
                    continue;

                {
                    auto_ptr<CTestInfo> TestInfo(new CTestInfo());
                    size_t UnderscorePosition;

                    /* Build the command line */
                    TestInfo->CommandLine = m_TestPath;
                    TestInfo->CommandLine += m_CurrentFile;
                    TestInfo->CommandLine += ' ';
                    TestInfo->CommandLine += AsciiToUnicode(m_CurrentTest);

                    /* Store the Module name */
                    UnderscorePosition = m_CurrentFile.find_last_of('_');

                    if(UnderscorePosition == m_CurrentFile.npos)
                    {
                        stringstream ss;

                        ss << "Invalid test file name: " << UnicodeToAscii(m_CurrentFile) << endl;
                        SSEXCEPTION;
                    }

                    TestInfo->Module = UnicodeToAscii(m_CurrentFile.substr(0, UnderscorePosition));

                    /* Store the test */
                    TestInfo->Test = m_CurrentTest;

                    return TestInfo.release();
                }
            }
        }
        catch(CTestException& e)
        {
            stringstream ss;

            ss << "An exception occurred trying to list tests for: " << UnicodeToAscii(m_CurrentFile) << endl;
            StringOut(ss.str());
            StringOut(e.GetMessage());
            StringOut("\n");
            m_CurrentFile.clear();
            delete[] m_ListBuffer;
        }
    }

    return NULL;
}

/**
 * Runs a Wine test and captures the output
 *
 * @param TestInfo
 * Pointer to a CTestInfo object containing information about the test.
 * Will contain the test log afterwards if the user wants to submit data.
 */
void
CWineTest::RunTest(CTestInfo* TestInfo)
{
    DWORD BytesAvailable;
    stringstream ss, ssFinish;
    DWORD StartTime;
    float TotalTime;
    CPipe Pipe;
    char Buffer[1024];

    ss << "Running Wine Test, Module: " << TestInfo->Module << ", Test: " << TestInfo->Test << endl;
    StringOut(ss.str());

    StartTime = GetTickCount();

    try
    {
        /* Execute the test */
        CPipedProcess Process(TestInfo->CommandLine, Pipe);

        /* Receive all the data from the pipe */
        while(Pipe.Read(Buffer, sizeof(Buffer) - 1, &BytesAvailable) && BytesAvailable)
        {
            /* Output text through StringOut, even while the test is still running */
            Buffer[BytesAvailable] = 0;
            StringOut(string(Buffer));

            if(Configuration.DoSubmit())
                TestInfo->Log += Buffer;
        }
        if(GetLastError() != ERROR_BROKEN_PIPE)
            TESTEXCEPTION("CPipe::Read failed for the test run\n");
    }
    catch(CTestException& e)
    {
        StringOut(e.GetMessage());
        TestInfo->Log += e.GetMessage();
    }

    TotalTime = ((float)GetTickCount() - StartTime)/1000;
    ssFinish << "Test " << TestInfo->Test << " completed in ";
    ssFinish << setprecision(2) << fixed << TotalTime << " seconds." << endl;
    StringOut(ssFinish.str());
    TestInfo->Log += ssFinish.str();
}

/**
 * Interface to other classes for running all desired Wine tests.
 */
void
CWineTest::Run()
{
    auto_ptr<CTestList> TestList;
    auto_ptr<CWebService> WebService;
    CTestInfo* TestInfo;
    DWORD ErrorMode;

    /* The virtual test list is of course faster, so it should be preferred over
       the journaled one.
       Enable the journaled one only in case ...
          - we're running under ReactOS (as the journal is only useful in conjunction with sysreg2)
          - we shall keep information for Crash Recovery
          - and the user didn't specify a module (then doing Crash Recovery doesn't really make sense) */
    if(Configuration.IsReactOS() && Configuration.DoCrashRecovery() && Configuration.GetModule().empty())
    {
        /* Use a test list with a permanent journal */
        TestList.reset(new CJournaledTestList(this));
    }
    else
    {
        /* Use the fast virtual test list with no additional overhead */
        TestList.reset(new CVirtualTestList(this));
    }

    /* Initialize the Web Service interface if required */
    if(Configuration.DoSubmit())
        WebService.reset(new CWebService());

    /* Disable error dialogs if we're running in non-interactive mode */
    if(!Configuration.IsInteractive())
        ErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

    /* Get information for each test to run */
    while((TestInfo = TestList->GetNextTestInfo()) != 0)
    {
        auto_ptr<CTestInfo> TestInfoPtr(TestInfo);

        RunTest(TestInfo);

        if(Configuration.DoSubmit() && !TestInfo->Log.empty())
            WebService->Submit("wine", TestInfo);

        StringOut("\n\n");
    }

    /* We're done with all tests. Finish this run */
    if(Configuration.DoSubmit())
        WebService->Finish("wine");

    /* Restore the original error mode */
    if(!Configuration.IsInteractive())
        SetErrorMode(ErrorMode);
}
