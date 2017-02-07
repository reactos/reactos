
spec2def(bootvid.dll bootvid.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    bootvid.rc
    ${CMAKE_CURRENT_BINARY_DIR}/bootvid.def)

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

add_library(bootvid SHARED ${SOURCE})

set_entrypoint(bootvid 0)
set_subsystem(bootvid native)
set_image_base(bootvid 0x00010000)

add_importlibs(bootvid ntoskrnl hal)
add_pch(bootvid precomp.h)
add_dependencies(bootvid psdk bugcodes)
add_cd_file(TARGET bootvid DESTINATION reactos/system32 NO_CAB FOR all)

