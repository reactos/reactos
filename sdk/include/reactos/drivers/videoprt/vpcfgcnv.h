/*
 * PROJECT:     ReactOS Video Port Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Conversion helpers between legacy and newer
 *              video/monitor configuration data structures.
 * COPYRIGHT:   Copyright 2023-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/**
 * @defgroup VpCfgConv  ReactOS Video Port Configuration Data Conversion Helpers
 *
 * @brief
 * Conversion helpers between legacy video/monitor configuration data
 * structures, and newer ones compatible with CM_PARTIAL_RESOURCE_LIST
 * resource descriptors.
 *
 * RATIONALE:
 *
 * The legacy, MIPS-derived, video display controller and monitor
 * peripheral configuration data is specified, respectively, with
 * the VIDEO_HARDWARE_CONFIGURATION_DATA (see DDK video.h, with
 * data starting at the 'Version' field and following) and the
 * MONITOR_CONFIGURATION_DATA structures (see arc.h), both with
 * Version == 0 or 1, Revision == 0, as set by the NT OS loader.
 * They are compatible with the ARC Specification Revision 1.00.
 *
 * The newer way to specify configuration data, compatible with
 * the ARC Specification Revision 1.2, is to use a CM_PARTIAL_RESOURCE_LIST
 * with one or more CM_PARTIAL_RESOURCE_DESCRIPTOR's for enumerating
 * standard resources (Interrupts, I/O ports, Memory...) as well
 * as free-form device-specific information data.
 *
 * For video display controllers, all the legacy data can be
 * described by a set of standard CM_PARTIAL_RESOURCE_DESCRIPTOR's:
 * - The Irql/Vector interrupt settings are now provided with
 *   a CmResourceTypeInterrupt descriptor.
 * - Both ControlBase/Size and CursorBase/Size memory I/O ports
 *   are provided with successive CmResourceTypePort descriptors,
 *   *respectively in this order*.
 * - The framebuffer's FrameBase/Size is provided, in a 64-bit
 *   compatible way, with a CmResourceTypeMemory descriptor.
 *
 * Any other extended video controller-specific data, that would
 * have been described with a vendor-specific extended version of
 * the VIDEO_HARDWARE_CONFIGURATION_DATA structure, can now be
 * specified with a CmResourceTypeDeviceSpecific descriptor appended
 * to the CM_PARTIAL_RESOURCE_LIST.
 * For example, in ReactOS, extended framebuffer format of the
 * boot console may be specified with the ReactOS-specific
 * CM_FRAMEBUF_DEVICE_DATA structure (see framebuf.h).
 *
 * For monitor peripherals, all the legacy data is extended by
 * the CM_MONITOR_DEVICE_DATA structure (see DDK ntddk.h or wdm.h),
 * to be specified with a CmResourceTypeDeviceSpecific descriptor.
 *
 * The ReactOS loader specifies this configuration data in the
 * new format. In order to ensure backward compatibility with
 * _legacy_ Windows video miniports that could use this data
 * (typically on NT <= 4 or 5, mostly for MIPS-based machines
 * but also few x86 ones), and for interoperability of our code
 * with the Windows NT loader, we define a set of conversion
 * functions that can translate between legacy (ARC Rev.1.00)
 * and newer (ARC Rev.1.2) configuration data structures.
 **/

#pragma once

// TEMPTEMP: Extra debugging
#if DBG
#define TEST_DPRINT DPRINT1
#else
#define TEST_DPRINT(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief
 * Legacy ARC Rev.1.00 video display controller configuration data.
 *
 * This is the configuration data structure exposed by DisplayController
 * CONFIGURATION_COMPONENT nodes in the NT loader block ConfigurationRoot
 * ARC hardware tree, if they do not use CM_PARTIAL_RESOURCE_LIST's.
 *
 * Once stored in the \Registry\Machine\Hardware\Description configuration
 * tree, the data is described by a VIDEO_HARDWARE_CONFIGURATION_DATA
 * structure.
 *
 * Note that the Version and Revision fields correspond to the first
 * two fields of CM_PARTIAL_RESOURCE_LIST.
 *
 * @see VIDEO_HARDWARE_CONFIGURATION_DATA defined in DDK video.h
 **/
