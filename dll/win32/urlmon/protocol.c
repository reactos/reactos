/*
 * Copyright 2007 Misha Koshelev
 * Copyright 2009 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "urlmon_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

/* Flags are needed for, among other things, return HRESULTs from the Read function
 * to conform to native. For example, Read returns:
 *
 * 1. E_PENDING if called before the request has completed,
 *        (flags = 0)
 * 2. S_FALSE after all data has been read and S_OK has been reported,
 *        (flags = FLAG_REQUEST_COMPLETE | FLAG_ALL_DATA_READ | FLAG_RESULT_REPORTED)
 * 3. INET_E_DATA_NOT_AVAILABLE if InternetQueryDataAvailable fails. The first time
 *    this occurs, INET_E_DATA_NOT_AVAILABLE will also be reported to the sink,
 *        (flags = FLAG_REQUEST_COMPLETE)
 *    but upon subsequent calls to Read no reporting will take place, yet
 *    InternetQueryDataAvailable will still be called, and, on failure,
 *    INET_E_DATA_NOT_AVAILABLE will still be returned.
 *        (flags = FLAG_REQUEST_COMPLETE | FLAG_RESULT_REPORTED)
 *
 * FLAG_FIRST_DATA_REPORTED and FLAG_LAST_DATA_REPORTED are needed for proper
 * ReportData reporting. For example, if OnResponse returns S_OK, Continue will
 * report BSCF_FIRSTDATANOTIFICATION, and when all data has been read Read will
 * report BSCF_INTERMEDIATEDATANOTIFICATION|BSCF_LASTDATANOTIFICATION. However,
 * if OnResponse does not return S_OK, Continue will not report data, and Read
 * will report BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION when all
 * data has been read.
 */
#define FLAG_REQUEST_COMPLETE         0x0001
#define FLAG_FIRST_CONTINUE_COMPLETE  0x0002
#define FLAG_FIRST_DATA_REPORTED      0x0004
#define FLAG_ALL_DATA_READ            0x0008
#define FLAG_LAST_DATA_REPORTED       0x0010
#define FLAG_RESULT_REPORTED          0x0020

static inline HRESULT report_progress(Protocol *protocol, ULONG status_code, LPCWSTR status_text)
{
    return IInternetProtocolSink_ReportProgress(protocol->protocol_sink, status_code, status_text);
}

static inline HRESULT report_result(Protocol *protocol, HRESULT hres)
{
    if (!(protocol->flags & FLAG_RESULT_REPORTED) && protocol->protocol_sink) {
        protocol->flags |= FLAG_RESULT_REPORTED;
        IInternetProtocolSink_ReportResult(protocol->protocol_sink, hres, 0, NULL);
    }

    return hres;
}

static void report_data(Protocol *protocol)
{
    DWORD bscf;

    if((protocol->flags & FLAG_LAST_DATA_REPORTED) || !protocol->protocol_sink)
        return;

    if(protocol->flags & FLAG_FIRST_DATA_REPORTED) {
        bscf = BSCF_INTERMEDIATEDATANOTIFICATION;
    }else {
        protocol->flags |= FLAG_FIRST_DATA_REPORTED;
        bscf = BSCF_FIRSTDATANOTIFICATION;
    }

    if(protocol->flags & FLAG_ALL_DATA_READ && !(protocol->flags & FLAG_LAST_DATA_REPORTED)) {
        protocol->flags |= FLAG_LAST_DATA_REPORTED;
        bscf |= BSCF_LASTDATANOTIFICATION;
    }

    IInternetProtocolSink_ReportData(protocol->protocol_sink, bscf,
            protocol->current_position+protocol->available_bytes,
            protocol->content_length);
}

static void all_data_read(Protocol *protocol)
{
    protocol->flags |= FLAG_ALL_DATA_READ;

    report_data(protocol);
    report_result(protocol, S_OK);
}

