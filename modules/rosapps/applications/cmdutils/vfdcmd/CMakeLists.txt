
add_message_headers(ANSI vfdmsg.mc)

include_directories(${REACTOS_SOURCE_DIR}/modules/rosapps/include/vfd)
add_executable(vfdcmd vfdcmd.c vfdcmd.rc)
set_module_type(vfdcmd win32cui)
add_importlibs(vfdcmd advapi32 vfd user32 msvcrt kernel32 ntdll)
add_dependencies(vfdcmd vfdmsg)
set_target_properties(vfdcmd PROPERTIES OUTPUT_NAME "vfd")
add_cd_file(TARGET vfdcmd DESTINATION reactos/system32 FOR all)