typedef struct _VIDEO_CONFIGURATION_DATA
{
    USHORT Version;
    USHORT Revision;
    USHORT Irql;
    USHORT Vector;
    ULONG ControlBase;
    ULONG ControlSize;
    ULONG CursorBase;
    ULONG CursorSize;
    ULONG FrameBase;
    ULONG FrameSize;
} VIDEO_CONFIGURATION_DATA, *PVIDEO_CONFIGURATION_DATA;

#ifndef __VIDEO_H__

/**
 * @brief
 * Data returned with VpControllerData by a call to VideoPortGetDeviceData().
 *
 * - The first two fields, InterfaceType and BusNumber, are common with
 *   the CM_FULL_RESOURCE_DESCRIPTOR header;
 * - The Version and Revision fields correspond to the first two fields
 *   of CM_PARTIAL_RESOURCE_LIST;
 * - The other fields are of legacy layout.
 **/
typedef struct _VIDEO_HARDWARE_CONFIGURATION_DATA
{
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    USHORT Version;
    USHORT Revision;
    USHORT Irql;
    USHORT Vector;
    ULONG ControlBase;
    ULONG ControlSize;
    ULONG CursorBase;
    ULONG CursorSize;
    ULONG FrameBase;
    ULONG FrameSize;
} VIDEO_HARDWARE_CONFIGURATION_DATA, *PVIDEO_HARDWARE_CONFIGURATION_DATA;

#endif /* __VIDEO_H__ */

/* Verify these two structures are compatible */
C_ASSERT(sizeof(VIDEO_HARDWARE_CONFIGURATION_DATA) ==
         FIELD_OFFSET(VIDEO_HARDWARE_CONFIGURATION_DATA, Version) + sizeof(VIDEO_CONFIGURATION_DATA));


/**
 * @brief
 * Data returned with VpMonitorData by a call to VideoPortGetDeviceData().
 *
 * This structure describes the legacy ARC Rev.1.00 monitor peripheral
 * configuration data, as stored in the \Registry\Machine\Hardware\Description
 * configuration tree.
 *
 * It has a similar layout as the display controller DDK video.h
 * VIDEO_HARDWARE_CONFIGURATION_DATA structure, where:
 * - The first two fields, InterfaceType and BusNumber, are common with
 *   the CM_FULL_RESOURCE_DESCRIPTOR header;
 * - The Version and Revision fields correspond to the first two fields
 *   of CM_PARTIAL_RESOURCE_LIST;
 * - The other fields are of legacy layout.
 *
 * @see MONITOR_CONFIGURATION_DATA defined in arc.h
 **/
#include <pshpack1.h>
typedef struct _MONITOR_HARDWARE_CONFIGURATION_DATA
{
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    MONITOR_CONFIGURATION_DATA;
} MONITOR_HARDWARE_CONFIGURATION_DATA, *PMONITOR_HARDWARE_CONFIGURATION_DATA;
#include <poppack.h>


/*
 * From a (VIDEO|MONITOR)_HARDWARE_CONFIGURATION_DATA structure, retrieve
 * where the actual legacy data starts from, based from the fact that the
 * first two fields, InterfaceType and BusNumber, are common with the
 * CM_FULL_RESOURCE_DESCRIPTOR header, and the actual data starts where
 * the CM_FULL_RESOURCE_DESCRIPTOR::PartialResourceList member would start.
 */
#define GET_LEGACY_DATA(fullConfigData) \
    ((PVOID)&(((PCM_FULL_RESOURCE_DESCRIPTOR)fullConfigData)->PartialResourceList))

#define GET_LEGACY_DATA_LEN(fullConfigDataLen) \
    ((fullConfigDataLen >= FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList)) \
   ? (fullConfigDataLen  - FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList)) \
   : 0)