static void request_complete(Protocol *protocol, INTERNET_ASYNC_RESULT *ar)
{
    PROTOCOLDATA data;

    if(!ar->dwResult) {
        WARN("request failed: %d\n", ar->dwError);
        return;
    }

    protocol->flags |= FLAG_REQUEST_COMPLETE;

    if(!protocol->request) {
        TRACE("setting request handle %p\n", (HINTERNET)ar->dwResult);
        protocol->request = (HINTERNET)ar->dwResult;
    }

    /* PROTOCOLDATA same as native */
    memset(&data, 0, sizeof(data));
    data.dwState = 0xf1000000;
    if(protocol->flags & FLAG_FIRST_CONTINUE_COMPLETE)
        data.pData = (LPVOID)BINDSTATUS_ENDDOWNLOADCOMPONENTS;
    else
        data.pData = (LPVOID)BINDSTATUS_DOWNLOADINGDATA;

    if (protocol->bindf & BINDF_FROMURLMON)
        IInternetProtocolSink_Switch(protocol->protocol_sink, &data);
    else
        protocol_continue(protocol, &data);
}

static void WINAPI internet_status_callback(HINTERNET internet, DWORD_PTR context,
        DWORD internet_status, LPVOID status_info, DWORD status_info_len)
{
    Protocol *protocol = (Protocol*)context;

    switch(internet_status) {
    case INTERNET_STATUS_RESOLVING_NAME:
        TRACE("%p INTERNET_STATUS_RESOLVING_NAME\n", protocol);
        report_progress(protocol, BINDSTATUS_FINDINGRESOURCE, (LPWSTR)status_info);
        break;

    case INTERNET_STATUS_CONNECTING_TO_SERVER:
        TRACE("%p INTERNET_STATUS_CONNECTING_TO_SERVER\n", protocol);
        report_progress(protocol, BINDSTATUS_CONNECTING, (LPWSTR)status_info);
        break;

    case INTERNET_STATUS_SENDING_REQUEST:
        TRACE("%p INTERNET_STATUS_SENDING_REQUEST\n", protocol);
        report_progress(protocol, BINDSTATUS_SENDINGREQUEST, (LPWSTR)status_info);
        break;

    case INTERNET_STATUS_REQUEST_COMPLETE:
        request_complete(protocol, status_info);
        break;

    case INTERNET_STATUS_HANDLE_CREATED:
        TRACE("%p INTERNET_STATUS_HANDLE_CREATED\n", protocol);
        IInternetProtocol_AddRef(protocol->protocol);
        break;

    case INTERNET_STATUS_HANDLE_CLOSING:
        TRACE("%p INTERNET_STATUS_HANDLE_CLOSING\n", protocol);

        if(*(HINTERNET *)status_info == protocol->request) {
            protocol->request = NULL;
            if(protocol->protocol_sink) {
                IInternetProtocolSink_Release(protocol->protocol_sink);
                protocol->protocol_sink = NULL;
            }

            if(protocol->bind_info.cbSize) {
                ReleaseBindInfo(&protocol->bind_info);
                memset(&protocol->bind_info, 0, sizeof(protocol->bind_info));
            }
        }else if(*(HINTERNET *)status_info == protocol->connection) {
            protocol->connection = NULL;
        }

        IInternetProtocol_Release(protocol->protocol);
        break;

    default:
        WARN("Unhandled Internet status callback %d\n", internet_status);
    }
}

