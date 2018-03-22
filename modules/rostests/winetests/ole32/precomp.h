
#ifndef _OLE32_WINETEST_PRECOMP_H_
#define _OLE32_WINETEST_PRECOMP_H_

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>

#include <winnls.h>
#include <winreg.h>
#include <wingdi.h>
#define USE_COM_CONTEXT_DEF
#include <ole2.h>

#endif /* !_OLE32_WINETEST_PRECOMP_H_ */
