/*
 * Handy secure string adapter functions for mbedTLS,
 * converting from `StringCchVPrintfEx` to `_vsnprintf_s`.
 *
 * Copyright 2015 Ismael Ferreras Morezuelas <swyterzone+ros@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windef.h>
#include <strsafe.h>

int _vsnprintf_s(char *buffer, size_t sizeOfBuffer, size_t count, const char *format, va_list argptr)
{
    size_t cchRemaining;

    HRESULT ret = StringCchVPrintfEx(buffer, sizeOfBuffer, NULL, &cchRemaining, 0, format, argptr);

    // EXAMPLE //////////////////////////
    // -------- > Size of provided buffer in chars (8).
    // ABC0____ > Buffer contents after StringCchVPrintfEx gets called.
    // ---      > What we actually need to return (3).
    //    ----- > Remaining chars in buffer, including post string NULL, returned by StringCchVPrintfEx (5).

    /* _vsnprintf_s returns the number of characters written, not including
       the terminating null, or a negative value if an output error occurs.  */

    switch (ret)
    {
        case S_OK:
            return (sizeOfBuffer - cchRemaining);

        case STRSAFE_E_INVALID_PARAMETER:
        case STRSAFE_E_INSUFFICIENT_BUFFER:
        default:
            return -1;
    }
}
