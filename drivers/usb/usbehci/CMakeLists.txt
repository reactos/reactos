
set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/lib/drivers/libusb)

list(APPEND SOURCE
    usbehci.cpp
    usb_request.cpp
    usb_queue.cpp
    hardware.cpp
    usbehci.h)

add_library(usbehci SHARED
    ${SOURCE}
    guid.cpp
    usbehci.rc)

target_link_libraries(usbehci
    uuid
    libusb
    libcntpr
    ${PSEH_LIB})

set_module_type(usbehci kernelmodedriver)
add_importlibs(usbehci ntoskrnl hal usbd)
add_pch(usbehci usbehci.h SOURCE)
add_cd_file(TARGET usbehci DESTINATION reactos/system32/drivers NO_CAB FOR all)
