/*
 * @(#)FileInputStream.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

DEFINE_CLASS_MEMBERS(FileInputStream, _T("FileInputStream"), InputStream);

/**
 */
FileInputStream *
FileInputStream::newFileInputStream(String * n)  
{
    FileInputStream * f = new FileInputStream();
    { // Scope RATCHAR
        RATCHAR file = n->toCharArrayZ();
        f->fd = ::CreateFile(file->getData(),
            GENERIC_READ, 
            FILE_SHARE_READ, 
            NULL, 
            OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL, 
            0);
    }

    if (f->fd == INVALID_HANDLE_VALUE)
    {
        Exception::throwLastError();
    }

    f->name = n;
    return f;
}

FileInputStream *
FileInputStream::newFileInputStream(DWORD handle)
{
    FileInputStream * f = new FileInputStream();

    f->fd = ::GetStdHandle(handle);

    if (f->fd == INVALID_HANDLE_VALUE)
    {
        Exception::throwLastError();
    }

    f->name = String::emptyString();
    return f;
}

/**
 */
int FileInputStream::read() //throws IOException;
{
    int c = 0;
    DWORD dw;

    if (!::ReadFile(fd, &c, 1, &dw, NULL))
    {
        return -1;
    }
    if (dw == 0)
        return -1;
    else
        return c;
}


/**
 */
int FileInputStream::read(abyte * b, int off, int len) //throws IOException 
{
    DWORD dw;

    if (b->length() - off < len)
    {
        len = b->length() - off;
    }

    if (!::ReadFile(fd, (byte *)b->getData() + off, len, &dw, NULL))
    {
        return -1;
    }
    if (dw == 0)
        return -1;
    else
        return dw;
}