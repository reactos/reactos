
add_definitions(-DUNICODE -D_UNICODE)

add_library(delayimp delayimp.c)
add_dependencies(delayimp psdk)
add_importlibs(delayimp kernel32)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_library(delayimp_hooks pfnDliFailureHook2.c pfnDliNotifyHook2.c)
    add_dependencies(delayimp_hooks psdk)
endif()
