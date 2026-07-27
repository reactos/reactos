
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WIN32_WINNT=0x600
    -DWINVER=0x600
    -DENTRY_PREFIX=QUARTZ_
    -DPROXY_DELEGATION
    -DWINE_REGISTER_DLL)

spec2def(quartz.dll quartz.spec)

list(APPEND SOURCE
    acmwrapper.c
    avidec.c
    decoder.c
    dsoundrender.c
    filesource.c
    filtergraph.c
    filtermapper.c
    main.c
    memallocator.c
    parser.c
    passthrough.c
    systemclock.c
    videorenderer.c
    vmr7.c
    vmr7_presenter.c
    vmr9.c
    window.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/quartz_strmif_p.c)

add_typelib(control_tlb.idl)
add_rpcproxy_files(quartz_strmif.idl)

add_library(quartz MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    quartz.rc
    ${CMAKE_CURRENT_BINARY_DIR}/quartz.def)

set_source_files_properties(quartz.rc PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/control_tlb.tlb;${CMAKE_CURRENT_SOURCE_DIR}/quartz.rgs")

set_module_type(quartz win32dll)
target_link_libraries(quartz wine wine_dll_canunload strmbase strmiids uuid dxguid oldnames ${PSEH_LIB})
add_delay_importlibs(quartz ddraw)
add_importlibs(quartz
    dsound msacm32 msvfw32 ole32 oleaut32 rpcrt4 user32 gdi32 advapi32 advapi32_vista winmm
    kernel32_vista msvcrt kernel32 ntdll)
add_dependencies(quartz wineheaders dxsdk stdole2)
add_cd_file(TARGET quartz DESTINATION reactos/system32 FOR all)
set_wine_module(quartz)
