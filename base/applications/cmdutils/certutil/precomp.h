#pragma once

/* INCLUDES ******************************************************************/

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <strsafe.h>

#include <conutils.h>


BOOL hash_file(LPCWSTR Filename);
BOOL asn_dump(LPCWSTR Filename);
