#include <windows.h>
#include "systrayp.h"
#include "trayvol.h"

#define MMSYS_UPDATEMIXER	3000	//This message tells the systray that the preferred device has
									//changed.

// Helper functions for things we care about
BOOL SetTrayVolumeEnabled(BOOL bEnable)
{
    return SysTray_EnableService(STSERVICE_VOLUME,bEnable);
}

BOOL GetTrayVolumeEnabled(void)
{
    return SysTray_IsServiceEnabled(STSERVICE_VOLUME);
}
