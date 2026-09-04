// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------
//

//
//  Shim windows.h.  This file is included in Jolt and other clients
//  should include windows.h instead
//
//------------------------------------------------------------------------

// Common macros

#ifndef FAILED
#define FAILED(hr)      ((hr) < S_OK)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr)   ((hr) >= S_OK)
#endif

#ifndef IFC
#define IFC(x) { hr = (x); if (FAILED(hr)) goto Cleanup; }
#endif

#ifndef IFCOOM
#define IFCOOM(x) if ((x) == NULL) {hr = E_OUTOFMEMORY; goto Cleanup;}
#endif

// Success code

#ifndef S_OK
#define S_OK                        XRESULT(0x00000000L)
#endif

#ifndef S_FALSE
#define S_FALSE                     XRESULT(0x00000001L)
#endif


#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(x)   (x)
#endif

#ifndef __STDCALL
#define __STDCALL __stdcall
#endif

#ifndef UINT_MAX
#define UINT_MAX        0xffffffff
#endif

#ifndef E_FAIL
#define E_FAIL 0x80004005L
#endif

#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY 0x8007000EL
#endif 

#ifndef E_INVALIDARG
#define E_INVALIDARG 0x80070057L
#endif 

#define PSTRANS_DISASM_STRING_LENGTH 128
#define PSTRANS_MAX_TEXTURE_SAMPLERS 16

#define XFLOAT_MIN     -3.402823466e+38F        /* min value */
#define XFLOAT_MAX     3.402823466e+38F         /* max value */

#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef RRETURN
#define RRETURN(x) return(x);
#endif

