
add_subdirectory(beep)
add_subdirectory(bootvid)

if(_WINKD_)
add_subdirectory(kddll)
else()
add_subdirectory(kdcom)
endif()
add_subdirectory(nmidebug)
add_subdirectory(null)
