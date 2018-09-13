/*
 * @(#)FileOutputStream.hxx    1.1 11/18/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 * This is a completely rewritten version of the Java URL class.
 * 
 */

#ifndef _CORE_IO_FILEOUTPUTSTREAM
#define _CORE_IO_FILEOUTPUTSTREAM

#ifndef _CORE_IO_OUTPUTSTREAM
#include "core/io/outputstream.hxx"
#endif

DEFINE_CLASS(String);
DEFINE_CLASS(Exception);

DEFINE_CLASS(FileOutputStream);

class FileOutputStream : public OutputStream,
                         public Stream
{
    DECLARE_CLASS_MEMBERS_I1(FileOutputStream, OutputStream, Stream);

    protected: FileOutputStream() : name(REF_NOINIT) {}

public:
    
    static FileOutputStream * newFileOutputStream(String * name, bool append = false); 

    static FileOutputStream * newFileOutputStream(DWORD handle);

    virtual void write(int b); 

    virtual void write(abyte * b); 

    virtual void write(abyte * b, int off, int len); 

    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten);

    virtual void close();

    virtual String* toString() { return name; }
    
private:

    void open(String *n);

private:

    HANDLE fd;
    bool append;
    RString name;

protected: 
    
    virtual void finalize()
    {
        name = null;
        if (fd != INVALID_HANDLE_VALUE) 
        {
            if (fd == ::GetStdHandle(STD_OUTPUT_HANDLE) || fd == ::GetStdHandle(STD_ERROR_HANDLE)) 
            {
                 flush();
             } 
            else 
            {
                 close();
             }
        }
        super::finalize();
    }
};


#endif _CORE_IO_FILEOUTPUTSTREAM
