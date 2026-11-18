add_executable(ipv6xchg ipv6xchg.c)
set_module_type(ipv6xchg win32cui UNICODE)
add_importlibs(ipv6xchg ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ipv6xchg DESTINATION VerOS/system32 FOR all)
