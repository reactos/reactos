
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msimtf ${CMAKE_CURRENT_SOURCE_DIR}/msimtf.spec ${CMAKE_CURRENT_BINARY_DIR}/msimtf.def)

add_library(msimtf SHARED
    activeimmapp.c
    main.c
    rsrc.rc)

set_module_type(msimtf win32dll)

target_link_libraries(msimtf
    ${CMAKE_CURRENT_BINARY_DIR}/msimtf.def
    uuid
    wine)

add_importlibs(msimtf msvcrt imm32 kernel32 ntdll)
add_dependencies(msimtf msimtf_def)
add_livecd_target(msimtf reactos/system32)
