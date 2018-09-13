/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#include "core.hxx"
#pragma hdrstop

#include "core/io/istreaminputstream.hxx"

DEFINE_CLASS_MEMBERS(IStreamInputStream, _T("IStreamInputStream"), InputStream);

IStreamInputStream * IStreamInputStream::newIStreamInputStream(IStream *s)
{
    IStreamInputStream * is = new IStreamInputStream();
	is->stream = s;
	return is;
}

int IStreamInputStream::read()
{
	int c;
	ULONG num;

    HRESULT hr = stream->Read((void *)&c, 1 , &num);

	if (FAILED(hr) || num < 1)
		return -1;
	return c;
}