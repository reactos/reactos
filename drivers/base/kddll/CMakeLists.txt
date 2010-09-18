


spec2def(kdcom ${CMAKE_CURRENT_SOURCE_DIR}/kdcom.spec ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def)

add_library(kdcom SHARED
    kdcom.c
    kddll.c
    kdserial.c)

set_target_properties(kdcom PROPERTIES LINK_FLAGS "-Wl,-entry,0 -Wl,--image-base,0x00010000")

target_link_libraries(kdcom
    ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def
    -lntoskrnl
    -lhal)

add_dependencies(kdcom kdcom_def psdk buildno_header bugcodes)
