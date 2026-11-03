
add_subdirectory(beep)
add_subdirectory(bootvid)
add_subdirectory(condrv)

if(_WINKD_)
    add_subdirectory(kdcom)
    if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")
        add_subdirectory(kdgdb)
        add_subdirectory(kdvm)
    endif()
else()
    add_subdirectory(kdrosdbg)
endif()
add_subdirectory(kdstub)

add_subdirectory(nmidebug)
add_subdirectory(null)
