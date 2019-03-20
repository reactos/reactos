
add_definitions(
    -D__WINESRC__
    -DCOM_NO_WINDOWS_H)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(wuapi.dll wuapi.spec)

list(APPEND SOURCE
    downloader.c
    installer.c
    main.c
    searcher.c
    session.c
    systeminfo.c
    updates.c
    precomp.h)

add_typelib(wuapi_tlb.idl)
set_source_files_properties(wuapi.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/wuapi_tlb.tlb)

add_library(wuapi MODULE
    ${SOURCE}
    wuapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wuapi.def)

set_module_type(wuapi win32dll)
target_link_libraries(wuapi uuid wine wuguid)
add_importlibs(wuapi msvcrt kernel32 ntdll)
add_dependencies(wuapi stdole2) # wuapi_tlb.tlb needs stdole2.tlb
add_pch(wuapi precomp.h SOURCE)
add_cd_file(TARGET wuapi DESTINATION reactos/system32 FOR all)
