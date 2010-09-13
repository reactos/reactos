
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

spec2def(wmilib ${CMAKE_CURRENT_SOURCE_DIR}/wmilib.spec ${CMAKE_CURRENT_BINARY_DIR}/wmilib.def)

add_library(wmilib SHARED wmilib.c wmilib.rc)

set_target_properties(wmilib PROPERTIES LINK_FLAGS "-Wl,-entry,0 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(wmilib
    ${CMAKE_CURRENT_BINARY_DIR}/wmilib.def
    ${REACTOS_SOURCE_DIR}/ntoskrnl/libntoskrnl.a)

add_dependencies(wmilib wmilib_def psdk buildno_header bugcodes)
