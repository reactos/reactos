#pragma once

void WrapLogOpen();
void WrapLogClose();
void __cdecl WrapLogMsg(_Printf_format_string_ const char* msg, ...);
void __cdecl WrapLogEnter(_Printf_format_string_ const char* msg, ...);
void __cdecl WrapLogExit(_Printf_format_string_ const char* msg, ...);

template <class T>
LPSTR Wrap(const T& value);
