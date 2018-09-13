/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_IO_STRINGBUFFEROUTPUTSTREAM
#define _CORE_IO_STRINGBUFFEROUTPUTSTREAM

#ifndef _CORE_IO_OUTPUTSTREAM
#    include "core/io/outputstream.hxx"
#endif


DEFINE_CLASS(OutputStream);

/**
 */
class StringBufferOutputStream: public OutputStream
{

    DECLARE_CLASS_MEMBERS(StringBufferOutputStream, OutputStream);

    protected: StringBufferOutputStream() : sb(REF_NOINIT) {}

    public: static StringBufferOutputStream * newStringBufferOutputStream(StringBuffer * sb);

    /**
     */
    public: virtual void write(int b) //throws IOException;
            {
                sb->append((TCHAR) b);
            }
    /**
     */
    public: virtual void write(abyte * b, int off, int len) //throws IOException 
            {
                sb->append(b, off, len);
            }

    public: virtual void write(ATCHAR * b) //throws IOException 
            {
                sb->append(b);
            }

    public: virtual void write(ATCHAR * b, int off, int len) //throws IOException 
            {
                sb->append(b, off, len);
            }

    public: virtual void write(TCHAR * pch, int len) //throws IOException 
            {
                sb->append(pch, 0, len);
            }

    /**
     * The string buffer.
     */
    private:    RStringBuffer sb;
};


#endif _CORE_IO_STRINGBUFFEROUTPUTSTREAM
