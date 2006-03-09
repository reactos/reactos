/* ARC LOADER TYPES *********************************************************/

#ifndef __INCLUDE_ARC_H
#define __INCLUDE_ARC_H

/* Types */

typedef enum _IDENTIFIER_FLAG
{
    Failed = 0x01,
    ReadOnly = 0x02,
    Removable = 0x04,
    ConsoleIn = 0x08,
    ConsoleOut = 0x10,
    Input = 0x20,
    Output = 0x40
} IDENTIFIER_FLAG;

typedef enum _CONFIGURATION_CLASS
{
    SystemClass,
    ProcessorClass,
    CacheClass,
    AdapterClass,
    ControllerClass,
    PeripheralClass,
    MemoryClass,
    MaximumClass
} CONFIGURATION_CLASS;

typedef struct _CONFIGURATION_COMPONENT
{
    CONFIGURATION_CLASS Class;
    CONFIGURATION_TYPE Type;
    IDENTIFIER_FLAG Flags;
    USHORT Version;
    USHORT Revision;
    ULONG Key;
    ULONG AffinityMask;
    ULONG ConfigurationDataLength;
    ULONG IdentifierLength;
    LPSTR Identifier;
} CONFIGURATION_COMPONENT, *PCONFIGURATION_COMPONENT;

#endif
