
list(APPEND SOURCE
    fatx.c
    vfatxlib.c
    vfatxlib.h)

add_library(vfatxlib ${SOURCE})
add_pch(vfatxlib vfatxlib.h SOURCE)
target_link_libraries(vfatxlib chkstk)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(vfatxlib -lgcc)
endif()

add_dependencies(vfatxlib psdk)
