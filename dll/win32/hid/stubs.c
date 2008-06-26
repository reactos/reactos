/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Hid User Library
 * FILE:            lib/hid/stubs.c
 * PURPOSE:         hid.dll stubs
 * NOTES:           If you implement a function, remove it from this file
 *
 * UPDATE HISTORY:
 *      07/12/2004  Created
 */
#include <precomp.h>

/*
 * @unimplemented
 */
HIDAPI
BOOLEAN DDKAPI
HidD_GetConfiguration(IN HANDLE HidDeviceObject,
                      OUT PHIDD_CONFIGURATION Configuration,
                      IN ULONG ConfigurationLength)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HIDAPI
BOOLEAN DDKAPI
HidD_GetIndexedString(IN HANDLE HidDeviceObject,
                      IN ULONG StringIndex,
                      OUT PVOID Buffer,
                      IN ULONG BufferLength)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HIDAPI
BOOLEAN DDKAPI
HidD_GetMsGenreDescriptor(IN HANDLE HidDeviceObject,
                          OUT PVOID Buffer,
                          IN ULONG BufferLength)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HIDAPI
BOOLEAN DDKAPI
HidD_SetConfiguration(IN HANDLE HidDeviceObject,
                      IN PHIDD_CONFIGURATION Configuration,
                      IN ULONG ConfigurationLength)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetCaps(IN PHIDP_PREPARSED_DATA PreparsedData,
             OUT PHIDP_CAPS Capabilities)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetData(IN HIDP_REPORT_TYPE ReportType,
             OUT PHIDP_DATA DataList,
             IN OUT PULONG DataLength,
             IN PHIDP_PREPARSED_DATA PreparsedData,
             IN PCHAR Report,
             IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetExtendedAttributes(IN HIDP_REPORT_TYPE ReportType,
                           IN USHORT DataIndex,
                           IN PHIDP_PREPARSED_DATA PreparsedData,
                           OUT PHIDP_EXTENDED_ATTRIBUTES Attributes,
                           IN OUT PULONG LengthAttributes)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetLinkCollectionNodes(OUT PHIDP_LINK_COLLECTION_NODE LinkCollectionNodes,
                            IN OUT PULONG LinkCollectionNodesLength,
                            IN PHIDP_PREPARSED_DATA PreparsedData)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetScaledUsageValue(IN HIDP_REPORT_TYPE ReportType,
                         IN USAGE UsagePage,
                         IN USHORT LinkCollection OPTIONAL,
                         IN USAGE Usage,
                         OUT PLONG UsageValue,
                         IN PHIDP_PREPARSED_DATA PreparsedData,
                         IN PCHAR Report,
                         IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetSpecificButtonCaps(IN HIDP_REPORT_TYPE ReportType,
                           IN USAGE UsagePage,
                           IN USHORT LinkCollection,
                           IN USAGE Usage,
                           OUT PHIDP_BUTTON_CAPS ButtonCaps,
                           IN OUT PULONG ButtonCapsLength,
                           IN PHIDP_PREPARSED_DATA PreparsedData)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetSpecificValueCaps(IN HIDP_REPORT_TYPE ReportType,
                          IN USAGE UsagePage,
                          IN USHORT LinkCollection,
                          IN USAGE Usage,
                          OUT PHIDP_VALUE_CAPS ValueCaps,
                          IN OUT PULONG ValueCapsLength,
                          IN PHIDP_PREPARSED_DATA PreparsedData)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetUsageValue(IN HIDP_REPORT_TYPE ReportType,
                   IN USAGE UsagePage,
                   IN USHORT LinkCollection,
                   IN USAGE Usage,
                   OUT PULONG UsageValue,
                   IN PHIDP_PREPARSED_DATA PreparsedData,
                   IN PCHAR Report,
                   IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetUsageValueArray(IN HIDP_REPORT_TYPE ReportType,
                        IN USAGE UsagePage,
                        IN USHORT LinkCollection  OPTIONAL,
                        IN USAGE Usage,
                        OUT PCHAR UsageValue,
                        IN USHORT UsageValueByteLength,
                        IN PHIDP_PREPARSED_DATA PreparsedData,
                        IN PCHAR Report,
                        IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetUsages(IN HIDP_REPORT_TYPE ReportType,
               IN USAGE UsagePage,
               IN USHORT LinkCollection  OPTIONAL,
               OUT USAGE *UsageList,
               IN OUT ULONG *UsageLength,
               IN PHIDP_PREPARSED_DATA PreparsedData,
               IN PCHAR Report,
               IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetUsagesEx(IN HIDP_REPORT_TYPE ReportType,
                 IN USHORT LinkCollection,
                 OUT PUSAGE_AND_PAGE ButtonList,
                 IN OUT ULONG *UsageLength,
                 IN PHIDP_PREPARSED_DATA PreparsedData,
                 IN PCHAR Report,
                 IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_InitializeReportForID(IN HIDP_REPORT_TYPE ReportType,
                           IN UCHAR ReportID,
                           IN PHIDP_PREPARSED_DATA PreparsedData,
                           IN OUT PCHAR Report,
                           IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
ULONG DDKAPI
HidP_MaxDataListLength(IN HIDP_REPORT_TYPE  ReportType,
                       IN PHIDP_PREPARSED_DATA  PreparsedData)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HIDAPI
ULONG DDKAPI
HidP_MaxUsageListLength(IN HIDP_REPORT_TYPE ReportType,
                        IN USAGE UsagePage OPTIONAL,
                        IN PHIDP_PREPARSED_DATA PreparsedData)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_SetData(IN HIDP_REPORT_TYPE ReportType,
             IN PHIDP_DATA DataList,
             IN OUT PULONG DataLength,
             IN PHIDP_PREPARSED_DATA PreparsedData,
             IN OUT PCHAR Report,
             IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_SetScaledUsageValue(IN HIDP_REPORT_TYPE ReportType,
                         IN USAGE UsagePage,
                         IN USHORT LinkCollection OPTIONAL,
                         IN USAGE Usage,
                         IN LONG UsageValue,
                         IN PHIDP_PREPARSED_DATA PreparsedData,
                         IN OUT PCHAR Report,
                         IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_SetUsageValue(IN HIDP_REPORT_TYPE ReportType,
                   IN USAGE UsagePage,
                   IN USHORT LinkCollection,
                   IN USAGE Usage,
                   IN ULONG UsageValue,
                   IN PHIDP_PREPARSED_DATA PreparsedData,
                   IN OUT PCHAR Report,
                   IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_SetUsageValueArray(IN HIDP_REPORT_TYPE ReportType,
                        IN USAGE UsagePage,
                        IN USHORT LinkCollection OPTIONAL,
                        IN USAGE Usage,
                        IN PCHAR UsageValue,
                        IN USHORT UsageValueByteLength,
                        IN PHIDP_PREPARSED_DATA PreparsedData,
                        OUT PCHAR Report,
                        IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_SetUsages(IN HIDP_REPORT_TYPE ReportType,
               IN USAGE UsagePage,
               IN USHORT LinkCollection OPTIONAL,
               IN PUSAGE UsageList,
               IN OUT PULONG UsageLength,
               IN PHIDP_PREPARSED_DATA PreparsedData,
               IN OUT PCHAR Report,
               IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_TranslateUsagesToI8042ScanCodes(IN PUSAGE ChangedUsageList,
                                     IN ULONG UsageListLength,
                                     IN HIDP_KEYBOARD_DIRECTION KeyAction,
                                     IN OUT PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
                                     IN PHIDP_INSERT_SCANCODES InsertCodesProcedure,
                                     IN PVOID InsertCodesContext)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_UnsetUsages(IN HIDP_REPORT_TYPE ReportType,
                 IN USAGE UsagePage,
                 IN USHORT LinkCollection OPTIONAL,
                 IN PUSAGE UsageList,
                 IN OUT PULONG UsageLength,
                 IN PHIDP_PREPARSED_DATA PreparsedData,
                 IN OUT PCHAR Report,
                 IN ULONG ReportLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_UsageListDifference(IN PUSAGE PreviousUsageList,
                         IN PUSAGE CurrentUsageList,
                         OUT PUSAGE BreakUsageList,
                         OUT PUSAGE MakeUsageList,
                         IN ULONG UsageListLength)
{
  UNIMPLEMENTED;
  return HIDP_STATUS_NOT_IMPLEMENTED;
}

/* EOF */
