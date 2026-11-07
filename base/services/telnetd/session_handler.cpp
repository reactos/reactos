/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#include "session_handler.hpp"

#include <cassert>

using cursor_type = const uint8_t*;

constexpr uint8_t CHAR_ETX = 3;
constexpr uint8_t CHAR_BELL = 7;
constexpr uint8_t CHAR_BS = 8;
constexpr uint8_t CHAR_NAK = 21;
constexpr uint8_t CHAR_DEL = 127;
constexpr uint8_t CHAR_CR = '\r';
constexpr uint8_t CHAR_LF = '\n';

TelnetSessionHandler::TelnetSessionHandler()
{
    m_echo_buffer.reserve(1024);
    m_sb_buffer.reserve(MAX_SUBNEGO_PAYLOAD_LENGTH);
    m_text_line_buffer.reserve(MAX_TEXT_LINE_LENGTH);
    InitializeCriticalSection(&m_out_text_cs);
}

TelnetSessionHandler::~TelnetSessionHandler()
{
    delete m_active_session;
    DeleteCriticalSection(&m_out_text_cs);
}

void TelnetSessionHandler::on_connect()
{
    send_option_command(ECommand::WILL, EOption::ECHO);
    m_echo_state = EEchoState::WE_OFFERED_TO;

    send_option_command(ECommand::WILL, EOption::SGA);
    send_option_command(ECommand::DO, EOption::SGA);

    send_string("ReactOS Telnet\r\n");

    set_state_to_waiting_for_username();
}

void TelnetSessionHandler::on_disconnect()
{
    delete m_active_session;
    m_active_session = nullptr;
}

void TelnetSessionHandler::on_data(const uint8_t* data, int length)
{
    const cursor_type end_cur = data + length;
    cursor_type scan_cur{data}, text_cur{data};

    while(scan_cur < end_cur)
    {
        switch(m_parser_state)
        {
            case EParserState::TEXT:
                if (*scan_cur++ == (uint8_t)ECommand::IAC) {
                    handle_text(text_cur, scan_cur - text_cur - 1);
                    text_cur = scan_cur;
                    m_parser_state = EParserState::AFTER_IAC;
                }
                break;

            case EParserState::AFTER_IAC:
            {
                const auto c = (ECommand)*scan_cur++;
                if(c == ECommand::SB) {
                    m_parser_state = EParserState::AFTER_SB;
                } else if(
                    (c >= ECommand::WILL) &&
                    (c <= ECommand::DONT)
                ) {
                    m_parser_state_data = (uint8_t)c;
                    m_parser_state = EParserState::AFTER_VERB;
                } else {
                    if (c == ECommand::IAC)
                        handle_text((const uint8_t*)&c, 1);
                    else
                        handle_command(c);
                    text_cur = scan_cur;
                    m_parser_state = EParserState::TEXT;
                }
            }
            break;

            case EParserState::AFTER_VERB:
                handle_option_command((ECommand)m_parser_state_data, (EOption)*scan_cur++);
                text_cur = scan_cur;
                m_parser_state = EParserState::TEXT;
                break;

            case EParserState::AFTER_SB:
                m_sb_buffer.clear();
                m_parser_state_data = *scan_cur++;
                m_parser_state = EParserState::SUBNEGO_PAYLOAD;
                break;

            case EParserState::SUBNEGO_PAYLOAD:
            {
                const auto c = *scan_cur++;

                if (c == (uint8_t)ECommand::IAC) {
                    m_parser_state = EParserState::SUBNEGO_AFTER_IAC;
                } else {
                    if (m_sb_buffer.size() >= MAX_SUBNEGO_PAYLOAD_LENGTH) {
                        m_parser_state = EParserState::MALFORMED_SUBNEGO;
                        break;
                    }

                    m_sb_buffer.push_back(c);
                }
            }
            break;

            case EParserState::SUBNEGO_AFTER_IAC:
            {
                const auto c = (ECommand)*scan_cur++;

                if (c == ECommand::IAC) {
                    if (m_sb_buffer.size() >= MAX_SUBNEGO_PAYLOAD_LENGTH) {
                        m_parser_state = EParserState::MALFORMED_SUBNEGO;
                        break;
                    }

                    m_sb_buffer.push_back(0xFF);
                    m_parser_state = EParserState::SUBNEGO_PAYLOAD;
                } else if (c == ECommand::SE) {
                    assert(m_sb_buffer.size() <= MAX_SUBNEGO_PAYLOAD_LENGTH);
                    handle_subnegotiation(m_parser_state_data, m_sb_buffer.data(), (uint8_t)m_sb_buffer.size());
                    text_cur = scan_cur;
                    m_parser_state = EParserState::TEXT;
                } else {
                    scan_cur--;
                    m_parser_state = EParserState::AFTER_IAC;
                }
            }
            break;

            case EParserState::MALFORMED_SUBNEGO:
                if (*scan_cur++ == (uint8_t)ECommand::IAC)
                    m_parser_state = EParserState::MALFORMED_SUBNEGO_AFTER_IAC;
            break;

            case EParserState::MALFORMED_SUBNEGO_AFTER_IAC:
            {
                const auto c = (ECommand)*scan_cur++;

                if (c == ECommand::SE) {
                    text_cur = scan_cur;
                    m_parser_state = EParserState::TEXT;
                } else if (c == ECommand::IAC)
                    m_parser_state = EParserState::MALFORMED_SUBNEGO;
                else {
                    scan_cur--;
                    m_parser_state = EParserState::AFTER_IAC;
                }
            }
            break;
        }
    }

    if (m_parser_state == EParserState::TEXT)
        handle_text(text_cur, scan_cur - text_cur);
}

