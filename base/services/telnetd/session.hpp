/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#pragma once

#include <windef.h>
#include <winbase.h>

#include <string>
#include <vector>
#include <stdint.h>

class ITelnetSessionHandler
{
public:
    virtual void close_session() = 0;

    virtual void send_string(const char* str) = 0;
    virtual void send_text(const uint8_t* data, int length) = 0;

    virtual void clear_text_line_buffer() = 0;
};

class TelnetSession
{
public:
    static constexpr DWORD PIPE_BUFFER_SIZE = 64 * 1024;
    static constexpr size_t MAX_CREDENTIAL_LENGTH = 256;

public:
    TelnetSession(ITelnetSessionHandler* handler);
    ~TelnetSession();

    bool connect(const std::string& username, HANDLE token);

    void on_sigint();

    void on_ttype(const char* ttype);
    void on_window_size(uint16_t w, uint16_t h);

    void on_text_line(const std::string& line);

    void discard_output();

private:
    bool load_user_data(const std::string& username);
    bool spawn_cmd_process();
    bool start_reader_thread();

private:
    DWORD read_loop();

    static inline DWORD WINAPI reader_thread_func(LPVOID self);

private:
    HANDLE m_token{};
    std::wstring m_username;
    std::wstring m_userdir;
    HANDLE m_user_profile_handle{INVALID_HANDLE_VALUE};

    HANDLE m_cmd_process_handle{INVALID_HANDLE_VALUE};
    HANDLE m_pipe_stdin{INVALID_HANDLE_VALUE};
    HANDLE m_pipe_std_out_err{INVALID_HANDLE_VALUE};

    HANDLE m_reader_thread_handle{};
    HANDLE m_reader_stop_event{};
    HANDLE m_reader_discard_event{};
    OVERLAPPED m_reader_overlapped{};
    std::vector<CHAR> m_reader_buffer{};

    bool m_is_read_pending{};
    volatile LONG m_is_discarding{};

    ITelnetSessionHandler* m_handler{};
};

DWORD WINAPI TelnetSession::reader_thread_func(LPVOID self)
{
    return ((TelnetSession*)self)->read_loop();
}
