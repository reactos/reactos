
list(APPEND SOURCE
    compbatt.c
    compmisc.c
    comppnp.c
    compbatt.rc)

add_library(compbatt SHARED ${CMAKE_CURRENT_BINARY_DIR}/compbatt_compbatt.h.gch ${SOURCE})

set_target_properties(compbatt PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(compbatt ntoskrnl hal battc)
add_pch(compbatt ${CMAKE_CURRENT_SOURCE_DIR}/compbatt.h ${SOURCE})
add_dependencies(compbatt psdk bugcodes)
