
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -DENTRY_PREFIX=QUARTZ_
    -DREGISTER_PROXY_DLL
    -DPROXY_DELEGATION
    -D_DLL
    -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(quartz.dll quartz.spec)

add_library(quartz SHARED
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
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/quartz.def)

set_module_type(quartz win32dll)

target_link_libraries(quartz
    ${CMAKE_CURRENT_BINARY_DIR}/quartz.def
    quartz_proxy
    strmiids
    uuid
    wine
    pseh)

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


rpcproxy(quartz quartz_strmif.idl)
add_cab_target(quartz 1)
