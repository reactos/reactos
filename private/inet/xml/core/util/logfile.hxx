/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _LOGFILE_HXX
#define _LOGFILE_HXX

#ifdef _DEBUG

class LogFile
{
public:
    LogFile(TCHAR* filename);
    ~LogFile();
    void Write(const char* buffer, int len);
    void WriteString(const TCHAR* string);
    void WriteString(String* s);
    void WriteInt(long i);
    void WriteName(Name* n);
    void WriteIID(REFIID riid);
private:
    HANDLE handle;
};

void _cdecl LogMessage(const char* message, ...);

#else

#define LogMessage(a)

#endif

#endif _LOGFILE_HXX