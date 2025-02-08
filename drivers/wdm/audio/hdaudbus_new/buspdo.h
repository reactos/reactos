/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/hdaudbus/buspdo.h
 * PURPOSE:         HDAUDBUS Driver
 * PROGRAMMER:      Coolstar TODO
                    Johannes Anderwald
 */

#if !defined(_SKLHDAUDBUS_BUSPDO_H_)
#define _SKLHDAUDBUS_BUSPDO_H_

#define MAX_INSTANCE_ID_LEN 80

typedef struct _CODEC_UNSOLIT_CALLBACK {
    BOOLEAN inUse;
    PVOID Context;
    PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK Routine;
} CODEC_UNSOLIT_CALLBACK, *PCODEC_UNSOLICIT_CALLBACK;


typedef struct _PDO_IDENTIFICATION_DESCRIPTION
{
    PFDO_CONTEXT FdoContext;
    CODEC_IDS CodecIds;
} PDO_IDENTIFICATION_DESCRIPTION, * PPDO_IDENTIFICATION_DESCRIPTION;

#define MAX_UNSOLICIT_CALLBACKS 64 // limit is 64 for hdaudbus (See: https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/hdaudio/nc-hdaudio-pregister_event_callback)
#define SUBTAG_MASK 0x3F
#define TAG_ADDR_SHIFT 6

//
// This is PDO device-extension.
//
typedef struct _PDO_DEVICE_DATA
{
    BOOL IsFDO;
    PFDO_CONTEXT FdoContext;
    PDEVICE_OBJECT ChildPDO;
    CODEC_IDS CodecIds;

    CODEC_UNSOLIT_CALLBACK unsolitCallbacks[MAX_UNSOLICIT_CALLBACKS];

} PDO_DEVICE_DATA, * PPDO_DEVICE_DATA;

#endif
