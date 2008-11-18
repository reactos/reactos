#! /usr/bin/env python2.2

# Copyright 1994 by Lance Ellinghouse
# Cathedral City, California Republic, United States of America.
#                        All Rights Reserved
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice appear in all copies and that
# both that copyright notice and this permission notice appear in
# supporting documentation, and that the name of Lance Ellinghouse
# not be used in advertising or publicity pertaining to distribution
# of the software without specific, written prior permission.
# LANCE ELLINGHOUSE DISCLAIMS ALL WARRANTIES WITH REGARD TO
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS, IN NO EVENT SHALL LANCE ELLINGHOUSE CENTRUM BE LIABLE
# FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
# OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# Modified by Jack Jansen, CWI, July 1995:
# - Use binascii module to do the actual line-by-line conversion
#   between ascii and binary. This results in a 1000-fold speedup. The C
#   version is still 5 times faster, though.
# - Arguments more compliant with python standard

"""Implementation of the UUencode and UUdecode functions.

encode(in_file, out_file [,name, mode])
decode(in_file [, out_file, mode])
"""

import binascii
import os
import sys
from types import StringType

__all__ = ["Error", "decode"]

class Error(Exception):
    pass

def decode(in_file, out_file=None, mode=None, quiet=0):
    """Decode uuencoded file"""
    #
    # Open the input file, if needed.
    #
    if in_file == '-':
        in_file = sys.stdin
    elif isinstance(in_file, StringType):
        in_file = open(in_file)
    #
    # Read until a begin is encountered or we've exhausted the file
    #
    while 1:
        hdr = in_file.readline()
        if not hdr:
            raise Error, 'No valid begin line found in input file'
        if hdr[:5] != 'begin':
            continue
        hdrfields = hdr.split(" ", 2)
        if len(hdrfields) == 3 and hdrfields[0] == 'begin':
            try:
                int(hdrfields[1], 8)
                start_pos = in_file.tell() - len (hdr)
                break
            except ValueError:
                pass
    if out_file is None:
        out_file = hdrfields[2].rstrip()
        if os.path.exists(out_file):
            raise Error, 'Cannot overwrite existing file: %s' % out_file
    if mode is None:
        mode = int(hdrfields[1], 8)
    #
    # Open the output file
    #
    if out_file == '-':
        out_file = sys.stdout
    elif isinstance(out_file, StringType):
        fp = open(out_file, 'wb')
        try:
            os.path.chmod(out_file, mode)
        except AttributeError:
            pass
        out_file = fp
    #
    # Main decoding loop
    #
    s = in_file.readline()
    while s and s.strip() != 'end':
        try:
            data = binascii.a2b_uu(s)
        except binascii.Error, v:
            # Workaround for broken uuencoders by /Fredrik Lundh
            nbytes = (((ord(s[0])-32) & 63) * 4 + 5) / 3
            data = binascii.a2b_uu(s[:nbytes])
            if not quiet:
                sys.stderr.write("Warning: %s\n" % str(v))
        out_file.write(data)
        s = in_file.readline()
#    if not s:
 #       raise Error, 'Truncated input file'
    return (hdrfields[2].rstrip(), start_pos, in_file.tell())
