/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Class implementing the interface to the "testman" Web Service
 * COPYRIGHT:   Copyright 2009-2020 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

static const CHAR szHostname[] = "reactos.org";
static const INTERNET_PORT ServerPort = 8443;
static const CHAR szServerFile[] = "testman/webservice/";

/**
 * Constructs a CWebService object and immediately establishes a connection to the "testman" Web Service.
 */
CWebService::CWebService()
{
    /* Zero-initialize variables */
    m_TestID = NULL;
}

/**
 * Destructs a CWebService object and closes all connections to the Web Service.
 */
CWebService::~CWebService()
{
    if(m_TestID)
        delete m_TestID;
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
        EXCEPTION("CWebService::Finish was called, but not a single result had been submitted!\n");

    Data = "action=finish";
    Data += Configuration.GetAuthenticationRequestString();
    Data += "&testtype=";
    Data += TestType;
    Data += "&testid=";
    Data += m_TestID;

    Response.reset(DoRequest(szHostname, ServerPort, szServerFile, Data));

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

    m_TestID = DoRequest(szHostname, ServerPort, szServerFile, Data);

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

    SuiteID.reset(DoRequest(szHostname, ServerPort, szServerFile, Data));

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

    Response.reset(DoRequest(szHostname, ServerPort, szServerFile, Data));

    if (strcmp(Response, "OK"))
    {
        ss << "When submitting the result, the server responded:" << endl << Response << endl;
        SSEXCEPTION;
    }
}
