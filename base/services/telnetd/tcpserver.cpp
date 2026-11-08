/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#include "tcpserver.hpp"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(TCPServer);

static constexpr size_t SEND_QUEUE_PREALLOCATED_SIZE = 1024;

TCPServerSessionHandler::TCPServerSessionHandler()
{
    m_send_queue.reserve(SEND_QUEUE_PREALLOCATED_SIZE);
    InitializeCriticalSection(&m_send_queue_cs);
}

TCPServerSessionHandler::~TCPServerSessionHandler()
{
    DeleteCriticalSection(&m_send_queue_cs);
}

void TCPServerSessionHandler::_internal_on_connect(TCPServer* server, SOCKET s, WSAEVENT e)
{
    m_server = server;
    m_socket = s;
    m_event = e;

    on_connect();
}

void TCPServerSessionHandler::disconnect()
{
    m_server->disconnect_session(this);
}

void TCPServerSessionHandler::send_data(const void* data, int length)
{
    if(length <= 0)
        return;

    EnterCriticalSection(&m_send_queue_cs);

    if(m_send_queue_cursor + length > m_send_queue.size())
        m_send_queue.resize(m_send_queue_cursor + length);
    RtlCopyMemory(&m_send_queue[m_send_queue_cursor], data, length);
    m_send_queue_cursor += length;

    _internal_flush_send_queue(false);

    LeaveCriticalSection(&m_send_queue_cs);
}

void TCPServerSessionHandler::clear_pending_send_queue()
{
    EnterCriticalSection(&m_send_queue_cs);

    m_send_queue.clear();
    m_send_queue_cursor = 0;

    LeaveCriticalSection(&m_send_queue_cs);
}

void TCPServerSessionHandler::clear_pending_queue_then_send(const void* data, int length)
{
    EnterCriticalSection(&m_send_queue_cs);
    
    m_send_queue.clear();
    m_send_queue_cursor = 0;

    if (length > 0) {
        if((size_t)length > m_send_queue.size())
            m_send_queue.resize(length);
        RtlCopyMemory(&m_send_queue[0], data, length);
        m_send_queue_cursor = length;

        _internal_flush_send_queue(false);
    }

    LeaveCriticalSection(&m_send_queue_cs);
}

bool TCPServerSessionHandler::_internal_flush_send_queue(bool lock)
{
    if (lock) EnterCriticalSection(&m_send_queue_cs);

    if (m_socket == INVALID_SOCKET) {
        m_send_queue_cursor = 0;
        if (lock) LeaveCriticalSection(&m_send_queue_cs);
        return false;
    }

    const char* buffer = (const char*)m_send_queue.data();

    DWORD sent{0};
    while(m_send_queue_cursor > 0)
    {
        const auto result = send(m_socket, buffer, (int)m_send_queue_cursor, 0);

        if(result == SOCKET_ERROR) {
            m_send_queue.erase(m_send_queue.begin(), m_send_queue.begin() + sent);
            const auto error = WSAGetLastError();
            if(error == WSAEWOULDBLOCK) {
                if (lock) LeaveCriticalSection(&m_send_queue_cs);
                return true;
            }
            ERR("socket send failed with: %d\n", error);
            if (lock) LeaveCriticalSection(&m_send_queue_cs);
            return false;
        }

        buffer += result;
        sent += result;
        m_send_queue_cursor -= result;
    }

    if (lock) LeaveCriticalSection(&m_send_queue_cs);

    return true;
}

