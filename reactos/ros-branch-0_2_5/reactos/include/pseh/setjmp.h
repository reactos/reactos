/*
 Copyright (c) 2004 KJK::Hyperion
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef KJK_PSEH_SETJMP_H_
#define KJK_PSEH_SETJMP_H_

#ifdef _M_IX86
typedef struct __SEHJmpBuf
{
 unsigned long JB_Ebp;
 unsigned long JB_Esp;
 unsigned long JB_Eip;
 unsigned long JB_Ebx;
 unsigned long JB_Esi;
 unsigned long JB_Edi;
}
_SEHJmpBuf_t[1];
#endif

extern __declspec(noreturn) void __stdcall _SEHLongJmp(_SEHJmpBuf_t, int);
extern int __stdcall _SEHSetJmp(_SEHJmpBuf_t);

#endif

/* EOF */
