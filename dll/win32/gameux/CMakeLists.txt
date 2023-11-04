
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(gameux.dll gameux.spec)

list(APPEND SOURCE
    factory.c
	gameexplorer.c
	gamestatistics.c)

add_library(gameux MODULE
    ${SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/gameux.def)

add_idl_headers(gameux_idl gameux_tlb.idl)

set_module_type(gameux win32dll UNICODE ENTRYPOINT 0)
target_link_libraries(gameux uuid wine)
add_dependencies(gameux gameux_idl)
add_importlibs(gameux shell32 oleaut32 ole32 user32 advapi32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET gameux DESTINATION reactos/system32 FOR all)
