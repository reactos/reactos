remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601)
add_definitions(-DNTDDI_VERSION=0x06010000)

include_directories(
    ${REACTOS_SOURCE_DIR}/dll/3rdparty/libtirpc/tirpc
    ${REACTOS_SOURCE_DIR}/drivers/filesystems/nfs
    ${REACTOS_SOURCE_DIR}/dll/np/nfs)

list(APPEND SOURCE 
    nfs41_daemon.c
    daemon_debug.c
    nfs41_ops.c
    nfs41_compound.c
    nfs41_xdr.c
    nfs41_server.c
    nfs41_client.c
    nfs41_superblock.c
    nfs41_session.c
    lookup.c
    mount.c
    open.c
    readwrite.c
    lock.c
    readdir.c
    getattr.c
    setattr.c
    upcall.c
    nfs41_rpc.c
    util.c
    pnfs_layout.c
    pnfs_device.c
    pnfs_debug.c
    pnfs_io.c
    name_cache.c
    namespace.c
    volume.c
    callback_server.c
    callback_xdr.c
    service.c
    symlink.c
    idmap.c
    delegation.c
    recovery.c
    acl.c
    ea.c)

add_executable(nfsd ${SOURCE} nfsd.rc)

if(MSVC)
else()
    # FIXME: Tons of warnings.
    add_target_compile_flags(nfsd "-w")
endif()

set_module_type(nfsd win32cui)
add_importlibs(nfsd advapi32 iphlpapi kernel32_vista libtirpc msvcrt shell32 ws2_32 wldap32 kernel32 ntdll)
add_cd_file(TARGET nfsd DESTINATION reactos/system32 FOR all)
add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/netconfig" DESTINATION reactos/system32/drivers/etc FOR all)
add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/ms-nfs41-idmap.conf" DESTINATION reactos/system32/drivers/etc FOR all)
