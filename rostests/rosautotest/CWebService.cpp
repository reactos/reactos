/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class implementing the interface to the "testman" Web Service
 * COPYRIGHT:   Copyright 2009-2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static const WCHAR szHostname[] = L"reactos.org";
static const WCHAR szServerFile[] = L"sites/all/modules/reactos/testman/webservice/";

/**
 * Constructs a CWebService object and immediately establishes a connection to the "testman" Web Service.
 */
CWebService::CWebService()
{
    /* Zero-initialize variables */
    m_hHTTP = NULL;
    m_hHTTPRequest = NULL;
    m_TestID = NULL;

    /* Establish an internet connection to the "testman" server */
    m_hInet = InternetOpenW(L"rosautotest", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    if(!m_hInet)
        FATAL("InternetOpenW failed\n");

    m_hHTTP = InternetConnectW(m_hInet, szHostname, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    if(!m_hHTTP)
        FATAL("InternetConnectW failed\n");
}

/**
 * Destructs a CWebService object and closes all connections to the Web Service.
 */
CWebService::~CWebService()
{
    if(m_hInet)
        InternetCloseHandle(m_hInet);

    if(m_hHTTP)
        InternetCloseHandle(m_hHTTP);

    if(m_hHTTPRequest)
        InternetCloseHandle(m_hHTTPRequest);

    if(m_TestID)
        delete m_TestID;
}

/**
 * Sends data to the Web Service.
 *
 * @param InputData
 * A std::string containing all the data, which is going to be submitted as HTTP POST data.
 *
 * @return
 * Returns a pointer to a char array containing the data received from the Web Service.
 * The caller needs to free that pointer.
 */
PCHAR
CWebService::DoRequest(const string& InputData)
{
    const WCHAR szHeaders[] = L"Content-Type: application/x-www-form-urlencoded";

    auto_array_ptr<char> Data;
    DWORD DataLength;

    /* Post our test results to the web service */
    m_hHTTPRequest = HttpOpenRequestW(m_hHTTP, L"POST", szServerFile, NULL, NULL, NULL, INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);

    if(!m_hHTTPRequest)
        FATAL("HttpOpenRequestW failed\n");

    Data.reset(new char[InputData.size() + 1]);
    strcpy(Data, InputData.c_str());

    if(!HttpSendRequestW(m_hHTTPRequest, szHeaders, wcslen(szHeaders), Data, InputData.size()))
        FATAL("HttpSendRequestW failed\n");

    /* Get the response */
    if(!InternetQueryDataAvailable(m_hHTTPRequest, &DataLength, 0, 0))
        FATAL("InternetQueryDataAvailable failed\n");

    Data.reset(new char[DataLength + 1]);

    if(!InternetReadFile(m_hHTTPRequest, Data, DataLength, &DataLength))
        FATAL("InternetReadFile failed\n");

    Data[DataLength] = 0;

    return Data.release();
}

/**
* Interface to other classes for finishing this test run
*
* @param TestType
* Constant pointer to a char array containing the test type to be run (i.e. "wine")
*/
void
CWebService::Finish(const char* TestType)
{
    auto_array_ptr<char> Response;
    string Data;
    stringstream ss;

    if (!m_TestID)
        EXCEPTION("CWebService::Finish was called, but not a single result had been submitted!");

    Data = "action=finish";
    Data += Configuration.GetAuthenticationRequestString();
    Data += "&testtype=";
    Data += TestType;
    Data += "&testid=";
    Data += m_TestID;

    Response.reset(DoRequest(Data));

    if (strcmp(Response, "OK"))
    {
        ss << "When finishing the test run, the server responded:" << endl << Response << endl;
        SSEXCEPTION;
    }
}

/**
 * Requests a Test ID from the Web Service for our test run.
 *
 * @param TestType
 * Constant pointer to a char array containing the test type to be run (i.e. "wine")
 */
void
CWebService::GetTestID(const char* TestType)
{
    string Data;

    Data = "action=gettestid";
    Data += Configuration.GetAuthenticationRequestString();
    Data += Configuration.GetSystemInfoRequestString();
    Data += "&testtype=";
    Data += TestType;

    if(!Configuration.GetComment().empty())
    {
        Data += "&comment=";
        Data += Configuration.GetComment();
    }

    m_TestID = DoRequest(Data);

    /* Verify that this is really a number */
    if(!IsNumber(m_TestID))
    {
        stringstream ss;

        ss << "Expected Test ID, but received:" << endl << m_TestID << endl;
        SSEXCEPTION;
    }
}

/**
 * Gets a Suite ID from the Web Service for this module/test combination.
 *
 * @param TestType
 * Constant pointer to a char array containing the test type to be run (i.e. "wine")
 *
 * @param TestInfo
 * Pointer to a CTestInfo object containing information about the test
 *
 * @return
 * Returns a pointer to a char array containing the Suite ID received from the Web Service.
 * The caller needs to free that pointer.
 */
PCHAR
CWebService::GetSuiteID(const char* TestType, CTestInfo* TestInfo)
{
    auto_array_ptr<char> SuiteID;
    string Data;

    Data = "action=getsuiteid";
    Data += Configuration.GetAuthenticationRequestString();
    Data += "&testtype=";
    Data += TestType;
    Data += "&module=";
    Data += TestInfo->Module;
    Data += "&test=";
    Data += TestInfo->Test;

    SuiteID.reset(DoRequest(Data));

    /* Verify that this is really a number */
    if(!IsNumber(SuiteID))
    {
        stringstream ss;

        ss << "Expected Suite ID, but received:" << endl << SuiteID << endl;
        SSEXCEPTION;
    }

    return SuiteID.release();
}

/**
 * Interface to other classes for submitting a result of one test
 *
 * @param TestType
 * Constant pointer to a char array containing the test type to be run (i.e. "wine")
 *
 * @param TestInfo
 * Pointer to a CTestInfo object containing information about the test
 */
void
CWebService::Submit(const char* TestType, CTestInfo* TestInfo)
{
    auto_array_ptr<char> Response;
    auto_array_ptr<char> SuiteID;
    string Data;
    stringstream ss;

    if(!m_TestID)
        GetTestID(TestType);

    SuiteID.reset(GetSuiteID(TestType, TestInfo));

    Data = "action=submit";
    Data += Configuration.GetAuthenticationRequestString();
    Data += "&testtype=";
    Data += TestType;
    Data += "&testid=";
    Data += m_TestID;
    Data += "&suiteid=";
    Data += SuiteID;
    Data += "&log=";
    Data += EscapeString(TestInfo->Log);

    Response.reset(DoRequest(Data));

    if (strcmp(Response, "OK"))
    {
        ss << "When submitting the result, the server responded:" << endl << Response << endl;
        SSEXCEPTION;
    }
}
