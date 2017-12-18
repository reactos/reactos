#ifndef _GDI32_APITEST_PRECOMP_H_
#define _GDI32_APITEST_PRECOMP_H_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <apitest.h>
#include <wingdi.h>
#include <winuser.h>
#include <winddi.h>
#include <winnls.h>
#include <include/ntgdityp.h>
#include <include/ntgdihdl.h>
#include <stdio.h>
#include <strsafe.h>

#define CLIPRGN 1

#define TEST(x) ok(x, #x"\n")
#define RTEST(x) ok(x, #x"\n")

#define ok_rect(_prc, _left, _top, _right, _bottom) \
    ok_int((_prc)->left, _left); \
    ok_int((_prc)->top, _top); \
    ok_int((_prc)->right, _right); \
    ok_int((_prc)->bottom, _bottom); \

#endif /* _GDI32_APITEST_PRECOMP_H_ */
