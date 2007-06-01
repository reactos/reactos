// See license at end of file

/* For gcc, compile with -fno-builtin to suppress warnings */

#include "1275.h"

int
decode_int(UCHAR *p)
{
	ULONG   i = *p++  << 8;
	i =    (i + *p++) << 8;
	i =    (i + *p++) << 8;
	return (i + *p);
}

int
get_int_prop(phandle node, char *key)
{
	int res;
	char buf[sizeof(int)];

	res = OFGetprop(node, key, buf, sizeof(int));
	if (res != sizeof(int)) {
#ifdef notdef
		fatal("get_int_prop(node %x, key '%s') returned %x\n",
		    node, key, res);
#endif
		return(-1);
	}
	return(decode_int((UCHAR *) buf));
}

int
get_int_prop_def(phandle node, char *key, int defval)
{
	int res;
	char buf[sizeof(int)];

	res = OFGetprop(node, key, buf, sizeof(int));
	if (res != sizeof(int)) {
		return(defval);
	}
	return(decode_int((UCHAR *) buf));
}

// LICENSE_BEGIN
// Copyright (c) 2006 FirmWorks
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// LICENSE_END
