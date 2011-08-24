
add_subdirectory(beep)
add_subdirectory(bootvid)

if(_WINKD_)
add_subdirectory(kddll)
else()
add_subdirectory(kdcom)
endif()
if(ARCH MATCHES i386)
    add_subdirectory(nmidebug)
endif()
add_subdirectory(null)
