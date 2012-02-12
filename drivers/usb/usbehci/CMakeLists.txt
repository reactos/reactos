
set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_library(usbehci SHARED
    usbehci.cpp
    usb_device.cpp
    usb_request.cpp
    usb_queue.cpp
    hcd_controller.cpp
    hardware.cpp
    misc.cpp
    purecall.cpp
    hub_controller.cpp
    memory_manager.cpp
    usbehci.rc)

target_link_libraries(usbehci
    libcntpr
    ${PSEH_LIB})

if(MSVC)
    set_target_properties(usbehci PROPERTIES COMPILE_FLAGS "/GR-")
else()
    target_link_libraries(usbehci -lgcc)
    set_target_properties(usbehci PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-rtti")
endif(MSVC)

set_module_type(usbehci kernelmodedriver)
add_importlibs(usbehci ntoskrnl hal)

add_cd_file(TARGET usbehci DESTINATION reactos/system32/drivers NO_CAB FOR all)