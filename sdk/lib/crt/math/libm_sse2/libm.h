/***********************************************************************************/
/** MIT License **/
/** ----------- **/
/** **/  
/** Copyright (c) 2002-2019 Advanced Micro Devices, Inc. **/
/** **/
/** Permission is hereby granted, free of charge, to any person obtaining a copy **/
/** of this Software and associated documentaon files (the "Software"), to deal **/
/** in the Software without restriction, including without limitation the rights **/
/** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell **/
/** copies of the Software, and to permit persons to whom the Software is **/
/** furnished to do so, subject to the following conditions: **/
/** **/ 
/** The above copyright notice and this permission notice shall be included in **/
/** all copies or substantial portions of the Software. **/
/** **/
/** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR **/
/** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, **/
/** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE **/
/** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER **/
/** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, **/
/** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN **/
/** THE SOFTWARE. **/
/***********************************************************************************/

#ifndef LIBM_AMD_H_INCLUDED
#define LIBM_AMD_H_INCLUDED 1

#define FN_PROTOTYPE(fname) fname

#include <math.h>
#include <fpieee.h>

#ifndef IS_64BIT
#define IS_64BIT
#endif

#ifndef _COMPLEX_DEFINED
struct _complex
{
  double x, y; /* real and imaginary parts */
};
#define _COMPLEX_DEFINED
#endif
#define COMPLEX struct _complex

extern void __remainder_piby2(double x, double *r, double *rr, int *region);

#endif /* LIBM_AMD_H_INCLUDED */
