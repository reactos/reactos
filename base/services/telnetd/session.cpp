/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#include "session.hpp"
#include "string_utils.hpp"

#include <wincon.h>
#include <winnls.h>
#include <userenv.h>

#include <algorithm>

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
    InitializeCriticalSection(&m_write_cs); 
}

TelnetSession::~TelnetSession()
{
    if (m_io_stop_event) {
        SetEvent(m_io_stop_event);
    }
    
    if (m_io_thread_handle) {
        WaitForSingleObject(m_io_thread_handle, INFINITE);
        CloseHandle(m_io_thread_handle);
    }

    DeleteCriticalSection(&m_write_cs);

    RtlSecureZeroMemory(&m_write_queue[0], m_write_queue.size());
    m_write_queue.clear();

    RtlSecureZeroMemory(&m_write_buffer[0], m_write_buffer.size());
    m_write_buffer.clear();

    if (m_write_request_event) {
        CloseHandle(m_write_request_event);
    }

    if (m_writer_overlapped.hEvent) {
        CloseHandle(m_writer_overlapped.hEvent);
    }

    if (m_io_stop_event) {
        CloseHandle(m_io_stop_event);
    }

    if (m_reader_discard_event) {
        CloseHandle(m_reader_discard_event);
    }

    if (m_reader_overlapped.hEvent) {
        CloseHandle(m_reader_overlapped.hEvent);
    }

    if (m_cmd_process_handle != INVALID_HANDLE_VALUE) {
        if (m_job_handle) {
            TerminateJobObject(m_job_handle, (DWORD)-1);
        }
        WaitForSingleObject(m_cmd_process_handle, INFINITE);
        CloseHandle(m_cmd_process_handle);
    }

    if (m_job_handle) { 
        CloseHandle(m_job_handle);
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

bool TelnetSession::connect(const std::wstring& username, HANDLE token)
{
    m_token = token;
    m_username = username;
    
    if (!load_user_data())
        return false;

    if (!spawn_cmd_process())
        return false;

    if (!start_io_thread())
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

void TelnetSession::on_text_line(const std::wstring& line)
{
    std::string mbline{};
    if (!line.empty()) {
        const auto len = WideCharToMultiByte(CP_OEMCP, WC_NO_BEST_FIT_CHARS, line.c_str(), (int)line.size(), nullptr, 0, NULL, NULL);
        if (!len) {
            ERR("WideCharToMultiByte failed with: %lu\n", GetLastError());
            return;   
        }    
        mbline.resize(len);
        if (!WideCharToMultiByte(CP_OEMCP, WC_NO_BEST_FIT_CHARS, line.c_str(), (int)line.size(), &mbline[0], len, NULL, NULL)) {
            ERR("WideCharToMultiByte failed with: %lu\n", GetLastError());
            return;
        }
    }
    mbline += "\r\n";

    bool queued = false;
    EnterCriticalSection(&m_write_cs);
    if (m_write_queue.size() + mbline.size() <= MAX_WRITE_QUEUE_LENGTH) {
        m_write_queue += mbline;
        queued = true;
    }
    LeaveCriticalSection(&m_write_cs);

    SecureZeroMemory(&mbline[0], mbline.size());

    if (queued) {
        SetEvent(m_write_request_event);
    } else {
        m_handler->send_text(&string_utils::CHAR_BELL, 1);
    }
}

void TelnetSession::discard_output()
{
    InterlockedExchange(&m_is_discarding, 1);
    SetEvent(m_reader_discard_event);
}

bool TelnetSession::load_user_data()
{
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

    SECURITY_ATTRIBUTES sec_attr{};
    sec_attr.bInheritHandle = true;
    sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    
    _snwprintf(pipe_name, sizeof(pipe_name)/sizeof(WCHAR), L"\\\\.\\pipe\\telnetd-stdin-%08x-%08x", GetCurrentProcessId(), s_session_counter++);
    
    m_pipe_stdin = CreateNamedPipeW(
        pipe_name, 
        PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE, 
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,
        PIPE_BUFFER_SIZE,
        PIPE_BUFFER_SIZE,
        0,
        nullptr
    );
    if (m_pipe_stdin == INVALID_HANDLE_VALUE) {
        ERR("CreateNamedPipeW(%ls) failed with: %lu\n", pipe_name, GetLastError());
        return false;
    }

    const auto child_pipe_stdin =CreateFileW(
        pipe_name,
        GENERIC_READ,
        0,
        &sec_attr,
        OPEN_EXISTING,
        0,
        nullptr
    );
    if (child_pipe_stdin == INVALID_HANDLE_VALUE) {
        ERR("CreateFileW(%ls) failed with: %lu\n", pipe_name, GetLastError());
        return false;
    }

    _snwprintf(pipe_name, sizeof(pipe_name)/sizeof(WCHAR), L"\\\\.\\pipe\\telnetd-stdout-%08x-%08x", GetCurrentProcessId(), s_session_counter++);
    
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
        CloseHandle(child_pipe_stdin);
        return false;
    }

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
        CloseHandle(child_pipe_stdin);
        return false;
    }

    m_job_handle = CreateJobObjectW(nullptr, nullptr);
    if (!m_job_handle) {
        const auto ec = GetLastError();
        CloseHandle(child_pipe_stdin);
        CloseHandle(child_pipe_std_out_err);
        ERR("CreateJobObjectW failed with: %lu\n", ec);
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_limit_info{};
    job_limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(m_job_handle, JobObjectExtendedLimitInformation, &job_limit_info, sizeof(job_limit_info))) {
        const auto ec = GetLastError();
        CloseHandle(child_pipe_stdin);
        CloseHandle(child_pipe_std_out_err);
        ERR("SetInformationJobObject failed with: %lu\n", ec);
        return false;
    }

    std::wstring cmd_path{}, cmd_path_quoted{};
    {
        const auto len = GetSystemDirectoryW(nullptr, 0);
        if (!len) {
            const auto ec = GetLastError();
            CloseHandle(child_pipe_stdin);
            CloseHandle(child_pipe_std_out_err);
            ERR("GetSystemDirectoryW failed with: %lu\n", ec);
            return false;
        }
        cmd_path.resize(len - 1);
        if (GetSystemDirectoryW(&cmd_path[0], len) != (len - 1)) {
            const auto ec = GetLastError();
            CloseHandle(child_pipe_stdin);
            CloseHandle(child_pipe_std_out_err);
            ERR("GetSystemDirectoryW failed with: %lu\n", ec);
            return false;
        }
        cmd_path += L"\\cmd.exe";
        cmd_path_quoted = L"\"" + cmd_path + L"\" /A";
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

    if (!CreateProcessAsUserW(
        m_token, 
        cmd_path.c_str(), 
        &cmd_path_quoted[0], 
        nullptr, nullptr, 
        true, 
        CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED, 
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

    if (!AssignProcessToJobObject(m_job_handle, process_info.hProcess)) {
        const auto ec = GetLastError();
        TerminateProcess(process_info.hProcess, ec);
        DestroyEnvironmentBlock(env_block);
        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);
        CloseHandle(child_pipe_stdin);
        CloseHandle(child_pipe_std_out_err);
        ERR("AssignProcessToJobObject failed with: %lu\n", ec);
        return false;
    }

    if (ResumeThread(process_info.hThread) == (DWORD)-1) {
        const auto ec = GetLastError();
        TerminateProcess(process_info.hProcess, ec);
        DestroyEnvironmentBlock(env_block);
        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);
        CloseHandle(child_pipe_stdin);
        CloseHandle(child_pipe_std_out_err);
        ERR("ResumeThread failed with: %lu\n", ec);
        return false;
    }

    CloseHandle(process_info.hThread);
    
    m_cmd_process_handle = process_info.hProcess;
    DestroyEnvironmentBlock(env_block);
    CloseHandle(child_pipe_stdin);
    CloseHandle(child_pipe_std_out_err);

    return true;
}

bool TelnetSession::start_io_thread()
{
    CPINFO cp_info{};
    if (GetCPInfo(CP_OEMCP, &cp_info)) {
        m_oem_max_char_size = cp_info.MaxCharSize;
    } else {
        ERR("GetCPInfo failed with: %lu\n", GetLastError());
        m_oem_max_char_size = 1;
    }

    m_io_stop_event = CreateEvent(nullptr, false, false, nullptr);
    if (!m_io_stop_event) {
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

    m_write_request_event = CreateEvent(nullptr, false, false, nullptr);
    if (!m_write_request_event) {
        ERR("write request event create failed with: %lu\n", GetLastError());
        return false;
    }

    m_writer_overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_writer_overlapped.hEvent) {
        ERR("writer overlapped event create failed with: %lu\n", GetLastError());
        return false;
    }

    m_write_buffer.reserve(PIPE_BUFFER_SIZE);
    m_reader_buffer.resize(PIPE_BUFFER_SIZE);

    DWORD thread_id{};
    m_io_thread_handle = CreateThread(
        nullptr,
        0,
        io_thread_func,
        this,
        0,
        &thread_id
    );
    if (!m_io_thread_handle) {
        ERR("reader thread create failed with: %lu\n", GetLastError());
        return false;
    }

    return true;
}

DWORD TelnetSession::io_loop()
{
    HANDLE wait_objects[6] = {m_io_stop_event, m_reader_overlapped.hEvent, m_reader_discard_event, m_write_request_event, m_writer_overlapped.hEvent, m_cmd_process_handle};

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
            m_cmd_exited ? 5 : 6, 
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
            m_handler->send_string("\r\n");
            m_handler->send_text((const uint8_t*)m_line_tail.data(), (int)m_line_tail.size());
            continue;
        }
        idx -= WAIT_OBJECT_0;

        DWORD bytes_transferred{};
        if (!idx) { // stop event
            CancelIo(m_pipe_stdin);
            CancelIo(m_pipe_std_out_err);

            if (m_is_write_pending) {
                GetOverlappedResult(m_pipe_stdin, &m_writer_overlapped, &bytes_transferred, true);
            }
            if (m_is_read_pending) {
                GetOverlappedResult(m_pipe_std_out_err, &m_reader_overlapped, &bytes_transferred, true);
            }
            
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

            if (!convert_output(m_reader_buffer.data(), bytes_transferred)) {
                continue;
            }

            if (!m_is_discarding && m_convbuf_utf8.size()) {
                m_handler->send_text((const uint8_t*)m_convbuf_utf8.data(), (int)m_convbuf_utf8.size());
            }

            const auto it = m_convbuf_utf8.rfind('\n');
            if (it == std::string::npos) {
                m_line_tail += m_convbuf_utf8;
            } else {
                m_line_tail.assign(m_convbuf_utf8.data(), it + 1);
            }

            if (m_line_tail.size() > MAX_LINE_TAIL_LENGTH) {
                m_line_tail.erase(0, m_line_tail.size() - MAX_LINE_TAIL_LENGTH);
            }

            while (m_line_tail.size() && ((m_line_tail[0] & 0xC0) == 0x80)) {
                m_line_tail.erase(0, 1);
            }
        } else if (idx == 2) {
            continue;
        } else if (idx == 3) {
            start_write();
        } else if (idx == 4) {
            if (!GetOverlappedResult(m_pipe_stdin, &m_writer_overlapped, &bytes_transferred, FALSE)) {
                ERR("GetOverlappedResult failed with: %lu\n", GetLastError());
            }

            m_is_write_pending = false;

            RtlSecureZeroMemory(&m_write_buffer[0], m_write_buffer.size());
            m_write_buffer.clear();

            ResetEvent(m_writer_overlapped.hEvent);
            start_write();
        } else if (idx == 5) {
            TerminateJobObject(m_job_handle, 0);
            m_cmd_exited = true;
        }
    }

    return 0;
}

