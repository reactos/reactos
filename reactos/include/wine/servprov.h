#ifndef __WINE_SERVPROV_H
#define __WINE_SERVPROV_H

#include <rpc.h>
#include <rpcndr.h>
#include_next <servprov.h>

DEFINE_GUID(IID_IServiceProvider, 0x6d5140c1, 0x7436, 0x11ce, 0x80,0x34, 0x00,0xaa,0x00,0x60,0x09,0xfa);

#endif  /* __WINE_SERVPROV_H */
