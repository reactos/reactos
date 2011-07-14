set_unicode()
set_cpp()

add_definitions(
    -D__WINESRC__
    -DROS_Headers)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/lib/atl)

set_rc_compiler()

spec2def(comtest.dll comtest.spec)

list(APPEND SOURCE
	IComTest.h
	IComTest_i.c
    comtest.cpp
    comtest.h
    dllmain.cpp
    stdafx.cpp
    stdafx.h
    targetver.h
    ${CMAKE_CURRENT_BINARY_DIR}/comtest.def)

	
add_library(comtest SHARED ${SOURCE})
set_module_type(comtest win32dll)

target_link_libraries(comtest
    atlnew
    uuid
    wine)

add_importlibs(comtest
    shlwapi
    shell32
    comctl32
    gdi32
    ole32
    oleaut32
    user32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_cab_target(comtest 1)