TCPServer::EStatus TCPServer::initialize(uint16_t port, const char* interface_addr, HandlerFactoryFuncT handler_factory)
{
    m_handler_factory = handler_factory;

    WSADATA _wsa_data{};
    int result = WSAStartup(MAKEWORD(2,2), &_wsa_data);
    if(result != 0) {
        ERR("winsock init failed with: %d\n", result);
        terminate();
        return EStatus::WSA_INIT_ERROR;
    }
    m_wsa_initialized = true;

    const auto stop_event = WSACreateEvent();
    if(stop_event == WSA_INVALID_EVENT) {
        ERR("stop event create failed with: %d\n", WSAGetLastError());
        terminate();
        return EStatus::EVENT_CREATE_ERROR;
    }
    m_session_data.events[0] = stop_event;
    m_session_data.current_index = 1;

    const auto listen_event = WSACreateEvent();
    if(listen_event == WSA_INVALID_EVENT) {
        ERR("listen event create failed with: %d\n", WSAGetLastError());
        terminate();
        return EStatus::EVENT_CREATE_ERROR;
    }
    const auto listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listen_socket == INVALID_SOCKET) {
        ERR("listen socket create failed with: %d\n", WSAGetLastError());
        WSACloseEvent(listen_event);
        terminate();
        return EStatus::SOCKET_CREATE_ERROR;
    }
    SetHandleInformation((HANDLE)listen_socket, HANDLE_FLAG_INHERIT, 0);
    m_session_data.events[1] = listen_event;
    m_session_data.sockets[1] = listen_socket;
    m_session_data.current_index = 2;

    SOCKADDR_IN sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(interface_addr);
    if(sin.sin_addr.s_addr == INADDR_NONE) {
        ERR("invalid interface address: %s\n", interface_addr);
        terminate();
        return EStatus::INTERFACE_ADDR_PARSE_ERROR;
    }

    if(bind(listen_socket, (PSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR) {
        ERR("listen socket bind failed with: %d\n", WSAGetLastError());
        terminate();
        return EStatus::SOCKET_BIND_ERROR;
    }

    if(listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        ERR("listen socket listen failed with: %d\n", WSAGetLastError());
        terminate();
        return EStatus::SOCKET_LISTEN_ERROR;
    }

    if(WSAEventSelect(listen_socket, listen_event, FD_ACCEPT) == SOCKET_ERROR) {
        ERR("event select failed with: %d\n", WSAGetLastError());
        terminate();
        return EStatus::EVENT_SELECT_ERROR;
    }

    return EStatus::OK;
}

void TCPServer::terminate()
{
    while (m_session_data.current_index > 2)
        remove_session(m_session_data.current_index - 1, false);

    for(DWORD i = 0; i < m_session_data.current_index; i++)
    {
        if(m_session_data.sockets[i] != INVALID_SOCKET)
            closesocket(m_session_data.sockets[i]);
        
        if(m_session_data.events[i] != WSA_INVALID_EVENT)
            WSACloseEvent(m_session_data.events[i]);

        if(m_session_data.handlers[i])
        {
            m_session_data.handlers[i]->on_disconnect();
            delete m_session_data.handlers[i];
        }
    }
    m_session_data = {};

    if(m_wsa_initialized) 
    {
        WSACleanup();
        m_wsa_initialized = false;
    }
}

BOOL TCPServer::request_stop()
{
    return WSASetEvent(m_session_data.events[0]);
}

void TCPServer::disconnect_session(TCPServerSessionHandler* handler)
{
    if (!handler)
        return;

    if (!InterlockedExchange(&handler->m_disconnect_requested, 1))
        WSASetEvent(handler->m_event);
}

TCPServer::EStatus TCPServer::run()
{
    CHAR read_buffer[1024];

    while(true)
    {
        auto idx = WSAWaitForMultipleEvents(m_session_data.current_index, m_session_data.events, FALSE, WSA_INFINITE, FALSE);
        if(idx == WSA_WAIT_FAILED) {
            ERR("wsa multi wait failed with: %d\n", WSAGetLastError());
            return EStatus::EVENT_WAIT_ERROR;
        }
        idx -= WSA_WAIT_EVENT_0;

        // This is stop event
        if(idx == 0) break;
        
        for(DWORD i = idx; i < m_session_data.current_index; i++)
        {
            const auto wait_result = WSAWaitForMultipleEvents(1, &m_session_data.events[i], TRUE, 0, FALSE);
            if (
                (wait_result == WSA_WAIT_FAILED) || 
                (wait_result == WSA_WAIT_TIMEOUT)
            )
                continue;

            WSANETWORKEVENTS net_events{};
            WSAEnumNetworkEvents(m_session_data.sockets[i], m_session_data.events[i], &net_events);

            if (i == 1) {
                if (net_events.lNetworkEvents & FD_ACCEPT)
                {
                    if (net_events.iErrorCode[FD_ACCEPT_BIT] != 0) {
                        ERR("network accept event failed with: %d\n", net_events.iErrorCode[FD_ACCEPT_BIT]);
                        continue;
                    }

                    const auto client_socket = accept(m_session_data.sockets[i], NULL, NULL);
                    if(client_socket == INVALID_SOCKET) {
                        ERR("client socket accept failed with: %d\n", WSAGetLastError());
                        continue;
                    }
                    SetHandleInformation((HANDLE)client_socket, HANDLE_FLAG_INHERIT, 0);

                    if(m_session_data.current_index >= SessionData::NUM_MAX_ENTRY_COUNT) {
                        ERR("cannot accept anymore connections.. dropping the latest :((\n");
                        closesocket(client_socket);
                        continue;
                    }

                    const auto client_event = WSACreateEvent();
                    if(client_event == WSA_INVALID_EVENT) {
                        ERR("client event create with: %d\n", WSAGetLastError());
                        closesocket(client_socket);
                        continue;
                    }

                    if(WSAEventSelect(client_socket, client_event, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR) {
                        ERR("client event select with: %d\n", WSAGetLastError());
                        closesocket(client_socket);
                        WSACloseEvent(client_event);
                        continue;
                    }

                    const auto handler = m_handler_factory();

                    handler->_internal_on_connect(this, client_socket, client_event);

                    m_session_data.events[m_session_data.current_index] = client_event;
                    m_session_data.sockets[m_session_data.current_index] = client_socket;
                    m_session_data.handlers[m_session_data.current_index] = handler;
                    m_session_data.current_index++;
                }

                continue;
            }

            if (net_events.lNetworkEvents & FD_READ)
            {
                if (net_events.iErrorCode[FD_READ_BIT] != 0) {
                    ERR("network read event failed with: %d\n", net_events.iErrorCode[FD_READ_BIT]);
                } else {
                    const auto read_count = recv(m_session_data.sockets[i], read_buffer, 
                            sizeof(read_buffer) - 1, 0);
                    if(read_count > 0) {
                        read_buffer[read_count] = '\0';
                        m_session_data.handlers[i]->on_data((uint8_t*)read_buffer, read_count);
                    }
                }
            }

            if (net_events.lNetworkEvents & FD_WRITE)
            {
                if (net_events.iErrorCode[FD_WRITE_BIT] != 0) {
                    ERR("network write event failed with: %d\n", net_events.iErrorCode[FD_WRITE_BIT]);
                } else {
                    m_session_data.handlers[i]->_internal_flush_send_queue();
                }
            }

            const auto closed_by_peer = (net_events.lNetworkEvents & FD_CLOSE) != 0;       

            if (closed_by_peer || m_session_data.handlers[i]->m_disconnect_requested) {

                if (closed_by_peer) {
                    if (net_events.iErrorCode[FD_CLOSE_BIT] != 0) {
                        ERR("network close event failed with: %d\n", net_events.iErrorCode[FD_CLOSE_BIT]);
                    }

                    while (true) {
                        const auto read_count = recv(m_session_data.sockets[i], read_buffer, 
                                sizeof(read_buffer) - 1, 0);
                        if(read_count <= 0) break;
                        read_buffer[read_count] = '\0';
                        m_session_data.handlers[i]->on_data((uint8_t*)read_buffer, read_count);
                    }
                }

                remove_session(i, closed_by_peer);
                i--;

                continue;
            }
        }
    }

    return EStatus::OK;
}

void TCPServer::remove_session(DWORD i, bool closed_by_peer)
{
    const auto handler = m_session_data.handlers[i];

    EnterCriticalSection(&handler->m_send_queue_cs);

    if (!closed_by_peer)
        handler->_internal_flush_send_queue(false);

    handler->m_socket = INVALID_SOCKET;

    LeaveCriticalSection(&handler->m_send_queue_cs);

    handler->on_disconnect();

    closesocket(m_session_data.sockets[i]);
    WSACloseEvent(m_session_data.events[i]);
    
    delete handler;

    for(DWORD j = i; j < m_session_data.current_index - 1; j++) {
        m_session_data.sockets[j] = m_session_data.sockets[j + 1];
        m_session_data.events[j] = m_session_data.events[j + 1];
        m_session_data.handlers[j] = m_session_data.handlers[j + 1];
    }
    m_session_data.sockets[m_session_data.current_index - 1] = INVALID_SOCKET;
    m_session_data.events[m_session_data.current_index - 1] = WSA_INVALID_EVENT;
    m_session_data.handlers[m_session_data.current_index - 1] = nullptr;

    m_session_data.current_index--;
}
