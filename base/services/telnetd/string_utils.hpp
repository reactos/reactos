/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#pragma once

#include <windef.h>
#include <winbase.h>
#include <winnls.h> // including this here (instead of in the cpp) so users of this header gets CP_*

#include <cstdint>
#include <string>

namespace string_utils
{
    constexpr uint8_t CHAR_ETX = 3;
    constexpr uint8_t CHAR_BELL = 7;
    constexpr uint8_t CHAR_BS = 8;
    constexpr uint8_t CHAR_NAK = 21;
    constexpr uint8_t CHAR_DEL = 127;
    constexpr uint8_t CHAR_SP = ' ';
    constexpr uint8_t CHAR_CR = '\r';
    constexpr uint8_t CHAR_LF = '\n';

    uint8_t get_utf8_sequence_length(uint8_t c);

    uint32_t convert_multibyte_to_widechar(uint32_t code_page, const char* in, int32_t n, std::wstring& out);
    uint32_t convert_widechar_to_multibyte(uint32_t code_page, const wchar_t* in, int32_t n, std::string& out);
    
    inline uint32_t convert_multibyte_to_widechar(uint32_t code_page, const std::string& in, std::wstring& out, int32_t n = INT32_MAX)
    {
        n = (n == INT32_MAX) ? (int32_t)in.size() : n;
        return convert_multibyte_to_widechar(code_page, in.data(), n, out);
    }

    inline uint32_t convert_widechar_to_multibyte(uint32_t code_page, const std::wstring& in, std::string& out, int32_t n = INT32_MAX)
    {
        n = (n == INT32_MAX) ? (int32_t)in.size() : n;
        return convert_widechar_to_multibyte(code_page, in.data(), n, out);
    }
}