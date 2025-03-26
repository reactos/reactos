
#ifndef _COMCTL32_WINETEST_PRECOMP_H_
#define _COMCTL32_WINETEST_PRECOMP_H_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define COBJMACROS
#define CONST_VTABLE

#include <stdio.h>

#include <wine/test.h>
#include <assert.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>
#include <objbase.h>
#include <wine/commctrl.h>

#include "msg.h"
#include "resources.h"
#include "v6util.h"

#ifdef __REACTOS__
#include <ole2.h>

#define WM_KEYF1 0x004d
#define WM_CTLCOLOR 0x0019

#define WC_DIALOG       (MAKEINTATOM(0x8002))
#endif

#endif /* !_COMCTL32_WINETEST_PRECOMP_H_ */
