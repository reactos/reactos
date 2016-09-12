#ifndef _MUPTYPES_H
#define _MUPTYPES_H

//
// Dependencies
//
#include <umtypes.h>

//
// FSCTL sent to MUP for UNC provider registration
//
#define FSCTL_MUP_REGISTER_PROVIDER CTL_CODE(FILE_DEVICE_MULTI_UNC_PROVIDER, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Input buffer for FSCTL_MUP_REGISTER_PROVIDER
//
typedef struct _MUP_PROVIDER_REGISTRATION_INFO
{
    ULONG RedirectorDeviceNameOffset;
    ULONG RedirectorDeviceNameLength;
    ULONG Reserved[2];
    BOOLEAN MailslotsSupported;
} MUP_PROVIDER_REGISTRATION_INFO, *PMUP_PROVIDER_REGISTRATION_INFO;

#endif // _MUPTYPES_H
