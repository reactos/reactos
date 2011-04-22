include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/audiosrv2)

spec2def(audiosrv2dll.dll audiosrv2dll.spec)

list(APPEND SOURCE
    dllmain.cpp
	audiosrv.cpp
	${CMAKE_CURRENT_BINARY_DIR}/audiosrv2dll.def)

add_library(audiosrv2dll SHARED ${SOURCE})
set_entrypoint(audiosrv2dll 0)
set_module_type(audiosrv2dll win32dll)

target_link_libraries(audiosrv2dll uuid wine)
add_importlibs(audiosrv2dll ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_cab_target(audiosrv2dll 1)
