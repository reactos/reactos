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

#include "core/io/istreamoutputstream.hxx"

DEFINE_CLASS_MEMBERS(IStreamOutputStream, _T("IStreamOutputStream"), OutputStream);

IStreamOutputStream * IStreamOutputStream::newIStreamOutputStream(IStream *s)
{
    IStreamOutputStream * os = new IStreamOutputStream();
	os->stream = s;
	return os;
}

void IStreamOutputStream::write(int b)
{
    UINT bytetotal = 2;
    ULONG l;

    stream->Write((void *)&b, bytetotal, &l);
}