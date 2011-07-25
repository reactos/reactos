
add_library(vfatxlib fatx.c vfatxlib.c)

target_link_libraries(vfatxlib chkstk)

if(NOT MSVC)
    target_link_libraries(vfatxlib -lgcc)
endif()

add_dependencies(vfatxlib psdk)
