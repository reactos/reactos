
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -D_SHLWAPI_)

spec2def(shcore.dll shcore.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/shcore_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shcore.def)

add_library(shcore MODULE ${SOURCE})
set_module_type(shcore win32dll UNICODE)
target_link_libraries(shcore wine wine_dll_canunload)
add_importlibs(shcore advapi32 advapi32_vista gdi32 ole32 user32 msvcrt kernel32 ntdll)
set_wine_module_FIXME(shcore) # CORE-5743: CONST_VTABLE

add_cd_file(TARGET shcore DESTINATION reactos/system32 FOR all)
