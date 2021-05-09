//
//    Copyright (C) Microsoft.  All rights reserved.
//
#pragma once

#include <WinUsb.h>

#define UMURB_FUNCTION_SELECT_CONFIGURATION            0x0000
#define UMURB_FUNCTION_SELECT_INTERFACE                0x0001
#define UMURB_FUNCTION_ABORT_PIPE                      0x0002
#define UMURB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL       0x0003
#define UMURB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL    0x0004
#define UMURB_FUNCTION_GET_FRAME_LENGTH                0x0005
#define UMURB_FUNCTION_SET_FRAME_LENGTH                0x0006
#define UMURB_FUNCTION_GET_CURRENT_FRAME_NUMBER        0x0007
#define UMURB_FUNCTION_CONTROL_TRANSFER                0x0008
#define UMURB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER      0x0009
#define UMURB_FUNCTION_ISOCH_TRANSFER                  0x000A
#define UMURB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE      0x000B
#define UMURB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE        0x000C
#define UMURB_FUNCTION_SET_FEATURE_TO_DEVICE           0x000D
#define UMURB_FUNCTION_SET_FEATURE_TO_INTERFACE        0x000E
#define UMURB_FUNCTION_SET_FEATURE_TO_ENDPOINT         0x000F
#define UMURB_FUNCTION_CLEAR_FEATURE_TO_DEVICE         0x0010
#define UMURB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE      0x0011
#define UMURB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT       0x0012
#define UMURB_FUNCTION_GET_STATUS_FROM_DEVICE          0x0013
#define UMURB_FUNCTION_GET_STATUS_FROM_INTERFACE       0x0014
#define UMURB_FUNCTION_GET_STATUS_FROM_ENDPOINT        0x0015
#define UMURB_FUNCTION_RESERVED_0X0016                 0x0016
#define UMURB_FUNCTION_VENDOR_DEVICE                   0x0017
#define UMURB_FUNCTION_VENDOR_INTERFACE                0x0018
#define UMURB_FUNCTION_VENDOR_ENDPOINT                 0x0019
#define UMURB_FUNCTION_CLASS_DEVICE                    0x001A
#define UMURB_FUNCTION_CLASS_INTERFACE                 0x001B
#define UMURB_FUNCTION_CLASS_ENDPOINT                  0x001C
#define UMURB_FUNCTION_RESERVE_0X001D                  0x001D
#define UMURB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL 0x001E
#define UMURB_FUNCTION_GET_INTERFACE                   0x0027
#define UMURB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE   0x0028
#define UMURB_FUNCTION_RESET_PORT                      0x0029

//
//These are specific to user-mode (WinUsb) and have no km counterpart
//
#define UMURB_FUNCTION_QUERY_PIPE                      0x0101 //in km this is done as a part of Interface info
#define UMURB_FUNCTION_SET_PIPE_POLICY                 0x0102
#define UMURB_FUNCTION_GET_PIPE_POLICY                 0x0103
#define UMURB_FUNCTION_SET_INTERFACE_POWER_POLICY      0x0104
#define UMURB_FUNCTION_GET_INTERFACE_POWER_POLICY      0x0105
#define UMURB_FUNCTION_ENABLE_INTERFACE_IDLE           0x0106
#define UMURB_FUNCTION_DISABLE_INTERFACE_IDLE          0x0107
#define UMURB_FUNCTION_FLUSH_PIPE                      0x0108
#define UMURB_FUNCTION_GET_ASSOCIATED_INTERFACE        0x0109
#define UMURB_FUNCTION_GET_DEVICE_INFORMATION          0x010A
#define UMURB_FUNCTION_GET_DESCRIPTOR                  0x010B //WinUsb has a common function for all descriptors
#define UMURB_FUNCTION_RELEASE_ASSOCIATED_INTERFACE    0x010C

struct _UMURB_HEADER
{
    //
    // Fields filled in by client driver
    //
    IN USHORT Length;
    IN USHORT Function;
    IN WINUSB_INTERFACE_HANDLE InterfaceHandle;

    OUT DWORD Status; //obtained using GetLastError();
};

typedef _UMURB_HEADER* PUMURB_HEADER;

struct _UMURB_PIPE_REQUEST {
    struct _UMURB_HEADER Hdr;                 // function code indicates get or set.
    IN UCHAR PipeID;
    IN ULONG Reserved;
};

struct _UMURB_SELECT_INTERFACE {
    struct _UMURB_HEADER Hdr;                 // function code indicates get or set.
    IN UCHAR AlternateSetting;
};

struct _UMURB_CONTROL_TRANSFER {
    struct _UMURB_HEADER Hdr;                 // function code indicates get or set.
    IN UCHAR Reserved; //maybe use for PipeID
    IN OUT PVOID TransferBuffer;
    IN OUT ULONG TransferBufferLength;
    IN WINUSB_SETUP_PACKET SetupPacket;
};

