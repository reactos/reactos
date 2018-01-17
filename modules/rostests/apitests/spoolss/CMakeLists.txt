
include_directories(${REACTOS_SOURCE_DIR}/win32ss/printing/include)

list(APPEND SOURCE
    AlignRpcPtr.c
    MarshallDownStructuresArray.c
    MarshallUpStructuresArray.c
    PackStrings.c
    ReallocSplStr.c
    SplInitializeWinSpoolDrv.c
    testlist.c)

add_executable(spoolss_apitest ${SOURCE})
target_link_libraries(spoolss_apitest wine ${PSEH_LIB})
set_module_type(spoolss_apitest win32cui)
add_importlibs(spoolss_apitest spoolss msvcrt kernel32 ntdll)
add_rostests_file(TARGET spoolss_apitest)
