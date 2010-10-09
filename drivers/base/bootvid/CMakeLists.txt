


spec2def(bootvid ${CMAKE_CURRENT_SOURCE_DIR}/bootvid.spec ${CMAKE_CURRENT_BINARY_DIR}/bootvid.def)

list(APPEND SOURCE bootvid.rc)

if(ARCH MATCHES i386 OR ARCH MATCHES amd64)
list(APPEND SOURCE
    i386/bootvid.c
    i386/bootdata.c
    i386/vga.c)
elseif(ARCH MATCHES arm)
list(APPEND SOURCE
    arm/bootvid.c
    arm/bootdata.c)
endif(ARCH MATCHES i386 OR ARCH MATCHES amd64)

add_library(bootvid SHARED ${CMAKE_CURRENT_BINARY_DIR}/bootvid_precomp.h.gch ${SOURCE})

set_target_properties(bootvid PROPERTIES LINK_FLAGS "-Wl,-entry,0 -Wl,--image-base,0x00010000")

target_link_libraries(bootvid
    ${CMAKE_CURRENT_BINARY_DIR}/bootvid.def
    -lntoskrnl
    -lhal)

add_pch(bootvid ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(bootvid bootvid_def psdk buildno_header bugcodes)
add_minicd_target(bootvid reactos bootvid.dll)