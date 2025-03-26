
include_directories(Include)

list(APPEND SOURCE
    udf_info/alloc.cpp
    udf_info/dirtree.cpp
    udf_info/extent.cpp
    udf_info/mount.cpp
    udf_info/phys_eject.cpp
    udf_info/physical.cpp
    udf_info/remap.cpp
    udf_info/udf_info.cpp
    cleanup.cpp
    close.cpp
    create.cpp
    devcntrl.cpp
    dircntrl.cpp
    env_spec.cpp
    fastio.cpp
    fileinfo.cpp
    filter.cpp
    flush.cpp
    fscntrl.cpp
    lockctrl.cpp
    mem.cpp
    misc.cpp
    namesup.cpp
    #pnp.cpp
    read.cpp
    secursup.cpp
    shutdown.cpp
    sys_spec.cpp
    udf_dbg.cpp
    udfinit.cpp
    unload.cpp
    verfysup.cpp
    volinfo.cpp
    wcache.cpp
    write.cpp
    udffs.h)

add_library(udfs MODULE ${SOURCE} udffs.rc)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(udfs PRIVATE -Wno-unused-but-set-variable)
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
add_cd_file(TARGET udfs DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(udfs_reg.inf)
