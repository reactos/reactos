
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)

spec2def(uianimation.dll uianimation.spec)

list(APPEND SOURCE
	dllmain.c
	main.c
    ${CMAKE_CURRENT_BINARY_DIR}/uianimation_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/uianimation.def)

add_idl_headers(uianimation_idlheader uianimation_reg.idl uianimation_typelib.idl)

add_library(uianimation SHARED ${SOURCE})
add_dependencies(uianimation uianimation_idlheader stdole2)
set_module_type(uianimation win32dll ENTRYPOINT DllMain)
target_link_libraries(uianimation wine uuid)
add_delay_importlibs(uianimation user32)
add_importlibs(uianimation msvcrt ntdll kernel32)
add_cd_file(TARGET uianimation DESTINATION reactos/system32 FOR all)
