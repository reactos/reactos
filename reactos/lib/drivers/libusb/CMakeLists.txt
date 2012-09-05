set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)


add_definitions(
    -DUNICODE -D_UNICODE)

list(APPEND SOURCE
    hcd_controller.cpp
    hub_controller.cpp
    memory_manager.cpp
    misc.cpp
    usb_device.cpp
    purecall.cpp
    libusb.cpp)

add_library(libusb ${SOURCE})
add_dependencies(libusb bugcodes)

# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(MSVC)
    #add_target_compile_flags(libusb "/GR-")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/GR-")
else()
    #add_target_compile_flags(libusb "-fno-exceptions -fno-rtti")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-rtti")
endif()
