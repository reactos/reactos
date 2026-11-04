/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#pragma once

#include <windef.h>

#include <string>

class ITelnetSessionHandler
{
public:
    virtual void send_string(const char* str) = 0;
    virtual void send_text(const uint8_t* data, int length) = 0;
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

    void on_break();
    void on_sigint();
    void on_interrupt_process();

    void on_text_line(const std::string& line);

    void discard_output();

private:
    bool load_user_data(const std::string& username);

private:
    HANDLE m_token{};
    ITelnetSessionHandler* m_handler{};

    std::wstring m_username;
    std::wstring m_userdir;
    HANDLE m_user_profile_handle{};
};