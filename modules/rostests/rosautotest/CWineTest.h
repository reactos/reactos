/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class implementing functions for handling Wine tests
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
 */

class CWineTest : public CTest
{
private:
    HANDLE m_hFind;
    PCHAR m_ListBuffer;
    string m_CurrentTest;
    wstring m_CurrentFile;
    wstring m_CurrentListCommand;
    wstring m_TestPath;

    bool GetNextFile();
    bool GetNextTest();
    CTestInfo* GetNextTestInfo();
    DWORD DoListCommand();
    void RunTest(CTestInfo* TestInfo);

public:
    CWineTest();
    ~CWineTest();

    void Run();
};
