/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#ifndef _ARRAY_HXX
#    include "_array.hxx"
#endif

DEFINE_ABSTRACT_CLASS_MEMBERS(__array, _T("_array"), Base);

void * 
__array::fetch(int at, int m)
{
    if (at < 0 || at >= _length)
    {
        Exception::throwE(E_INVALIDARG); // Exception::ArrayIndexOutOfBoundsException);
    }
    return _data + at * m;
}

void
__array::indexError()
{
    Exception::throwE(E_INVALIDARG); // Exception::ArrayIndexOutOfBoundsException);
}

void
__array::copy(int offset, int count, const __array * src, int src_offset)
{
    if (src_offset + count > src->_length ||
		offset + count > _length) 
	{
        Exception::throwE(E_INVALIDARG); // Exception::ArrayIndexOutOfBoundsException);
 	}

	int m = getDataSize();
    // BUGBUG get rid of typecast
	BYTE * s =  (BYTE *) (src->_data + m * src_offset);
	BYTE * d =  _data + m * offset;
	// check if we copy the same area...
	if (src == this && s < d)
	{
		count--;
		s = s + count * m;
		d = d + count * m;
		for (int i = count; i >= 0; i--)
		{
			assign(d, s);
			d -= m;
			s -= m;
		}
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			assign(d, s);
			d += m;
			s += m;
		}
	}
}
