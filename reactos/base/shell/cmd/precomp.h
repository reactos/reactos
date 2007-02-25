#ifdef _MSC_VER
#pragma warning ( disable : 4103 ) /* use #pragma pack to change alignment */
#undef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif//_MSC_VER

#include <stdlib.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winnt.h>
#include <shellapi.h>

#include <tchar.h>
#include <direct.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include "cmd.h"
#include "config.h"
#include "batch.h"

