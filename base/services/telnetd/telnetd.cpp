/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#include "telnetd.hpp"
#include "session_handler.hpp"

#include <winbase.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(telnetd);

static constexpr uint16_t TELNET_PORT = 23;

static Telnetd g_telnetd{};

extern "C" TELNETD_STATUS kickoff_telnetd(HANDLE* out_worker_thread)
{
    TELNETD_STATUS status = g_telnetd.initialize();
    if(status != TELNETD_STATUS_OK)
        return status;
    *out_worker_thread = g_telnetd.get_worker_thread();
    return status;
}

extern "C" void shutdown_telnetd(void)
{
    g_telnetd.terminate();
}

static TELNETD_STATUS tcp_server_status_to_telnetd(TCPServer::EStatus status)
{
    switch(status)
    {
        case TCPServer::EStatus::OK: return TELNETD_STATUS_OK;
     
        case TCPServer::EStatus::WSA_INIT_ERROR: return TELNETD_STATUS_ERR_WINSCK;
        case TCPServer::EStatus::EVENT_WAIT_ERROR: return TELNETD_STATUS_ERR_EVNT;
        case TCPServer::EStatus::EVENT_CREATE_ERROR: return TELNETD_STATUS_ERR_EVNT;
        case TCPServer::EStatus::EVENT_SELECT_ERROR: return TELNETD_STATUS_ERR_EVNT;
        case TCPServer::EStatus::SOCKET_CREATE_ERROR: return TELNETD_STATUS_ERR_SOCK_CREATE;
        case TCPServer::EStatus::SOCKET_BIND_ERROR: return TELNETD_STATUS_ERR_SOCK_BIND;
        case TCPServer::EStatus::SOCKET_LISTEN_ERROR: return TELNETD_STATUS_ERR_SOCK_LISTEN;
        case TCPServer::EStatus::SOCKET_ACCEPT_ERROR: return TELNETD_STATUS_ERR_SOCK_ACCEPT;
        case TCPServer::EStatus::SOCKET_READ_ERROR: return TELNETD_STATUS_ERR_SOCK_READ;
        case TCPServer::EStatus::SOCKET_WRITE_ERROR: return TELNETD_STATUS_ERR_SOCK_WRITE;

        default:
            return TELNETD_STATUS_ERR_UNKNOWN;
    }
}

TELNETD_STATUS Telnetd::initialize()
{
    const auto server_init_result = m_tcp_server.initialize<TelnetSessionHandler>(TELNET_PORT);
    if(server_init_result != TCPServer::EStatus::OK)
        return tcp_server_status_to_telnetd(server_init_result);

    DWORD thread_id{};
    m_worker_thread = CreateThread(
        nullptr,
        0,
        worker_thread_func,
        this,
        0,
        &thread_id
    );
    if(!m_worker_thread) {
        ERR("worker thread create failed with: %d\n", GetLastError());
        terminate();
        return TELNETD_STATUS_ERR_THRD;
    }

    return TELNETD_STATUS_OK;
}

void Telnetd::terminate()
{
    m_tcp_server.request_stop();
    if(m_worker_thread) WaitForSingleObject(m_worker_thread, INFINITE);
    m_tcp_server.terminate();

    if(m_worker_thread) CloseHandle(m_worker_thread);
    m_worker_thread = NULL;
}

TELNETD_STATUS Telnetd::serve()
{
    return tcp_server_status_to_telnetd(m_tcp_server.run());
}

DWORD WINAPI Telnetd::worker_thread_func(LPVOID self)
{
    return ((Telnetd*)self)->serve();
}
