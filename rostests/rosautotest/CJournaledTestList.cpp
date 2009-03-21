/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class implementing a journaled test list for the Crash Recovery feature
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static const char szJournalHeader[] = "RAT_J-V1";
static const WCHAR szJournalFileName[] = L"rosautotest.journal";

/**
 * Constructs a CJournaledTestList object for an associated CTest-derived object.
 *
 * @param Test
 * Pointer to a CTest-derived object, for which this test list shall serve.
 */
CJournaledTestList::CJournaledTestList(CTest* Test)
    : CTestList(Test)
{
    WCHAR JournalFile[MAX_PATH];

    m_hJournal = INVALID_HANDLE_VALUE;

    /* Build the path to the journal file */
    if(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, JournalFile) != S_OK)
        FATAL("SHGetFolderPathW failed\n");

    m_JournalFile = JournalFile;
    m_JournalFile += L"\\rosautotest\\";

    /* Create the directory if necessary */
    if(GetFileAttributesW(m_JournalFile.c_str()) == INVALID_FILE_ATTRIBUTES)
        CreateDirectoryW(m_JournalFile.c_str(), NULL);

    m_JournalFile += szJournalFileName;

    /* Check if the journal already exists */
    if(GetFileAttributesW(m_JournalFile.c_str()) == INVALID_FILE_ATTRIBUTES)
        WriteInitialJournalFile();
    else
        LoadJournalFile();
}

/**
 * Destructs a CJournaledTestList object.
 */
CJournaledTestList::~CJournaledTestList()
{
    if(m_hJournal != INVALID_HANDLE_VALUE)
        CloseHandle(m_hJournal);
}

/**
 * Opens the journal file through the CreateFileW API using the m_hJournal handle.
 *
 * @param DesiredAccess
 * dwDesiredAccess parameter passed to CreateFileW
 *
 * @param CreateNew
 * true if the journal file shall be created, false if an existing one shall be opened
 */
void
CJournaledTestList::OpenJournal(DWORD DesiredAccess, bool CreateNew)
{
    m_hJournal = CreateFileW(m_JournalFile.c_str(), DesiredAccess, 0, NULL, (CreateNew ? CREATE_ALWAYS : OPEN_EXISTING), FILE_ATTRIBUTE_NORMAL, NULL);

    if(m_hJournal == INVALID_HANDLE_VALUE)
        FATAL("CreateFileW failed\n");
}

/**
 * Serializes a std::string and writes it into the opened journal file.
 *
 * @param String
 * The std::string to serialize
 *
 * @see UnserializeFromBuffer
 */
void
CJournaledTestList::SerializeIntoJournal(const string& String)
{
    DWORD BytesWritten;
    WriteFile(m_hJournal, String.c_str(), String.size() + 1, &BytesWritten, NULL);
}

/**
 * Serializes a std::wstring and writes it into the opened journal file.
 *
 * @param String
 * The std::wstring to serialize
 *
 * @see UnserializeFromBuffer
 */
void
CJournaledTestList::SerializeIntoJournal(const wstring& String)
{
    DWORD BytesWritten;
    WriteFile(m_hJournal, String.c_str(), (String.size() + 1) * sizeof(WCHAR), &BytesWritten, NULL);
}

/**
 * Unserializes the next std::string from the journal buffer.
 * The passed buffer pointer will point at the next element afterwards.
 *
 * @param Buffer
 * Pointer to a pointer to a char array containing the journal buffer
 *
 * @param Output
 * The std::string to unserialize the value into.
 */
void
CJournaledTestList::UnserializeFromBuffer(char** Buffer, string& Output)
{
    Output = string(*Buffer);
    *Buffer += Output.size() + 1;
}

/**
 * Unserializes the next std::wstring from the journal buffer.
 * The passed buffer pointer will point at the next element afterwards.
 *
 * @param Buffer
 * Pointer to a pointer to a char array containing the journal buffer
 *
 * @param Output
 * The std::wstring to unserialize the value into.
 */
void
CJournaledTestList::UnserializeFromBuffer(char** Buffer, wstring& Output)
{
    Output = wstring((PWSTR)*Buffer);
    *Buffer += (Output.size() + 1) * sizeof(WCHAR);
}

/**
 * Gets all tests to be run and writes an initial journal file with this information.
 */
