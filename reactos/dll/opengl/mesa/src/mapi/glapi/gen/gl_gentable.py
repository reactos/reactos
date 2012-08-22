#!/usr/bin/env python

# (C) Copyright IBM Corporation 2004, 2005
# (C) Copyright Apple Inc. 2011
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# on the rights to use, copy, modify, merge, publish, distribute, sub
# license, and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
# IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Authors:
#    Jeremy Huddleston <jeremyhu@apple.com>
#
# Based on code ogiginally by:
#    Ian Romanick <idr@us.ibm.com>

import license
import gl_XML, glX_XML
import sys, getopt

header = """/* GLXEXT is the define used in the xserver when the GLX extension is being
 * built.  Hijack this to determine whether this file is being built for the
 * server or the client.
 */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#if (defined(GLXEXT) && defined(HAVE_BACKTRACE)) \\
	|| (!defined(GLXEXT) && defined(DEBUG) && !defined(_WIN32_WCE))
#define USE_BACKTRACE
#endif

#ifdef USE_BACKTRACE
#include <execinfo.h>
#endif

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

#include <GL/gl.h>

#include "glapi.h"
#include "glapitable.h"

#ifdef GLXEXT
#include "os.h"
#endif

static void
__glapi_gentable_NoOp(void) {
    const char *fstr = "Unknown";

    /* Silence potential GCC warning for some #ifdef paths.
     */
    (void) fstr;
#if defined(USE_BACKTRACE)
#if !defined(GLXEXT)
    if (getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG"))
#endif
    {
        void *frames[2];

        if(backtrace(frames, 2) == 2) {
            Dl_info info;
            dladdr(frames[1], &info);
            if(info.dli_sname)
                fstr = info.dli_sname;
        }

#if !defined(GLXEXT)
        fprintf(stderr, "Call to unimplemented API: %s\\n", fstr);
#endif
    }
#endif
#if defined(GLXEXT)
    LogMessage(X_ERROR, "GLX: Call to unimplemented API: %s\\n", fstr);
#endif
}

static void
__glapi_gentable_set_remaining_noop(struct _glapi_table *disp) {
    GLuint entries = _glapi_get_dispatch_table_size();
    void **dispatch = (void **) disp;
    int i;

    /* ISO C is annoying sometimes */
    union {_glapi_proc p; void *v;} p;
    p.p = __glapi_gentable_NoOp;

    for(i=0; i < entries; i++)
        if(dispatch[i] == NULL)
            dispatch[i] = p.v;
}

struct _glapi_table *
_glapi_create_table_from_handle(void *handle, const char *symbol_prefix) {
    struct _glapi_table *disp = calloc(1, sizeof(struct _glapi_table));
    char symboln[512];

    if(!disp)
        return NULL;

    if(symbol_prefix == NULL)
        symbol_prefix = "";
"""

footer = """
    __glapi_gentable_set_remaining_noop(disp);

    return disp;
}
"""

body_template = """
    if(!disp->%(name)s) {
        void ** procp = (void **) &disp->%(name)s;
        snprintf(symboln, sizeof(symboln), "%%s%(entry_point)s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }
"""

class PrintCode(gl_XML.gl_print_base):

	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "gl_gen_table.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004, 2005
(C) Copyright Apple Inc 2011""", "BRIAN PAUL, IBM")

		return


	def get_stack_size(self, f):
		size = 0
		for p in f.parameterIterator():
			if p.is_padding:
				continue

			size += p.get_stack_size()

		return size


	def printRealHeader(self):
		print header
		return


	def printRealFooter(self):
		print footer
		return


	def printBody(self, api):
		for f in api.functionIterateByOffset():
			for entry_point in f.entry_points:
				vars = { 'entry_point' : entry_point,
				         'name' : f.name }

				print body_template % vars
		return

def show_usage():
	print "Usage: %s [-f input_file_name]" % sys.argv[0]
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "m:f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == "-f":
			file_name = val

	printer = PrintCode()

	api = gl_XML.parse_GL_API(file_name, glX_XML.glx_item_factory())
	printer.Print(api)