void TelnetSessionHandler::handle_text(const uint8_t* data, int length)
{
    const cursor_type end_cur = data + length;

    for(cursor_type cur = data; cur != end_cur; cur++)
    {
        const auto c = *cur;

        if (m_text_last_byte_was_cr && (!c || (c == CHAR_LF))) {
            m_text_last_byte_was_cr = false;
            continue;
        }

        if ((m_text_last_byte_was_cr = (c == CHAR_CR)) || (c == CHAR_LF)) {
            if (should_echo()) {
                m_echo_buffer.push_back(CHAR_CR);
                m_echo_buffer.push_back(CHAR_LF);
            }

            flush_echo_buffer();

            handle_text_line(m_text_line_buffer);
            SecureZeroMemory(&m_text_line_buffer[0], m_text_line_buffer.size());
            m_text_line_buffer.clear();
        } else if ((c >= 32) && (c <= 126)) { // [IATODO]: Only accept ASCII for now
            if (m_text_line_buffer.size() < MAX_TEXT_LINE_LENGTH) {
                m_text_line_buffer.push_back(c);

                if (should_echo())
                    m_echo_buffer.push_back(c);
            } else
                m_echo_buffer.push_back(CHAR_BELL);
        } else if ((c == CHAR_BS) || (c == CHAR_DEL)) {
            erase_char();
        } else if (c == CHAR_NAK) {
            erase_line();
        } else if (c == CHAR_ETX) {
            if (m_active_session)
                m_active_session->on_sigint();
        }
    }

    flush_echo_buffer();
}

void TelnetSessionHandler::handle_text_line(const std::string& line)
{
    const auto authenticate = [](const std::string& username, const std::string& password) -> HANDLE
    {
        HANDLE token{};
        // [IATODO]: Add widechar support later
        return LogonUserA((char*)username.c_str(), (char*)".", (char*)password.c_str(), LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &token) ? token : NULL;
    };

    switch (m_state)
    {
        case EState::WAITING_FOR_USERNAME:
            m_username = line;
            set_state_to_waiting_for_password();
            break;

        case EState::WAITING_FOR_PASSWORD:
        {
            HANDLE token{};
            if (
                (m_username.size() <= TelnetSession::MAX_CREDENTIAL_LENGTH) && 
                (line.size() <= TelnetSession::MAX_CREDENTIAL_LENGTH) && 
                ((token = authenticate(m_username, line)) != NULL)
            ) {
                if (m_echo_state == EEchoState::AGREED_TO) {
                    send_string("\r\n");
                }

                m_active_session = new TelnetSession(this);

                if (!m_active_session->connect(m_username, token)) {
                    m_state = EState::INVALID;
                    send_string("server error\r\n");
                    disconnect();
                    return;
                }

                set_state_to_active_session();
            } else {
                m_failed_login_attempt_count++;
                if (m_failed_login_attempt_count >= MAX_FAILED_LOGIN_ATTEMPTS) {
                    m_state = EState::INVALID;
                    disconnect();
                    return;
                }
                if (m_echo_state == EEchoState::AGREED_TO) {
                    send_string("\r\n");
                }
                send_string("Login incorrect\r\n");
                set_state_to_waiting_for_username();
            }
        }
        break;

        case EState::ACTIVE_SESSION:
            if (!m_active_session) {
                m_state = EState::INVALID;
                disconnect();
                return;
            }

            m_active_session->on_text_line(line);
            break;
        
        default:
            break;
    }
}

void TelnetSessionHandler::handle_command(ECommand cmd)
{
    switch (cmd)
    {
        case ECommand::AYT:
        {
            send_string("\r\n[ReactOS telnetd: yes]\r\n");
        }
        break;

        case ECommand::AO:
        {
            if (m_active_session)
                m_active_session->discard_output();

            uint8_t data[2] = { (uint8_t)ECommand::IAC, (uint8_t)ECommand::DM };
            send_data(data, sizeof(data));
        }
        break;

        case ECommand::IP:
            if (m_active_session)
                m_active_session->on_interrupt_process();
            break;

        case ECommand::BRK:
            if (m_active_session)
                m_active_session->on_break();
            break;

        case ECommand::EC:
            erase_char();
            flush_echo_buffer();
            break;

        case ECommand::EL:
            erase_line();
            flush_echo_buffer();
            break;

        case ECommand::WILL:
        case ECommand::WONT:
        case ECommand::DO:
        case ECommand::DONT:
            assert(false);
            break;

        default:
            break;
    }
}