FORCEINLINE
BOOLEAN
IsLegacyConfigData(
    _In_ PVOID ConfigurationData,
    _In_ ULONG ConfigurationDataLength,
    _In_ ULONG ExpectedConfigurationDataLength)
{
    /* Cast to PCM_PARTIAL_RESOURCE_LIST to access
     * the common Version and Revision fields */
    PCM_PARTIAL_RESOURCE_LIST ResourceList =
        (PCM_PARTIAL_RESOURCE_LIST)ConfigurationData;

    if (!ConfigurationData)
    {
        TEST_DPRINT("IsLegacyConfigData:FALSE - ConfigurationData == NULL\n");
        return FALSE;
    }

    /*
     * A valid legacy configuration data covers the first two fields,
     * Version and Revision, of CM_PARTIAL_RESOURCE_LIST.
     */
    if ((ConfigurationDataLength < FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, Count)) ||
        (ConfigurationDataLength != ExpectedConfigurationDataLength))
    {
        TEST_DPRINT("IsLegacyConfigData:FALSE - ConfigurationDataLength == %lu\n",
                ConfigurationDataLength);
        return FALSE;
    }

    /* If Version/Revision is larger or equal to 1.2,
     * this cannot be a legacy configuration data */
    if ( (ResourceList->Version >  1) ||
        ((ResourceList->Version == 1) && (ResourceList->Revision > 1)) )
    {
        TEST_DPRINT("IsLegacyConfigData:FALSE - Version %lu, Revision %lu\n",
                ResourceList->Version, ResourceList->Revision);
        return FALSE;
    }

    TEST_DPRINT("IsLegacyConfigData:TRUE\n");
    return TRUE;
}

FORCEINLINE
BOOLEAN
IsLegacyConfigDataFull(
    _In_ PVOID ConfigurationData,
    _In_ ULONG ConfigurationDataLength,
    _In_ ULONG ExpectedConfigurationDataLength)
{
    if (!ConfigurationData)
    {
        TEST_DPRINT("IsLegacyConfigDataFull:FALSE - ConfigurationData == NULL\n");
        return FALSE;
    }

    /*
     * A valid legacy configuration data covers:
     * - The first two fields, InterfaceType and BusNumber,
     *   of the CM_FULL_RESOURCE_DESCRIPTOR header;
     * - The first two fields, Version and Revision,
     *   of CM_PARTIAL_RESOURCE_LIST.
     */
    if ((ConfigurationDataLength < FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                                PartialResourceList.Count)) ||
        (ConfigurationDataLength != ExpectedConfigurationDataLength))
    {
        TEST_DPRINT("IsLegacyConfigDataFull:FALSE - ConfigurationDataLength == %lu\n",
                ConfigurationDataLength);
        return FALSE;
    }

    /* Verify the actual legacy configuration data */
    return IsLegacyConfigData(
                GET_LEGACY_DATA(ConfigurationData),
                GET_LEGACY_DATA_LEN(ConfigurationDataLength),
                GET_LEGACY_DATA_LEN(ExpectedConfigurationDataLength));
}


FORCEINLINE
BOOLEAN
IsNewConfigData(
    _In_ PVOID ConfigurationData,
    _In_ ULONG ConfigurationDataLength)
{
    /* Cast to PCM_PARTIAL_RESOURCE_LIST to access
     * the common Version and Revision fields */
    PCM_PARTIAL_RESOURCE_LIST ResourceList =
        (PCM_PARTIAL_RESOURCE_LIST)ConfigurationData;

    if (!ConfigurationData)
    {
        TEST_DPRINT("IsNewConfigData:FALSE - ConfigurationData == NULL\n");
        return FALSE;
    }

    if (ConfigurationDataLength < FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors))
    {
        TEST_DPRINT("IsNewConfigData:FALSE - ConfigurationDataLength == %lu\n",
                ConfigurationDataLength);
        return FALSE;
    }

    /* If Version/Revision is strictly lower than 1.2, this cannot be
     * a new configuration data (even if the length appears to match
     * a CM_FULL_RESOURCE_DESCRIPTOR with zero or more descriptors) */
    if ( (ResourceList->Version <  1) ||
        ((ResourceList->Version == 1) && (ResourceList->Revision <= 1)) )
    {
        TEST_DPRINT("IsNewConfigData:FALSE - Version %lu, Revision %lu\n",
                ResourceList->Version, ResourceList->Revision);
        return FALSE;
    }

    /* This should be a new configuration data */
    TEST_DPRINT("IsNewConfigData:TRUE\n");
    return TRUE;
}

