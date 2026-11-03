/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#include "session.hpp"

#include <winbase.h>

TelnetSession::~TelnetSession()
{
    CloseHandle(m_token);
}

void TelnetSession::on_text_line(const std::string& line)
{

}