void TelnetSession::start_write()
{
    if (m_is_write_pending) {
        return;
    }

    EnterCriticalSection(&m_write_cs);
    const int size_to_write = (int)((std::min)(m_write_queue.size(), (size_t)PIPE_BUFFER_SIZE));
    if (!size_to_write) {
        LeaveCriticalSection(&m_write_cs);
        return;
    }
    m_write_buffer.assign(m_write_queue, 0, size_to_write);
    m_write_queue.erase(m_write_queue.begin(), m_write_queue.begin() + size_to_write);
    LeaveCriticalSection(&m_write_cs);

    const auto success = WriteFile(m_pipe_stdin, m_write_buffer.data(), size_to_write, nullptr, &m_writer_overlapped);
    const auto ec = GetLastError();
    if (success || (ec == ERROR_IO_PENDING)) {
        m_is_write_pending = true;
        return;
    }

    ERR("WriteFile failed with: %lu\n", ec);

    RtlSecureZeroMemory(&m_write_buffer[0], m_write_buffer.size());
    m_write_buffer.clear();

    EnterCriticalSection(&m_write_cs);
    RtlSecureZeroMemory(&m_write_queue[0], m_write_queue.size());
    m_write_queue.clear();
    LeaveCriticalSection(&m_write_cs);
}

