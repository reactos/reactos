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

static HRESULT start_downloading(Protocol *protocol)
{
    HRESULT hres;

    hres = protocol->vtbl->start_downloading(protocol);
    if(FAILED(hres)) {
        if(hres == INET_E_REDIRECT_FAILED)
            return S_OK;
        protocol_close_connection(protocol);
        report_result(protocol, hres);
        return hres;
    }

    if(protocol->bindf & BINDF_NEEDFILE) {
        WCHAR cache_file[MAX_PATH];
        DWORD buflen = sizeof(cache_file);

        if(InternetQueryOptionW(protocol->request, INTERNET_OPTION_DATAFILE_NAME, cache_file, &buflen)) {
            report_progress(protocol, BINDSTATUS_CACHEFILENAMEAVAILABLE, cache_file);
        }else {
            FIXME("Could not get cache file\n");
        }
    }

    protocol->flags |= FLAG_FIRST_CONTINUE_COMPLETE;
    return S_OK;
}

HRESULT protocol_syncbinding(Protocol *protocol)
{
    BOOL res;
    HRESULT hres;

    protocol->flags |= FLAG_SYNC_READ;

    hres = start_downloading(protocol);
    if(FAILED(hres))
        return hres;

    res = InternetQueryDataAvailable(protocol->request, &protocol->query_available, 0, 0);
    if(res)
        protocol->available_bytes = protocol->query_available;
    else
        WARN("InternetQueryDataAvailable failed: %lu\n", GetLastError());

    protocol->flags |= FLAG_FIRST_DATA_REPORTED|FLAG_LAST_DATA_REPORTED;
    IInternetProtocolSink_ReportData(protocol->protocol_sink, BSCF_LASTDATANOTIFICATION|BSCF_DATAFULLYAVAILABLE,
            protocol->available_bytes, protocol->content_length);
    return S_OK;
}

static void request_complete(Protocol *protocol, INTERNET_ASYNC_RESULT *ar)
{
    PROTOCOLDATA data;

    TRACE("(%p)->(%p)\n", protocol, ar);

    /* PROTOCOLDATA same as native */
    memset(&data, 0, sizeof(data));
    data.dwState = 0xf1000000;

    if(ar->dwResult) {
        protocol->flags |= FLAG_REQUEST_COMPLETE;

        if(!protocol->request) {
            TRACE("setting request handle %p\n", (HINTERNET)ar->dwResult);
            protocol->request = (HINTERNET)ar->dwResult;
        }

        if(protocol->flags & FLAG_FIRST_CONTINUE_COMPLETE)
            data.pData = UlongToPtr(BINDSTATUS_ENDDOWNLOADCOMPONENTS);
        else
            data.pData = UlongToPtr(BINDSTATUS_DOWNLOADINGDATA);

    }else {
        protocol->flags |= FLAG_ERROR;
        data.pData = UlongToPtr(ar->dwError);
    }

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

    case INTERNET_STATUS_CONNECTING_TO_SERVER: {
        WCHAR *info;

        TRACE("%p INTERNET_STATUS_CONNECTING_TO_SERVER %s\n", protocol, (const char*)status_info);

        info = strdupAtoW(status_info);
        if(!info)
            return;

        report_progress(protocol, BINDSTATUS_CONNECTING, info);
        free(info);
        break;
    }

    case INTERNET_STATUS_SENDING_REQUEST:
        TRACE("%p INTERNET_STATUS_SENDING_REQUEST\n", protocol);
        report_progress(protocol, BINDSTATUS_SENDINGREQUEST, (LPWSTR)status_info);
        break;

    case INTERNET_STATUS_REDIRECT:
        TRACE("%p INTERNET_STATUS_REDIRECT\n", protocol);
        report_progress(protocol, BINDSTATUS_REDIRECTING, (LPWSTR)status_info);
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
        WARN("Unhandled Internet status callback %ld\n", internet_status);
    }
}

static HRESULT write_post_stream(Protocol *protocol)
{
    BYTE buf[0x20000];
    DWORD written;
    ULONG size;
    BOOL res;
    HRESULT hres;

    protocol->flags &= ~FLAG_REQUEST_COMPLETE;

    while(1) {
        size = 0;
        hres = IStream_Read(protocol->post_stream, buf, sizeof(buf), &size);
        if(FAILED(hres) || !size)
            break;
        res = InternetWriteFile(protocol->request, buf, size, &written);
        if(!res) {
            FIXME("InternetWriteFile failed: %lu\n", GetLastError());
            hres = E_FAIL;
            break;
        }
    }

    if(SUCCEEDED(hres)) {
        IStream_Release(protocol->post_stream);
        protocol->post_stream = NULL;

        hres = protocol->vtbl->end_request(protocol);
    }

    if(FAILED(hres))
        return report_result(protocol, hres);

    return S_OK;
}

