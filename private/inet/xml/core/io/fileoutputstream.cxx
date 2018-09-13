/*
 * @(#)FileOutputStream.cxx    1.1 11/18/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

DEFINE_CLASS_MEMBERS(FileOutputStream, _T("FileOutputStream"), OutputStream);

FileOutputStream * 
FileOutputStream::newFileOutputStream(String * n, bool append) //throws IOException 
{
    FileOutputStream * f = new FileOutputStream();
    f->append = append;
    f->open(n);
    return f;
}

void FileOutputStream::open(String * n)
{
    fd = ::CreateFileA((char *)AsciiText(n),
        GENERIC_WRITE, 
        FILE_SHARE_READ, 
        NULL, 
        append ? OPEN_EXISTING : CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL, 
        0);
    if (fd == INVALID_HANDLE_VALUE)
    {
        Exception::throwLastError();
    }
    if (append)
    {
        int i = ::SetFilePointer(fd, 0, NULL, FILE_END);
        if (i == 0xFFFFFFFF)
        {
            DWORD error = ::GetLastError();
            if (error != NO_ERROR)
                Exception::throwLastError();
        }
    }

    name = n;
}

/**
 */
FileOutputStream *
FileOutputStream::newFileOutputStream(DWORD handle)
{
    FileOutputStream * f = new FileOutputStream();

    f->fd = ::GetStdHandle(handle);
    if (f->fd == INVALID_HANDLE_VALUE) 
    {
        Exception::throwLastError();
    }
    f->append = false;
    f->name = String::emptyString();
    return f;
}

/**
 */
void FileOutputStream::write(int b) //throws IOException;
{
    DWORD dw;
    if (!::WriteFile(fd, &b, 1, &dw, NULL) || dw != 1)
    {
        Exception::throwLastError();
    }
}

/**
 */
void FileOutputStream::write(abyte * b) //throws IOException 
{
    write(b, 0, b->length());
}

/**
 */
void FileOutputStream::write(abyte * b, int off, int len) //throws IOException 
{
    if (b->length() - off < len)
    {
        len = b->length() - off;
    }
    DWORD dw;
    Assert(len >= 0);
    if (!::WriteFile(fd, (byte *)b->getData() + off, len, &dw, NULL) || dw != (DWORD)len)
    {
        Exception::throwLastError();
    }
}

HRESULT STDMETHODCALLTYPE FileOutputStream::Write(void const* pv, ULONG cb, ULONG * pcbWritten)
{
    DWORD dw;
    Assert(cb >= 0);
    if (!::WriteFile(fd, pv, cb, &dw, NULL) || dw != (DWORD)cb)
    {
        return E_FAIL;
    }
    *pcbWritten = dw;
    return S_OK;
}

/**
 */
void FileOutputStream::close() //throws IOException;
{
    if (fd != INVALID_HANDLE_VALUE)
    {
        flush();
        ::CloseHandle(fd);
        fd = INVALID_HANDLE_VALUE;
    }
}