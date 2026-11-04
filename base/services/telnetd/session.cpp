/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#include "session.hpp"

#include <winbase.h>
#include <winnls.h>
#include <userenv.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(telnetd);

static DWORD __internal_telnet_session_counter = 0;

TelnetSession::TelnetSession(ITelnetSessionHandler* handler): 
    m_handler(handler)
{
}

TelnetSession::~TelnetSession()
{
    if (m_token) {
        if (m_user_profile_handle) {
            UnloadUserProfile(m_token, m_user_profile_handle);
        }

        CloseHandle(m_token);
    }
}

bool TelnetSession::connect(const std::string& username, HANDLE token)
{
    m_token = token;
    
    if (!load_user_data(username))
        return false;

    WCHAR pipe_name[36];
    _snwprintf(pipe_name, sizeof(pipe_name)/sizeof(WCHAR), L"\\\\.\\pipe\\telnetd-%08x-%08x", GetCurrentProcessId(), __internal_telnet_session_counter++);

    const auto parent_pipe_std_out_err = CreateNamedPipeW(
        pipe_name, 
        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE, 
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,
        PIPE_BUFFER_SIZE,
        PIPE_BUFFER_SIZE,
        0,
        nullptr
    );
    if (parent_pipe_std_out_err == INVALID_HANDLE_VALUE) {
        ERR("CreateNamedPipeW(%ls) failed: %lu\n", pipe_name, GetLastError());
        return false;
    }

    SECURITY_ATTRIBUTES sec_attr{};
    sec_attr.bInheritHandle = true;
    sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    const auto child_pipe_std_out_err =CreateFileW(
        pipe_name,
        GENERIC_WRITE,
        0,
        &sec_attr,
        OPEN_EXISTING,
        0,
        nullptr
    );
    if (child_pipe_std_out_err == INVALID_HANDLE_VALUE) {
        ERR("CreateFileW(%ls) failed: %lu\n", pipe_name, GetLastError());
        return false;
    }

    HANDLE parent_pipe_stdin{}, child_pipe_stdin{};
    if (!CreatePipe(&child_pipe_stdin, &parent_pipe_stdin, &sec_attr, PIPE_BUFFER_SIZE)) {
        ERR("CreatePipe failed: %lu\n", GetLastError());
        return false;
    }

    LPVOID env_block{};
    if (!CreateEnvironmentBlock(&env_block, m_token, false)) {
        ERR("CreateEnvironmentBlock failed: %lu\n", GetLastError());
        return false;
    }

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(STARTUPINFOW);
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdOutput = child_pipe_std_out_err;
    startup_info.hStdError = child_pipe_std_out_err;
    startup_info.hStdInput = child_pipe_stdin;

    PROCESS_INFORMATION process_info{};
    WCHAR cmd[256] = L"C:\\ReactOS\\system32\\cmd.exe";
    if (!CreateProcessAsUserW(
        m_token, 
        nullptr, 
        cmd, 
        nullptr, nullptr, 
        true, 
        CREATE_UNICODE_ENVIRONMENT, 
        env_block, 
        m_userdir.c_str(), 
        &startup_info, 
        &process_info
    )) {
        ERR("CreateProcessAsUserW failed: %lu\n", GetLastError());
        return false;
    }

    m_handler->send_string("Created Process\r\n");

    CHAR data[256];
    DWORD size_read{};
    ReadFile(parent_pipe_std_out_err, data, sizeof(data), &size_read, nullptr);

    m_handler->send_text((const uint8_t*)data, size_read);

    return true;
}

void TelnetSession::on_break()
{

}

void TelnetSession::on_sigint()
{

}

void TelnetSession::on_interrupt_process()
{

}

void TelnetSession::on_text_line(const std::string& line)
{

}

void TelnetSession::discard_output()
{

}

bool TelnetSession::load_user_data(const std::string& username)
{
    m_username.resize(MultiByteToWideChar(CP_UTF8, 0, username.c_str(), -1, nullptr, 0) - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, username.c_str(), -1, &m_username[0], m_username.size() + 1);

    PROFILEINFOW profile_info{};
    profile_info.lpUserName = &m_username[0];
    profile_info.dwSize = sizeof(profile_info);
    if (!LoadUserProfileW(m_token, &profile_info)) {
        ERR("LoadUserProfileW failed: %lu\n", GetLastError());
        return false;
    }
    m_user_profile_handle = profile_info.hProfile;

    {
        DWORD len{};
        GetUserProfileDirectoryW(m_token, nullptr, &len);
        m_userdir.resize(len - 1, L'\0');
        if (!GetUserProfileDirectoryW(m_token, &m_userdir[0], &len)) {
            ERR("GetUserProfileDirectoryW failed: %lu\n", GetLastError());
            return false;
        }
    }

    return true;
}