/*
 * @(#)FileInputStream.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _CORE_IO_FILEINPUTSTREAM
#define _CORE_IO_FILEINPUTSTREAM

#ifndef _CORE_IO_INPUTSTREAM
#include "core/io/inputstream.hxx"
#endif

DEFINE_CLASS(String);
DEFINE_CLASS(Exception);
DEFINE_CLASS(URL);

DEFINE_CLASS(FileInputStream);

class FileInputStream : public InputStream 
{
    DECLARE_CLASS_MEMBERS(FileInputStream, InputStream);

    protected: FileInputStream() : name(REF_NOINIT) {}

public:

    static FileInputStream * newFileInputStream(String * name); 

    static FileInputStream * newFileInputStream(DWORD handle);

    int read(); 

    int read(abyte * b, int off, int len);

    void close() 
    { 
        if (fd != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(fd); 
            fd = INVALID_HANDLE_VALUE;
        }
    }

    virtual String* toString() { return name; }

private: 

    /* handle to the open file */
     HANDLE fd;

     /* file name */
     RString name;

protected:

    /**
     */
    virtual void finalize() //throws IOException 
    {
        name = null;
        close();
        super::finalize();
    }
};


#endif _CORE_IO_FILEINPUTSTREAM