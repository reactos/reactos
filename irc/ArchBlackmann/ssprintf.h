// ssprintf.h

#ifndef SSPRINTF_H
#define SSPRINTF_H

#include <string>
#include <stdarg.h>

std::string ssprintf ( const char* fmt, ... );
std::string ssvprintf ( const char* fmt, va_list args );

#endif//SSPRINTF_H
