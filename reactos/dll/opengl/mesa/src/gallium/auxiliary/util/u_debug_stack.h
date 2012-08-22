/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef U_DEBUG_STACK_H_
#define U_DEBUG_STACK_H_


/**
 * @file
 * Stack backtracing.
 * 
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#ifdef	__cplusplus
extern "C" {
#endif


struct debug_stack_frame 
{
   const void *function;
};
   

void
debug_backtrace_capture(struct debug_stack_frame *backtrace,
                        unsigned start_frame, 
                        unsigned nr_frames);

void
debug_backtrace_dump(const struct debug_stack_frame *backtrace, 
                     unsigned nr_frames);


#ifdef	__cplusplus
}
#endif

#endif /* U_DEBUG_STACK_H_ */
