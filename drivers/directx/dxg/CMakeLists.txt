
spec2def(dxg.sys dxg.spec)

add_library(dxg SHARED
    main.c
    ddhmg.c
    eng.c
    historic.c
    dxg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxg.def)

set_target_properties(dxg PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")


add_importlibs(dxg dxgthk ntoskrnl)
add_dependencies(dxg psdk bugcodes)
