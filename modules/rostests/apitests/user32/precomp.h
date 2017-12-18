#ifndef _USER32_APITEST_PRECOMP_H_
#define _USER32_APITEST_PRECOMP_H_

#include <assert.h>
#include <stdio.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define WIN32_NO_STATUS

#include <apitest.h>
#include <wingdi.h>
#include <winuser.h>
#include <msgtrace.h>
#include <user32testhelpers.h>
#include <undocuser.h>

#include "resource.h"

#define DESKTOP_ALL_ACCESS 0x01ff

#endif /* _USER32_APITEST_PRECOMP_H_ */
