/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#ifdef _DEBUG

#include <stdio.h>

// The LogMessage function opens and closes the log file each time it writes
// to it -- so we can always read it.  But this means it has to always append
// to the log file so to ensure it doesn't grow infinitely, we truncate
// the log file the first time it is opened by this process.
// 
static bool truncate = true;

LogFile::LogFile(TCHAR* filename)
{
    handle = ::CreateFile(filename,
        GENERIC_WRITE, 
        FILE_SHARE_READ, 
        NULL, 
        truncate ? TRUNCATE_EXISTING : OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, 
        0);
    truncate = false;
    if (handle == INVALID_HANDLE_VALUE)
    {
        handle = ::CreateFile(filename,
            GENERIC_WRITE, 
            FILE_SHARE_READ, 
            NULL, 
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, 
            0);
    }
    else
    {
        // append to the file.
        int i = ::SetFilePointer(handle, 0, NULL, FILE_END);
    }
}

LogFile::~LogFile()
{
    ::CloseHandle(handle);
}

void LogFile::Write(const char* buffer, int len)
{
    DWORD dw;
    ::WriteFile(handle, buffer, len, &dw, NULL);
}

void LogFile::WriteString(const TCHAR* string)
{
    while (*string != 0)
    {
        char ch = (char)*string++;
        Write(&ch, 1);            
    }
}

void LogFile::WriteString(String* s)
{
    if (s == null)
        Write("(null)", 6);
    else
        WriteString(s->toCharArrayZ()->getData());
}

void LogFile::WriteInt(long i)
{
    if (i == 0)
    {
        Write("0",1); // special case.
        return;
    }
    char num[30];
    // standard C library routines (sprintf or atoi) cannot be used presently because of
    // a bug in out crt startup code.
    char* ptr = &num[29];
    *ptr = 0;
    while (i > 0)
    {
        ptr--;
        *ptr = (i % 10) + '0';
        i /= 10;
    }
    Write(ptr, strlen(ptr)); 
}

void LogFile::WriteName(Name* n)
{
    WriteString(n == null ? null : n->toString());
}

void LogFile::WriteIID(REFIID riid)
{
    TCHAR buf[40];
    ::StringFromGUID2(riid, buf, 39);
    buf[39] = '\0';
    WriteString(buf);
}

//-----------------------------------------------------------------------------------------
void LogMessage(const char* message, ...)
{
#if 0
    STACK_ENTRY;

    LogFile log(_T("\\msxml.log"));
    TRY 
    {
        va_list arglist;
        va_start(arglist, message);

        for (int i = 0; message[i] != 0; i++)
        {
            if (message[i] == '%')
            {
                if (i > 0) 
                {
                    log.Write(message, i); // write out message so far
                }
                i++;
                switch (message[i])
                {
                case 's':   // String*
                    {
                        i++;
                        String* s = (String*)va_arg(arglist, String *);
                        log.WriteString(s);
                    }
                    break;
                case 'n':   // Name*
                    {
                        i++;
                        Name* n = (Name*)va_arg(arglist, Name *);
                        log.WriteName(n);
                    }
                    break;
                case 'l':   // long
                    {
                        i++;
                        long l = va_arg(arglist, long);
                        log.WriteInt(l);
                    }
                    break;
                case 'i':   // int
                    {
                        i++;
                        int l = va_arg(arglist, int);
                        log.WriteInt((long)l);
                    }
                    break;
                case 'r':   // REFIID
                    {
                        i++;
                        REFIID riid = (REFIID)va_arg(arglist, IID*);
                        log.WriteIID(riid);
                    }
                    break;
                case '%':
                    log.Write("%", 1); // write one percent sign.
                    i++;
                    break;
                default: 
                    log.Write("%", 1); // unrecognized.
                    break;
                }
                message = &message[i]; // move pointer forward.
                i = -1; // following i++ will result in i = 0.
            }
        }
        if (i > 0)
            log.Write(message, strlen(message)); // write remainder of message.
        va_end(arglist);

        log.Write("\r\n", 2);
    }
    CATCH
    {
    }
#endif
}

#endif