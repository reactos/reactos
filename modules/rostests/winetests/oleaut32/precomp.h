
#ifndef _OLEAUT32_WINETEST_PRECOMP_H_
#define _OLEAUT32_WINETEST_PRECOMP_H_

#include <stdio.h>
#include <math.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>

#include <winreg.h>
#include <winnls.h>
#include <wingdi.h>
#include <ole2.h>
#include <olectl.h>
#include <tmarshal.h>
#include <test_tlb.h>

#endif /* !_OLEAUT32_WINETEST_PRECOMP_H_ */
