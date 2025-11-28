#if !defined(_SKLHDAUDBUS_BUSPDO_H_)
#define _SKLHDAUDBUS_BUSPDO_H_

#define MAX_INSTANCE_ID_LEN 80

typedef struct _CODEC_UNSOLIT_CALLBACK {
    BOOLEAN inUse;
    PVOID Context;
    PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK Routine;
} CODEC_UNSOLIT_CALLBACK, *PCODEC_UNSOLICIT_CALLBACK;

typedef struct _CODEC_IDS {
    UINT32 CodecAddress;
    BOOL IsGraphicsCodec;

    UINT8 FunctionGroupStartNode;

    UINT16 CtlrDevId;
    UINT16 CtlrVenId;

    BOOL IsDSP;

    UINT16 FuncId;
    UINT16 VenId;
    UINT16 DevId;
    UINT32 SubsysId;
    UINT16 RevId;
} CODEC_IDS, * PCODEC_IDS;

typedef struct _PDO_IDENTIFICATION_DESCRIPTION
{
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header; // should contain this header

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
    PFDO_CONTEXT FdoContext;

    CODEC_IDS CodecIds;

    CODEC_UNSOLIT_CALLBACK unsolitCallbacks[MAX_UNSOLICIT_CALLBACKS];

} PDO_DEVICE_DATA, * PPDO_DEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PDO_DEVICE_DATA, PdoGetData)

extern "C" {

    NTSTATUS NTAPI
        Bus_EvtChildListIdentificationDescriptionDuplicate(
            WDFCHILDLIST DeviceList,
            PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SourceIdentificationDescription,
            PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER DestinationIdentificationDescription
        );

    BOOLEAN NTAPI
        Bus_EvtChildListIdentificationDescriptionCompare(
            WDFCHILDLIST DeviceList,
            PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER FirstIdentificationDescription,
            PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SecondIdentificationDescription
        );

    VOID NTAPI
        Bus_EvtChildListIdentificationDescriptionCleanup(
            _In_ WDFCHILDLIST DeviceList,
            _Inout_ PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
        );

    NTSTATUS NTAPI
        Bus_EvtDeviceListCreatePdo(
            WDFCHILDLIST DeviceList,
            PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
            PWDFDEVICE_INIT ChildInit
        );

}

#endif
