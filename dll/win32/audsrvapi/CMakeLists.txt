include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs)

spec2def(audsrvapi.dll audsrvapi.spec)

list(APPEND SOURCE
    dllmain.c
	audsrvapi.c
	${CMAKE_CURRENT_BINARY_DIR}/audsrvapi.def)

add_library(audsrvapi SHARED ${SOURCE})
set_module_type(audsrvapi win32dll)

target_link_libraries(audsrvapi audsrvrpc uuid wine ${PSEH_LIB})


add_pch(audsrv ${CMAKE_CURRENT_SOURCE_DIR}/audsrv.h ${SOURCE})

add_importlibs(audsrvapi rpcrt4 ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_cab_target(audsrvapi 1)
add_importlib_target(audsrvapi.spec)