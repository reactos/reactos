/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class implementing a journaled test list for the Crash Recovery feature
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
 */

class CJournaledTestList : public CTestList
{
private:
    HANDLE m_hJournal;
    size_t m_ListIterator;
    vector<CTestInfo> m_List;
    wstring m_JournalFile;

    void LoadJournalFile();
    void OpenJournal(DWORD DesiredAccess, bool CreateNew = false);
    void SerializeIntoJournal(const string& String);
    void SerializeIntoJournal(const wstring& String);
    void UnserializeFromBuffer(char** Buffer, string& Output);
    void UnserializeFromBuffer(char** Buffer, wstring& Output);
    void UpdateJournal();
    void WriteInitialJournalFile();

public:
    CJournaledTestList(CTest* Test);
    ~CJournaledTestList();

    CTestInfo* GetNextTestInfo();
};
