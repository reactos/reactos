
add_subdirectory(beep)
add_subdirectory(bootvid)

if(_WINKD_ MATCHES 1)
add_subdirectory(kddll)
else()
add_subdirectory(kdcom)
endif(_WINKD_ MATCHES 1)

add_subdirectory(nmidebug)
add_subdirectory(null)
