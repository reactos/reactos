
add_definitions(
    -D__WINESRC__
    -DENTRY_PREFIX=OLEACC_
    -DPROXY_DELEGATION
    -DWINE_REGISTER_DLL)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(oleacc.dll oleacc.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    client.c
    main.c
    propservice.c
    window.c
    oleacc_private.h
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c)

add_idl_headers(oleacc_idlheader oleacc_classes.idl)
add_typelib(oleacc_classes.idl)
add_rpcproxy_files(oleacc_classes.idl)

list(APPEND oleacc_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/oleacc_classes_r.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/oleacc_classes_t.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc_classes.tlb)

set_source_files_properties(oleacc.rc PROPERTIES OBJECT_DEPENDS "${oleacc_rc_deps}")

add_library(oleacc SHARED
    ${SOURCE}
    guid.c
    oleacc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc_classes_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc.def)

add_dependencies(oleacc oleacc_idlheader stdole2)
set_module_type(oleacc win32dll)
target_link_libraries(oleacc uuid wine ${PSEH_LIB})
add_importlibs(oleacc oleaut32 ole32 user32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(oleacc oleacc_private.h SOURCE)
add_cd_file(TARGET oleacc DESTINATION reactos/system32 FOR all)
