/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/

#include "intel_batchbuffer.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_hal.h"
#include <dlfcn.h>

static void *brw_hal_lib;
static GLboolean brw_hal_tried;

void *
brw_hal_find_symbol (char *symbol)
{
    if (!brw_hal_tried)
    {
	char *brw_hal_name = getenv ("INTEL_HAL");
    
	if (!brw_hal_name)
	    brw_hal_name = "/usr/lib/xorg/modules/drivers/intel_hal.so";

	brw_hal_lib = dlopen (brw_hal_name, RTLD_LAZY|RTLD_LOCAL);
	brw_hal_tried = 1;
    }
    if (!brw_hal_lib)
	return NULL;
    return dlsym (brw_hal_lib, symbol);
}
