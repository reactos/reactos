list(APPEND SOURCE
    nfs41_driver.c
    nfs41_debug.c
    nfs41_driver.h
    nfs41_debug.h)

include_directories(
    ${REACTOS_SOURCE_DIR}/dll/np/nfs)

add_definitions(-DRDBSS_TRACKER)

add_library(nfs41_driver MODULE ${SOURCE} nfs.rc)
set_module_type(nfs41_driver kernelmodedriver)
target_link_libraries(nfs41_driver ntoskrnl_vista rdbsslib rxce copysup memcmp ${PSEH_LIB})
add_importlibs(nfs41_driver ntoskrnl hal)

if(GCC OR CLANG)
    target_compile_options(nfs41_driver PRIVATE "-Wno-switch")
    if(CLANG)
        target_compile_options(nfs41_driver PRIVATE "-Wno-unused-value")
        if(GCC)
            # When CLang mimicks GCC, it uses C17 as default, which breaks __inline
            target_compile_options(nfs41_driver PRIVATE "-std=gnu89")
        endif()
    endif()
endif()

add_cd_file(TARGET nfs41_driver DESTINATION reactos/system32/drivers FOR all)
