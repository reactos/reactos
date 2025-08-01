/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class implementing the interface to the "testman" Web Service
 * COPYRIGHT:   Copyright 2009-2015 Colin Finck (colin@reactos.org)
 */

class CWebService
{
private:
    PCHAR m_TestID;

    virtual PCHAR DoRequest(const char* Hostname, INTERNET_PORT Port, const char* ServerFile, const string& InputData) = 0;
    void GetTestID(const char* TestType);
    PCHAR GetSuiteID(const char* TestType, CTestInfo* TestInfo);

public:
    CWebService();
    virtual ~CWebService();

    void Finish(const char* TestType);
    void Submit(const char* TestType, CTestInfo* TestInfo);
};