static HINTERNET create_internet_session(IInternetBindInfo *bind_info)
{
    LPWSTR global_user_agent = NULL;
    LPOLESTR user_agent = NULL;
    ULONG size = 0;
    HINTERNET ret;
    HRESULT hres;

    hres = IInternetBindInfo_GetBindString(bind_info, BINDSTRING_USER_AGENT, &user_agent, 1, &size);
    if(hres != S_OK || !size)
        global_user_agent = get_useragent();

    ret = InternetOpenW(user_agent ? user_agent : global_user_agent, 0, NULL, NULL, INTERNET_FLAG_ASYNC);
    free(global_user_agent);
    CoTaskMemFree(user_agent);
    if(!ret) {
        WARN("InternetOpen failed: %ld\n", GetLastError());
        return NULL;
    }

    InternetSetStatusCallbackW(ret, internet_status_callback);
    return ret;
}

static HINTERNET internet_session;

HINTERNET get_internet_session(IInternetBindInfo *bind_info)
{
    HINTERNET new_session;

    if(internet_session)
        return internet_session;

    if(!bind_info)
        return NULL;

    new_session = create_internet_session(bind_info);
    if(new_session && InterlockedCompareExchangePointer((void**)&internet_session, new_session, NULL))
        InternetCloseHandle(new_session);

    return internet_session;
}

void update_user_agent(WCHAR *user_agent)
{
    if(internet_session)
        InternetSetOptionW(internet_session, INTERNET_OPTION_USER_AGENT, user_agent, lstrlenW(user_agent));
}

HRESULT protocol_start(Protocol *protocol, IInternetProtocol *prot, IUri *uri,
        IInternetProtocolSink *protocol_sink, IInternetBindInfo *bind_info)
{
    DWORD request_flags;
    HRESULT hres;

    protocol->protocol = prot;

    IInternetProtocolSink_AddRef(protocol_sink);
    protocol->protocol_sink = protocol_sink;

    memset(&protocol->bind_info, 0, sizeof(protocol->bind_info));
    protocol->bind_info.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(bind_info, &protocol->bindf, &protocol->bind_info);
    if(hres != S_OK) {
        WARN("GetBindInfo failed: %08lx\n", hres);
        return report_result(protocol, hres);
    }

    if(!(protocol->bindf & BINDF_FROMURLMON))
        report_progress(protocol, BINDSTATUS_DIRECTBIND, NULL);

    if(!get_internet_session(bind_info))
        return report_result(protocol, INET_E_NO_SESSION);

    request_flags = INTERNET_FLAG_KEEP_CONNECTION;
    if(protocol->bindf & BINDF_NOWRITECACHE)
        request_flags |= INTERNET_FLAG_NO_CACHE_WRITE;
    if(protocol->bindf & BINDF_NEEDFILE)
        request_flags |= INTERNET_FLAG_NEED_FILE;
    if(protocol->bind_info.dwOptions & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS)
        request_flags |= INTERNET_FLAG_NO_AUTO_REDIRECT;

    hres = protocol->vtbl->open_request(protocol, uri, request_flags, internet_session, bind_info);
    if(FAILED(hres)) {
        protocol_close_connection(protocol);
        return report_result(protocol, hres);
    }

    return S_OK;
}

