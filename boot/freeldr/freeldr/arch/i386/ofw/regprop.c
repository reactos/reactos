// See license at end of file

/* For gcc, compile with -fno-builtin to suppress warnings */

#include "1275.h"

reg *
decode_reg(UCHAR *buf, int buflen)
{
	static reg staticreg;
	reg *sregp;
	int i;

	if (buflen)
		staticreg.hi = decode_int(buf);
	if (buflen > 4)
		staticreg.lo = decode_int(buf+4);
	if (buflen > 8)
		staticreg.size = decode_int(buf+8);
	return (sregp = &staticreg);
}

reg *
get_reg_prop(phandle node, char *key)
{
	int res;
	char *buf;
	reg *regp;
	int len = OFGetproplen(node, key);

	buf = (char *)malloc(len);
	res = OFGetprop(node, key, buf, len);
	if (res != len) {
		fatal("get_reg_prop(node %x, key '%s', len %x) returned %x\n",
		    node, key, len, res);
		return ((reg *) 0);
	}
	regp = decode_reg(buf, len);
	free(buf);
	return (regp);
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
