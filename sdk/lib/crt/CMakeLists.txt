
include_directories(include)
#include_directories(.)

add_definitions(-D_CRTBLD)

include(conio/conio.cmake)
include(direct/direct.cmake)
include(except/except.cmake)
include(float/float.cmake)
include(math/math.cmake)
include(mbstring/mbstring.cmake)
include(mem/mem.cmake)
include(misc/misc.cmake)
include(printf/printf.cmake)
include(process/process.cmake)
include(search/search.cmake)
include(setjmp/setjmp.cmake)
include(startup/startup.cmake)
include(stdio/stdio.cmake)
include(stdlib/stdlib.cmake)
include(string/string.cmake)
include(time/time.cmake)
include(wine/wine.cmake)
include(wstring/wstring.cmake)

include(crt.cmake)
include(libcntpr.cmake)
include(msvcrtex.cmake)
include(oldnames.cmake)

add_library(user32_wsprintf
    printf/streamout.c
    printf/wstreamout.c
    printf/wsprintfA.c
    printf/wsprintfW.c
    printf/wvsprintfA.c
    printf/wvsprintfW.c
    printf/wvsnprintfA.c
    printf/wvsnprintfW.c
    string/strnlen.c
    string/wcsnlen.c
    string/mbstowcs_nt.c
    string/wcstombs_nt.c)

add_dependencies(user32_wsprintf psdk)
target_compile_definitions(user32_wsprintf PRIVATE _USER32_WSPRINTF)
