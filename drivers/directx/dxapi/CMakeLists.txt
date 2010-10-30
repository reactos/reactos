
add_definitions(-D_DXAPI_)

spec2def(dxapi.sys dxapi.spec)

add_library(dxapi SHARED main.c dxapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxapi.def)

set_target_properties(dxapi PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(dxapi ntoskrnl hal videoprt)
add_dependencies(dxapi psdk bugcodes)
