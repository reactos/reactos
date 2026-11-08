/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#include "session.hpp"

#include <wincon.h>
#include <winnls.h>
#include <userenv.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(telnetd);

extern CRITICAL_SECTION g_cs_console;

static DWORD s_session_counter = 0;

static BOOL WINAPI ctrl_handler(DWORD type)
{
    return ((type == CTRL_C_EVENT) || (type == CTRL_BREAK_EVENT)) ? TRUE : FALSE;
}

TelnetSession::TelnetSession(ITelnetSessionHandler* handler): 
    m_handler(handler)
{
}

TelnetSession::~TelnetSession()
{
    if (m_reader_stop_event) {
        SetEvent(m_reader_stop_event);
    }
    
    if (m_reader_thread_handle) {
        WaitForSingleObject(m_reader_thread_handle, INFINITE);
        CloseHandle(m_reader_thread_handle);
    }

    if (m_reader_stop_event) {
        CloseHandle(m_reader_stop_event);
    }

    if (m_reader_discard_event) {
        CloseHandle(m_reader_discard_event);
    }

    if (m_reader_overlapped.hEvent) {
        CloseHandle(m_reader_overlapped.hEvent);
    }

    if (m_cmd_process_handle != INVALID_HANDLE_VALUE) {
        TerminateProcess(m_cmd_process_handle, -1);
        WaitForSingleObject(m_cmd_process_handle, INFINITE);
        CloseHandle(m_cmd_process_handle);
    }

    if (m_pipe_stdin != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipe_stdin);
    }

    if (m_pipe_std_out_err != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipe_std_out_err);
    }

    if (m_token) {
        if (m_user_profile_handle != INVALID_HANDLE_VALUE) {
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

    if (!spawn_cmd_process())
        return false;

    if (!start_reader_thread())
        return false;

    return true;
}

void TelnetSession::on_sigint()
{
    m_handler->send_string("^C\r\n");
    m_handler->clear_text_line_buffer();

    EnterCriticalSection(&g_cs_console);

    if (!AttachConsole(GetProcessId(m_cmd_process_handle))) {
        ERR("AttachConsole failed with: %lu\n", GetLastError());
        LeaveCriticalSection(&g_cs_console);
        return;
    }

    if (!SetConsoleCtrlHandler(ctrl_handler, TRUE)) {
        ERR("SetConsoleCtrlHandler failed with: %lu\n", GetLastError());
    }

    if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0)) {
        ERR("GenerateConsoleCtrlEvent(CTRL_C_EVENT) failed with: %lu\n", GetLastError());
    }

    if (!FreeConsole()) {
        ERR("FreeConsole failed with: %lu\n", GetLastError());
    }

    LeaveCriticalSection(&g_cs_console);
}

void TelnetSession::on_ttype(const char* ttype)
{
    // [IATODO]: Need ConPTY

    TRACE("TTYPE %s\n", ttype);
}

void TelnetSession::on_window_size(uint16_t w, uint16_t h)
{
    // [IATODO]: Need ConPTY

    TRACE("NAWS %u,%u\n", w, h);
}

void TelnetSession::on_text_line(const std::string& line)
{
    DWORD bytes_written{};

    const auto t = line + "\r\n";
    if (!WriteFile(m_pipe_stdin, t.c_str(), (DWORD)t.size(), &bytes_written, nullptr)) {
        ERR("WriteFile failed with: %lu\n", GetLastError());
    }
}

void TelnetSession::discard_output()
{
    InterlockedExchange(&m_is_discarding, 1);
    SetEvent(m_reader_discard_event);
}

bool TelnetSession::load_user_data(const std::string& username)
{
    m_username.resize(MultiByteToWideChar(CP_UTF8, 0, username.c_str(), -1, nullptr, 0) - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, username.c_str(), -1, &m_username[0], (int)(m_username.size() + 1));

    PROFILEINFOW profile_info{};
    profile_info.lpUserName = &m_username[0];
    profile_info.dwSize = sizeof(profile_info);
    if (!LoadUserProfileW(m_token, &profile_info)) {
        ERR("LoadUserProfileW failed with: %lu\n", GetLastError());
        return false;
    }
    m_user_profile_handle = profile_info.hProfile;

    {
        DWORD len{};
        GetUserProfileDirectoryW(m_token, nullptr, &len);
        if (!len) {
            ERR("GetUserProfileDirectoryW failed with: %lu\n", GetLastError());
            return false;
        }
        m_userdir.resize(len - 1, L'\0');
        if (!GetUserProfileDirectoryW(m_token, &m_userdir[0], &len)) {
            ERR("GetUserProfileDirectoryW failed with: %lu\n", GetLastError());
            return false;
        }
    }

    return true;
}