FORCEINLINE
BOOLEAN
IsNewConfigDataFull(
    _In_ PVOID ConfigurationData,
    _In_ ULONG ConfigurationDataLength)
{
    if (!ConfigurationData)
    {
        TEST_DPRINT("IsNewConfigDataFull:FALSE - ConfigurationData == NULL\n");
        return FALSE;
    }

    if (ConfigurationDataLength < FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                               PartialResourceList.PartialDescriptors))
    {
        TEST_DPRINT("IsNewConfigDataFull:FALSE - ConfigurationDataLength == %lu\n",
                ConfigurationDataLength);
        return FALSE;
    }

    /* Verify the actual new configuration data */
    return IsNewConfigData(
                GET_LEGACY_DATA(ConfigurationData),
                GET_LEGACY_DATA_LEN(ConfigurationDataLength));
}


/**
 * @brief
 * Given a CM_PARTIAL_RESOURCE_LIST, obtain pointers to resource descriptors
 * for legacy video configuration: interrupt, control and cursor I/O ports,
 * and framebuffer memory descriptors.  In addition, retrieve any
 * device-specific resource present.
 **/
FORCEINLINE
VOID
GetVideoData(
    _In_ PCM_PARTIAL_RESOURCE_LIST ResourceList,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* Interrupt,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* ControlPort,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* CursorPort,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* FrameBuffer,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* DeviceSpecific)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG PortCount = 0, IntCount = 0, MemCount = 0;
    ULONG i;

    /* Initialize the return values */
    if (Interrupt)   *Interrupt   = NULL;
    if (ControlPort) *ControlPort = NULL;
    if (CursorPort)  *CursorPort  = NULL;
    *FrameBuffer = NULL;
    if (DeviceSpecific) *DeviceSpecific = NULL;

    TEST_DPRINT("GetVideoData\n");

    for (i = 0; i < ResourceList->Count; ++i)
    {
        Descriptor = &ResourceList->PartialDescriptors[i];

        switch (Descriptor->Type)
        {
            case CmResourceTypePort:
            {
                TEST_DPRINT("    CmResourceTypePort\n");
                /* We only check for memory I/O ports */
                // if (!(Descriptor->Flags & CM_RESOURCE_PORT_MEMORY))
                if (Descriptor->Flags & CM_RESOURCE_PORT_IO)
                    break;

                /* If more than two memory I/O ports
                 * have been encountered, ignore them */
                if (PortCount > 2)
                    break;
                ++PortCount;

                /* First port is Control; Second port is Cursor */
                if (PortCount == 1)
                {
                    // Descriptor->u.Port;
                    if (ControlPort)
                        *ControlPort = Descriptor;
                }
                else // if (PortCount == 2)
                {
                    // Descriptor->u.Port;
                    if (CursorPort)
                        *CursorPort = Descriptor;
                }
                break;
            }

            case CmResourceTypeInterrupt:
            {
                TEST_DPRINT("    CmResourceTypeInterrupt\n");
                /* If more than one interrupt resource
                 * has been encountered, ignore them */
                if (IntCount > 1)
                    break;
                ++IntCount;

                // Descriptor->u.Interrupt;
                if (Interrupt)
                    *Interrupt = Descriptor;
                break;
            }

            case CmResourceTypeMemory:
            {
                TEST_DPRINT("    CmResourceTypeMemory\n");
                /* If more than one memory resource
                 * has been encountered, ignore them */
                if (MemCount > 1)
                    break;
                ++MemCount;

                // or CM_RESOURCE_MEMORY_WRITE_ONLY ??
                // if (!(Descriptor->Flags & CM_RESOURCE_MEMORY_READ_WRITE))
                // {
                //     /* Cannot use this framebuffer */
                //     break;
                // }

                // Descriptor->u.Memory;
                *FrameBuffer = Descriptor;
                break;
            }

            case CmResourceTypeDeviceSpecific:
            {
                TEST_DPRINT("    CmResourceTypeDeviceSpecific\n");
                /* NOTE: This descriptor *MUST* be the last one.
                 * The actual device data follows the descriptor. */
                ASSERT(i == ResourceList->Count - 1);
                i = ResourceList->Count; // To force-break the for-loop.

                if (DeviceSpecific)
                    *DeviceSpecific = Descriptor;
                break;
            }
        }
    }
}

