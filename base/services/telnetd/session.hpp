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
    static constexpr DWORD PIPE_BUFFER_SIZE = 64 * 1024;
    static constexpr size_t MAX_LINE_TAIL_LENGTH = 1024;
    static constexpr size_t MAX_WRITE_QUEUE_LENGTH = PIPE_BUFFER_SIZE;

public:
    TelnetSession(ITelnetSessionHandler* handler);
    ~TelnetSession();

    bool connect(const std::wstring& username, HANDLE token);

    void on_sigint();

    void on_ttype(const char* ttype);
    void on_window_size(uint16_t w, uint16_t h);

    void on_text_line(const std::wstring& line);

    void discard_output();

private:
    bool load_user_data();
    bool spawn_cmd_process();
    bool start_io_thread();
    
    void start_write();

private:
    DWORD io_loop();

    static inline DWORD WINAPI io_thread_func(LPVOID self);

private:
    HANDLE m_token{};
    std::wstring m_username;
    std::wstring m_userdir;
    HANDLE m_user_profile_handle{INVALID_HANDLE_VALUE};

    HANDLE m_job_handle{};
    HANDLE m_cmd_process_handle{INVALID_HANDLE_VALUE};
    HANDLE m_pipe_stdin{INVALID_HANDLE_VALUE};
    HANDLE m_pipe_std_out_err{INVALID_HANDLE_VALUE};
    bool m_cmd_exited{};

    HANDLE m_io_thread_handle{};
    HANDLE m_io_stop_event{};
    HANDLE m_reader_discard_event{};
    OVERLAPPED m_reader_overlapped{};
    std::vector<CHAR> m_reader_buffer{};

    bool m_is_read_pending{};
    volatile LONG m_is_discarding{};
    std::string m_line_tail{};

    CRITICAL_SECTION m_write_cs{};
    std::string m_write_queue{};
    std::string m_write_buffer{};
    HANDLE m_write_request_event{};
    OVERLAPPED m_writer_overlapped{};
    bool m_is_write_pending{};

    ITelnetSessionHandler* m_handler{};
};

DWORD WINAPI TelnetSession::io_thread_func(LPVOID self)
{
    return ((TelnetSession*)self)->io_loop();
}
