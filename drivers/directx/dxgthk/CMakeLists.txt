
spec2def(dxgthk.sys dxgthk.spec)

add_library(dxgthk SHARED main.c dxgthk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxgthk.def)

set_target_properties(dxgthk PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(dxgthk win32k)
add_dependencies(dxgthk psdk bugcodes)
