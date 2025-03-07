spec2def(libtirpc.dll libtirpc/libtirpc.spec ADD_IMPORTLIB)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
add_definitions(-DUNICODE -D_WIN32 -DPORTMAP -D_CRT_NONSTDC_NO_DEPRECATE=1)

include_directories(${REACTOS_SOURCE_DIR}/dll/3rdparty/libtirpc/tirpc)

list(APPEND SOURCE
    src/asprintf.c
#    src/auth_des.c
    src/authdes_prot.c
#    src/auth_gss.c
#    src/authgss_prot.c
    src/auth_none.c
    src/auth_sspi.c
    src/authsspi_prot.c
    src/auth_time.c
    src/auth_unix.c
    src/authunix_prot.c
    src/bindresvport.c
    src/clnt_bcast.c
    src/clnt_dg.c
    src/clnt_generic.c
    src/clnt_perror.c
    src/clnt_raw.c
    src/clnt_simple.c
    src/clnt_vc.c
#    src/crypt_client.c
#    src/des_crypt.c
    src/des_soft.c
    src/epoll_sub.c
    src/getnetconfig.c
    src/getnetpath.c
    src/getpeereid.c
    src/getpublickey.c
    src/getrpcent.c
    src/getrpcport.c
    src/gettimeofday.c
    src/key_call.c
    src/key_prot_xdr.c
    #src/makefile
    #src/Makefile.am
    src/mt_misc.c
    src/netname.c
    src/netnamer.c
    src/pmap_clnt.c
    src/pmap_getmaps.c
    src/pmap_getport.c
    src/pmap_prot2.c
    src/pmap_prot.c
    src/pmap_rmt.c
    src/rpcb_clnt.c
    src/rpcb_prot.c
    src/rpcb_st_xdr.c
    src/rpc_callmsg.c
    src/rpc_com.h
    src/rpc_commondata.c
    src/rpcdname.c
    src/rpc_dtablesize.c
    src/rpc_generic.c
    src/rpc_prot.c
    src/rpc_soc.c
    src/rtime.c
    src/sources
    src/svc_auth.c
#    src/svc_auth_des.c
#    src/svc_auth_gss.c
    src/svc_auth_none.c
    src/svc_auth_unix.c
    src/svc.c
    src/svc_dg.c
    src/svc_generic.c
    src/svc_raw.c
    src/svc_run.c
    src/svc_simple.c
    src/svc_vc.c
    src/winstubs.c
    src/wintirpc.c
    src/xdr_array.c
    src/xdr.c
    src/xdr_float.c
    src/xdr_mem.c
    src/xdr_rec.c
    src/xdr_reference.c
    src/xdr_sizeof.c
    src/xdr_stdio.c
    precomp.h)

add_library(libtirpc MODULE
    ${SOURCE}
#    libtirpc/libtirpc.rc
    libtirpc/libtirpc_ros.rc
    ${CMAKE_CURRENT_BINARY_DIR}/libtirpc.def)

if(MSVC)
    # Disable warning C4273: 'strtok_s': inconsistent dll linkage
    # Disable warning C4313: 'fprintf': '%x' in format string conflicts with argument 2 of type 'HANDLE'
    target_compile_options(libtirpc PRIVATE /wd4273 /wd4313)
    if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(libtirpc PRIVATE /wd4101 /wd4133 /wd4473 /wd4477)
    endif()
else()
    # FIXME: Tons of warnings.
    target_compile_options(libtirpc PRIVATE "-w")
endif()

set_module_type(libtirpc win32dll)
target_link_libraries(libtirpc oldnames)
add_importlibs(libtirpc user32 advapi32 kernel32_vista ws2_32 msvcrt kernel32 ntdll)
add_delay_importlibs(libtirpc secur32)
add_pch(libtirpc precomp.h SOURCE)
add_cd_file(TARGET libtirpc DESTINATION reactos/system32 FOR all)
