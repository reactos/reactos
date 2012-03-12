
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -DENTRY_PREFIX=QUARTZ_
    -DREGISTER_PROXY_DLL
    -DPROXY_DELEGATION)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(quartz.dll quartz.spec)

add_rpcproxy_files(quartz_strmif.idl)

list(APPEND SOURCE
    avidec.c
    acmwrapper.c
    waveparser.c
    videorenderer.c
    transform.c
    systemclock.c
    regsvr.c
    pin.c
    parser.c
    nullrenderer.c
    mpegsplit.c
    memallocator.c
    main.c
    filtermapper.c
    filtergraph.c
    filesource.c
    enumregfilters.c
    enumpins.c
    enummoniker.c
    enumfilters.c
    dsoundrender.c
    enummedia.c
    control.c
    avisplit.c
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/quartz_strmif_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/quartz.def)

add_library(quartz SHARED
    ${SOURCE}
    version.rc)

set_module_type(quartz win32dll)

target_link_libraries(quartz
    strmiids
    uuid
    wine
    ${PSEH_LIB})

add_importlibs(quartz
    msvcrt
    advapi32
    dsound
    ole32
    oleaut32
    shlwapi
    user32
    gdi32
    advapi32
    msvfw32
    msacm32
    rpcrt4
    kernel32
    ntdll)

add_pch(quartz quartz_private.h)
add_cd_file(TARGET quartz DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(quartz)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()