/*static*/ inline
VOID
// DoConvertVideoDeviceToConfigData
DoConvertVideoDataToLegacyConfigData(
    _Out_ PVIDEO_CONFIGURATION_DATA configData,
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Interrupt,
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR ControlPort,
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CursorPort,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR FrameBuffer)
{
    if (Interrupt)
    {
        if ((Interrupt->u.Interrupt.Level & 0xFFFF0000) != 0)
        {
            DPRINT1("WARNING: Interrupt Level %lu truncated to 16 bits!\n",
                    Interrupt->u.Interrupt.Level);
        }
        configData->Irql   = (USHORT)Interrupt->u.Interrupt.Level;
        configData->Vector = Interrupt->u.Interrupt.Vector;
    }

    if (ControlPort)
    {
        if (ControlPort->u.Port.Start.HighPart != 0)
        {
            DPRINT1("WARNING: Port %I64u truncated to 32 bits!\n",
                    ControlPort->u.Port.Start.QuadPart);
        }
        configData->ControlBase = ControlPort->u.Port.Start.LowPart;
        configData->ControlSize = ControlPort->u.Port.Length;
    }

    if (CursorPort)
    {
        if (CursorPort->u.Port.Start.HighPart != 0)
        {
            DPRINT1("WARNING: Port %I64u truncated to 32 bits!\n",
                    CursorPort->u.Port.Start.QuadPart);
        }
        configData->CursorBase = CursorPort->u.Port.Start.LowPart;
        configData->CursorSize = CursorPort->u.Port.Length;
    }

    if (FrameBuffer)
    {
        if (FrameBuffer->u.Memory.Start.HighPart != 0)
        {
            DPRINT1("WARNING: Memory %I64u truncated to 32 bits!\n",
                    FrameBuffer->u.Memory.Start.QuadPart);
        }
        configData->FrameBase = FrameBuffer->u.Memory.Start.LowPart;
        configData->FrameSize = FrameBuffer->u.Memory.Length;
    }
}

/**
 * @brief
 * Convert video resource descriptor data into legacy video configuration.
 **/
FORCEINLINE
VOID
// ConvertVideoDeviceToConfigData
ConvertVideoDataToLegacyConfigData(
    _Out_ PVIDEO_HARDWARE_CONFIGURATION_DATA configData,
    _In_ PCM_FULL_RESOURCE_DESCRIPTOR cmDescriptor)
{
    PCM_PARTIAL_RESOURCE_LIST ResourceList = &cmDescriptor->PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Interrupt;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ControlPort;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CursorPort;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR FrameBuffer;

    configData->InterfaceType = cmDescriptor->InterfaceType;
    configData->BusNumber     = cmDescriptor->BusNumber;

    /* The legacy configuration data is from ARC Specification Revision 1.00 */
    configData->Version  = 1;
    configData->Revision = 0;

    GetVideoData(ResourceList,
                 &Interrupt,
                 &ControlPort,
                 &CursorPort,
                 &FrameBuffer,
                 NULL); // The would-be PCM_VIDEO_DEVICE_DATA, unused.

    DoConvertVideoDataToLegacyConfigData(GET_LEGACY_DATA(configData),
                                         Interrupt,
                                         ControlPort,
                                         CursorPort,
                                         FrameBuffer);
}

/**
 * @brief
 * Convert legacy video configuration data into video resource descriptor.
 *
 * @note    CM_VIDEO_DEVICE_DATA::VideoClock is in Hertz.
 **/
