
add_subdirectory(beep)
add_subdirectory(bootvid)
add_subdirectory(condrv)

if(_WINKD_)
    add_subdirectory(kdcom)
    add_subdirectory(kdgdb)
    if(NOT ARCH STREQUAL "arm")
        add_subdirectory(kdvm)
    endif()
else()
    add_subdirectory(kdrosdbg)
endif()

if(NOT ARCH STREQUAL "arm")
add_subdirectory(nmidebug)
endif()
add_subdirectory(null)
