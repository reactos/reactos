#pragma once

#include <stdarg.h>
#include <tchar.h>
#include <stdio.h>

#include <string>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#define NOMINMAX

#include <windows.h>
#include <objbase.h>
#include <objsafe.h>

#include <strsafe.h>

//#pragma warning(push)
//#pragma warning(disable:4192)
#import <mstscax.dll> named_guids exclude("LONG_PTR", "UINT_PTR")

//#pragma warning(pop)

// EOF
