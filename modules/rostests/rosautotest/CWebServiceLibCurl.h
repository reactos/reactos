/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Class implementing the curl interface to the "testman" Web Service
 * COPYRIGHT:   Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

typedef void CURL;

class CWebServiceLibCurl : public CWebService
{
private:
    CURL *m_hCurl;

    PCHAR DoRequest(const char *Hostname, INTERNET_PORT Port, const char *ServerFile, const string& InputData) override;

public:
    CWebServiceLibCurl();
    virtual ~CWebServiceLibCurl();

    static bool CanUseLibCurl();
};
