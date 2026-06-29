
spec2def(psapi.dll psapi.spec ADD_IMPORTLIB)

list(APPEND PSAPI_STATIC_SOURCE psapi.c)
add_library(psapi_static ${PSAPI_STATIC_SOURCE})
add_dependencies(psapi_static psdk)

list(APPEND SOURCE
    main.c
    psapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/psapi.def)

add_library(psapi MODULE ${SOURCE})
set_module_type(psapi win32dll ENTRYPOINT DllMain 12)
target_link_libraries(psapi psapi_static ${PSEH_LIB})
add_importlibs(psapi kernel32 ntdll)
add_cd_file(TARGET psapi DESTINATION reactos/system32 FOR all)
