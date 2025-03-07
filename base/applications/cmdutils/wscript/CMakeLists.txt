
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    arguments.c
    host.c
    main.c
    precomp.h)

add_executable(wscript ${SOURCE} rsrc.rc)
add_idl_headers(wscript_idlheader ihost.idl)
add_typelib(ihost.idl)
set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ihost.tlb)
target_link_libraries(wscript uuid wine oldnames)
set_module_type(wscript win32gui UNICODE)
add_delay_importlibs(wscript shlwapi)
add_importlibs(wscript shell32 oleaut32 ole32 user32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(wscript stdole2 wscript_idlheader)
add_pch(wscript precomp.h SOURCE)
add_cd_file(TARGET wscript DESTINATION reactos/system32 FOR all)
