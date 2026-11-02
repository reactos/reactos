
include_directories(Include)

list(APPEND SOURCE
    udf_info/alloc.c
    udf_info/dirtree.c
    udf_info/extent.c
    udf_info/mount.c
    udf_info/physical.c
    udf_info/recovery_core.c
    udf_info/remap.c
    udf_info/udf_info.c
    cleanup.c
    cow.c
    close.c
    create.c
    devcntrl.c
    deviosup.c
    dirappend.c
    metabatch.c
    udf_info/fe_batch.c
    dircntrl.c
    fastio.c
    fileinfo.c
    flush.c
    fscntrl.c
    lockctrl.c
    misc.c
    namesup.c
    prefxsup.c
    pnp.c
    recovery_mount.c
    read.c
    secursup.c
    shutdown.c
    sys_spec.c
    udf_dbg.c
    udfinit.c
    unload.c
    verfysup.c
    volinfo.c
    write.c
    strucsup.c
    filobsup.c
    udfdata.c
    udffs.h
)

add_library(udfs MODULE ${SOURCE} udffs.rc)

target_compile_definitions(udfs PRIVATE _CRT_NON_CONFORMING_SWPRINTFS)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(udfs PRIVATE -Wno-unused-but-set-variable)
    target_compile_options(udfs PRIVATE -Werror)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(udfs PRIVATE -Wno-extern-c-compat -Wno-unused-value)
    target_compile_options(udfs PRIVATE -Wno-tautological-constant-out-of-range-compare)
    target_compile_options(udfs PRIVATE -Wno-tautological-unsigned-zero-compare -Wno-self-assign)
    target_compile_options(udfs PRIVATE -Wno-sometimes-uninitialized -Wno-parentheses-equality)
endif()

set_module_type(udfs kernelmodedriver)
target_link_libraries(udfs ${PSEH_LIB})
add_importlibs(udfs ntoskrnl hal)
add_pch(udfs udffs.h SOURCE)
add_cd_file(TARGET udfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(udfs_reg.inf)
