/*
    ReactOS Operating System
    Sound Blaster KS Driver

    AUTHORS:
        Andrew Greenwood

    NOTES:
        WaveTable is not supported.
*/

#include <sb16.h>

/* How many miniports do we support? */
#define MAX_MINIPORTS 1


typedef struct
{
    PRESOURCELIST Wave;
    PRESOURCELIST WaveTable;
    PRESOURCELIST FmSynth;
    PRESOURCELIST Uart;
    PRESOURCELIST Adapter;
} Resources;


DWORD
DetectPlatform(
    PPORTTOPOLOGY Port)
{
    /* ASSERT(Port); */

#if 0
    PPORTCLSVERSION portcls_version;
    PDRMPORT drm_port;
    PPORTEVENTS port_events;
    DWORD version;

/*
    TODO: This stuff needs IID impls

    Port->QueryInterface( IID_IPortClsVersion, (PVOID*) &portcls_version);
    Port->QueryInterface( IID_IDrmPort, (PVOID*) &drm_port);
    Port->QueryInterface( IID_IPortEvents, (PVOID*) &port_events);
*/

    if ( portcls_version )
    {
        version = portcls_version->GetVersion();
        portcls_version->Release();
    }

    /* If we don't support portcls' GetVersion, we can try other methods */
    else if ( drm_port )
    {
        version = kVersionWinME;
        // ASSERT(IoIsWdmVersionAvailable(0x01, 0x05));
    }

    /* If portcls GetVersion and DRMPort not supported, it'll be Win98 */
    else if ( port_events )
    {
        version = kVersionWin98SE;
    }

    /* IPortEvents was added in Win 98 SE so if not supported, it's not 98 SE */
    else
    {
        version = kVersionWin98;
    }

    return version;
#else
    return kVersionWin98;
#endif
}


NTSTATUS
DetectFeatures(
    IN  PRESOURCELIST ResourceList,
    OUT PBOOLEAN HasUart,
    OUT PBOOLEAN HasFmSynth,
    OUT PBOOLEAN HasWaveTable)
{
    NTSTATUS status = STATUS_SUCCESS;

    BOOLEAN DetectedWaveTable = FALSE;
    BOOLEAN DetectedUart = FALSE;
    BOOLEAN DetectedFmSynth = FALSE;

    ULONG IoCount = ResourceList->NumberOfPorts();
    ULONG IrqCount = ResourceList->NumberOfInterrupts();
    ULONG DmaCount = ResourceList->NumberOfDmas();

    switch ( IoCount )
    {
        case 1 :    /* No FM / UART */
        {
            if ( ( ResourceList->FindTranslatedPort(0)->u.Port.Length < 16 ) ||
                 ( IrqCount < 1 ) ||
                 ( DmaCount < 1 ) )
            {
                status = STATUS_DEVICE_CONFIGURATION_ERROR;
            }

            break;
        }

        case 2 :
        {
            if ( ( ResourceList->FindTranslatedPort(0)->u.Port.Length < 16 ) ||
                 ( IrqCount < 1 ) ||
                 ( DmaCount < 1 ) )
            {
                status = STATUS_DEVICE_CONFIGURATION_ERROR;
            }

            else
            {
                /* The length of the port indicates the function provided */

                switch ( ResourceList->FindTranslatedPort(1)->u.Port.Length )
                {
                    case 2 :
                    {
                        DetectedUart = TRUE;
                        break;
                    }
                    case 4:
                    {
                        DetectedFmSynth = TRUE;
                        break;
                    }
                    default :
                    {
                        status = STATUS_DEVICE_CONFIGURATION_ERROR;
                    }
                }
            }

            break;
        }

        case 3 :
        {
            if ( ( ResourceList->FindTranslatedPort(0)->u.Port.Length < 16 ) ||
                 ( ResourceList->FindTranslatedPort(1)->u.Port.Length != 2 ) ||
                 ( ResourceList->FindTranslatedPort(2)->u.Port.Length != 4 ) ||
                 ( IrqCount < 1 ) ||
                 ( DmaCount < 1 ) )
            {
                status = STATUS_DEVICE_CONFIGURATION_ERROR;
            }
            else
            {
                DetectedUart = TRUE;
                DetectedFmSynth = TRUE;
            }

            break;
        }

        default :
        {
            status = STATUS_DEVICE_CONFIGURATION_ERROR;
            break;
        }
    }

    if ( HasUart )
        *HasUart = DetectedUart;
    if ( HasFmSynth )
        *HasFmSynth = DetectedFmSynth;
    if ( HasWaveTable )
        *HasWaveTable = DetectedWaveTable;

    return status;
}


