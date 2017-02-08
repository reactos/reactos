
set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE
    hcd_controller.cpp
    hub_controller.cpp
    memory_manager.cpp
    misc.cpp
    usb_device.cpp
    purecall.cpp
    libusb.cpp
    libusb.h)

add_library(libusb ${SOURCE})
add_dependencies(libusb bugcodes xdk)
add_pch(libusb libusb.h SOURCE)
