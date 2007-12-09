/*
 * PROJECT:         ReactOS Boot Loader (FreeLDR)
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/reactos/archwsup.c
 * PURPOSE:         Routines for ARC Hardware Tree and Configuration Data
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
FldrSetComponentInformation(IN FRLDRHKEY ComponentKey,
                            IN IDENTIFIER_FLAG Flags,
                            IN ULONG Key,
                            IN ULONG Affinity)
{
    CONFIGURATION_COMPONENT ConfigurationComponent;
    LONG Error;
    
    /* Build the component information */
    ConfigurationComponent.Flags = Flags;
    ConfigurationComponent.Version = 0;
    ConfigurationComponent.Revision = 0;
    ConfigurationComponent.Key = Key;
    ConfigurationComponent.AffinityMask = Affinity;
    
    /* Set the value */
    Error = RegSetValue(ComponentKey,
                        L"Component Information",
                        REG_BINARY,
                        (PVOID)&ConfigurationComponent.Flags,
                        FIELD_OFFSET(CONFIGURATION_COMPONENT, ConfigurationDataLength) -
                        FIELD_OFFSET(CONFIGURATION_COMPONENT, Flags));
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", Error));
    }
}

VOID
NTAPI
FldrSetIdentifier(IN FRLDRHKEY ComponentKey,
                  IN PWCHAR Identifier)
{
    LONG Error;
    ULONG IdentifierLength = (wcslen(Identifier) + 1) * sizeof(WCHAR);
    
    /* Set the key */
    Error = RegSetValue(ComponentKey,
                        L"Identifier",
                        REG_SZ,
                        (PCHAR)Identifier,
                        IdentifierLength);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", Error));
        return;
    }
}

VOID
NTAPI
FldrCreateSystemKey(OUT FRLDRHKEY *SystemKey)
{
    LONG Error;
    
    /* Create the key */
    Error = RegCreateKey(NULL,
                         L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System",
                         SystemKey);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", Error));
        return;
    }    
}

VOID
NTAPI
FldrCreateComponentKey(IN FRLDRHKEY SystemKey,
                       IN PWCHAR BusName,
                       IN ULONG BusNumber,
                       OUT FRLDRHKEY *ComponentKey)
{
    LONG Error;
    WCHAR Buffer[80];
    
    /* Build the key name */
    swprintf(Buffer, L"%s\\%u", BusName, BusNumber);
    
    /* Create the key */
    Error = RegCreateKey(SystemKey, Buffer, ComponentKey);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", Error));
        return;
    }    
}

VOID
NTAPI
FldrSetConfigurationData(IN FRLDRHKEY ComponentKey,
                         IN PVOID ConfigurationData,
                         IN ULONG Size)
{
    LONG Error;
    
    /* Set 'Configuration Data' value */
    Error = RegSetValue(ComponentKey,
                        L"Configuration Data",
                        REG_FULL_RESOURCE_DESCRIPTOR,
                        ConfigurationData,
                        Size);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT,
                  "RegSetValue(Configuration Data) failed (Error %u)\n",
                  Error));
    }    
}