HRESULT protocol_continue(Protocol *protocol, PROTOCOLDATA *data)
{
    BOOL is_start;
    HRESULT hres;

    is_start = !data || data->pData == UlongToPtr(BINDSTATUS_DOWNLOADINGDATA);

    if(!protocol->request) {
        WARN("Expected request to be non-NULL\n");
        return S_OK;
    }

    if(!protocol->protocol_sink) {
        WARN("Expected IInternetProtocolSink pointer to be non-NULL\n");
        return S_OK;
    }

    if(protocol->flags & FLAG_ERROR) {
        protocol->flags &= ~FLAG_ERROR;
        protocol->vtbl->on_error(protocol, PtrToUlong(data->pData));
        return S_OK;
    }

    if(protocol->post_stream)
        return write_post_stream(protocol);

    if(is_start) {
        hres = start_downloading(protocol);
        if(FAILED(hres))
            return S_OK;
    }

    if(!data || data->pData >= UlongToPtr(BINDSTATUS_DOWNLOADINGDATA)) {
        if(!protocol->available_bytes) {
            if(protocol->query_available) {
                protocol->available_bytes = protocol->query_available;
            }else {
                BOOL res;

                /* InternetQueryDataAvailable may immediately fork and perform its asynchronous
                 * read, so clear the flag _before_ calling so it does not incorrectly get cleared
                 * after the status callback is called */
                protocol->flags &= ~FLAG_REQUEST_COMPLETE;
                res = InternetQueryDataAvailable(protocol->request, &protocol->query_available, 0, 0);
                if(res) {
                    TRACE("available %lu bytes\n", protocol->query_available);
                    if(!protocol->query_available) {
                        all_data_read(protocol);
                        return S_OK;
                    }
                    protocol->available_bytes = protocol->query_available;
                }else if(GetLastError() != ERROR_IO_PENDING) {
                    protocol->flags |= FLAG_REQUEST_COMPLETE;
                    WARN("InternetQueryDataAvailable failed: %ld\n", GetLastError());
                    report_result(protocol, INET_E_DATA_NOT_AVAILABLE);
                    return S_OK;
                }
            }

            protocol->flags |= FLAG_REQUEST_COMPLETE;
        }

        report_data(protocol);
    }

    return S_OK;
}

HRESULT protocol_read(Protocol *protocol, void *buf, ULONG size, ULONG *read_ret)
{
    ULONG read = 0;
    BOOL res;
    HRESULT hres = S_FALSE;

    if(protocol->flags & FLAG_ALL_DATA_READ) {
        *read_ret = 0;
        return S_FALSE;
    }

    if(!(protocol->flags & FLAG_SYNC_READ) && (!(protocol->flags & FLAG_REQUEST_COMPLETE) || !protocol->available_bytes)) {
        *read_ret = 0;
        return E_PENDING;
    }

    while(read < size && protocol->available_bytes) {
        ULONG len;

        res = InternetReadFile(protocol->request, ((BYTE *)buf)+read,
                protocol->available_bytes > size-read ? size-read : protocol->available_bytes, &len);
        if(!res) {
            WARN("InternetReadFile failed: %ld\n", GetLastError());
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

        TRACE("current_position %ld, available_bytes %ld\n", protocol->current_position, protocol->available_bytes);

        if(!protocol->available_bytes) {
            /* InternetQueryDataAvailable may immediately fork and perform its asynchronous
             * read, so clear the flag _before_ calling so it does not incorrectly get cleared
             * after the status callback is called */
            protocol->flags &= ~FLAG_REQUEST_COMPLETE;
            res = InternetQueryDataAvailable(protocol->request, &protocol->query_available, 0, 0);
            if(!res) {
                if (GetLastError() == ERROR_IO_PENDING) {
                    hres = E_PENDING;
                }else {
                    WARN("InternetQueryDataAvailable failed: %ld\n", GetLastError());
                    hres = INET_E_DATA_NOT_AVAILABLE;
                    report_result(protocol, hres);
                }
                break;
            }

            if(!protocol->query_available) {
                all_data_read(protocol);
                break;
            }

            protocol->available_bytes = protocol->query_available;
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
        WARN("InternetLockRequestFile failed: %ld\n", GetLastError());

    return S_OK;
}

HRESULT protocol_unlock_request(Protocol *protocol)
{
    if(!protocol->lock)
        return S_OK;

    if(!InternetUnlockRequestFile(protocol->lock))
        WARN("InternetUnlockRequestFile failed: %ld\n", GetLastError());
    protocol->lock = 0;

    return S_OK;
}

HRESULT protocol_abort(Protocol *protocol, HRESULT reason)
{
    if(!protocol->protocol_sink)
        return S_OK;

    /* NOTE: IE10 returns S_OK here */
    if(protocol->flags & FLAG_RESULT_REPORTED)
        return INET_E_RESULT_DISPATCHED;

    report_result(protocol, reason);
    return S_OK;
}

void protocol_close_connection(Protocol *protocol)
{
    protocol->vtbl->close_connection(protocol);

    if(protocol->request)
        InternetCloseHandle(protocol->request);

    if(protocol->connection)
        InternetCloseHandle(protocol->connection);

    if(protocol->post_stream) {
        IStream_Release(protocol->post_stream);
        protocol->post_stream = NULL;
    }

    protocol->flags = 0;
}
