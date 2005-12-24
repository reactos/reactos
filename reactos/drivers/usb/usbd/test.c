#include <stdio.h>
#include <windows.h>
#include <usbdi.h>

typedef ULONG STDCALL
(*USBD_GetInterfaceLengthTYPE)(
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    PUCHAR BufferEnd
    );

int main()
{
    HMODULE Lib;
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    USBD_GetInterfaceLengthTYPE USBD_GetInterfaceLength;

    InterfaceDescriptor.bLength = 10; 
    InterfaceDescriptor.bNumEndpoints = 2; 
    InterfaceDescriptor.bDescriptorType = /*USB_INTERFACE_DESCRIPTOR_TYPE*/2;
    InterfaceDescriptor.iInterface = 0x1;

    Lib = LoadLibraryEx("usbd.sys", NULL, DONT_RESOLVE_DLL_REFERENCES);
    USBD_GetInterfaceLength = (USBD_GetInterfaceLengthTYPE)GetProcAddress(Lib, "USBD_GetInterfaceLength");
    printf("%X\n", USBD_GetInterfaceLength(&InterfaceDescriptor, (PUCHAR)((DWORD)&InterfaceDescriptor + sizeof(InterfaceDescriptor))));
    FreeLibrary(Lib);

    Lib = LoadLibraryEx("usbd.ms", NULL, DONT_RESOLVE_DLL_REFERENCES);
    USBD_GetInterfaceLength = (USBD_GetInterfaceLengthTYPE)GetProcAddress(Lib, "USBD_GetInterfaceLength");
    printf("%X\n", USBD_GetInterfaceLength(&InterfaceDescriptor, (PUCHAR)((DWORD)&InterfaceDescriptor + sizeof(InterfaceDescriptor))));
    FreeLibrary(Lib);
    return 0;
}
                                                            
