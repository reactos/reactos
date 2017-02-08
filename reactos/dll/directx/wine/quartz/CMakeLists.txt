
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -DENTRY_PREFIX=QUARTZ_
    -DWINE_REGISTER_DLL
    -DPROXY_DELEGATION)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(quartz.dll quartz.spec)
add_rpcproxy_files(quartz_strmif.idl)

list(APPEND SOURCE
    acmwrapper.c
    avidec.c
    avisplit.c
    dsoundrender.c
    enumfilters.c
    enummedia.c
    enummoniker.c
    enumregfilters.c
    filesource.c
    filtergraph.c
    filtermapper.c
    main.c
    memallocator.c
    mpegsplit.c
    nullrenderer.c
    parser.c
    pin.c
    regsvr.c
    systemclock.c
    videorenderer.c
    vmr9.c
    waveparser.c
    quartz_private.h)

add_library(quartz SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/quartz_strmif_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/quartz.def
    version.rc)

add_idl_headers(quartz_idlheader fil_data.idl)
add_typelib(control_tlb.idl)

list(APPEND quartz_rc_deps
    ${CMAKE_CURRENT_BINARY_DIR}/control_tlb.tlb
    ${CMAKE_CURRENT_SOURCE_DIR}/control_tlb.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/quartz_strmif.rgs)

set_source_files_properties(version.rc PROPERTIES OBJECT_DEPENDS "${quartz_rc_deps}")
set_module_type(quartz win32dll)
target_link_libraries(quartz strmbase strmiids uuid dxguid wine ${PSEH_LIB})
add_importlibs(quartz dsound msacm32 msvfw32 ole32 oleaut32 rpcrt4 user32 gdi32 advapi32 advapi32_vista shlwapi msvcrt kernel32 ntdll)
add_dependencies(quartz dxsdk quartz_idlheader stdole2)
add_pch(quartz quartz_private.h SOURCE)
add_cd_file(TARGET quartz DESTINATION reactos/system32 FOR all)
