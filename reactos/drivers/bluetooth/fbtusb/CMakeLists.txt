
include_directories(
    BEFORE include
    include)

add_library(fbtusb SHARED
    fbtdev.c
    fbtpnp.c
    fbtpwr.c
    fbtrwr.c
    fbtusb.c
#   fbtwmi.c
    fbtusb.rc)

allow_warnings(fbtusb)
set_module_type(fbtusb kernelmodedriver)
add_importlibs(fbtusb ntoskrnl hal usbd)
add_cd_file(TARGET fbtusb DESTINATION reactos/system32/drivers FOR all)