void
CJournaledTestList::WriteInitialJournalFile()
{
    char TerminatingNull = 0;
    CTestInfo* TestInfo;
    DWORD BytesWritten;

    StringOut("Writing initial journal file...\n\n");

    m_ListIterator = 0;

    /* Store all command lines in the m_List vector */
    while((TestInfo = m_Test->GetNextTestInfo()) != 0)
    {
        m_List.push_back(*TestInfo);
        delete TestInfo;
    }

    /* Serialize the vector and the iterator into a file */
    OpenJournal(GENERIC_WRITE, true);

    WriteFile(m_hJournal, szJournalHeader, sizeof(szJournalHeader), &BytesWritten, NULL);
    WriteFile(m_hJournal, &m_ListIterator, sizeof(m_ListIterator), &BytesWritten, NULL);

    for(size_t i = 0; i < m_List.size(); i++)
    {
        SerializeIntoJournal(m_List[i].CommandLine);
        SerializeIntoJournal(m_List[i].Module);
        SerializeIntoJournal(m_List[i].Test);
    }

    WriteFile(m_hJournal, &TerminatingNull, sizeof(TerminatingNull), &BytesWritten, NULL);

    CloseHandle(m_hJournal);
    m_hJournal = INVALID_HANDLE_VALUE;

    /* m_ListIterator will be incremented before its first use */
    m_ListIterator = (size_t)-1;
}

/**
 * Loads the existing journal file and sets all members to the values saved in that file.
 */
void
CJournaledTestList::LoadJournalFile()
{
    char* Buffer;
    char* pBuffer;
    char FileHeader[sizeof(szJournalHeader)];
    DWORD BytesRead;
    DWORD RemainingSize;

    StringOut("Loading journal file...\n\n");

    OpenJournal(GENERIC_READ);
    RemainingSize = GetFileSize(m_hJournal, NULL);

    /* Verify the header of the journal file */
    ReadFile(m_hJournal, FileHeader, sizeof(szJournalHeader), &BytesRead, NULL);
    RemainingSize -= BytesRead;

    if(BytesRead != sizeof(szJournalHeader))
        EXCEPTION("Journal file contains no header!\n");

    if(strcmp(FileHeader, szJournalHeader))
        EXCEPTION("Journal file has an unsupported header!\n");

    /* Read the iterator */
    ReadFile(m_hJournal, &m_ListIterator, sizeof(m_ListIterator), &BytesRead, NULL);
    RemainingSize -= BytesRead;

    if(BytesRead != sizeof(m_ListIterator))
        EXCEPTION("Journal file contains no m_ListIterator member!\n");

    /* Read the rest of the file into a buffer */
    Buffer = new char[RemainingSize];
    pBuffer = Buffer;
    ReadFile(m_hJournal, Buffer, RemainingSize, &BytesRead, NULL);

    CloseHandle(m_hJournal);
    m_hJournal = NULL;

    /* Now recreate the m_List vector out of that information */
    while(*pBuffer)
    {
        CTestInfo TestInfo;

        UnserializeFromBuffer(&pBuffer, TestInfo.CommandLine);
        UnserializeFromBuffer(&pBuffer, TestInfo.Module);
        UnserializeFromBuffer(&pBuffer, TestInfo.Test);

        m_List.push_back(TestInfo);
    }

    delete[] Buffer;
}

/**
 * Writes the current m_ListIterator value into the journal.
 */
void
CJournaledTestList::UpdateJournal()
{
    DWORD BytesWritten;

    OpenJournal(GENERIC_WRITE);

    /* Skip the header */
    SetFilePointer(m_hJournal, sizeof(szJournalHeader), NULL, FILE_CURRENT);

    WriteFile(m_hJournal, &m_ListIterator, sizeof(m_ListIterator), &BytesWritten, NULL);

    CloseHandle(m_hJournal);
    m_hJournal = NULL;
}

/**
 * Interface to other classes for receiving information about the next test to be run.
 *
 * @return
 * A pointer to a CTestInfo object, which contains all available information about the next test.
 * The caller needs to free that object.
 */
CTestInfo*
CJournaledTestList::GetNextTestInfo()
{
    CTestInfo* TestInfo;

    /* Always jump to the next test here.
       - If we're at the beginning of a new test list, the iterator will be set to 0.
       - If we started with a loaded one, we assume that the test m_ListIterator is currently set
         to crashed, so we move to the next test. */
    ++m_ListIterator;

    /* Check whether the iterator would already exceed the number of stored elements */
    if(m_ListIterator == m_List.size())
    {
        /* Delete the journal and return no pointer */
        DeleteFileW(m_JournalFile.c_str());

        TestInfo = NULL;
    }
    else
    {
        /* Update the journal with the current iterator and return the test information */
        UpdateJournal();

        TestInfo = new CTestInfo(m_List[m_ListIterator]);
    }

    return TestInfo;
}
