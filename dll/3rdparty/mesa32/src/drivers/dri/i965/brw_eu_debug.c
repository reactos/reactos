/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
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
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
    

#include "mtypes.h"
#include "brw_eu.h"
#include "imports.h"

void brw_print_reg( struct brw_reg hwreg )
{
   static const char *file[] = {
      "arf",
      "grf",
      "msg",
      "imm"
   };

   static const char *type[] = {
      "ud",
      "d",
      "uw",
      "w",
      "ub",
      "vf",
      "hf",
      "f"
   };

   _mesa_printf("%s%s", 
		hwreg.abs ? "abs/" : "",
		hwreg.negate ? "-" : "");
     
   if (hwreg.file == BRW_GENERAL_REGISTER_FILE &&
       hwreg.nr % 2 == 0 &&
       hwreg.subnr == 0 &&
       hwreg.vstride == BRW_VERTICAL_STRIDE_8 &&
       hwreg.width == BRW_WIDTH_8 &&
       hwreg.hstride == BRW_HORIZONTAL_STRIDE_1 &&
       hwreg.type == BRW_REGISTER_TYPE_F) {
      _mesa_printf("vec%d", hwreg.nr);
   }
   else if (hwreg.file == BRW_GENERAL_REGISTER_FILE &&
	    hwreg.vstride == BRW_VERTICAL_STRIDE_0 &&
	    hwreg.width == BRW_WIDTH_1 &&
	    hwreg.hstride == BRW_HORIZONTAL_STRIDE_0 &&
	    hwreg.type == BRW_REGISTER_TYPE_F) {      
      _mesa_printf("scl%d.%d", hwreg.nr, hwreg.subnr / 4);
   }
   else {
      _mesa_printf("%s%d.%d<%d;%d,%d>:%s", 
		   file[hwreg.file],
		   hwreg.nr,
		   hwreg.subnr / type_sz(hwreg.type),
		   hwreg.vstride ? (1<<(hwreg.vstride-1)) : 0,
		   1<<hwreg.width,
		   hwreg.hstride ? (1<<(hwreg.hstride-1)) : 0,		
		   type[hwreg.type]);
   }
}



