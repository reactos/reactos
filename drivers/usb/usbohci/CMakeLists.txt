
set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/drivers/libusb)

list(APPEND SOURCE
    usbohci.cpp
    usb_request.cpp
    usb_queue.cpp
    hardware.cpp
    usbohci.h)

add_library(usbohci MODULE
    ${SOURCE}
    guid.cpp
    usbohci.rc)

target_link_libraries(usbohci uuid libusb libcntpr ${PSEH_LIB})
set_module_type(usbohci kernelmodedriver)
add_importlibs(usbohci ntoskrnl hal usbd)
add_pch(usbohci usbohci.h SOURCE)
add_cd_file(TARGET usbohci DESTINATION reactos/system32/drivers NO_CAB FOR all)
