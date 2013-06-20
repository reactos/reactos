/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class implementing the interface to the "testman" Web Service
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
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

    void Submit(const char* TestType, CTestInfo* TestInfo);
};
