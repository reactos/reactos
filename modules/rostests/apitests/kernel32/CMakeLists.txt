
add_subdirectory(redirptest)

add_message_headers(ANSI FormatMessage.mc)

list(APPEND SOURCE
    ConsoleCP.c
    CreateProcess.c
    DefaultActCtx.c
    DeviceIoControl.c
    dosdev.c
    FindActCtxSectionStringW.c
    FindFiles.c
    FLS.c
    FormatMessage.c
    GetComputerNameEx.c
    GetCurrentDirectory.c
    GetDriveType.c
    GetModuleFileName.c
    GetVolumeInformation.c
    interlck.c
    IsDBCSLeadByteEx.c
    JapaneseCalendar.c
    LoadLibraryExW.c
    lstrcpynW.c
    lstrlen.c
    Mailslot.c
    MultiByteToWideChar.c
    PrivMoveFileIdentityW.c
    SetComputerNameExW.c
    SetConsoleWindowInfo.c
    SetCurrentDirectory.c
    SetUnhandledExceptionFilter.c
    SystemFirmware.c
    TerminateProcess.c
    TunnelCache.c
    WideCharToMultiByte.c
    precomp.h)

add_executable(kernel32_apitest ${SOURCE} testlist.c kernel32_apitest.rc)
target_link_libraries(kernel32_apitest wine ${PSEH_LIB})
set_module_type(kernel32_apitest win32cui)
add_delay_importlibs(kernel32_apitest advapi32 shlwapi)
add_importlibs(kernel32_apitest msvcrt kernel32 ntdll)
add_dependencies(kernel32_apitest FormatMessage)
add_pch(kernel32_apitest precomp.h SOURCE)
add_rostests_file(TARGET kernel32_apitest)

list(APPEND MANIFEST_FILES
    classtest.manifest
    classtest2.manifest
    comctl32dep.manifest
    dep1.manifest
    deptest.manifest)

foreach(item ${MANIFEST_FILES})
    add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}")
endforeach(item)
