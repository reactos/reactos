
spec2def(usbd.sys usbd.spec)

add_library(usbd SHARED usbd.c usbd.rc
    ${CMAKE_CURRENT_BINARY_DIR}/usbd.def)

set_target_properties(usbd PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(usbd ntoskrnl hal)
add_dependencies(usbd psdk bugcodes)
