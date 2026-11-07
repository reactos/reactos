/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#pragma once

#include <windef.h>

typedef int TELNETD_STATUS;

#define TELNETD_STATUS_OK 0
#define TELNETD_STATUS_ERR_SOCK_CREATE 1
#define TELNETD_STATUS_ERR_SOCK_BIND 2
#define TELNETD_STATUS_ERR_SOCK_LISTEN 3
#define TELNETD_STATUS_ERR_SOCK_ACCEPT 4
#define TELNETD_STATUS_ERR_SOCK_READ 5
#define TELNETD_STATUS_ERR_SOCK_WRITE 6
#define TELNETD_STATUS_ERR_THRD 7
#define TELNETD_STATUS_ERR_EVNT 8
#define TELNETD_STATUS_ERR_WINSCK 9
#define TELNETD_STATUS_ERR_UNKNOWN 10

EXTERN_C TELNETD_STATUS kickoff_telnetd(HANDLE* out_worker_thread);
EXTERN_C void shutdown_telnetd(void);

#ifdef __cplusplus

#include "tcpserver.hpp"

class Telnetd
{
    static DWORD WINAPI worker_thread_func(LPVOID self);

public:
    TELNETD_STATUS initialize();
    void terminate();

    HANDLE get_worker_thread() const { return m_worker_thread; } 

private:
    TELNETD_STATUS serve();

private:
    TCPServer m_tcp_server{};
    HANDLE m_worker_thread {NULL};
};

#endif