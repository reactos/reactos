
set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_library(usbuhci SHARED
    usbuhci.cpp
    usb_device.cpp
    usb_request.cpp
    usb_queue.cpp
    hcd_controller.cpp
    hardware.cpp
    misc.cpp
    purecall.cpp
    hub_controller.cpp
    memory_manager.cpp
    usbuhci.rc)

target_link_libraries(usbuhci
    libcntpr
    ${PSEH_LIB})

if(MSVC)
    set_target_properties(usbuhci PROPERTIES COMPILE_FLAGS "/GR-")
else()
    target_link_libraries(usbuhci -lgcc)
    set_target_properties(usbuhci PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-rtti")
endif(MSVC)

set_module_type(usbuhci kernelmodedriver)
add_importlibs(usbuhci ntoskrnl hal usbd)

add_cd_file(TARGET usbuhci DESTINATION reactos/system32/drivers NO_CAB FOR all)
