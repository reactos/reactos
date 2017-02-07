
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()
spec2def(oleacc.dll oleacc.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    oleacc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc.def)

add_library(oleacc SHARED ${SOURCE})

set_module_type(oleacc win32dll)

target_link_libraries(oleacc wine)

add_importlibs(oleacc user32 msvcrt kernel32 ntdll)


add_cd_file(TARGET oleacc DESTINATION reactos/system32 FOR all)
