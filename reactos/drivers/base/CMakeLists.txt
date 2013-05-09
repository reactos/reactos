
add_subdirectory(beep)
add_subdirectory(bootvid)

if(_WINKD_)
add_subdirectory(kdcom)
else()
add_subdirectory(kdrosdbg)
endif()

add_subdirectory(nmidebug)
add_subdirectory(null)
