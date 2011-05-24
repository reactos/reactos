
set_unicode()

include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_executable(umpnpmgr umpnpmgr.c umpnpmgr.rc)
target_link_libraries(umpnpmgr pnprpc wdmguid ${PSEH_LIB})

set_module_type(umpnpmgr win32cui)
add_importlibs(umpnpmgr advapi32 rpcrt4 userenv msvcrt kernel32 ntdll)
add_cd_file(TARGET umpnpmgr DESTINATION reactos/system32 FOR all)
