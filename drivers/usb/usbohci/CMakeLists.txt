
set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    ${REACTOS_SOURCE_DIR}/lib/drivers/libusb)

add_library(usbohci SHARED
    usbohci.cpp
    usb_request.cpp
    usb_queue.cpp
    hardware.cpp
    usbohci.rc)

target_link_libraries(usbohci
    libusb
    libcntpr
    ${PSEH_LIB})

if(MSVC)
    set_target_properties(usbohci PROPERTIES COMPILE_FLAGS "/GR-")
else()
    target_link_libraries(usbohci -lgcc)
    set_target_properties(usbohci PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-rtti")
endif(MSVC)

set_module_type(usbohci kernelmodedriver)
add_importlibs(usbohci ntoskrnl hal usbd)

add_cd_file(TARGET usbohci DESTINATION reactos/system32/drivers NO_CAB FOR all)