// ssprintf.h

#ifndef SSPRINTF_H
#define SSPRINTF_H

#include <string>
#include <stdarg.h>

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

#endif//SSPRINTF_H
