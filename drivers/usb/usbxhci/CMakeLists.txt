
set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/drivers/libusb)

list(APPEND SOURCE
    usbxhci.cpp
    usb_request.cpp
    usb_queue.cpp
    hardware.cpp
    usbxhci.h)

add_library(usbxhci SHARED
    ${SOURCE}
    guid.cpp
    usbxhci.rc)

target_link_libraries(usbxhci uuid libusb libcntpr ${PSEH_LIB})
set_module_type(usbxhci kernelmodedriver)
add_importlibs(usbxhci ntoskrnl hal usbd)
add_pch(usbxhci usbxhci.h SOURCE)
add_cd_file(TARGET usbxhci DESTINATION reactos/system32/drivers NO_CAB FOR all)
