add_subdirectory(ncftp)
if(NOT MSVC OR NOT ARCH STREQUAL "arm") # FIXME: CORE-17606.
add_subdirectory(netreg)
endif()
add_subdirectory(niclist)
add_subdirectory(roshttpd)
add_subdirectory(tditest)
