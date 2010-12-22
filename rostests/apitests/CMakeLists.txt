
add_library(apitest apitest.c)

add_subdirectory(dciman32)
add_subdirectory(gdi32)
#add_subdirectory(ntdll) The asm file needs to be fixed.
add_subdirectory(user32)

if(ARCH MATCHES i386)
add_subdirectory(w32kdll)
add_subdirectory(w32knapi)
endif()

add_subdirectory(ws2_32)
