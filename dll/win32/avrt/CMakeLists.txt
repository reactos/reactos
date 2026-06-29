
spec2def(avrt.dll avrt.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/avrt_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/avrt.def)

add_library(avrt MODULE ${SOURCE})
set_module_type(avrt win32dll)
target_link_libraries(avrt wine)
add_importlibs(avrt user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET avrt DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(avrt) # CORE-5743: ???
