
set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/drivers/libusb)

list(APPEND SOURCE
    usbuhci.cpp
    usb_request.cpp
    usb_queue.cpp
    hardware.cpp
    usbuhci.h)

add_library(usbuhci MODULE
    ${SOURCE}
    guid.cpp
    usbuhci.rc)

target_link_libraries(usbuhci uuid libusb libcntpr ${PSEH_LIB})
set_module_type(usbuhci kernelmodedriver)
add_importlibs(usbuhci ntoskrnl hal usbd)
add_pch(usbuhci usbuhci.h SOURCE)
add_cd_file(TARGET usbuhci DESTINATION reactos/system32/drivers NO_CAB FOR all)
