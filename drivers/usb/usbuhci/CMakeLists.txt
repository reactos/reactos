
set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    ${REACTOS_SOURCE_DIR}/lib/drivers/libusb)

list(APPEND SOURCE
    usbuhci.cpp
    usb_request.cpp
    usb_queue.cpp
    hardware.cpp)

add_library(usbuhci SHARED
    ${SOURCE}
    usbuhci.rc)

target_link_libraries(usbuhci
    libusb
    libcntpr
    ${PSEH_LIB})

set_module_type(usbuhci kernelmodedriver)
add_importlibs(usbuhci ntoskrnl hal usbd)

add_cd_file(TARGET usbuhci DESTINATION reactos/system32/drivers NO_CAB FOR all)
