
spec2def(kdcom.dll kdcom.spec)

if(ARCH MATCHES i386)
list(APPEND SOURCE i386/kdbg.c)
elseif(ARCH MATCHES amd64)
list(APPEND SOURCE i386/kdbg.c)
elseif(ARCH MATCHES arm)
list(APPEND SOURCE arm/kdbg.c)
endif(ARCH MATCHES i386)
list(APPEND SOURCE ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def)

add_library(kdcom SHARED ${SOURCE})

set_target_properties(kdcom PROPERTIES LINK_FLAGS "-Wl,-entry,0 -Wl,--image-base,0x00010000")

target_link_libraries(kdcom
    -lntoskrnl
    -lhal)

add_dependencies(kdcom psdk buildno_header bugcodes)
add_minicd_target(kdcom reactos kdcom.dll)
add_livecd_target(kdcom reactos/system32)