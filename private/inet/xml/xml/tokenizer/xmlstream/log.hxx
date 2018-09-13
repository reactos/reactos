/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _LOG_HXX
#define _LOG_HXX

class Log
{
public:
    Log(char* filename);
    ~Log();

    void WriteLong(long value);
    void WriteShort(short value);
    void WriteByte(unsigned char value);
    void WriteChar(char value);
    void WriteString(char* string);
    void WriteString(TCHAR* string);
private:
    HANDLE h;
};


#endif _LOG_HXX