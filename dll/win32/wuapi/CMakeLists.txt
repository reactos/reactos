
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(wuapi ${CMAKE_CURRENT_SOURCE_DIR}/wuapi.spec ${CMAKE_CURRENT_BINARY_DIR}/wuapi.def)

add_library(wuapi SHARED
    downloader.c
    installer.c
    main.c
    regsvr.c
    searcher.c
    session.c
    updates.c)

set_module_type(wuapi win32dll)

target_link_libraries(wuapi
    ${CMAKE_CURRENT_BINARY_DIR}/wuapi.def
    wine)

add_importlibs(wuapi uuid ole32 advapi32 ntdll)
add_dependencies(wuapi wuapi_def psdk)
