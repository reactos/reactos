
add_subdirectory(apitests)
#add_subdirectory(dibtests)
#add_subdirectory(drivers)
#add_subdirectory(dxtest)
add_subdirectory(kmtests)
#add_subdirectory(regtests)
if(NOT MSVC) # FIXME: msvc build
    add_subdirectory(rosautotest)
endif()
add_subdirectory(tests)
#add_subdirectory(win32)
add_subdirectory(winetests)
