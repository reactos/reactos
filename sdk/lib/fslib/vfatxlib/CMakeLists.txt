
list(APPEND SOURCE
    fatx.c
    vfatxlib.c
    vfatxlib.h)

add_library(vfatxlib STATIC ${SOURCE})
add_pch(vfatxlib vfatxlib.h SOURCE)
target_link_libraries(vfatxlib chkstk)
add_dependencies(vfatxlib psdk)
