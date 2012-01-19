
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/mingw-w64)

add_definitions(-D_CRTBLD)

if(NOT MSVC)
    add_compile_flags("-Wno-main")
endif()

list(APPEND MINGW_COMMON_SOURCE
    _newmode.c
    atonexit.c
    binmode.c
    charmax.c
    cinitexe.c
    CRT_glob.c
    crt_handler.c
    dllentry.c
    gs_support.c
    merr.c
    mingw_helpers.c
    natstart.c
    pesect.c
    pseudo-reloc.c
    pseudo-reloc-list.c
    tlsmcrt.c
    tlsmthread.c
    tlssup.c
    tlsthrd.c
    txtmode.c
    wildcard.c
    xncommod.c
    xthdloc.c
    xtxtmode.c
    ofmt_stub.c
    cxa_pure_virtual.c)

if(MSVC)
    list(APPEND MINGW_COMMON_SOURCE mscmain.c)
else()
    list(APPEND MINGW_COMMON_SOURCE gccmain.c)
endif()

add_library(mingw_common ${MINGW_COMMON_SOURCE})

if(NOT MSVC)
    target_link_libraries(mingw_common oldnames)
endif()

add_target_compile_definitions(mingw_common _M_CEE_PURE)
add_dependencies(mingw_common psdk)

add_library(mingw_main crt0_c.c crtexe.c dllargv.c)
add_target_compile_definitions(mingw_main _DLL __USE_CRTIMP)
add_dependencies(mingw_main psdk)

add_library(mingw_wmain crt0_w.c crtexe.c dllargv.c)
add_target_compile_definitions(mingw_wmain WPRFLAG UNICODE _UNICODE)
add_dependencies(mingw_wmain psdk)

add_library(mingw_dllmain crtdll.c dllargv.c)
add_target_compile_definitions(mingw_dllmain CRTDLL _WINDOWS_)
add_dependencies(mingw_dllmain psdk)

if(NOT MSVC)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/liboldnames.a
        COMMAND ${CMAKE_DLLTOOL} --def ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def --kill-at --output-lib ${CMAKE_CURRENT_BINARY_DIR}/liboldnames.a)

    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/liboldnames.a PROPERTIES GENERATED TRUE)

    add_custom_target(oldnames ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/liboldnames.a)
endif()

add_library(getopt getopt.c)
add_target_compile_definitions(getopt _DLL __USE_CRTIMP)
add_dependencies(getopt psdk)