FORCEINLINE
VOID
// ConvertVideoConfigToDeviceData
ConvertLegacyVideoConfigDataToDeviceData(
    _In_ PVIDEO_CONFIGURATION_DATA configData,
#if 0
    _Out_opt_ PCM_VIDEO_DEVICE_DATA VideoData,
#endif
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Interrupt,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR ControlPort,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CursorPort,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR FrameBuffer)
{
#if 0
    if (VideoData)
    {
        /* The new configuration data is from ARC Specification Revision 1.2 */
        VideoData->Version  = 0;
        VideoData->Revision = 0;

        VideoData->VideoClock = 0; // FIXME: Use a default "sane" value?
    }
#endif

    if (Interrupt)
    {
        Interrupt->Type = CmResourceTypeInterrupt;

        Interrupt->u.Interrupt.Level  = configData->Irql;
        Interrupt->u.Interrupt.Vector = configData->Vector;
    }

    if (ControlPort)
    {
        ControlPort->Type = CmResourceTypePort;

        /* We only check for memory I/O ports */
        ControlPort->Flags &= ~CM_RESOURCE_PORT_IO;
        ControlPort->Flags |= CM_RESOURCE_PORT_MEMORY;

        ControlPort->u.Port.Start.HighPart = 0;
        ControlPort->u.Port.Start.LowPart  = configData->ControlBase;
        ControlPort->u.Port.Length = configData->ControlSize;
    }

    if (CursorPort)
    {
        CursorPort->Type = CmResourceTypePort;

        /* We only check for memory I/O ports */
        CursorPort->Flags &= ~CM_RESOURCE_PORT_IO;
        CursorPort->Flags |= CM_RESOURCE_PORT_MEMORY;

        CursorPort->u.Port.Start.HighPart = 0;
        CursorPort->u.Port.Start.LowPart  = configData->CursorBase;
        CursorPort->u.Port.Length = configData->CursorSize;
    }

    FrameBuffer->Type = CmResourceTypeMemory;

    // FrameBuffer->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
    // or CM_RESOURCE_MEMORY_WRITE_ONLY ??

    FrameBuffer->u.Memory.Start.HighPart = 0;
    FrameBuffer->u.Memory.Start.LowPart  = configData->FrameBase;
    FrameBuffer->u.Memory.Length = configData->FrameSize;
}


/**
 * @brief
 * Given a CM_PARTIAL_RESOURCE_LIST, obtain a pointer to resource descriptor
 * for monitor configuration data, listed as a device-specific resource.
 **/
FORCEINLINE
VOID
GetMonitorData(
    _In_ PCM_PARTIAL_RESOURCE_LIST ResourceList,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* DeviceSpecific)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG i;

    /* Initialize the return values */
    *DeviceSpecific = NULL;

    /* Find the CmResourceTypeDeviceSpecific CM_MONITOR_DEVICE_DATA */
    for (i = 0; i < ResourceList->Count; ++i)
    {
        Descriptor = &ResourceList->PartialDescriptors[i];
        if (Descriptor->Type == CmResourceTypeDeviceSpecific)
        {
            /* NOTE: This descriptor *MUST* be the last one.
             * The actual device data follows the descriptor. */
            // PCM_MONITOR_DEVICE_DATA MonitorData = (PCM_MONITOR_DEVICE_DATA)(Descriptor + 1);

            ASSERT(i == ResourceList->Count - 1);

            if (DeviceSpecific)
                *DeviceSpecific = Descriptor;
            break;
        }
    }
}

