
#ifndef _MSVCRT_WINETEST_PRECOMP_H_
#define _MSVCRT_WINETEST_PRECOMP_H_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#if !defined(_CRT_NON_CONFORMING_SWPRINTFS)
#define _CRT_NON_CONFORMING_SWPRINTFS
#endif

#include <wine/test.h>

#include <stdio.h>
#include <winnls.h>
#include <process.h>
#include <locale.h>

#endif /* !_MSVCRT_WINETEST_PRECOMP_H_ */