struct _UMURB_BULK_OR_INTERRUPT_TRANSFER {
    struct _UMURB_HEADER Hdr;                 // function code indicates get or set.
    IN UCHAR PipeID;
    IN BOOL  InPipe;   //in km this is determined based on the pipe,
                       //but WinUsb has two different functions
                       //since host can't store pipe type and it would be expensive to discover it
                       //with every transfer, it is better if direction is sent in as a parameter
    IN OUT ULONG TransferBufferLength;
    IN OUT PVOID TransferBuffer;
};

struct _UMURB_CONTROL_DESCRIPTOR_REQUEST {
    struct _UMURB_HEADER Hdr;                 // function code indicates get or set.
    IN PVOID Reserved;
    IN ULONG Reserved0;
    IN OUT ULONG TransferBufferLength;
    OUT PVOID TransferBuffer;
    IN UCHAR Index;
    IN UCHAR DescriptorType;
    IN USHORT LanguageId;
    IN USHORT Reserved2;
};

struct _UMURB_PIPE_POLICY_REQUEST
{
    struct _UMURB_HEADER Hdr;
    IN UCHAR PipeID;
    IN ULONG PolicyType;
    IN OUT ULONG ValueLength;
    IN OUT PVOID Value;
};

struct _UMURB_INTERFACE_POLICY_REQUEST
{
    struct _UMURB_HEADER Hdr;
    IN ULONG PolicyType;
    IN OUT ULONG ValueLength;
    IN OUT PVOID Value;
};

struct _UMURB_QUERY_PIPE
{
    struct _UMURB_HEADER Hdr;
    IN UCHAR AlternateSetting;
    IN UCHAR PipeID;
    OUT WINUSB_PIPE_INFORMATION PipeInformation;
};

struct _UMURB_CONTROL_GET_INTERFACE_REQUEST {
    struct _UMURB_HEADER Hdr;                 // function code indicates get or set.
    IN UCHAR InterfaceIndex;
};

struct _UMURB_GET_ASSOCIATED_INTERFACE {
    IN struct _UMURB_HEADER Hdr;
    IN UCHAR InterfaceIndex;
    OUT WINUSB_INTERFACE_HANDLE InterfaceHandle;
};

struct _UMURB_INTERFACE_INFORMATION {
    IN struct _UMURB_HEADER Hdr;
    IN UCHAR AlternateSetting;
    OUT USB_INTERFACE_DESCRIPTOR UsbInterfaceDescriptor;
};

struct _UMURB_DEVICE_INFORMATION {
    IN struct _UMURB_HEADER Hdr;
    IN ULONG InformationType;
    IN OUT ULONG BufferLength;
    OUT PVOID Buffer;
};

struct _UMURB_DESCRIPTOR_REQUEST {
    IN struct _UMURB_HEADER Hdr;
    IN  UCHAR DescriptorType;
    IN  UCHAR Index;
    IN  USHORT LanguageID;
    IN OUT ULONG BufferLength;
    OUT PVOID Buffer;
};

typedef struct _UMURB {
    union {
        struct _UMURB_HEADER
            UmUrbHeader;

        struct _UMURB_SELECT_INTERFACE
            UmUrbSelectInterface;






        struct _UMURB_PIPE_REQUEST
            UmUrbPipeRequest;












        struct _UMURB_CONTROL_TRANSFER
            UmUrbControlTransfer;

        struct _UMURB_BULK_OR_INTERRUPT_TRANSFER
            UmUrbBulkOrInterruptTransfer;

        // for standard control transfers on the default pipe
        struct _UMURB_CONTROL_DESCRIPTOR_REQUEST
            UmUrbControlDescriptorRequest;










        struct _UMURB_CONTROL_GET_INTERFACE_REQUEST
            UmUrbControlGetInterfaceRequest;












        struct _UMURB_INTERFACE_POLICY_REQUEST
            UmUrbInterfacePolicyRequest;
        struct _UMURB_PIPE_POLICY_REQUEST
            UmUrbPipePolicyRequest;
        struct _UMURB_QUERY_PIPE
            UmUrbQueryPipe;
        struct _UMURB_GET_ASSOCIATED_INTERFACE
            UmUrbGetAssociatedInterface;
        struct _UMURB_INTERFACE_INFORMATION
            UmUrbInterfaceInformation;
        struct _UMURB_DEVICE_INFORMATION
            UmUrbDeviceInformation;
        struct _UMURB_DESCRIPTOR_REQUEST
            UmUrbDescriptorRequest;
    };
} UMURB, *PUMURB;


#define FILE_DEVICE_UMDF      ((ULONG)(0x8002))

//
// Device IO Control
//
#define UMDF_IOCTL_CODE(id)        \
        CTL_CODE(FILE_DEVICE_UMDF, (id), METHOD_BUFFERED, FILE_READ_ACCESS|FILE_WRITE_ACCESS)

#define IOCTL_INETRNAL_USB_SUBMIT_UMURB              UMDF_IOCTL_CODE(0x100)

