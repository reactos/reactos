
if(ARCH STREQUAL "amd64")
add_library(ndk_tests
    win2003_x64.c
    winvista_x64.c
    win7_x64.c
    win8_x64.c
    win81_x64.c
    win10_x64.c)
else()
add_library(ndk_tests
    win2003_x86.c
    winvista_x86.c
    win7_x86.c
    win8_x86.c
    win81_x86.c
    win10_x86.c)
endif()
