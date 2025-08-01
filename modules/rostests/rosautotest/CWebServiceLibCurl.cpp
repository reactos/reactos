/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Class implementing the curl interface to the "testman" Web Service
 * COPYRIGHT:   Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"
#include <curl/curl.h>

static HMODULE g_hLibCurl = nullptr;
static decltype(&curl_global_init) pcurl_global_init = nullptr;
static decltype(&curl_easy_init) pcurl_easy_init = nullptr;
static decltype(&curl_easy_setopt) pcurl_easy_setopt = nullptr;
static decltype(&curl_easy_perform) pcurl_easy_perform = nullptr;
static decltype(&curl_easy_strerror) pcurl_easy_strerror = nullptr;
static decltype(&curl_easy_cleanup) pcurl_easy_cleanup = nullptr;
static decltype(&curl_global_cleanup) pcurl_global_cleanup = nullptr;


bool
CWebServiceLibCurl::CanUseLibCurl()
{
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        g_hLibCurl = LoadLibraryA("___libcurl.dll");
        if (!g_hLibCurl)
            return false;

        pcurl_global_init = (decltype(&curl_global_init))GetProcAddress(g_hLibCurl, "curl_global_init");
        pcurl_easy_init = (decltype(&curl_easy_init))GetProcAddress(g_hLibCurl, "curl_easy_init");
        pcurl_easy_setopt = (decltype(&curl_easy_setopt))GetProcAddress(g_hLibCurl, "curl_easy_setopt");
        pcurl_easy_perform = (decltype(&curl_easy_perform))GetProcAddress(g_hLibCurl, "curl_easy_perform");
        pcurl_easy_strerror = (decltype(&curl_easy_strerror))GetProcAddress(g_hLibCurl, "curl_easy_strerror");
        pcurl_easy_cleanup = (decltype(&curl_easy_cleanup))GetProcAddress(g_hLibCurl, "curl_easy_cleanup");
        pcurl_global_cleanup = (decltype(&curl_global_cleanup))GetProcAddress(g_hLibCurl, "curl_global_cleanup");
    }
    if (!pcurl_global_init || !pcurl_easy_init || !pcurl_easy_setopt || !pcurl_easy_perform || !pcurl_easy_strerror ||
        !pcurl_easy_cleanup || !pcurl_global_cleanup)
    {
        return false;
    }
    return true;
}

/**
 * Constructs a CWebServiceLibCurl object
 */
CWebServiceLibCurl::CWebServiceLibCurl()
{
    /* Initialize libcurl */
    if (pcurl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
    {
        FATAL("Failed to initialize libcurl\n");
    }

    m_hCurl = pcurl_easy_init();
    if (!m_hCurl)
    {
        FATAL("Failed to create a libcurl handle\n");
    }

    CHAR BaseName[MAX_PATH*2];
    GetModuleFileNameA(g_hLibCurl, BaseName, _countof(BaseName));
    PCHAR FileName = strrchr(BaseName, '\\');
    FileName = FileName ? (FileName + 1) : BaseName;
    strcpy(FileName, "curl-ca-bundle.crt");
    pcurl_easy_setopt(m_hCurl, CURLOPT_CAINFO, BaseName);
}

/**
 * Destructs a CWebServiceLibCurl object and closes all connections to the Web Service.
 */
CWebServiceLibCurl::~CWebServiceLibCurl()
{
    if (m_hCurl)
    {
        pcurl_easy_cleanup(m_hCurl);
        m_hCurl = nullptr;
    }

    pcurl_global_cleanup();
}


static size_t
callback_func(void *ptr, size_t size, size_t count, void *userdata)
{
    string *ResultData = (string *)userdata;
    ResultData->append((const char*)ptr, count);
    return count;
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
CWebServiceLibCurl::DoRequest(const char *Hostname, INTERNET_PORT Port, const char *ServerFile, const string &InputData)
{
    char buffer[1024];
    sprintf(buffer, "https://%s:%u/%s", Hostname, Port, ServerFile);

    pcurl_easy_setopt(m_hCurl, CURLOPT_URL, buffer);
    pcurl_easy_setopt(m_hCurl, CURLOPT_POSTFIELDS, InputData.c_str());
    pcurl_easy_setopt(m_hCurl, CURLOPT_USERAGENT, "rosautotest/curl");

    string ResultData;
    pcurl_easy_setopt(m_hCurl, CURLOPT_WRITEFUNCTION, callback_func);
    pcurl_easy_setopt(m_hCurl, CURLOPT_WRITEDATA, &ResultData);

    CURLcode res = pcurl_easy_perform(m_hCurl);
    if (res != CURLE_OK)
    {
        string errorMsg = string("curl_easy_perform failed: ") + pcurl_easy_strerror(res);
        FATAL(errorMsg.c_str());
    }

    auto_array_ptr<char> Data;
    Data.reset(new char[ResultData.size() + 1]);
    strncpy(Data, ResultData.c_str(), ResultData.size());
    Data[ResultData.size()] = '\0';

    return Data.release();
}

