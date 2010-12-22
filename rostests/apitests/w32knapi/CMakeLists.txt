
add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    osver.c
    testlist.c
    w32knapi.c
    w32knapi.rc)

add_executable(w32knapi ${SOURCE})
target_link_libraries(w32knapi apitest)
set_module_type(w32knapi win32cui)
add_importlibs(w32knapi w32kdll_ros gdi32 user32 shell32 advapi32 msvcrt kernel32 ntdll)
