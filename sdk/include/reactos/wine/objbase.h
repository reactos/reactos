
#pragma once

#include <rpc.h>
#include <rpcndr.h>

#if !defined(__cplusplus) || defined(CINTERFACE)
#ifndef CONST_VTABLE
#define CONST_VTABLE
#endif
#endif /* !defined(__cplusplus) || defined(CINTERFACE) */

#ifndef NOURLMON
#define NOURLMON
#endif

#ifndef NOPROPIDL
#define NOPROPIDL
#endif

#include <psdk/wtypes.h>
#include <psdk/objbase.h>

HRESULT WINAPI DllRegisterServer(void) DECLSPEC_HIDDEN;
HRESULT WINAPI DllUnregisterServer(void) DECLSPEC_HIDDEN;
