
include_directories(include)

list(APPEND SOURCE
    fbtdev.c
    fbtpnp.c
    fbtpwr.c
    fbtrwr.c
#   fbtwmi.c
    fbtusb.c
    precomp.h)

add_library(fbtusb SHARED ${SOURCE} fbtusb.rc)

if(NOT MSVC)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-unused-but-set-variable")
endif()

set_module_type(fbtusb kernelmodedriver)
add_importlibs(fbtusb ntoskrnl hal usbd)
add_pch(fbtusb precomp.h SOURCE)
add_cd_file(TARGET fbtusb DESTINATION reactos/system32/drivers FOR all)
