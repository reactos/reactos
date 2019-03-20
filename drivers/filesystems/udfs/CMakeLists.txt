
include_directories(Include)

set_cpp(WITH_RUNTIME)

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
    pnp.cpp
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

if(MSVC)
    # FIXME: Make sure we marked all what needs to be marked with stdcall before removing this.
    add_target_compile_flags(udfs "/Gz")
    if(USE_CLANG_CL)
        add_target_compile_flags(udfs "-Wno-extern-c-compat -Wno-unused-value")
        add_target_compile_flags(udfs "-Wno-tautological-constant-out-of-range-compare")
        add_target_compile_flags(udfs "-Wno-tautological-unsigned-zero-compare -Wno-self-assign")
        add_target_compile_flags(udfs "-Wno-sometimes-uninitialized -Wno-parentheses-equality")
    endif()
else()
    add_target_compile_flags(udfs "-Wno-unused-but-set-variable")
endif()

set_module_type(udfs kernelmodedriver)
target_link_libraries(udfs ${PSEH_LIB})
add_importlibs(udfs ntoskrnl hal)
add_pch(udfs udffs.h SOURCE)
add_cd_file(TARGET udfs DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(udfs_reg.inf)
