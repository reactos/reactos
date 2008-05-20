/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/hal.h
 * PURPOSE:         Internal header for PCI Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */
#ifndef _PCI_
#define _PCI_

//
// PCI Type 1 Ports
//
#define PCI_TYPE1_ADDRESS_PORT      (PULONG)0xCF8
#define PCI_TYPE1_DATA_PORT         0xCFC

//
// PCI Type 2 Ports
//
#define PCI_TYPE2_CSE_PORT          (PUCHAR)0xCF8
#define PCI_TYPE2_FORWARD_PORT      (PUCHAR)0xCFA
#define PCI_TYPE2_ADDRESS_BASE      0xC

//
// PCI Type 1 Configuration Register
//
typedef struct _PCI_TYPE1_CFG_BITS
{
    union
    {
        struct
        {
            ULONG Reserved1:2;
            ULONG RegisterNumber:6;
            ULONG FunctionNumber:3;
            ULONG DeviceNumber:5;
            ULONG BusNumber:8;
            ULONG Reserved2:7;
            ULONG Enable:1;
        } bits;

        ULONG AsULONG;
    } u;
} PCI_TYPE1_CFG_BITS, *PPCI_TYPE1_CFG_BITS;

//
// PCI Type 2 CSE Register
//
typedef struct _PCI_TYPE2_CSE_BITS
{
    union
    {
        struct
        {
            UCHAR Enable:1;
            UCHAR FunctionNumber:3;
            UCHAR Key:4;
        } bits;

        UCHAR AsUCHAR;
    } u;
} PCI_TYPE2_CSE_BITS, PPCI_TYPE2_CSE_BITS;

//
// PCI Type 2 Address Register
//
typedef struct _PCI_TYPE2_ADDRESS_BITS
{
    union
    {
        struct
        {
            USHORT RegisterNumber:8;
            USHORT Agent:4;
            USHORT AddressBase:4;
        } bits;

        USHORT AsUSHORT;
    } u;
} PCI_TYPE2_ADDRESS_BITS, *PPCI_TYPE2_ADDRESS_BITS;

//
// PCI Registry Information
//
typedef struct _PCI_REGISTRY_INFO
{
    UCHAR MajorRevision;
    UCHAR MinorRevision;
    UCHAR NoBuses;
    UCHAR HardwareMechanism;
} PCI_REGISTRY_INFO, *PPCI_REGISTRY_INFO;

//
// PCI Card Descriptor in Registry
//
typedef struct _PCI_CARD_DESCRIPTOR
{
    ULONG Flags;
    USHORT VendorID;
    USHORT DeviceID;
    USHORT RevisionID;
    USHORT SubsystemVendorID;
    USHORT SubsystemID;
    USHORT Reserved;
} PCI_CARD_DESCRIPTOR, *PPCI_CARD_DESCRIPTOR;

#endif
