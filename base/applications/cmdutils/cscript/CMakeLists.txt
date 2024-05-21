
add_definitions(-DCSCRIPT_BUILD -D__WINESRC__)
set(wscript_folder ${REACTOS_SOURCE_DIR}/base/applications/cmdutils/wscript)
include_directories(${wscript_folder})

list(APPEND SOURCE
    ${wscript_folder}/arguments.c
    ${wscript_folder}/host.c
    ${wscript_folder}/main.c
    ${wscript_folder}/precomp.h)

add_executable(cscript ${SOURCE} rsrc.rc)
add_idl_headers(cscript_idlheader ihost.idl)
add_typelib(ihost.idl)
set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ihost.tlb)
target_link_libraries(cscript uuid wine oldnames)
set_module_type(cscript win32cui UNICODE)
add_delay_importlibs(cscript shlwapi)
add_importlibs(cscript shell32 oleaut32 ole32 advapi32 user32 msvcrt kernel32 ntdll)
add_dependencies(cscript stdole2 cscript_idlheader)
add_pch(cscript ${wscript_folder}/precomp.h SOURCE)
add_cd_file(TARGET cscript DESTINATION reactos/system32 FOR all)
