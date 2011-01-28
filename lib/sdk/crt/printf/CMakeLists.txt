
include_directories(.. ../include)

add_definitions(-D_USER32_WSPRINTF)

add_library(user32_wsprintf
    streamout.c
    wstreamout.c
    wsprintfA.c
    wsprintfW.c
    wvsprintfA.c
    wvsprintfW.c
    wvsnprintfA.c
    wvsnprintfW.c
    ../string/mbstowcs_nt.c
    ../string/wcstombs_nt.c)
