#pragma once

#include <stdarg.h>
#include <tchar.h>
#include <stdio.h>

#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#define NOMINMAX

#include <windows.h>
#include <objbase.h>
#include <objsafe.h>

#include <ocidl.h>

#include <strsafe.h>

#import "./rdesktop-core-tester.tlb" \
	exclude("LONG_PTR", "UINT_PTR", "wireHWND", "_RemotableHandle", "__MIDL_IWinTypes_0009") \
	named_guids \
	no_implementation \
	no_smart_pointers \
	raw_dispinterfaces \
	raw_interfaces_only \
	raw_native_types

// EOF
