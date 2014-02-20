#pragma once

void WrapLogOpen();
void WrapLogClose();
void __cdecl WrapLogPre(_Printf_format_string_ const char* msg, ...);
void __cdecl WrapLogPost(_Printf_format_string_ const char* msg, ...);
void __cdecl WrapLogEnter(_Printf_format_string_ const char* msg, ...);
void __cdecl WrapLogExit(const char* msg, HRESULT code);

template <class T>
LPSTR Wrap(const T& value);
