
set_cpp(WITH_EXCEPTIONS WITH_STL)

add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)

add_executable(telnet
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
    telnet.rc)

set_module_type(telnet win32cui)
add_pch(telnet src/precomp.h)
add_importlibs(telnet ws2_32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET telnet DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    add_compile_flags_language("-Wno-narrowing" "CXX")
endif()
