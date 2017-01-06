
list(APPEND SOURCE
    dosdev.c
    FindActCtxSectionStringW.c
    FindFiles.c
    GetComputerNameEx.c
    GetCurrentDirectory.c
    GetDriveType.c
    GetModuleFileName.c
    interlck.c
    lstrcpynW.c
    MultiByteToWideChar.c
    PrivMoveFileIdentityW.c
    SetConsoleWindowInfo.c
    SetCurrentDirectory.c
    SetUnhandledExceptionFilter.c
    TerminateProcess.c
    TunnelCache.c
    WideCharToMultiByte.c
    testlist.c
    Mailslot.c)

add_executable(kernel32_apitest ${SOURCE})
target_link_libraries(kernel32_apitest wine ${PSEH_LIB})
set_module_type(kernel32_apitest win32cui)
add_delay_importlibs(kernel32_apitest advapi32 shlwapi)
add_importlibs(kernel32_apitest msvcrt kernel32 ntdll)
add_cd_file(TARGET kernel32_apitest DESTINATION reactos/bin FOR all)

list(APPEND MANIFEST_FILES
    classtest.manifest
    comctl32dep.manifest
    dep1.manifest
    deptest.manifest)

foreach(item ${MANIFEST_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/bin FOR all)
endforeach(item)
