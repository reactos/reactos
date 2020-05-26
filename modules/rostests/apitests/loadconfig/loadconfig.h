#pragma once

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <apitest.h>
#include <apitest_guard.h>
#include <ndk/ntndk.h>
#include <strsafe.h>


/* common.c */
BOOL check_loadconfig();