/**
 * @note
 * Units of the MONITOR_CONFIGURATION_DATA and CM_MONITOR_DEVICE_DATA members
 * (all of them are USHORTs):
 *
 * HorizontalScreenSize     Pixels
 * HorizontalResolution     Pixels
 *
 * HorizontalDisplayTime    Nanoseconds (ns)
 *     Values larger than 65535 (MAXUSHORT) are stored in the
 *     HorizontalDisplayTime(High:Low) pair.
 *
 * HorizontalBackPorch      ns
 *     Values larger than 65535 (MAXUSHORT) are stored in the
 *     HorizontalBackPorch(High:Low) pair.
 *
 * HorizontalFrontPorch     ns
 *     Values larger than 65535 (MAXUSHORT) are stored in the
 *     HorizontalFrontPorch(High:Low) pair.
 *
 * HorizontalSync           ns
 *     Values larger than 65535 (MAXUSHORT) are stored in the
 *     HorizontalSync(High:Low) pair.
 *
 * VerticalScreenSize       Lines
 *     One line corresponds to HorizontalScreenSize pixels, scanned during
 *     Horizontal(BackPorch + DisplayTime + FrontPorch + Sync) nanoseconds.
 *
 * VerticalResolution       Lines
 *
 * VerticalBackPorch        Lines
 *     Values larger than 65535 (MAXUSHORT) are stored in the
 *     VerticalBackPorch(High:Low) pair.
 *
 * VerticalFrontPorch       Lines
 *     Values larger than 65535 (MAXUSHORT) are stored in the
 *     VerticalFrontPorch(High:Low) pair.
 *
 * VerticalSync             Lines
 *     Values larger than 65535 (MAXUSHORT) are stored in the
 *     VerticalSync(High:Low) pair.
 **/
/*static*/ inline
VOID
// DoConvertMonitorDeviceToConfigData
DoConvertMonitorDataToLegacyConfigData(
    _Out_ PMONITOR_CONFIGURATION_DATA configData,
    _In_ PCM_MONITOR_DEVICE_DATA MonitorData)
{
    configData->HorizontalResolution = MonitorData->HorizontalResolution;
    if (MonitorData->HorizontalDisplayTimeHigh != 0)
    {
        DPRINT1("WARNING: Monitor HorizontalDisplayTime truncated to 16 bits!\n");
        configData->HorizontalDisplayTime = MonitorData->HorizontalDisplayTimeLow;
    }
    else
    {
        configData->HorizontalDisplayTime = MonitorData->HorizontalDisplayTime;
    }
    if (MonitorData->HorizontalBackPorchHigh != 0)
    {
        DPRINT1("WARNING: Monitor HorizontalBackPorch truncated to 16 bits!\n");
        configData->HorizontalBackPorch = MonitorData->HorizontalBackPorchLow;
    }
    else
    {
        configData->HorizontalBackPorch = MonitorData->HorizontalBackPorch;
    }
    if (MonitorData->HorizontalFrontPorchHigh != 0)
    {
        DPRINT1("WARNING: Monitor HorizontalFrontPorch truncated to 16 bits!\n");
        configData->HorizontalFrontPorch = MonitorData->HorizontalFrontPorchLow;
    }
    else
    {
        configData->HorizontalFrontPorch = MonitorData->HorizontalFrontPorch;
    }
    if (MonitorData->HorizontalSyncHigh != 0)
    {
        DPRINT1("WARNING: Monitor HorizontalSync truncated to 16 bits!\n");
        configData->HorizontalSync = MonitorData->HorizontalSyncLow;
    }
    else
    {
        configData->HorizontalSync = MonitorData->HorizontalSync;
    }

    configData->VerticalResolution = MonitorData->VerticalResolution;
    if (MonitorData->VerticalBackPorchHigh != 0)
    {
        DPRINT1("WARNING: Monitor VerticalBackPorch truncated to 16 bits!\n");
        configData->VerticalBackPorch = MonitorData->VerticalBackPorchLow;
    }
    else
    {
        configData->VerticalBackPorch = MonitorData->VerticalBackPorch;
    }
    if (MonitorData->VerticalFrontPorchHigh != 0)
    {
        DPRINT1("WARNING: Monitor VerticalFrontPorch truncated to 16 bits!\n");
        configData->VerticalFrontPorch = MonitorData->VerticalFrontPorchLow;
    }
    else
    {
        configData->VerticalFrontPorch = MonitorData->VerticalFrontPorch;
    }
    if (MonitorData->VerticalSyncHigh != 0)
    {
        DPRINT1("WARNING: Monitor VerticalSync truncated to 16 bits!\n");
        configData->VerticalSync = MonitorData->VerticalSyncLow;
    }
    else
    {
        configData->VerticalSync = MonitorData->VerticalSync;
    }

    configData->HorizontalScreenSize = MonitorData->HorizontalScreenSize;
    configData->VerticalScreenSize = MonitorData->VerticalScreenSize;
}

