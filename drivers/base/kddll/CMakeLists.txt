
spec2def(kdcom.dll kdcom.spec)

add_library(kdcom SHARED
    kdcom.c
    kddll.c
    kdserial.c
    ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def)

set_target_properties(kdcom PROPERTIES LINK_FLAGS "-Wl,-entry,0 -Wl,--image-base,0x00010000")

target_link_libraries(kdcom
    -lntoskrnl
    -lhal)

add_dependencies(kdcom psdk buildno_header bugcodes)
