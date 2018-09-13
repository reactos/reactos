/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/

#include <windows.h>
#include "log.hxx"

Log::Log(char* filename)
{
    h = ::CreateFileA(filename,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
    if (h != NULL)
    {
        ::SetFilePointer(h, 0, NULL, FILE_END); // append to file.
    }
}

Log::~Log()
{
    if (h != NULL)
        ::CloseHandle(h);
}

static const char* hex = "0123456789ABCDEF";


void Log::WriteByte(unsigned char value)
{
    DWORD dw;
    unsigned int index = value >> 4;
    char ch = hex[index];
    WriteFile(h, &ch, 1, &dw, NULL);
    index = value & 0xf;
    ch = hex[index];
    WriteFile(h, &ch, 1, &dw, NULL);
}

void Log::WriteShort(short value)
{
    char* ptr = (char*)&value;
    for (int i = 0; i < 2; i++)
        WriteByte(ptr[i]);
}

void Log::WriteLong(long value)
{
    DWORD dw;
    char* ptr = (char*)&value;
    for (int i = 0; i < 4; i++)
        WriteByte(ptr[i]);
}

void Log::WriteChar(char value)
{
    DWORD dw;
    WriteFile(h, &value, 1, &dw, NULL);
}

void Log::WriteString(char* string)
{
    for (char* ptr = string; *ptr != NULL; ptr++)
        WriteChar(*ptr);
}

void Log::WriteString(TCHAR* string)
{
    for (TCHAR* ptr = string; *ptr != NULL; ptr++)
        WriteChar((char)*ptr);
}
