#ifndef __WINE_SERVPROV_H
#define __WINE_SERVPROV_H

#include <rpc.h>
#include <rpcndr.h>
#include_next <servprov.h>

DEFINE_GUID(IID_IServiceProvider, 0x6d5140c1, 0x7436, 0x11ce, 0x80,0x34, 0x00,0xaa,0x00,0x60,0x09,0xfa);

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

/*** IUnknown methods ***/
#define IServiceProvider_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IServiceProvider_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IServiceProvider_Release(p) (p)->lpVtbl->Release(p)
/*** IServiceProvider methods ***/
#define IServiceProvider_QueryService(p,a,b,c) (p)->lpVtbl->QueryService(p,a,b,c)

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

#endif  /* __WINE_SERVPROV_H */
