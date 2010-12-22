
add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    InitializeLpkHooks.c
    RealGetWindowClass.c
    ScrollDC.c
    ScrollWindowEx.c
    GetSystemMetrics.c
    testlist.c)

add_executable(user32_apitest ${SOURCE})
target_link_libraries(user32_apitest wine)
set_module_type(user32_apitest win32cui)
add_importlibs(user32_apitest gdi32 user32 msvcrt kernel32 ntdll)
