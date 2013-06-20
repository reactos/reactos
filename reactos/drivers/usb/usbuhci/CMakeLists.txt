
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

# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(MSVC)
    #add_target_compile_flags(usbuhci "/GR-")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/GR-")
else()
    target_link_libraries(usbuhci -lgcc)
    #add_target_compile_flags(usbuhci "-fno-exceptions -fno-rtti")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-rtti")
endif()

set_module_type(usbuhci kernelmodedriver)
add_importlibs(usbuhci ntoskrnl hal usbd)

add_cd_file(TARGET usbuhci DESTINATION reactos/system32/drivers NO_CAB FOR all)
