spec2def(ntlmssp.dll ntlmssp.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    	avl.c
	calculations.c
	ciphers.c
	context.c
	credentials.c
	crypt.c
	debug.c
	dllmain.c
	messages.c
	ntlmssp.c
	protocol.c
	stubs.c
	sign.c
	util.c
	${CMAKE_CURRENT_BINARY_DIR}/ntlmssp_stubs.c
	${CMAKE_CURRENT_BINARY_DIR}/ntlmssp.def)

add_library(ntlmssp SHARED ${SOURCE})
allow_warnings(ntlmssp)
set_module_type(ntlmssp win32dll UNICODE)

target_link_libraries(ntlmssp wine)

add_importlibs(ntlmssp advapi32 crypt32 netapi32 msvcrt kernel32 ntdll)

add_cd_file(TARGET ntlmssp DESTINATION reactos/system32 FOR all)
#add_importlib_target(ntlmssp.spec)