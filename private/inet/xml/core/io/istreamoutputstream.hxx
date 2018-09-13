/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_IO_ISTREAMOUTPUTSTREAM
#define _CORE_IO_ISTREAMOUTPUTSTREAM

#ifndef _CORE_IO_OUTPUTSTREAM
#include "core/io/outputstream.hxx"
#endif

DEFINE_CLASS(IStreamOutputStream);

/**
 */
class IStreamOutputStream: public OutputStream
{

    DECLARE_CLASS_MEMBERS(IStreamOutputStream, OutputStream);

protected:

	IStreamOutputStream(): stream(REF_NOINIT) {}

public:

	static IStreamOutputStream * newIStreamOutputStream(IStream *s);

    void write(int b);

    void flush()
	{
        stream->Commit(0);
	}
 
    void close()
    {
        if (stream)
		{
			flush();
			stream = null;
		}
    }

private:

	_reference<IStream> stream;

protected:

	virtual void finalize()
	{
		close();
		super::finalize();
	}
};


#endif _CORE_IO_ISTREAMOUTPUTSTREAM
