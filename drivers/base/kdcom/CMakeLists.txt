
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

spec2def(kdcom ${CMAKE_CURRENT_SOURCE_DIR}/kdcom.spec ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def)

if(ARCH MATCHES i386)
list(APPEND SOURCE i386/kdbg.c)
elseif(ARCH MATCHES amd64)
list(APPEND SOURCE i386/kdbg.c)
elseif(ARCH MATCHES arm)
list(APPEND SOURCE arm/kdbg.c)
endif(ARCH MATCHES i386)

add_library(kdcom SHARED ${SOURCE})

set_target_properties(kdcom PROPERTIES LINK_FLAGS "-Wl,-entry,0 -Wl,--image-base,0x00010000")

target_link_libraries(kdcom
    ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def
    -lntoskrnl
    -lhal)

add_dependencies(kdcom kdcom_def psdk buildno_header bugcodes)