bool TelnetSession::convert_output(const char* data, size_t size)
{
    std::string _input;
    if (m_oem_carry.size()) {
        _input = m_oem_carry + std::string(data, size);
        data = _input.data();
        size = _input.size();
    }
    m_oem_carry.clear();

    size_t cut_point = size;
    if (m_oem_max_char_size > 1) {
        for (size_t i = 0; i < size; i++) {
            if (IsDBCSLeadByteEx(CP_OEMCP, (BYTE)data[i])) {
                if (i+1 == size) {
                    cut_point = i;
                    break;
                }
                i++;
            }
        }
    }
    m_oem_carry.assign(data + cut_point, size - cut_point);
    if (!cut_point) {
        m_convbuf_utf8.clear();
        return true;        
    }

    auto error = string_utils::convert_multibyte_to_widechar(CP_OEMCP, data, (int)cut_point, m_convbuf_wide);
    if (error != ERROR_SUCCESS) {
        ERR("convert_multibyte_to_widechar failed with: %lu\n", error);
        return false;
    }

    error = string_utils::convert_widechar_to_multibyte(CP_UTF8, m_convbuf_wide, m_convbuf_utf8);
    if (error != ERROR_SUCCESS) {
        ERR("convert_widechar_to_multibyte failed with: %lu\n", error);
        return false;
    }

    return true;
}
