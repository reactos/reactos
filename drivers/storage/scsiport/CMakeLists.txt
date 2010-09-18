


add_library(scsiport SHARED scsiport.c stubs.c scsiport.rc)

set_target_properties(scsiport PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(scsiport
    ${CMAKE_CURRENT_SOURCE_DIR}/scsiport_i386.def
    -lntoskrnl
    -lhal)

add_dependencies(scsiport psdk bugcodes)
