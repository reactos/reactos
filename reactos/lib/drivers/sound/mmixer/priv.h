#ifndef PRIV_H__
#define PRIV_H__

#include <pseh/pseh2.h>
#include <ntddk.h>

#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmreg.h>
#include <mmsystem.h>

#include "mmixer.h"

typedef struct
{
    MIXERCAPSW    MixCaps;
    HANDLE        hMixer;
    LIST_ENTRY    LineList;
    ULONG         ControlId;
}MIXER_INFO, *LPMIXER_INFO;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG PinId;
    ULONG DeviceIndex;
    MIXERLINEW Line;
    LPMIXERCONTROLW LineControls;
    PULONG          NodeIds;
    LIST_ENTRY LineControlsExtraData;
}MIXERLINE_EXT, *LPMIXERLINE_EXT;

#define DESTINATION_LINE 0xFFFF0000

#endif
