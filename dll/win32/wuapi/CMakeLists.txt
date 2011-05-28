
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine)


spec2def(wuapi.dll wuapi.spec)

list(APPEND SOURCE
    downloader.c
    installer.c
    main.c
    regsvr.c
    searcher.c
    session.c
    updates.c
    ${CMAKE_CURRENT_BINARY_DIR}/wuapi.def)

add_library(wuapi SHARED ${SOURCE})

set_module_type(wuapi win32dll)
target_link_libraries(wuapi uuid wine wuguid)
add_importlibs(wuapi ole32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wuapi DESTINATION reactos/system32 FOR all)
