
#include <stdarg.h>

#define COM_NO_WINDOWS_H
#include "initguid.h"

#include <wdmguid.h>
#include <umpnpmgr/sysguid.h>

/* FIXME: shouldn't go there! */
DEFINE_GUID(GUID_DEVICE_SYS_BUTTON,
  0x4AFA3D53L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57);

/* EOF */
