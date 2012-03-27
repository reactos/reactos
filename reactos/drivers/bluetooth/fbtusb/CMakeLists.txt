
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
if(MSVC)
    #add_target_compile_flags(fbtusb "/Gz")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/Gz")
else()
    #allow_warnings(fbtusb)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error -mrtd -fno-builtin")
endif()

set_module_type(fbtusb kernelmodedriver)
add_importlibs(fbtusb ntoskrnl hal usbd)
add_cd_file(TARGET fbtusb DESTINATION reactos/system32/drivers FOR all)
