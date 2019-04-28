
set_cpp(WITH_EXCEPTIONS WITH_STL)

add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)

if(NOT MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
    add_compile_flags("-Wno-restrict")
endif()

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

if(NOT MSVC)
    add_compile_flags_language("-Wno-narrowing" "CXX")
endif()

add_executable(telnet ${SOURCE} telnet.rc)
set_module_type(telnet win32cui)
add_importlibs(telnet ws2_32 user32 msvcrt kernel32 ntdll)
add_pch(telnet precomp.h SOURCE)
add_cd_file(TARGET telnet DESTINATION reactos/system32 FOR all)
