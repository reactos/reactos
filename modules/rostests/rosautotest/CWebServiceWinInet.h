/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Class implementing the WinInet version of the interface to the "testman" Web Service
 * COPYRIGHT:   Copyright 2009-2015 Colin Finck <colin@reactos.org>
 *              Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

class CWebServiceWinInet : public CWebService
{
private:
    HINTERNET m_hInet;
    HINTERNET m_hHTTP;
    HINTERNET m_hHTTPRequest;

    PCHAR DoRequest(const char* Hostname, INTERNET_PORT Port, const char* ServerFile, const string& InputData) override;

public:
    CWebServiceWinInet();
    virtual ~CWebServiceWinInet();
};
