
PROJECT(drwtsn32)

set_cpp(WITH_RUNTIME WITH_EXCEPTIONS WITH_STL)
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/atl
    ${REACTOS_SOURCE_DIR}/sdk/lib/udmihelp)

list(APPEND CPP_SOURCE
    drwtsn32.cpp
    main.cpp
    stacktrace.cpp
    sysinfo.cpp
    drwtsn32.h
    precomp.h)

add_executable(drwtsn32 ${CPP_SOURCE} drwtsn32.rc)
add_pch(drwtsn32 precomp.h CPP_SOURCE)
set_module_type(drwtsn32 win32gui UNICODE)
target_link_libraries(drwtsn32 atlnew udmihelp)
add_importlibs(drwtsn32 dbghelp psapi advapi32 shell32 shlwapi msvcrt user32 kernel32 ntdll)
add_cd_file(TARGET drwtsn32 DESTINATION reactos/system32 FOR all)
