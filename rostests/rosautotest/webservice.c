/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Submitting test results to the Web Service
 * COPYRIGHT:   Copyright 2008-2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static const CHAR ActionProp[] = "action=";
static const CHAR TestIDProp[] = "&testid=";
static const CHAR TestTypeProp[] = "&testtype=";
static const CHAR WineTestType[] = "wine";

/**
 * Sends data to the ReactOS Web Test Manager web service.
 *
 * @param Data
 * Pointer to a CHAR pointer, which contains the data to submit as HTTP POST data.
 * The buffer behind this pointer had to be allocated with HeapAlloc.
 * Returns the data received by the web service after the call.
 *
 * @param DataLength
 * Pointer to a DWORD, which contains the length of the data to submit (in bytes).
 * Returns the length of the data received by the web service after the call (in bytes).
 *
 * @return
 * TRUE if everything went well, FALSE if an error occured while submitting the request.
 * In case of an error, the function will output an appropriate error message through StringOut.
 */
static BOOL
IntDoRequest(char** Data, PDWORD DataLength)
{
    const WCHAR Headers[] = L"Content-Type: application/x-www-form-urlencoded";

    BOOL ReturnValue = FALSE;
    HINTERNET hHTTP = NULL;
    HINTERNET hHTTPRequest = NULL;
    HINTERNET hInet = NULL;

    /* Establish an internet connection to the "testman" server */
    hInet = InternetOpenW(L"rosautotest", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    if(!hInet)
    {
        StringOut("InternetOpenW failed\n");
        goto Cleanup;
    }

    hHTTP = InternetConnectW(hInet, SERVER_HOSTNAME, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    if(!hHTTP)
    {
        StringOut("InternetConnectW failed\n");
        goto Cleanup;
    }

    /* Post our test results to the web service */
    hHTTPRequest = HttpOpenRequestW(hHTTP, L"POST", SERVER_FILE, NULL, NULL, NULL, INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);

    if(!hHTTPRequest)
    {
        StringOut("HttpOpenRequestW failed\n");
        goto Cleanup;
    }

    if(!HttpSendRequestW(hHTTPRequest, Headers, wcslen(Headers), *Data, *DataLength))
    {
        StringOut("HttpSendRequestW failed\n");
        goto Cleanup;
    }

    HeapFree(hProcessHeap, 0, *Data);
    *Data = NULL;

    /* Get the response */
    if(!InternetQueryDataAvailable(hHTTPRequest, DataLength, 0, 0))
    {
        StringOut("InternetQueryDataAvailable failed\n");
        goto Cleanup;
    }

    *Data = HeapAlloc(hProcessHeap, 0, *DataLength + 1);

    if(!InternetReadFile(hHTTPRequest, *Data, *DataLength, DataLength))
    {
        StringOut("InternetReadFile failed\n");
        goto Cleanup;
    }

    (*Data)[*DataLength] = 0;
    ReturnValue = TRUE;

Cleanup:
    if(hHTTPRequest)
        InternetCloseHandle(hHTTPRequest);

    if(hHTTP)
        InternetCloseHandle(hHTTP);

    if(hInet)
        InternetCloseHandle(hInet);

    return ReturnValue;
}

/**
 * Determines whether a string contains entirely numeric values.
 *
 * @param Input
 * The string to check.
 *
 * @return
 * TRUE if the string is entirely numeric, FALSE otherwise.
 */
static BOOL
IsNumber(PCHAR Input)
{
    do
    {
        if(!isdigit(*Input))
            return FALSE;

        ++Input;
    }
    while(*Input);

    return TRUE;
}

/**
 * Requests a Test ID from the web service for our test run.
 *
 * @param TestType
 * Value from the TESTTYPES enum indicating the type of test we are about to submit.
 *
 * @return
 * Returns the Test ID as a CHAR array if successful or NULL otherwise.
 * The caller needs to HeapFree the returned pointer in case of success.
 */
PCHAR
GetTestID(TESTTYPES TestType)
{
    const CHAR GetTestIDAction[] = "gettestid";

    DWORD DataLength;
    PCHAR Data;
    PCHAR ReturnValue = NULL;

    /* Build the full request string */
    DataLength  = sizeof(ActionProp) - 1 + sizeof(GetTestIDAction) - 1;
    DataLength += strlen(AuthenticationRequestString) + strlen(SystemInfoRequestString);
    DataLength += sizeof(TestTypeProp) - 1;

    switch(TestType)
    {
        case WineTest:
            DataLength += sizeof(WineTestType) - 1;
            break;
    }

    Data = HeapAlloc(hProcessHeap, 0, DataLength + 1);
    strcpy(Data, ActionProp);
    strcat(Data, GetTestIDAction);
    strcat(Data, AuthenticationRequestString);
    strcat(Data, SystemInfoRequestString);
    strcat(Data, TestTypeProp);

    switch(TestType)
    {
        case WineTest:
            strcat(Data, WineTestType);
            break;
    }

    if(!IntDoRequest(&Data, &DataLength))
        goto Cleanup;

    /* Verify that this is really a number */
    if(!IsNumber(Data))
    {
        StringOut("Expected Test ID, but received:\n");
        StringOut(Data);
        StringOut("\n");
        goto Cleanup;
    }

    ReturnValue = Data;

Cleanup:
    if(Data && ReturnValue != Data)
        HeapFree(hProcessHeap, 0, Data);

    return ReturnValue;
}

/**
 * Requests a Suite ID from the web service for our module/test combination.
 *
 * @param TestType
 * Value from the TESTTYPES enum indicating the type of test we are about to submit.
 *
 * @param TestData
 * Pointer to a *_GETSUITEID_DATA structure appropriate for our selected test type.
 * Contains other input information for this request.
 *
 * @return
 * Returns the Suite ID as a CHAR array if successful or NULL otherwise.
 * The caller needs to HeapFree the returned pointer in case of success.
 */
PCHAR
GetSuiteID(TESTTYPES TestType, const PVOID TestData)
{
    const CHAR GetSuiteIDAction[] = "getsuiteid";
    const CHAR ModuleProp[] = "&module=";
    const CHAR TestProp[] = "&test=";

    DWORD DataLength;
    PCHAR Data;
    PCHAR ReturnValue = NULL;
    PWINE_GETSUITEID_DATA WineData;

    DataLength  = sizeof(ActionProp) - 1 + sizeof(GetSuiteIDAction) - 1;
    DataLength += strlen(AuthenticationRequestString);
    DataLength += sizeof(TestTypeProp) - 1;

    switch(TestType)
    {
        case WineTest:
            DataLength += sizeof(WineTestType) - 1;

            WineData = (PWINE_GETSUITEID_DATA)TestData;
            DataLength += sizeof(ModuleProp) - 1;
            DataLength += strlen(WineData->Module);
            DataLength += sizeof(TestProp) - 1;
            DataLength += strlen(WineData->Test);

            break;
    }

    Data = HeapAlloc(hProcessHeap, 0, DataLength + 1);
    strcpy(Data, ActionProp);
    strcat(Data, GetSuiteIDAction);
    strcat(Data, AuthenticationRequestString);
    strcat(Data, TestTypeProp);

    switch(TestType)
    {
        case WineTest:
            strcat(Data, WineTestType);

            /* Stupid GCC and MSVC: WineData is already initialized above, still it's reported as a potentially uninitialized variable :-( */
            WineData = (PWINE_GETSUITEID_DATA)TestData;
            strcat(Data, ModuleProp);
            strcat(Data, WineData->Module);
            strcat(Data, TestProp);
            strcat(Data, WineData->Test);

            break;
    }

    if(!IntDoRequest(&Data, &DataLength))
        goto Cleanup;

    /* Verify that this is really a number */
    if(!IsNumber(Data))
    {
        StringOut("Expected Suite ID, but received:\n");
        StringOut(Data);
        StringOut("\n");
        goto Cleanup;
    }

    ReturnValue = Data;

Cleanup:
    if(Data && ReturnValue != Data)
        HeapFree(hProcessHeap, 0, Data);

    return ReturnValue;
}

/**
 * Submits the result of one test call to the web service.
 *
 * @param TestType
 * Value from the TESTTYPES enum indicating the type of test we are about to submit.
 *
 * @param TestData
 * Pointer to a *_SUBMIT_DATA structure appropriate for our selected test type.
 * Contains other input information for this request.
 *
 * @return
 * TRUE if everything went well, FALSE otherwise.
 */
BOOL
Submit(TESTTYPES TestType, const PVOID TestData)
{
    const CHAR SubmitAction[] = "submit";
    const CHAR SuiteIDProp[] = "&suiteid=";
    const CHAR LogProp[] = "&log=";

    BOOL ReturnValue = FALSE;
    DWORD DataLength;
    PCHAR Data;
    PCHAR pData;
    PGENERAL_SUBMIT_DATA GeneralData;
    PWINE_SUBMIT_DATA WineData;

    /* Compute the full length of the POST data */
    DataLength  = sizeof(ActionProp) - 1 + sizeof(SubmitAction) - 1;
    DataLength += strlen(AuthenticationRequestString);

    GeneralData = (PGENERAL_SUBMIT_DATA)TestData;
    DataLength += sizeof(TestIDProp) - 1;
    DataLength += strlen(GeneralData->TestID);
    DataLength += sizeof(SuiteIDProp) - 1;
    DataLength += strlen(GeneralData->SuiteID);

    /* The rest of the POST data depends on the test type */
    DataLength += sizeof(TestTypeProp) - 1;

    switch(TestType)
    {
        case WineTest:
            DataLength += sizeof(WineTestType) - 1;

            WineData = (PWINE_SUBMIT_DATA)TestData;
            DataLength += sizeof(LogProp) - 1;
            DataLength += 3 * strlen(WineData->Log);

            break;
    }

    /* Now collect all the POST data */
    Data = HeapAlloc(hProcessHeap, 0, DataLength + 1);
    strcpy(Data, ActionProp);
    strcat(Data, SubmitAction);
    strcat(Data, AuthenticationRequestString);

    strcat(Data, TestIDProp);
    strcat(Data, GeneralData->TestID);
    strcat(Data, SuiteIDProp);
    strcat(Data, GeneralData->SuiteID);

    strcat(Data, TestTypeProp);

    switch(TestType)
    {
        case WineTest:
            strcat(Data, WineTestType);

            /* Stupid GCC and MSVC: WineData is already initialized above, still it's reported as a potentially uninitialized variable :-( */
            WineData = (PWINE_SUBMIT_DATA)TestData;

            strcat(Data, LogProp);
            pData = Data + strlen(Data);
            EscapeString(pData, WineData->Log);

            break;
    }

    /* DataLength still contains the maximum length of the buffer, but not the actual data length we need for the request.
       Determine that one now. */
    DataLength = strlen(Data);

    /* Send all the stuff */
    if(!IntDoRequest(&Data, &DataLength))
        goto Cleanup;

    /* Output the response */
    StringOut("The server responded:\n");
    StringOut(Data);
    StringOut("\n");

    if(!strcmp(Data, "OK"))
        ReturnValue = TRUE;

Cleanup:
    if(Data)
        HeapFree(hProcessHeap, 0, Data);

    return ReturnValue;
}

/**
 * Finishes a test run for the web service.
 *
 * @param TestType
 * Value from the TESTTYPES enum indicating the type of test we are about to submit.
 *
 * @param TestData
 * Pointer to a *_FINISH_DATA structure appropriate for our selected test type.
 * Contains other input information for this request.
 *
 * @return
 * TRUE if everything went well, FALSE otherwise.
 */
BOOL
Finish(TESTTYPES TestType, const PVOID TestData)
{
    const CHAR FinishAction[] = "finish";

    BOOL ReturnValue = FALSE;
    DWORD DataLength;
    PCHAR Data;
    PGENERAL_FINISH_DATA GeneralData;

    /* Build the full request string */
    DataLength  = sizeof(ActionProp) - 1 + sizeof(FinishAction) - 1;
    DataLength += strlen(AuthenticationRequestString);

    GeneralData = (PGENERAL_FINISH_DATA)TestData;
    DataLength += sizeof(TestIDProp) - 1;
    DataLength += strlen(GeneralData->TestID);

    DataLength += sizeof(TestTypeProp) - 1;

    switch(TestType)
    {
        case WineTest:
            DataLength += sizeof(WineTestType) - 1;
            break;
    }

    Data = HeapAlloc(hProcessHeap, 0, DataLength + 1);
    strcpy(Data, ActionProp);
    strcat(Data, FinishAction);
    strcat(Data, AuthenticationRequestString);
    strcat(Data, TestIDProp);
    strcat(Data, GeneralData->TestID);
    strcat(Data, TestTypeProp);

    switch(TestType)
    {
        case WineTest:
            strcat(Data, WineTestType);
            break;
    }

    if(!IntDoRequest(&Data, &DataLength))
        goto Cleanup;

    if(!strcmp(Data, "OK"))
        ReturnValue = TRUE;

Cleanup:
    if(Data)
        HeapFree(hProcessHeap, 0, Data);

    return ReturnValue;
}
