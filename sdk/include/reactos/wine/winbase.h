
#pragma once

#include <psdk/winbase.h>

/* Wine specific. Basically MultiByteToWideChar for us. */
WCHAR * CDECL wine_get_dos_file_name(LPCSTR str);
