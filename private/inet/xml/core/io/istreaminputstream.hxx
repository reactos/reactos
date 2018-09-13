/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_IO_ISTREAMINPUTSTREAM
#define _CORE_IO_ISTREAMINPUTSTREAM

#ifndef _CORE_IO_INPUTSTREAM
#include "core/io/inputstream.hxx"
#endif

DEFINE_CLASS(IStreamInputStream);

/**
 */
class IStreamInputStream: public InputStream
{

    DECLARE_CLASS_MEMBERS(IStreamInputStream, InputStream);

protected:

    IStreamInputStream() : stream(REF_NOINIT) {};

public:

	static IStreamInputStream * newIStreamInputStream(IStream *s);

    int read();

    void close()
	{
        if (stream)
		{
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


#endif _CORE_IO_ISTREAMINPUTSTREAM
