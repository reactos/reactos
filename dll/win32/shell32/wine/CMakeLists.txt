
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

if(MSVC)
    # error C4311: 'type cast': pointer truncation from 'HANDLE' to 'INT'
    replace_compile_flags("/we4311" " ")
endif()

list(APPEND SOURCE
    appbar.c
    brsfolder.c
    classes.c
    clipboard.c
    control.c
    pidl.c
    shell32_main.c
    shellole.c
    shellord.c
    shellpath.c
    shellreg.c
    shellstring.c
    shlmenu.c
    shpolicy.c
    precomp.h)

add_library(shell32_wine OBJECT ${SOURCE})
add_pch(shell32_wine precomp.h)
target_compile_definitions(shell32_wine PRIVATE UNICODE _UNICODE _WINE)
add_dependencies(shell32_wine psdk)