/**
 * @brief
 * Convert monitor resource descriptor data into legacy monitor configuration.
 **/
FORCEINLINE
VOID
// ConvertMonitorDeviceToConfigData
ConvertMonitorDataToLegacyConfigData(
    _Out_ PMONITOR_HARDWARE_CONFIGURATION_DATA configData,
    _In_ PCM_FULL_RESOURCE_DESCRIPTOR cmDescriptor)
{
    PCM_PARTIAL_RESOURCE_LIST ResourceList = &cmDescriptor->PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    configData->InterfaceType = cmDescriptor->InterfaceType;
    configData->BusNumber     = cmDescriptor->BusNumber;

    /* The legacy configuration data is from ARC Specification Revision 1.00 */
    configData->Version  = 1;
    configData->Revision = 0;

    GetMonitorData(ResourceList, &Descriptor);

    if (Descriptor)
    {
        /* NOTE: This descriptor *MUST* be the last one.
         * The actual device data follows the descriptor. */
        PCM_MONITOR_DEVICE_DATA MonitorData = (PCM_MONITOR_DEVICE_DATA)(Descriptor + 1);
        DoConvertMonitorDataToLegacyConfigData(GET_LEGACY_DATA(configData), MonitorData);
    }
}

/**
 * @brief
 * Convert legacy monitor configuration data into monitor resource descriptor.
 **/
FORCEINLINE
VOID
// ConvertMonitorConfigToDeviceData
ConvertLegacyMonitorConfigDataToDeviceData(
    _In_ PMONITOR_CONFIGURATION_DATA configData,
    _Out_ PCM_MONITOR_DEVICE_DATA MonitorData)
{
    /* The new configuration data is from ARC Specification Revision 1.2 */
    MonitorData->Version  = 0;
    MonitorData->Revision = 0;

    MonitorData->HorizontalScreenSize = configData->HorizontalScreenSize;
    MonitorData->VerticalScreenSize   = configData->VerticalScreenSize;
    MonitorData->HorizontalResolution = configData->HorizontalResolution;
    MonitorData->VerticalResolution   = configData->VerticalResolution;

    MonitorData->HorizontalDisplayTimeLow  = 0;
    MonitorData->HorizontalDisplayTime     = configData->HorizontalDisplayTime;
    MonitorData->HorizontalDisplayTimeHigh = 0;

    MonitorData->HorizontalBackPorchLow  = 0;
    MonitorData->HorizontalBackPorch     = configData->HorizontalBackPorch;
    MonitorData->HorizontalBackPorchHigh = 0;

    MonitorData->HorizontalFrontPorchLow  = 0;
    MonitorData->HorizontalFrontPorch     = configData->HorizontalFrontPorch;
    MonitorData->HorizontalFrontPorchHigh = 0;

    MonitorData->HorizontalSyncLow  = 0;
    MonitorData->HorizontalSync     = configData->HorizontalSync;
    MonitorData->HorizontalSyncHigh = 0;

    MonitorData->VerticalBackPorchLow  = 0;
    MonitorData->VerticalBackPorch     = configData->VerticalBackPorch;
    MonitorData->VerticalBackPorchHigh = 0;

    MonitorData->VerticalFrontPorchLow  = 0;
    MonitorData->VerticalFrontPorch     = configData->VerticalFrontPorch;
    MonitorData->VerticalFrontPorchHigh = 0;

    MonitorData->VerticalSyncLow  = 0;
    MonitorData->VerticalSync     = configData->VerticalSync;
    MonitorData->VerticalSyncHigh = 0;
}

#ifdef __cplusplus
}
#endif

/* EOF */