void TelnetSessionHandler::handle_option_command(ECommand verb, EOption option)
{
    switch (option) {
        case EOption::ECHO:
        {
            if (m_echo_state == EEchoState::WE_OFFERED_TO) {
                if (verb == ECommand::DO)
                    m_echo_state = EEchoState::AGREED_TO;
                else if (verb == ECommand::DONT)
                    m_echo_state = EEchoState::AGREED_NOT_TO;
            } else if ((m_echo_state == EEchoState::AGREED_TO) && (verb == ECommand::DONT)) {
                m_echo_state = EEchoState::AGREED_NOT_TO;
                send_option_command(ECommand::WONT, EOption::ECHO);
            } else if ((m_echo_state == EEchoState::AGREED_NOT_TO) && (verb == ECommand::DO)) {
                m_echo_state = EEchoState::AGREED_TO;
                send_option_command(ECommand::WILL, EOption::ECHO);
            }

            if (verb == ECommand::WILL)
                send_option_command(ECommand::DONT, EOption::ECHO);
        }
        break;

        case EOption::SGA:
        {
            if ((verb == ECommand::DONT) || (verb == ECommand::WONT)) {
                // Any reasonably modern telnet client supports full duplex.
                // If a client doesn't support full duplex, we just drop them. 
                disconnect();
            }
        }
        break;

        case EOption::TM:
        {
            if (verb == ECommand::DO)
                send_option_command(ECommand::WILL, EOption::TM);
        }
        break;

        case EOption::NAWS:
        case EOption::TTYPE:
        {
            if (verb == ECommand::DO)
                send_option_command(ECommand::WONT, option);
            else if (verb == ECommand::WILL)
                send_option_command(ECommand::DO, option);
        }
        break;

        default:
        {
            if (verb == ECommand::DO)
                send_option_command(ECommand::WONT, option);
            else if (verb == ECommand::WILL)
                send_option_command(ECommand::DONT, option);
        }
        break;
    }
}

void TelnetSessionHandler::handle_subnegotiation(uint8_t option, const uint8_t* params, uint8_t length)
{

}

void TelnetSessionHandler::send_text(const uint8_t* data, int length)
{
    if (length <= 0)
        return;

    const cursor_type end_cur = data + length;
    cursor_type write_cur{data}, text_cur{data};

    EnterCriticalSection(&m_out_text_cs);

    // 1.25 * length to account for potential escaped IACs, CRs, LFs
    m_out_text_buffer.reserve(length + (length >> 2)); 
    m_out_text_buffer.clear();

    while(write_cur < end_cur)
    {        
        const auto c = *write_cur;

        if (m_last_out_was_cr && c != '\n' && c != '\0') {
            m_out_text_buffer.insert(m_out_text_buffer.end(), text_cur, write_cur);
            m_out_text_buffer.push_back(0);
            text_cur = write_cur;
            m_last_out_was_cr = false;
        }

        if (c == 0xFF) {
            m_out_text_buffer.insert(m_out_text_buffer.end(), text_cur, write_cur);
            m_out_text_buffer.push_back(0xFF);
            m_out_text_buffer.push_back(0xFF);
            text_cur = write_cur + 1;
        } else if (c == '\n') {
            m_out_text_buffer.insert(m_out_text_buffer.end(), text_cur, write_cur);
            if(!m_last_out_was_cr)
                m_out_text_buffer.push_back('\r');
            m_out_text_buffer.push_back('\n');
            m_last_out_was_cr = false;
            text_cur = write_cur + 1;
        } else
            m_last_out_was_cr = (c == '\r');

        write_cur++;
    }
    m_out_text_buffer.insert(m_out_text_buffer.end(), text_cur, write_cur);

    send_data(m_out_text_buffer.data(), (int)m_out_text_buffer.size());

    LeaveCriticalSection(&m_out_text_cs);
}

void TelnetSessionHandler::erase_char()
{
    if (m_text_line_buffer.size()) {
        m_text_line_buffer.pop_back();
        
        if (should_echo()) {
            m_echo_buffer.push_back(CHAR_BS);
            m_echo_buffer.push_back(' ');
            m_echo_buffer.push_back(CHAR_BS);
        }
    }

    m_text_last_byte_was_cr = false;
}

void TelnetSessionHandler::erase_line()
{
    if (should_echo()) {
        const auto current_echo_len = m_echo_buffer.size();
        m_echo_buffer.resize(current_echo_len + m_text_line_buffer.size() * 3);

        for (size_t i = 0; i < m_text_line_buffer.size(); i++) {
            const auto idx= current_echo_len + (i * 3);
            m_echo_buffer[idx + 1] = ' ';
            m_echo_buffer[idx + 0] = m_echo_buffer[idx + 2] = CHAR_BS;
        }
    }

    SecureZeroMemory(&m_text_line_buffer[0], m_text_line_buffer.size());
    m_text_line_buffer.clear();

    m_text_last_byte_was_cr = false;
}
