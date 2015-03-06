
add_definitions(-D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/core/include)

list(APPEND SOURCE
    osver.c
    testlist.c
    w32knapi.c
    w32knapi.rc)

add_executable(w32knapi ${SOURCE})
target_link_libraries(w32knapi apitest)
set_module_type(w32knapi win32cui)
add_importlibs(w32knapi 
    w32kdll_ros # w32kdll_2ksp4 w32kdll_xpsp2 w32kdll_2k3sp2 w32kdll_vista
    gdi32 
    user32 
    shell32 
    advapi32 
    msvcrt 
    kernel32 
    ntdll)
