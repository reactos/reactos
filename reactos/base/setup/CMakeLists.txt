
add_subdirectory(reactos)
add_subdirectory(setup)
add_subdirectory(usetup)
if(NOT ARCH STREQUAL "arm")
add_subdirectory(vmwinst)
endif()
add_subdirectory(welcome)
