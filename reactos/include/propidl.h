#include <rpc.h>
#include <rpcndr.h>

#ifndef _PROPIDL_H
#define _PROPIDL_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#include <objidl.h>

HRESULT WINAPI FreePropVariantArray(ULONG cVariants, PROPVARIANT *rgvars);

#endif
