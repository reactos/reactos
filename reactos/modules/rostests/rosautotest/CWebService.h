/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class implementing the interface to the "testman" Web Service
 * COPYRIGHT:   Copyright 2009-2015 Colin Finck (colin@reactos.org)
 */

class CWebService
{
private:
    HINTERNET m_hInet;
    HINTERNET m_hHTTP;
    HINTERNET m_hHTTPRequest;
    PCHAR m_TestID;

    PCHAR DoRequest(const string& InputData);
    void GetTestID(const char* TestType);
    PCHAR GetSuiteID(const char* TestType, CTestInfo* TestInfo);

public:
    CWebService();
    ~CWebService();

    void Finish(const char* TestType);
    void Submit(const char* TestType, CTestInfo* TestInfo);
};
