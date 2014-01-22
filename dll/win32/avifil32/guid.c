/* DO NOT USE THE PRECOMPILED HEADER FOR THIS FILE! */

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <initguid.h>
#include <vfw.h>

/* editstream.c */
DEFINE_AVIGUID(IID_IEditStreamInternal, 0x0002000A,0,0);

/* avifile_private.h */
DEFINE_AVIGUID(CLSID_ICMStream, 0x00020001, 0, 0);
DEFINE_AVIGUID(CLSID_WAVFile,   0x00020003, 0, 0);
DEFINE_AVIGUID(CLSID_ACMStream, 0x0002000F, 0, 0);

/* NO CODE HERE, THIS IS JUST REQUIRED FOR THE GUID DEFINITIONS */
