/*
 * PROJECT:     ReactOS Telnet Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Telnet service
 * COPYRIGHT:   Copyright 2026 Aravinda Samarathunga (I-A-S)
 */

#include "string_utils.hpp"

namespace string_utils
{
    uint8_t get_utf8_sequence_length(uint8_t c)
    {
        if (c < 0x80) {
            return 1;
        } else if ((c & 0xE0) == 0xC0) {
            return 2;
        } else if ((c & 0xF0) == 0xE0) {
            return 3;
        } else if ((c & 0xF8) == 0xF0) {
            return 4;
        }

        return 1;
    }

    uint32_t convert_multibyte_to_widechar(uint32_t code_page, const char* in, int32_t n, std::wstring& out)
    {
        const auto len = MultiByteToWideChar(code_page, 0, in, n, nullptr, 0);
        if (!len) {
            return GetLastError();
        }
        out.resize(len, '\0');
        if (!MultiByteToWideChar(code_page, 0, in, n, &out[0], len)) {
            return GetLastError();
        }
        return ERROR_SUCCESS;
    }

    uint32_t convert_widechar_to_multibyte(uint32_t code_page, const wchar_t* in, int32_t n, std::string& out)
    {
        const auto len = WideCharToMultiByte(code_page, 0, in, n, nullptr, 0, nullptr, nullptr);
        if (!len) {
            return GetLastError();
        }
        out.resize(len, '\0');
        if (!WideCharToMultiByte(code_page, 0, in, n, &out[0], len, nullptr, nullptr)) {
            return GetLastError();
        }
        return ERROR_SUCCESS;
    }
}
