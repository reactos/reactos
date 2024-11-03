
list(APPEND SOURCE
    src/ansiprsr.cpp
    src/keytrans.cpp
    src/tcharmap.cpp
    src/tconsole.cpp
    src/tkeydef.cpp
    src/tkeymap.cpp
    src/tmapldr.cpp
    src/tmouse.cpp
    src/tnclass.cpp
    src/tnclip.cpp
    src/tncon.cpp
    src/tnconfig.cpp
    src/tnerror.cpp
    src/tnetwork.cpp
    src/tnmain.cpp
    src/tnmisc.cpp
    src/tscript.cpp
    src/tscroll.cpp
    src/ttelhndl.cpp
    precomp.h)

add_executable(telnet ${SOURCE} telnet.rc)
target_link_libraries(telnet cppstl)
set_target_cpp_properties(telnet WITH_EXCEPTIONS)

if (MSVC)
    # C4838: conversion from 'int' to 'SHORT' requires a narrowing conversion
    target_compile_options(telnet PRIVATE /wd4838)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(telnet PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-narrowing>)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(telnet PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-restrict>)
    endif()
endif()

set_module_type(telnet win32cui)
add_importlibs(telnet ws2_32 user32 msvcrt kernel32 ntdll)
add_pch(telnet precomp.h SOURCE)
add_cd_file(TARGET telnet DESTINATION reactos/system32 FOR all)