HRESULT protocol_start(Protocol *protocol, IInternetProtocol *prot, LPCWSTR url,
        IInternetProtocolSink *protocol_sink, IInternetBindInfo *bind_info)
{
    LPOLESTR user_agent = NULL;
    DWORD request_flags;
    ULONG size = 0;
    HRESULT hres;

    protocol->protocol = prot;

    IInternetProtocolSink_AddRef(protocol_sink);
    protocol->protocol_sink = protocol_sink;

    memset(&protocol->bind_info, 0, sizeof(protocol->bind_info));
    protocol->bind_info.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(bind_info, &protocol->bindf, &protocol->bind_info);
    if(hres != S_OK) {
        WARN("GetBindInfo failed: %08x\n", hres);
        return report_result(protocol, hres);
    }

    if(!(protocol->bindf & BINDF_FROMURLMON))
        report_progress(protocol, BINDSTATUS_DIRECTBIND, NULL);

    hres = IInternetBindInfo_GetBindString(bind_info, BINDSTRING_USER_AGENT, &user_agent, 1, &size);
    if (hres != S_OK || !size) {
        DWORD len;
        CHAR null_char = 0;
        LPSTR user_agenta = NULL;

        len = 0;
        if ((hres = ObtainUserAgentString(0, &null_char, &len)) != E_OUTOFMEMORY) {
            WARN("ObtainUserAgentString failed: %08x\n", hres);
        }else if (!(user_agenta = heap_alloc(len*sizeof(CHAR)))) {
            WARN("Out of memory\n");
        }else if ((hres = ObtainUserAgentString(0, user_agenta, &len)) != S_OK) {
            WARN("ObtainUserAgentString failed: %08x\n", hres);
        }else {
            if(!(user_agent = CoTaskMemAlloc((len)*sizeof(WCHAR))))
                WARN("Out of memory\n");
            else
                MultiByteToWideChar(CP_ACP, 0, user_agenta, -1, user_agent, len);
        }
        heap_free(user_agenta);
    }

    protocol->internet = InternetOpenW(user_agent, 0, NULL, NULL, INTERNET_FLAG_ASYNC);
    CoTaskMemFree(user_agent);
    if(!protocol->internet) {
        WARN("InternetOpen failed: %d\n", GetLastError());
        return report_result(protocol, INET_E_NO_SESSION);
    }

    /* Native does not check for success of next call, so we won't either */
    InternetSetStatusCallbackW(protocol->internet, internet_status_callback);

    request_flags = INTERNET_FLAG_KEEP_CONNECTION;
    if(protocol->bindf & BINDF_NOWRITECACHE)
        request_flags |= INTERNET_FLAG_NO_CACHE_WRITE;
    if(protocol->bindf & BINDF_NEEDFILE)
        request_flags |= INTERNET_FLAG_NEED_FILE;

    hres = protocol->vtbl->open_request(protocol, url, request_flags, bind_info);
    if(FAILED(hres)) {
        protocol_close_connection(protocol);
        return report_result(protocol, hres);
    }

    return S_OK;
}

HRESULT protocol_continue(Protocol *protocol, PROTOCOLDATA *data)
{
    HRESULT hres;

    if (!data) {
        WARN("Expected pProtocolData to be non-NULL\n");
        return S_OK;
    }

    if(!protocol->request) {
        WARN("Expected request to be non-NULL\n");
        return S_OK;
    }

    if(!protocol->protocol_sink) {
        WARN("Expected IInternetProtocolSink pointer to be non-NULL\n");
        return S_OK;
    }

    if(data->pData == (LPVOID)BINDSTATUS_DOWNLOADINGDATA) {
        hres = protocol->vtbl->start_downloading(protocol);
        if(FAILED(hres)) {
            protocol_close_connection(protocol);
            report_result(protocol, hres);
            return S_OK;
        }

        if(protocol->bindf & BINDF_NEEDFILE) {
            WCHAR cache_file[MAX_PATH];
            DWORD buflen = sizeof(cache_file);

            if(InternetQueryOptionW(protocol->request, INTERNET_OPTION_DATAFILE_NAME,
                    cache_file, &buflen)) {
                report_progress(protocol, BINDSTATUS_CACHEFILENAMEAVAILABLE, cache_file);
            }else {
                FIXME("Could not get cache file\n");
            }
        }

        protocol->flags |= FLAG_FIRST_CONTINUE_COMPLETE;
    }

    if(data->pData >= (LPVOID)BINDSTATUS_DOWNLOADINGDATA) {
        BOOL res;

        /* InternetQueryDataAvailable may immediately fork and perform its asynchronous
         * read, so clear the flag _before_ calling so it does not incorrectly get cleared
         * after the status callback is called */
        protocol->flags &= ~FLAG_REQUEST_COMPLETE;
        res = InternetQueryDataAvailable(protocol->request, &protocol->available_bytes, 0, 0);
        if(res) {
            protocol->flags |= FLAG_REQUEST_COMPLETE;
            report_data(protocol);
        }else if(GetLastError() != ERROR_IO_PENDING) {
            protocol->flags |= FLAG_REQUEST_COMPLETE;
            WARN("InternetQueryDataAvailable failed: %d\n", GetLastError());
            report_result(protocol, INET_E_DATA_NOT_AVAILABLE);
        }
    }

    return S_OK;
}

