
add_typelib(wuapi_tlb.idl)

add_definitions(
    -D__WINESRC__
    -DCOM_NO_WINDOWS_H)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine)


spec2def(wuapi.dll wuapi.spec)

list(APPEND SOURCE
    downloader.c
    installer.c
    main.c
    searcher.c
    session.c
    systeminfo.c
    updates.c
    wuapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wuapi.def)

set_source_files_properties(wuapi.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/wuapi_tlb.tlb)

add_library(wuapi SHARED ${SOURCE})

set_module_type(wuapi win32dll)
target_link_libraries(wuapi uuid wine wuguid)
add_importlibs(wuapi ole32 advapi32 msvcrt kernel32 ntdll)

# wuapi_tlb.tlb needs stdole2.tlb
add_dependencies(wuapi stdole2)

add_cd_file(TARGET wuapi DESTINATION reactos/system32 FOR all)