bool TelnetSession::spawn_cmd_process()
{
    WCHAR pipe_name[64];
    _snwprintf(pipe_name, sizeof(pipe_name)/sizeof(WCHAR), L"\\\\.\\pipe\\telnetd-%08x-%08x", GetCurrentProcessId(), s_session_counter++);

    m_pipe_std_out_err = CreateNamedPipeW(
        pipe_name, 
        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE, 
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,
        PIPE_BUFFER_SIZE,
        PIPE_BUFFER_SIZE,
        0,
        nullptr
    );
    if (m_pipe_std_out_err == INVALID_HANDLE_VALUE) {
        ERR("CreateNamedPipeW(%ls) failed with: %lu\n", pipe_name, GetLastError());
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
        ERR("CreateFileW(%ls) failed with: %lu\n", pipe_name, GetLastError());
        return false;
    }

    HANDLE child_pipe_stdin{};
    if (!CreatePipe(&child_pipe_stdin, &m_pipe_stdin, &sec_attr, PIPE_BUFFER_SIZE)) {
        const auto ec = GetLastError();
        CloseHandle(child_pipe_std_out_err);
        ERR("CreatePipe failed with: %lu\n", ec);
        return false;
    }
    if (!SetHandleInformation(m_pipe_stdin, HANDLE_FLAG_INHERIT, 0)) {
        const auto ec = GetLastError();
        CloseHandle(child_pipe_stdin);
        CloseHandle(child_pipe_std_out_err);
        ERR("SetHandleInformation failed with: %lu\n", ec);
        return false;
    }

    LPVOID env_block{};
    if (!CreateEnvironmentBlock(&env_block, m_token, false)) {
        const auto ec = GetLastError();
        CloseHandle(child_pipe_stdin);
        CloseHandle(child_pipe_std_out_err);
        ERR("CreateEnvironmentBlock failed with: %lu\n", ec);
        return false;
    }

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(STARTUPINFOW);
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdOutput = child_pipe_std_out_err;
    startup_info.hStdError = child_pipe_std_out_err;
    startup_info.hStdInput = child_pipe_stdin;

    PROCESS_INFORMATION process_info{};

    WCHAR cmd[32] = L"C:\\ReactOS\\system32\\cmd.exe"; // [IATODO]: Probably should use GetSystemDirectoryW later
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
        const auto ec = GetLastError();
        DestroyEnvironmentBlock(env_block);
        CloseHandle(child_pipe_stdin);
        CloseHandle(child_pipe_std_out_err);
        ERR("CreateProcessAsUserW failed with: %lu\n", ec);
        return false;
    }
    
    m_cmd_process_handle = process_info.hProcess;
    DestroyEnvironmentBlock(env_block);
    CloseHandle(process_info.hThread);
    CloseHandle(child_pipe_stdin);
    CloseHandle(child_pipe_std_out_err);

    return true;
}

bool TelnetSession::start_reader_thread()
{
    m_reader_stop_event = CreateEvent(nullptr, false, false, nullptr);
    if (!m_reader_stop_event) {
        ERR("reader stop event create failed with: %lu\n", GetLastError());
        return false;
    }

    m_reader_discard_event = CreateEvent(nullptr, false, false, nullptr);
    if (!m_reader_discard_event) {
        ERR("reader discard event create failed with: %lu\n", GetLastError());
        return false;
    }

    m_reader_overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_reader_overlapped.hEvent) {
        ERR("reader overlapped event create failed with: %lu\n", GetLastError());
        return false;
    }

    m_reader_buffer.resize(PIPE_BUFFER_SIZE);

    DWORD thread_id{};
    m_reader_thread_handle = CreateThread(
        nullptr,
        0,
        reader_thread_func,
        this,
        0,
        &thread_id
    );
    if (!m_reader_thread_handle) {
        ERR("reader thread create failed with: %lu\n", GetLastError());
        return false;
    }

    return true;
}

DWORD TelnetSession::read_loop()
{
    HANDLE wait_objects[3] = {m_reader_stop_event, m_reader_overlapped.hEvent, m_reader_discard_event};

    while (true) {
        DWORD read_size{};

        if (!m_is_read_pending && !ReadFile(m_pipe_std_out_err, m_reader_buffer.data(), (DWORD)m_reader_buffer.size(), &read_size, &m_reader_overlapped)) {
            const auto ec = GetLastError();
            if (ec == ERROR_BROKEN_PIPE) {
                m_handler->close_session();
                return ec;
            } else if (ec == ERROR_IO_PENDING) {
                m_is_read_pending = true;
            } else {
                ERR("ReadFile failed with: %lu\n", ec);
                return ec;
            }
        }

        auto idx = WaitForMultipleObjects(
            sizeof(wait_objects)/sizeof(wait_objects[0]), 
            wait_objects,
            false,
            m_is_discarding ? 200 : INFINITE
        );
        if (idx == WAIT_FAILED) {
            const auto ec = GetLastError();
            ERR("WaitForMultipleObjects failed with: %lu\n", ec);
            return ec;
        } else if ((idx == WAIT_TIMEOUT) && m_is_discarding) {
            InterlockedExchange(&m_is_discarding, 0);
            continue;
        }
        idx -= WAIT_OBJECT_0;

        DWORD bytes_transferred{};
        if (!idx) { // stop event
            CancelIo(m_pipe_std_out_err);
            GetOverlappedResult(m_pipe_std_out_err, &m_reader_overlapped, &bytes_transferred, true);
            m_is_read_pending = false;
            break;
        } else if (idx == 1) {
            if (!GetOverlappedResult(m_pipe_std_out_err, &m_reader_overlapped, &bytes_transferred, true)) {
                const auto ec = GetLastError();
                if (ec == ERROR_BROKEN_PIPE) {
                    m_handler->close_session();
                }
                ERR("GetOverlappedResult failed with: %lu\n", ec);
                return ec;
            }
            m_is_read_pending = false;
            if (!m_is_discarding) {
                m_handler->send_text((const uint8_t*)m_reader_buffer.data(), bytes_transferred);
            }
        } else if (idx == 2) {
            continue;
        }
    }

    return 0;
}
