
add_library(apitest apitest.c)

add_subdirectory(advapi32)
add_subdirectory(crt)
add_subdirectory(dciman32)
add_subdirectory(gdi32)
add_subdirectory(msvcrt)
add_subdirectory(ntdll)
add_subdirectory(user32)
add_subdirectory(kernel32)

if(NOT MSVC)
if(ARCH MATCHES i386)
add_subdirectory(w32kdll)
add_subdirectory(w32knapi)
endif()
endif()

add_subdirectory(ws2_32)
