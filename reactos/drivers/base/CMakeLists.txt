
add_subdirectory(beep)
add_subdirectory(bootvid)
add_subdirectory(condrv)

if(_WINKD_)
	if (GDB)
		add_subdirectory(kdgdb)
	else()
		add_subdirectory(kdcom)
		add_subdirectory(kdvm)
	endif()
else()
	add_subdirectory(kdrosdbg)
endif()

add_subdirectory(nmidebug)
add_subdirectory(null)
