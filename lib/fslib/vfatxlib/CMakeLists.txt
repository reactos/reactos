
add_library(vfatxlib fatx.c vfatxlib.c)
add_pch(vfatxlib vfatxlib.h)
target_link_libraries(vfatxlib chkstk)

if(NOT MSVC)
    target_link_libraries(vfatxlib -lgcc)
endif()

add_dependencies(vfatxlib psdk)
