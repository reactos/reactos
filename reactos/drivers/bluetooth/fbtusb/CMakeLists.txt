
include_directories(include)

list(APPEND SOURCE
    fbtdev.c
    fbtpnp.c
    fbtpwr.c
    fbtrwr.c
#   fbtwmi.c
    fbtusb.c)

add_library(fbtusb SHARED
    ${SOURCE}
    fbtusb.rc)

# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(NOT MSVC)
    #allow_warnings(fbtusb)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

set_module_type(fbtusb kernelmodedriver)
add_importlibs(fbtusb ntoskrnl hal usbd)
add_cd_file(TARGET fbtusb DESTINATION reactos/system32/drivers FOR all)
