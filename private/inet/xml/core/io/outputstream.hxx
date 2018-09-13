/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_IO_OUTPUTSTREAM
#define _CORE_IO_OUTPUTSTREAM


DEFINE_CLASS(OutputStream);

/**
 */
class NOVTABLE OutputStream: public Base
{

    DECLARE_CLASS_MEMBERS(OutputStream, Base);

    /**
     */
    public: virtual void write(int b) = 0; //throws IOException;

    /**
     */
    public: virtual void write(abyte * b) //throws IOException 
            {
                write(b, 0, b->length());
            }

    /**
     */
    public: virtual void write(abyte * b, int off, int len) //throws IOException 
            {
                for (int i = 0 ; i < len ; i++) 
                {
                    write((*b)[off + i]);
                }
            }

    /**
     */
    public: virtual void write(ATCHAR * b) //throws IOException 
            {
                write(b, 0, b->length());
            }

    /**
     */
    public: virtual void write(ATCHAR * b, int off, int len) //throws IOException 
            {
                for (int i = 0 ; i < len ; i++) 
                {
                    write((*b)[off + i]);
                }
            }

    /**
     */
    public: virtual void write(TCHAR * pch, int len) //throws IOException 
            {
                while (len-- > 0) 
                {
                    write(*pch++);
                }
            }

    /**
     */
    public: virtual void flush() //throws IOException 
            {
            }

    /**
     */
    public: virtual void close() //throws IOException 
            {
            }
};


#endif _CORE_IO_OUTPUTSTREAM