NTSTATUS
AssignResources(
    IN  PRESOURCELIST ResourceList,
    OUT Resources* Resources)
{
    NTSTATUS status;
    BOOLEAN HasUart, HasFmSynth, HasWaveTable;

    Resources->Adapter = NULL;
    Resources->Wave = NULL;
    Resources->Uart = NULL;
    Resources->FmSynth = NULL;
    Resources->WaveTable = NULL;

    status = DetectFeatures(ResourceList, &HasUart, &HasFmSynth, &HasWaveTable);

    if ( ! NT_SUCCESS(status) )
    {
        return status;
    }

    /* Wave I/O resources */

    status = PcNewResourceSublist(&Resources->Wave,
                                  NULL,
                                  PagedPool,
                                  ResourceList,
                                  ResourceList->NumberOfDmas() +
                                      ResourceList->NumberOfInterrupts() + 1);

    if ( NT_SUCCESS(status) )
    {
        ULONG i;

        /* Base port address */
        status = (*Resources->Wave).AddPortFromParent(ResourceList, 0);

        /* DMA channels */
        if ( NT_SUCCESS(status) )
        {
            for ( i = 0; i < ResourceList->NumberOfDmas(); i ++ )
            {
                status = (*Resources->Wave).AddDmaFromParent(ResourceList, i);

                if ( ! NT_SUCCESS(status) )
                    break;
            }
        }

        /* IRQs */
        if ( NT_SUCCESS(status) )
        {
            for ( i = 0; i < ResourceList->NumberOfInterrupts(); i ++ )
            {
                status = (*Resources->Wave).AddInterruptFromParent(ResourceList, i);

                if ( ! NT_SUCCESS(status) )
                    break;
            }
        }
    }

    /* UART resources */

    if ( NT_SUCCESS(status) && HasUart )
    {
        /* TODO */
    }

    /* FM Synth resources */

    if ( NT_SUCCESS(status) && HasFmSynth )
    {
        /* TODO */
    }

    /* Adapter resources */

    if ( NT_SUCCESS(status) )
    {
        status = PcNewResourceSublist(&Resources->Adapter,
                                      NULL,
                                      PagedPool,
                                      ResourceList,
                                      3);

        if ( NT_SUCCESS(status) )
        {
            status = (*Resources->Adapter).AddInterruptFromParent(ResourceList, 0);
        }

        if ( NT_SUCCESS(status) )
        {
            status = (*Resources->Adapter).AddPortFromParent(ResourceList, 0);
        }

        if ( NT_SUCCESS(status) && HasUart )
        {
            /* TODO */
        }
    }

    /* Cleanup - TODO: Make this cleanup UART, FM etc. */

    if ( ! NT_SUCCESS(status) )
    {
        if ( (*Resources).Wave != NULL )
        {
            (*Resources->Wave).Release();
            (*Resources).Wave = NULL;
        }

        if ( (*Resources).Adapter != NULL )
        {
            (*Resources->Adapter).Release();
            (*Resources).Adapter = NULL;
        }
    }

    return status;
}


NTSTATUS
StartDevice(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PRESOURCELIST ResourceList)
{
    NTSTATUS status = STATUS_SUCCESS;

    Resources DeviceResources;

    PUNKNOWN UnknownTopology = NULL;
    PUNKNOWN UnknownWave = NULL;
    PUNKNOWN UnknownWaveTable = NULL;
    PUNKNOWN UnknownFmSynth = NULL;

//    PADAPTERCOMMON AdapterCommon = NULL;
    PUNKNOWN UnknownCommon = NULL;

    status = AssignResources(ResourceList, &DeviceResources);

    if ( NT_SUCCESS(status) )
    {
    }

    return status;
}

extern "C"
NTSTATUS
AddDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject)
{
    return PcAddAdapterDevice(DriverObject,
                              PhysicalDeviceObject,
                              StartDevice,
                              MAX_MINIPORTS,
                              0);
}

extern "C"
NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPathName)
{
    NTSTATUS status;

    status = PcInitializeAdapterDriver(DriverObject,
                                       RegistryPathName,
                                       (PDRIVER_ADD_DEVICE) AddDevice);

    /* TODO: Add our own IRP handlers here */

    return status;
}

