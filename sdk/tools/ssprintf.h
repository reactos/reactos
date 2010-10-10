/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include <string>
#include <stdarg.h>

#ifdef __CYGWIN__
namespace std {
	typedef basic_string<wchar_t> wstring;
}
#endif

std::string ssprintf ( const char* fmt, ... );
std::string ssvprintf ( const char* fmt, va_list args );

std::wstring sswprintf ( const wchar_t* fmt, ... );
std::wstring sswvprintf ( const wchar_t* fmt, va_list args );

#ifdef _UNICODE
#define sstprintf sswprintf
#define sstvprintf sswvprintf
#else
#define sstprintf ssprintf
#define sstvprintf ssvprintf
#endif
