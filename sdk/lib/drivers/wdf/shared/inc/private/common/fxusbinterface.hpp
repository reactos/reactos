//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXUSBINTERFACE_H_
#define _FXUSBINTERFACE_H_

extern "C" {
#include <usbdrivr.h>
#include <WdfUsb.h>
}

#include "FxUsbRequestContext.hpp"

#define FX_USB_INTERFACE_TAG   'uItG' //using a random uniqure value



struct FxUsbInterfaceSetting{
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
#if (FX_CORE_MODE == FX_CORE_USER_MODE)







    USB_INTERFACE_DESCRIPTOR InterfaceDescriptorAlloc;
#endif
};

class FxUsbInterface : public FxNonPagedObject {   //any base class
public:
    friend FxUsbDevice;
    friend FxUsbPipe;
    //friend FxUsbTarget;

    FxUsbInterface(
        _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
        _In_ FxUsbDevice* UsbDevice,
        _In_ PUSB_INTERFACE_DESCRIPTOR  InterfaceDescriptor
        );

    VOID
    SetInfo(
        __in PUSBD_INTERFACE_INFORMATION Interface
        );

    VOID
    CleanUpAndDelete(
        __in BOOLEAN ClearDestroyCallback
        );

    _Must_inspect_result_
    NTSTATUS
    SelectSetting(
        __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
        __in PURB Urb
        );

    UCHAR
    GetNumConfiguredPipes(
        VOID
        )
    {
        return m_NumberOfConfiguredPipes;
    }

    UCHAR
    GetInterfaceNumber(
        VOID
        )
    {
        return m_InterfaceNumber;
    }

    UCHAR
    GetNumSettings(
        VOID
        )
    {
        return m_NumSettings;
    }

    UCHAR
    GetNumEndpoints(
        __in UCHAR SettingIndex
        );

    VOID
    GetEndpointInformation(
        __in UCHAR SettingIndex,
        __in UCHAR PipeIndex,
        __in PWDF_USB_PIPE_INFORMATION PipeInfo
        );

    VOID
    GetDescriptor(
        __in PUSB_INTERFACE_DESCRIPTOR  UsbInterfaceDescriptor,
        __in UCHAR SettingIndex
        );

        //post config

    UCHAR
    GetConfiguredSettingIndex(
        VOID
        ) ;

    WDFUSBPIPE
    GetConfiguredPipe(
        __in  UCHAR PipeIndex,
        __out_opt PWDF_USB_PIPE_INFORMATION PipeInfo
        );

    _Must_inspect_result_
    NTSTATUS
    CreateSettings(
        VOID
        );

    VOID
    SetNumConfiguredPipes(
        __in UCHAR NumberOfPipes
        )
    {
        m_NumberOfConfiguredPipes = NumberOfPipes;
    }

    VOID
    SetConfiguredPipes(
        __in FxUsbPipe **ppPipes
        )
    {
        m_ConfiguredPipes = ppPipes;
    }

    BOOLEAN
    IsInterfaceConfigured(
        VOID
        )
    {
        return m_ConfiguredPipes != NULL ? TRUE : FALSE;
    }

    _Must_inspect_result_
    NTSTATUS
    SelectSettingByDescriptor(
        __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
        __in PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor
        );

    _Must_inspect_result_
    NTSTATUS
    SelectSettingByIndex(
        __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
        __in UCHAR SettingIndex
        );

    VOID
    CopyEndpointFieldsFromDescriptor(
        __in PWDF_USB_PIPE_INFORMATION PipeInfo,
        __in PUSB_ENDPOINT_DESCRIPTOR EndpointDesc,
        __in UCHAR SettingIndex
        );

    ULONG
    DetermineDefaultMaxTransferSize(
        VOID
        );

    WDFUSBINTERFACE
    GetHandle(VOID)
    {
         return (WDFUSBINTERFACE) GetObjectHandle();
    }

    PUSB_INTERFACE_DESCRIPTOR
    GetSettingDescriptor(
        __in UCHAR Setting
        );

    NTSTATUS
    CheckAndSelectSettingByIndex(
        __in UCHAR SettingIndex
        );

    NTSTATUS
    UpdatePipeAttributes(
        __in PWDF_OBJECT_ATTRIBUTES PipesAttributes
        );

protected:
    ~FxUsbInterface(
        VOID
        );

    VOID
    RemoveDeletedPipe(
        __in FxUsbPipe* Pipe
        );

    VOID
    FormatSelectSettingUrb(
        __in_bcount(GET_SELECT_INTERFACE_REQUEST_SIZE(NumEndpoints)) PURB Urb,
        __in USHORT NumEndpoints,
        __in UCHAR SettingNumber
        );

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
public:
    NTSTATUS
    SetWinUsbHandle(
        _In_ UCHAR FrameworkInterfaceIndex
    );

    NTSTATUS
    MakeAndConfigurePipes(
        __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
        __in UCHAR NumPipes
        );
#endif

protected:
    //
    // Backpointer to the owning device
    //
    FxUsbDevice* m_UsbDevice;

    //
    // Array of pipe pointers
    //
    FxUsbPipe** m_ConfiguredPipes;

    //
    // Array of alternative settings for the interface
    //
    __field_ecount(m_NumSettings) FxUsbInterfaceSetting* m_Settings;

    //
    // Number of elements in m_Settings
    //
    UCHAR m_NumSettings;

    //
    // Number of elements in m_ConfiguredPipes
    //
    UCHAR m_NumberOfConfiguredPipes;

    //
    // Information out of the interface descriptor
    //
    UCHAR m_InterfaceNumber;
    UCHAR m_CurAlternateSetting;
    UCHAR m_Class;
    UCHAR m_SubClass;
    UCHAR m_Protocol;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
private:
    //
    // Handle to USB interface exposed by WinUsb
    //
    WINUSB_INTERFACE_HANDLE m_WinUsbHandle;
#endif
};


#endif // _FXUSBINTERFACE_H_

