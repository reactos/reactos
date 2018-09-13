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

#include "core/io/stringbufferoutputstream.hxx"

DEFINE_CLASS_MEMBERS(StringBufferOutputStream, _T("StringBufferOutputStream"), OutputStream);

StringBufferOutputStream * 
StringBufferOutputStream::newStringBufferOutputStream(StringBuffer * sb)
{
    StringBufferOutputStream * s = new StringBufferOutputStream();
    s->sb = sb;
    return s;
}
