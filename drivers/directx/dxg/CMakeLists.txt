
spec2def(dxg ${CMAKE_CURRENT_SOURCE_DIR}/dxg.spec ${CMAKE_CURRENT_BINARY_DIR}/dxg.def)

add_library(dxg SHARED
    main.c
    ddhmg.c
    eng.c
    historic.c
    dxg.rc)

set_target_properties(dxg PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(dxg ${CMAKE_CURRENT_BINARY_DIR}/dxg.def)
add_importlibs(dxg dxgthk ntoskrnl)
add_dependencies(dxg dxg_def psdk bugcodes)
