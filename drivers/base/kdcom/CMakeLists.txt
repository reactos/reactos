
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

set_entrypoint(kdcom 0)
set_subsystem(kdcom native)
set_image_base(kdcom 0x00010000)

add_importlibs(kdcom ntoskrnl hal)

add_dependencies(kdcom psdk bugcodes)
add_minicd_target(kdcom reactos kdcom.dll)
add_livecd_target(kdcom reactos/system32)
add_importlib_target(kdcom.spec)
