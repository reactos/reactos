
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(wuapi SHARED
    downloader.c
    installer.c
    main.c
    regsvr.c
    searcher.c
    session.c
    updates.c
    wuapi.def)

if(NOT MSVC)
    set_source_files_properties(wuapi.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(wuapi win32dll)

target_link_libraries(wuapi
    wine
    uuid)

add_importlibs(wuapi ole32 advapi32 kernel32 ntdll)

add_cab_target(wuapi 1)
