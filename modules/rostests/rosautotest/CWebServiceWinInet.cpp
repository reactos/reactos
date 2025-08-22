/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Class implementing the WinInet version of the interface to the "testman" Web Service
 * COPYRIGHT:   Copyright 2009-2015 Colin Finck <colin@reactos.org>
 *              Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"

/**
 * Constructs a CWebService object and immediately establishes a connection to the "testman" Web Service.
 */
CWebServiceWinInet::CWebServiceWinInet()
{
    /* Zero-initialize variables */
    m_hHTTP = NULL;
    m_hHTTPRequest = NULL;

    /* Establish an internet connection to the "testman" server */
    m_hInet = InternetOpenW(L"rosautotest", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    if(!m_hInet)
        FATAL("InternetOpenW failed\n");
}

/**
 * Destructs a CWebService object and closes all connections to the Web Service.
 */
CWebServiceWinInet::~CWebServiceWinInet()
{
    if(m_hInet)
        InternetCloseHandle(m_hInet);

    if(m_hHTTP)
        InternetCloseHandle(m_hHTTP);

    if(m_hHTTPRequest)
        InternetCloseHandle(m_hHTTPRequest);
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
CWebServiceWinInet::DoRequest(const char *Hostname, INTERNET_PORT Port, const char *ServerFile, const string &InputData)
{
    const WCHAR szHeaders[] = L"Content-Type: application/x-www-form-urlencoded";

    auto_array_ptr<char> Data;
    DWORD DataLength;

    if (!m_hHTTP)
    {
        m_hHTTP = InternetConnectA(m_hInet, Hostname, Port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

        if (!m_hHTTP)
            FATAL("InternetConnectW failed\n");

    }

    /* Post our test results to the web service */
    m_hHTTPRequest = HttpOpenRequestA(m_hHTTP, "POST", ServerFile, NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);

    if(!m_hHTTPRequest)
        FATAL("HttpOpenRequestW failed\n");

    Data.reset(new char[InputData.size() + 1]);
    strcpy(Data, InputData.c_str());

    if(!HttpSendRequestW(m_hHTTPRequest, szHeaders, lstrlenW(szHeaders), Data, (DWORD)InputData.size()))
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
