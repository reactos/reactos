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
    virtual void send_text(const uint8_t* data, int length) = 0;
};

class TelnetSession
{
public:
    TelnetSession(ITelnetSessionHandler* handler, const std::string& username, HANDLE token): m_token(token), m_username(username), m_handler(handler) {}
    ~TelnetSession();

    void on_text_line(const std::string& line);

private:
    HANDLE m_token{};
    std::string m_username{};
    ITelnetSessionHandler* m_handler{};
};