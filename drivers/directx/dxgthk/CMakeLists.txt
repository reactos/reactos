
spec2def(dxgthk ${CMAKE_CURRENT_SOURCE_DIR}/dxgthk.spec ${CMAKE_CURRENT_BINARY_DIR}/dxgthk.def)

add_library(dxgthk SHARED main.c dxgthk.rc)

set_target_properties(dxgthk PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(dxgthk ${CMAKE_CURRENT_BINARY_DIR}/dxgthk.def)
add_importlibs(dxgthk win32k)
add_dependencies(dxgthk dxgthk_def psdk bugcodes)
