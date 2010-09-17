
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_library(nmidebug SHARED nmidebug.c nmidebug.rc)

set_target_properties(nmidebug PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(nmidebug
    -lntoskrnl
    -lhal)

add_dependencies(nmidebug psdk bugcodes)
