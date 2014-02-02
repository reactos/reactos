
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(oleacc.dll oleacc.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc.def)

add_typelib(oleacc_classes.idl)

list(APPEND oleacc_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/oleacc_classes.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc_classes.tlb)

set_source_files_properties(oleacc.rc PROPERTIES OBJECT_DEPENDS "${oleacc_rc_deps}")

add_library(oleacc SHARED ${SOURCE} oleacc.rc)
set_module_type(oleacc win32dll)
target_link_libraries(oleacc wine)
add_importlibs(oleacc user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET oleacc DESTINATION reactos/system32 FOR all)
