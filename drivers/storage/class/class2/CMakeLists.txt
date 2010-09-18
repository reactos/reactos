


include_directories(..)

spec2def(class2 ${CMAKE_CURRENT_SOURCE_DIR}/class2.spec ${CMAKE_CURRENT_BINARY_DIR}/class2.def)

add_library(class2 SHARED class2.c class2.rc)

set_target_properties(class2 PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(class2
    ${CMAKE_CURRENT_BINARY_DIR}/class2.def
    -lscsiport
    -lntoskrnl
    -lhal)

add_dependencies(class2 class2_def psdk bugcodes)
