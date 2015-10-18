
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers
                    inc)

list(APPEND SOURCE
    src/init.c
    src/ext3/generic.c
    src/ext3/htree.c
    src/ext3/indirect.c
    src/ext3/recover.c
    src/ext4/ext4_bh.c
    src/ext4/ext4_extents.c
    src/ext4/ext4_jbd2.c
    src/ext4/extents.c
    src/jbd/recovery.c
    src/jbd/replay.c
    src/jbd/revoke.c
    src/nls/nls_ascii.c
    src/nls/nls_base.c
    src/nls/nls_cp437.c
    src/nls/nls_cp737.c
    src/nls/nls_cp775.c
    src/nls/nls_cp850.c
    src/nls/nls_cp852.c
    src/nls/nls_cp855.c
    src/nls/nls_cp857.c
    src/nls/nls_cp860.c
    src/nls/nls_cp861.c
    src/nls/nls_cp862.c
    src/nls/nls_cp863.c
    src/nls/nls_cp864.c
    src/nls/nls_cp865.c
    src/nls/nls_cp866.c
    src/nls/nls_cp869.c
    src/nls/nls_cp874.c
    src/nls/nls_cp932.c
    src/nls/nls_cp936.c
    src/nls/nls_cp949.c
    src/nls/nls_cp950.c
    src/nls/nls_cp1250.c
    src/nls/nls_cp1251.c
    src/nls/nls_cp1255.c
    src/nls/nls_euc-jp.c
    src/nls/nls_iso8859-1.c
    src/nls/nls_iso8859-2.c
    src/nls/nls_iso8859-3.c
    src/nls/nls_iso8859-4.c
    src/nls/nls_iso8859-5.c
    src/nls/nls_iso8859-6.c
    src/nls/nls_iso8859-7.c
    src/nls/nls_iso8859-9.c
    src/nls/nls_iso8859-13.c
    src/nls/nls_iso8859-14.c
    src/nls/nls_iso8859-15.c
    src/nls/nls_koi8-r.c
    src/nls/nls_koi8-ru.c
    src/nls/nls_koi8-u.c
    src/nls/nls_utf8.c
    src/block.c
    src/cleanup.c
    src/close.c
    src/cmcb.c
    src/create.c
    src/debug.c
    src/devctl.c
    src/dirctl.c
    src/dispatch.c
    src/except.c
    src/fastio.c
    src/fileinfo.c
    src/flush.c
    src/fsctl.c
    src/linux.c
    src/lock.c
    src/memory.c
    src/misc.c
    src/nls.c
    src/pnp.c
    src/rbtree.c
    src/read.c
    src/shutdown.c
    src/volinfo.c
    src/write.c
    inc/ext2fs.h)

add_library(ext2fs SHARED ${SOURCE} ext2fs.rc)

if(NOT MSVC)
    add_target_compile_flags(ext2fs "-Wno-pointer-sign -Wno-unused-function")
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_target_compile_flags(ext2fs "-Wno-unused-but-set-variable -Wno-unused-variable -Wno-missing-braces")
    endif()
else()
    #disable warnings: "unreferenced local variable", "initialized, but not used variable", "benign include"
    replace_compile_flags("/we\"4189\"" " ")
    add_target_compile_flags(ext2fs "/wd\"4189\" /wd\"4142\" /wd\"4101\"")
endif()

target_link_libraries(ext2fs memcmp ${PSEH_LIB})
add_definitions(-D__KERNEL__ -D_X86_)
set_module_type(ext2fs kernelmodedriver)
add_importlibs(ext2fs ntoskrnl hal)
add_pch(ext2fs inc/ext2fs.h SOURCE)
add_cd_file(TARGET ext2fs DESTINATION reactos/system32/drivers NO_CAB FOR all)