HRESULT protocol_read(Protocol *protocol, void *buf, ULONG size, ULONG *read_ret)
{
    ULONG read = 0;
    BOOL res;
    HRESULT hres = S_FALSE;

    if(!(protocol->flags & FLAG_REQUEST_COMPLETE)) {
        *read_ret = 0;
        return E_PENDING;
    }

    if(protocol->flags & FLAG_ALL_DATA_READ) {
        *read_ret = 0;
        return S_FALSE;
    }

    while(read < size) {
        if(protocol->available_bytes) {
            ULONG len;

            res = InternetReadFile(protocol->request, ((BYTE *)buf)+read,
                    protocol->available_bytes > size-read ? size-read : protocol->available_bytes, &len);
            if(!res) {
                WARN("InternetReadFile failed: %d\n", GetLastError());
                hres = INET_E_DOWNLOAD_FAILURE;
                report_result(protocol, hres);
                break;
            }

            if(!len) {
                all_data_read(protocol);
                break;
            }

            read += len;
            protocol->current_position += len;
            protocol->available_bytes -= len;
        }else {
            /* InternetQueryDataAvailable may immediately fork and perform its asynchronous
             * read, so clear the flag _before_ calling so it does not incorrectly get cleared
             * after the status callback is called */
            protocol->flags &= ~FLAG_REQUEST_COMPLETE;
            res = InternetQueryDataAvailable(protocol->request, &protocol->available_bytes, 0, 0);
            if(!res) {
                if (GetLastError() == ERROR_IO_PENDING) {
                    hres = E_PENDING;
                }else {
                    WARN("InternetQueryDataAvailable failed: %d\n", GetLastError());
                    hres = INET_E_DATA_NOT_AVAILABLE;
                    report_result(protocol, hres);
                }
                break;
            }

            if(!protocol->available_bytes) {
                all_data_read(protocol);
                break;
            }
        }
    }

    *read_ret = read;

    if (hres != E_PENDING)
        protocol->flags |= FLAG_REQUEST_COMPLETE;
    if(FAILED(hres))
        return hres;

    return read ? S_OK : S_FALSE;
}

HRESULT protocol_lock_request(Protocol *protocol)
{
    if (!InternetLockRequestFile(protocol->request, &protocol->lock))
        WARN("InternetLockRequest failed: %d\n", GetLastError());

    return S_OK;
}

HRESULT protocol_unlock_request(Protocol *protocol)
{
    if(!protocol->lock)
        return S_OK;

    if(!InternetUnlockRequestFile(protocol->lock))
        WARN("InternetUnlockRequest failed: %d\n", GetLastError());
    protocol->lock = 0;

    return S_OK;
}

void protocol_close_connection(Protocol *protocol)
{
    protocol->vtbl->close_connection(protocol);

    if(protocol->request)
        InternetCloseHandle(protocol->request);

    if(protocol->connection)
        InternetCloseHandle(protocol->connection);

    if(protocol->internet) {
        InternetCloseHandle(protocol->internet);
        protocol->internet = 0;
    }

    protocol->flags = 0;
}
